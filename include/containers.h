#ifndef OI_CONTAINERS_H_
#define OI_CONTAINERS_H_

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <iterator>
#include <map>
#include <type_traits>
#include <vector>

/*!
 * Small std-only helpers for the places where the old Qt containers offered a
 * convenience with no one-line std equivalent. Kept deliberately minimal.
 */
namespace oi {

namespace detail {
template <typename C>
concept Associative = requires {
  typename C::key_type;
  typename C::mapped_type;
};
}  // namespace detail

//! QMap::value / QMultiMap::value: the mapped value, or `def` if absent. For
//! a multimap this is the most recently inserted value of the key.
template <detail::Associative M, typename K>
[[nodiscard]] typename M::mapped_type value(
    const M& m, const K& k,
    std::type_identity_t<typename M::mapped_type> def = {}) {
  const auto range = m.equal_range(k);
  if (range.first == range.second) {
    return def;
  }
  return std::prev(range.second)->second;
}

//! QList::value(i): the element at `i`, or `def` if `i` is out of range.
template <typename S, std::integral I>
  requires(!detail::Associative<S>)
[[nodiscard]] typename S::value_type value(
    const S& s, I i, std::type_identity_t<typename S::value_type> def = {}) {
  if (i < 0 || static_cast<std::size_t>(i) >= s.size()) {
    return def;
  }
  return s[static_cast<std::size_t>(i)];
}

//! QList::contains
template <typename S, typename T>
  requires(!detail::Associative<S>)
[[nodiscard]] bool contains(const S& s, const T& v) {
  return std::find(s.begin(), s.end(), v) != s.end();
}

//! QList::indexOf: index of the first match or -1.
template <typename S, typename T>
[[nodiscard]] int indexOf(const S& s, const T& v, int from = 0) {
  if (from < 0) {
    from = 0;
  }
  if (static_cast<std::size_t>(from) >= s.size()) {
    return -1;
  }
  const auto it = std::find(s.begin() + from, s.end(), v);
  return it == s.end() ? -1 : static_cast<int>(it - s.begin());
}

//! QList::insert(i, v): insert before position `i` (clamped to the size).
template <typename S, typename T>
void insertAt(S& s, int i, const T& v) {
  if (i < 0) {
    i = 0;
  }
  if (static_cast<std::size_t>(i) > s.size()) {
    i = static_cast<int>(s.size());
  }
  s.insert(s.begin() + i, v);
}

//! QList::removeOne: remove the first match; true if one was removed.
template <typename S, typename T>
bool removeOne(S& s, const T& v) {
  const auto it = std::find(s.begin(), s.end(), v);
  if (it == s.end()) {
    return false;
  }
  s.erase(it);
  return true;
}

//! QList::removeAll: remove all matches; returns how many were removed.
template <typename S, typename T>
std::size_t removeAll(S& s, const T& v) {
  const auto it = std::remove(s.begin(), s.end(), v);
  const auto n = static_cast<std::size_t>(s.end() - it);
  s.erase(it, s.end());
  return n;
}

//! QList::removeAt: remove the element at `i`; false if out of range.
template <typename S, std::integral I>
bool removeAt(S& s, I i) {
  if (i < 0 || static_cast<std::size_t>(i) >= s.size()) {
    return false;
  }
  s.erase(s.begin() + static_cast<std::ptrdiff_t>(i));
  return true;
}

//! QMultiMap::remove(key, value): remove all entries equal to (key, value).
template <detail::Associative M, typename K, typename V>
std::size_t eraseValue(M& m, const K& k, const V& v) {
  std::size_t n = 0;
  const auto range = m.equal_range(k);
  for (auto it = range.first; it != range.second;) {
    if (it->second == v) {
      it = m.erase(it);
      ++n;
    } else {
      ++it;
    }
  }
  return n;
}

//! QList::append(list) / operator+=
template <typename S, typename R>
void appendAll(S& s, const R& other) {
  s.insert(s.end(), other.begin(), other.end());
}

//! QMap::keys (with duplicates for a multimap, like QMultiMap::keys)
template <detail::Associative M>
[[nodiscard]] std::vector<typename M::key_type> keys(const M& m) {
  std::vector<typename M::key_type> out;
  out.reserve(m.size());
  for (const auto& kv : m) {
    out.push_back(kv.first);
  }
  return out;
}

//! QMap::values
template <detail::Associative M>
[[nodiscard]] std::vector<typename M::mapped_type> values(const M& m) {
  std::vector<typename M::mapped_type> out;
  out.reserve(m.size());
  for (const auto& kv : m) {
    out.push_back(kv.second);
  }
  return out;
}

//! QMultiMap::values(key): all values of `k`, most recently inserted first
//! (the order QMultiMap used).
template <detail::Associative M, typename K>
[[nodiscard]] std::vector<typename M::mapped_type> values(const M& m,
                                                          const K& k) {
  std::vector<typename M::mapped_type> out;
  const auto range = m.equal_range(k);
  for (auto it = range.second; it != range.first;) {
    --it;
    out.push_back(it->second);
  }
  return out;
}

//! QMap::key: the first key mapped to `v`, or `def`.
template <detail::Associative M, typename V>
[[nodiscard]] typename M::key_type key(
    const M& m, const V& v,
    std::type_identity_t<typename M::key_type> def = {}) {
  for (const auto& kv : m) {
    if (kv.second == v) {
      return kv.first;
    }
  }
  return def;
}

}  // namespace oi

#endif  // OI_CONTAINERS_H_
