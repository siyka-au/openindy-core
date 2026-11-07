#ifndef SCALARENTITYANGLE_H
#define SCALARENTITYANGLE_H
#include <QtCore>
#include <map>

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

  [[nodiscard]] std::map<GeometryParameters, QString> getUnknownParameters(
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

  [[nodiscard]] QString getDisplayAngle(
      const UnitType& type, const int& digits,
      const bool& showDiff = false) const override;

 private:
  // ################
  // angle attributes
  // ################

  double angle{};
};

}  // namespace oi

#endif  // SCALARENTITYANGLE_H
