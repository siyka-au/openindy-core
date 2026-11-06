#ifndef RADIUS_H
#define RADIUS_H

#include "core_export.h"

namespace oi {

/*!
 * \brief A radius. A plain value type: copyable, movable, no identity, no Qt.
 */
class OI_CORE_EXPORT Radius {
 public:
  Radius() = default;
  explicit Radius(double r) : radius(r) {}

  void setRadius(double r) { radius = r; }

  [[nodiscard]] const double& getRadius() const { return radius; }

 private:
  double radius{0.0};
};

}  // namespace oi

#endif  // RADIUS_H
