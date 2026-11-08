#ifndef OI_CORE_DIRECTION_H_
#define OI_CORE_DIRECTION_H_

#include "core_export.h"
#include "oi_vec.h"

namespace oi {

using namespace math;

/*!
 * \brief A direction (i, j, k) with its homogeneous form (i, j, k, h).
 *
 * A plain value type: copyable, movable, no identity, no Qt.
 */
class OI_CORE_EXPORT Direction {
 public:
  Direction();
  explicit Direction(const OiVec& v);
  explicit Direction(double x, double y, double z, double h = 1.0);

  // ###############################
  // get or set direction vector
  // ###############################

  void setVector(double x, double y, double z, double h = 1.0);
  void setVector(const OiVec& v);

  [[nodiscard]] const OiVec& getVector() const;
  [[nodiscard]] const OiVec& getVectorH() const;

 private:
  OiVec ijk;   // vector of size 3 (i, j, k)
  OiVec ijkH;  // vector of size 4 (i, j, k, h)
};

}  // namespace oi

#endif  // OI_CORE_DIRECTION_H_
