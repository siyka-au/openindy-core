#ifndef OI_TEST_CATCH_COMPAT_H_
#define OI_TEST_CATCH_COMPAT_H_

// Helpers shared by the Catch2 tests of the core: a comparison with the
// semantics the tests had under Qt Test (floating point values are compared
// like qFuzzyCompare, everything else with ==) that shows both values when it
// fails. The comparison is one full expression, so operands may be temporaries
// (`OI_COMPARE(f().front().id, 3)`).
#include <algorithm>
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <string>
#include <type_traits>
#include <utility>

namespace oi_test {

struct CompareResult {
  bool ok = false;
  std::string text;
  explicit operator bool() const { return ok; }
};

template <typename A, typename B>
bool equal(const A& a, const B& b) {
  if constexpr (std::is_floating_point_v<A> && std::is_floating_point_v<B>) {
    if (a == b) {
      return true;
    }
    if (std::isnan(a) && std::isnan(b)) {
      return true;
    }
    return std::abs(a - b) * 1e12 <= std::min(std::abs(a), std::abs(b));
  } else if constexpr (std::is_integral_v<A> && std::is_integral_v<B> &&
                       !std::is_same_v<A, bool> && !std::is_same_v<B, bool>) {
    return std::cmp_equal(a, b);
  } else {
    return a == b;
  }
}

template <typename A, typename B>
CompareResult compare(const A& actual, const B& expected) {
  CompareResult result;
  result.ok = equal(actual, expected);
  if (!result.ok) {
    result.text = "actual: " + Catch::Detail::stringify(actual) +
                  ", expected: " + Catch::Detail::stringify(expected);
  }
  return result;
}

}  // namespace oi_test

namespace Catch {
template <>
struct StringMaker<oi_test::CompareResult> {
  static std::string convert(const oi_test::CompareResult& result) {
    return result.ok ? std::string("equal") : result.text;
  }
};
}  // namespace Catch

#define OI_COMPARE(actual, expected) \
  REQUIRE(oi_test::compare((actual), (expected)))

// a condition with an explanation (const char* or std::string)
#define OI_REQUIRE2(condition, message) \
  do {                                  \
    INFO(message);                      \
    REQUIRE(condition);                 \
  } while (false)

#endif  // OI_TEST_CATCH_COMPAT_H_
