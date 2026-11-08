#ifndef OI_CORE_POSITION_H_
#define OI_CORE_POSITION_H_

#include "core_export.h"
#include "oi_vec.h"

namespace oi {

using namespace math;

/*!
 * \brief A point in space (x, y, z) with its homogeneous form (x, y, z, h).
 *
 * A plain value type: copyable, movable, no identity, no Qt. (It used to derive
 * Element, which gave it an id and signals that nothing used.)
 */
class OI_CORE_EXPORT Position {
 public:
  Position();
  explicit Position(bool isNullObject);
  explicit Position(const OiVec& v);
  explicit Position(double x, double y, double z, double h = 1.0);

  // ##########################
  // get or set position vector
  // ##########################

  void setVector(const OiVec& v);
  void setVector(double x, double y, double z, double h = 1.0);

  [[nodiscard]] const OiVec& getVector() const;
  [[nodiscard]] const OiVec& getVectorH() const;

  /*! True for the NullObject placeholder (no position set). */
  [[nodiscard]] bool isNull() const noexcept { return isNullObject; }

  static const Position NullObject;

 private:
  OiVec xyz;   // vector of size 3 (x, y, z)
  OiVec xyzH;  // homogeneous vector of size 4 (x, y, z, h)
  bool isNullObject{false};
};

}  // namespace oi

#endif  // OI_CORE_POSITION_H_
