#ifndef OI_STRINGUTIL_H_
#define OI_STRINGUTIL_H_

// Small std-only string helpers.
//
// Text convention: every std::string in the core API is UTF-8. The type does
// not validate that; validate at the input boundaries (files, the network,
// plugins, the UI). size() and the indices of these helpers count bytes, not
// characters: slicing or padding a name by position can cut a multi-byte
// character, and a field width counted in bytes looks narrower than intended
// for non-ASCII text. That is exact for ASCII, the only case the case and
// trim helpers are meant for; non-ASCII bytes pass through untouched.
//
// (This file is not called strings.h because that is a POSIX header the C
// library includes.)
#include <algorithm>
#include <cctype>
#include <charconv>
#include <cmath>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>
#include <vector>

namespace oi {

// ---- numbers to text --------------------------------------------------------

namespace detail {

//! Adds one unit in the last place to a string of decimal digits (a '.' is
//! skipped over). A carry out of the first digit prepends a '1'.
inline void incrementDecimal(std::string& digits) {
  for (std::size_t i = digits.size(); i-- > 0;) {
    if (digits[i] == '.') {
      continue;
    }
    if (digits[i] == '9') {
      digits[i] = '0';
    } else {
      ++digits[i];
      return;
    }
  }
  digits.insert(digits.begin(), '1');
}

//! digits after the decimal point beyond the requested ones that are looked at
//! to tell an exact tie from a value that merely lies close to one
inline constexpr int kTieWindow = 40;

//! |value| in fixed notation with `places` decimals. Like QString::number
//! (double-conversion), and unlike printf, a value exactly halfway between two
//! results rounds up (0.125 -> "0.13", 2.5 -> "3").
[[nodiscard]] inline std::string fixedDigits(double absValue, int places) {
  std::string out(static_cast<std::size_t>(places) + kTieWindow + 330, '\0');
  const auto format = [&](int p) {
    const auto r = std::to_chars(out.data(), out.data() + out.size(), absValue,
                                 std::chars_format::fixed, p);
    return std::string(out.data(), r.ptr);
  };
  const std::string wide = format(places + kTieWindow);
  const std::size_t point = wide.find('.');
  const std::size_t tieAt = point + 1 + static_cast<std::size_t>(places);
  if (wide[tieAt] == '5' &&
      std::ranges::all_of(wide.begin() + static_cast<std::ptrdiff_t>(tieAt) + 1,
                          wide.end(), [](char c) { return c == '0'; })) {
    std::string kept = wide.substr(0, places == 0 ? point : tieAt);
    incrementDecimal(kept);
    return kept;
  }
  return format(places);
}

//! The `sig` significant digits of |value| (not zero) and the decimal
//! exponent of the first, rounded as fixedDigits does.
struct Scientific {
  std::string digits;
  int exponent{0};
};

[[nodiscard]] inline Scientific scientificDigits(double absValue, int sig) {
  std::string buffer(static_cast<std::size_t>(sig) + kTieWindow + 16, '\0');
  const auto format = [&](int p) {
    const auto r = std::to_chars(buffer.data(), buffer.data() + buffer.size(),
                                 absValue, std::chars_format::scientific, p);
    return std::string(buffer.data(), r.ptr);
  };
  const auto split = [](const std::string& text) {
    Scientific s;
    const std::size_t e = text.find('e');
    for (std::size_t i = 0; i < e; ++i) {
      if (text[i] != '.') {
        s.digits.push_back(text[i]);
      }
    }
    s.exponent = std::stoi(text.substr(e + 1));
    return s;
  };
  Scientific wide = split(format(sig - 1 + kTieWindow));
  const std::size_t tieAt = static_cast<std::size_t>(sig);
  if (wide.digits[tieAt] == '5' &&
      std::ranges::all_of(
          wide.digits.begin() + static_cast<std::ptrdiff_t>(tieAt) + 1,
          wide.digits.end(), [](char c) { return c == '0'; })) {
    Scientific s;
    s.digits = wide.digits.substr(0, tieAt);
    s.exponent = wide.exponent;
    incrementDecimal(s.digits);
    if (s.digits.size() > tieAt) {  // 9.5 -> 10: keep `sig` digits
      s.digits.pop_back();
      ++s.exponent;
    }
    return s;
  }
  return split(format(sig - 1));
}

[[nodiscard]] inline std::string exponentText(int exponent) {
  std::string out = exponent < 0 ? "e-" : "e+";
  const std::string digits =
      std::to_string(exponent < 0 ? -exponent : exponent);
  if (digits.size() < 2) {
    out.push_back('0');
  }
  return out + digits;
}

}  // namespace detail

//! A floating point number as text, locale independent, the way
//! QString::number(value, format, precision) wrote it: `format` is one of
//! 'f' (fixed), 'e'/'E' (scientific), 'g'/'G' (shortest of both, trailing
//! zeros removed); `precision` is the number of digits after the point for
//! 'f' and 'e', and the number of significant digits for 'g'. Not a number
//! and infinities come out as "nan", "inf" and "-inf"; zero has no sign. A
//! value exactly halfway between two results rounds away from zero, as Qt
//! does (printf rounds to even).
[[nodiscard]] inline std::string number(double value, char format = 'g',
                                        int precision = 6) {
  if (std::isnan(value)) {
    return "nan";
  }
  if (std::isinf(value)) {
    return value < 0 ? "-inf" : "inf";
  }
  if (precision < 0) {
    precision = 6;
  }
  const bool upper = format == 'E' || format == 'G';
  const bool isZero = value == 0.0;
  const double absValue = std::fabs(value);
  std::string out = value < 0.0 ? "-" : "";

  if (format == 'f' || format == 'F') {
    out += detail::fixedDigits(absValue, precision);
    return out;
  }

  if (format == 'e' || format == 'E') {
    if (isZero) {
      out += "0";
      if (precision > 0) {
        out += "." + std::string(static_cast<std::size_t>(precision), '0');
      }
      out += "e+00";
    } else {
      const detail::Scientific s =
          detail::scientificDigits(absValue, precision + 1);
      out += s.digits.substr(0, 1);
      if (precision > 0) {
        out += "." + s.digits.substr(1);
      }
      out += detail::exponentText(s.exponent);
    }
  } else {  // 'g'
    const int sig = precision == 0 ? 1 : precision;
    if (isZero) {
      return "0";
    }
    const detail::Scientific s = detail::scientificDigits(absValue, sig);
    const auto stripZeros = [](std::string text) {
      if (text.find('.') != std::string::npos) {
        while (text.back() == '0') {
          text.pop_back();
        }
        if (text.back() == '.') {
          text.pop_back();
        }
      }
      return text;
    };
    if (s.exponent >= -4 && s.exponent < sig) {
      out += stripZeros(detail::fixedDigits(absValue, sig - 1 - s.exponent));
    } else {
      std::string mantissa = s.digits.substr(0, 1);
      if (sig > 1) {
        mantissa += "." + s.digits.substr(1);
      }
      out += stripZeros(mantissa) + detail::exponentText(s.exponent);
    }
  }
  if (upper) {
    std::ranges::transform(out, out.begin(), [](unsigned char c) {
      return static_cast<char>(std::toupper(c));
    });
  }
  return out;
}

//! An integer as text (QString::number(int)).
template <typename Integer>
  requires(std::is_integral_v<Integer> && !std::is_same_v<Integer, bool>)
[[nodiscard]] std::string number(Integer value) {
  return std::to_string(value);
}

// ---- text to numbers --------------------------------------------------------

namespace detail {

[[nodiscard]] inline bool isSpace(char c) {
  return c == ' ' || c == '\t' || c == '\n' || c == '\v' || c == '\f' ||
         c == '\r';
}

}  // namespace detail

//! Removes leading and trailing whitespace (QString::trimmed).
[[nodiscard]] inline std::string trim(std::string_view s) {
  while (!s.empty() && detail::isSpace(s.front())) {
    s.remove_prefix(1);
  }
  while (!s.empty() && detail::isSpace(s.back())) {
    s.remove_suffix(1);
  }
  return std::string(s);
}

//! A number from the whole text (surrounding whitespace allowed, C locale),
//! or nothing if the text is not exactly a number (QString::toDouble with
//! its ok flag).
[[nodiscard]] inline std::optional<double> toDouble(std::string_view s) {
  while (!s.empty() && detail::isSpace(s.front())) {
    s.remove_prefix(1);
  }
  while (!s.empty() && detail::isSpace(s.back())) {
    s.remove_suffix(1);
  }
  if (!s.empty() && s.front() == '+') {
    s.remove_prefix(1);  // from_chars does not take a plus sign
  }
  if (s.empty()) {
    return std::nullopt;
  }
  double value = 0.0;
  const auto result = std::from_chars(s.data(), s.data() + s.size(), value);
  if (result.ec == std::errc::result_out_of_range) {
    return std::nullopt;
  }
  if (result.ec != std::errc() || result.ptr != s.data() + s.size()) {
    return std::nullopt;
  }
  return value;
}

//! An integer from the whole text (QString::toInt with its ok flag).
[[nodiscard]] inline std::optional<int> toInt(std::string_view s,
                                              int base = 10) {
  while (!s.empty() && detail::isSpace(s.front())) {
    s.remove_prefix(1);
  }
  while (!s.empty() && detail::isSpace(s.back())) {
    s.remove_suffix(1);
  }
  if (!s.empty() && s.front() == '+') {
    s.remove_prefix(1);
  }
  if (s.empty()) {
    return std::nullopt;
  }
  int value = 0;
  const auto result =
      std::from_chars(s.data(), s.data() + s.size(), value, base);
  if (result.ec != std::errc() || result.ptr != s.data() + s.size()) {
    return std::nullopt;
  }
  return value;
}

// ---- case -------------------------------------------------------------------

[[nodiscard]] inline std::string toLower(std::string_view s) {
  std::string out(s);
  std::ranges::transform(out, out.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  return out;
}

[[nodiscard]] inline std::string toUpper(std::string_view s) {
  std::string out(s);
  std::ranges::transform(out, out.begin(), [](unsigned char c) {
    return static_cast<char>(std::toupper(c));
  });
  return out;
}

//! Equal apart from the case of ASCII letters (QString::compare with
//! Qt::CaseInsensitive == 0).
[[nodiscard]] inline bool equalsIgnoreCase(std::string_view a,
                                           std::string_view b) {
  return std::ranges::equal(a, b, [](unsigned char x, unsigned char y) {
    return std::tolower(x) == std::tolower(y);
  });
}

//! Three-way comparison ignoring the case of ASCII letters (<0, 0, >0), like
//! QString::compare with Qt::CaseInsensitive. Bytes above ASCII compare by
//! value, so non-ASCII text sorts by code point order of its UTF-8 bytes (the
//! same as by code point), not by language rules.
[[nodiscard]] inline int compareIgnoreCase(std::string_view a,
                                           std::string_view b) {
  const std::size_t n = std::min(a.size(), b.size());
  for (std::size_t i = 0; i < n; ++i) {
    const int x = std::tolower(static_cast<unsigned char>(a[i]));
    const int y = std::tolower(static_cast<unsigned char>(b[i]));
    if (x != y) {
      return x < y ? -1 : 1;
    }
  }
  if (a.size() == b.size()) {
    return 0;
  }
  return a.size() < b.size() ? -1 : 1;
}

//! Starts with `prefix`, apart from the case of ASCII letters
//! (QString::startsWith with Qt::CaseInsensitive).
[[nodiscard]] inline bool startsWithIgnoreCase(std::string_view s,
                                               std::string_view prefix) {
  return s.size() >= prefix.size() &&
         equalsIgnoreCase(s.substr(0, prefix.size()), prefix);
}

// ---- splitting and joining --------------------------------------------------

//! The pieces of `s` between occurrences of `separator`
//! (QString::split(sep, Qt::KeepEmptyParts / SkipEmptyParts)). An empty text
//! gives one empty piece with keepEmpty, none without. An empty separator
//! gives the text as the only piece.
[[nodiscard]] inline std::vector<std::string> split(std::string_view s,
                                                    std::string_view separator,
                                                    bool keepEmpty = true) {
  std::vector<std::string> pieces;
  if (separator.empty()) {
    if (keepEmpty || !s.empty()) {
      pieces.emplace_back(s);
    }
    return pieces;
  }
  std::size_t start = 0;
  while (true) {
    const std::size_t at = s.find(separator, start);
    const std::string_view piece =
        s.substr(start, at == std::string_view::npos ? std::string_view::npos
                                                     : at - start);
    if (keepEmpty || !piece.empty()) {
      pieces.emplace_back(piece);
    }
    if (at == std::string_view::npos) {
      break;
    }
    start = at + separator.size();
  }
  return pieces;
}

//! The pieces joined by `separator` (QStringList::join).
[[nodiscard]] inline std::string join(const std::vector<std::string>& pieces,
                                      std::string_view separator = {}) {
  std::string out;
  for (std::size_t i = 0; i < pieces.size(); ++i) {
    if (i != 0) {
      out += separator;
    }
    out += pieces[i];
  }
  return out;
}

}  // namespace oi

#endif  // OI_STRINGUTIL_H_
