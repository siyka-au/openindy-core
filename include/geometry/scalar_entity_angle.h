#ifndef OI_CORE_GEOMETRY_SCALAR_ENTITY_ANGLE_H_
#define OI_CORE_GEOMETRY_SCALAR_ENTITY_ANGLE_H_
#include <map>
#include <string>

#include "core_export.h"
#include "geometry.h"

namespace oi {

/*!
 * \brief The ScalarEntityAngle class
 */
class OI_CORE_EXPORT ScalarEntityAngle : public Geometry {
  friend struct codec::Access;  // the protobuf codec (see src/codec)

 public:
  explicit ScalarEntityAngle();
  explicit ScalarEntityAngle(const double& angle);

  ScalarEntityAngle(const ScalarEntityAngle& copy);

  ScalarEntityAngle& operator=(const ScalarEntityAngle& copy);

  ~ScalarEntityAngle() override;

  // ########################################
  // order of unknown parameters (Qxx-matrix)
  // ########################################

  enum AngleUnknowns { unknownAngle = 0 };

  // ###########################
  // get or set angle parameters
  // ###########################

  [[nodiscard]] const double& getAngle() const;

  void setAngle(const double& angle);

  // #############################
  // get or set unknown parameters
  // #############################

  [[nodiscard]] std::map<GeometryParameters, std::string> getUnknownParameters(
      const std::map<DimensionType, UnitType>& displayUnits,
      const std::map<DimensionType, int>& displayDigits) const override;
  void applyUnknownParameters(
      const std::map<GeometryParameters, double>& parameters) override;

  // ###########################
  // reexecute the function list
  // ###########################

  void recalc() override;

  // ###############
  // display methods
  // ###############

  [[nodiscard]] std::string getDisplayAngle(
      const UnitType& type, const int& digits,
      const bool& showDiff = false) const override;

 private:
  // ################
  // angle attributes
  // ################

  double angle{};
};

}  // namespace oi

#endif  // OI_CORE_GEOMETRY_SCALAR_ENTITY_ANGLE_H_
