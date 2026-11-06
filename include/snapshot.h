#ifndef OI_SNAPSHOT_H_
#define OI_SNAPSHOT_H_

#include <type_traits>

namespace oi {

/*!
 * A copy of a container, for iterating it while the loop body may change the
 * original (directly or through a destructor, slot or notification).
 *
 *     for (const auto& feature : oi::snapshot(this->featuresList)) { ... }
 *
 * This is what Qt's foreach did implicitly. Where the loop body cannot touch
 * the container, iterate it directly (use std::as_const on a non-const lvalue
 * to avoid detaching a shared Qt container) and skip the copy.
 */
template <typename Container>
[[nodiscard]] std::remove_cvref_t<Container> snapshot(const Container& c) {
  return c;
}

}  // namespace oi

#endif  // OI_SNAPSHOT_H_
