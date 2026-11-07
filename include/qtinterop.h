#ifndef OI_QTINTEROP_H_
#define OI_QTINTEROP_H_

#include <QList>
#include <vector>

namespace oi {

//! Qt API results (QString::split, ...) as std containers. Goes away with
//! QString (phase 6b).
template <typename T>
[[nodiscard]] std::vector<T> toVector(const QList<T>& list) {
  return std::vector<T>(list.begin(), list.end());
}

}  // namespace oi

#endif  // OI_QTINTEROP_H_
