#ifndef SCALARENTITYTEMPERATURE_H
#define SCALARENTITYTEMPERATURE_H
#include <QtCore>
#include <map>

#include "core_export.h"
#include "geometry.h"

namespace oi {

/*!
 * \brief The ScalarEntityTemperature class
 */
class OI_CORE_EXPORT ScalarEntityTemperature : public Geometry {
  friend struct codec::Access;  // the protobuf codec (see src/codec)

 public:
  explicit ScalarEntityTemperature();
  explicit ScalarEntityTemperature(const double& temperature);

  ScalarEntityTemperature(const ScalarEntityTemperature& copy);

  ScalarEntityTemperature& operator=(const ScalarEntityTemperature& copy);

  ~ScalarEntityTemperature() override;

  // ########################################
  // order of unknown parameters (Qxx-matrix)
  // ########################################

  enum TemperatureUnknowns { unknownTemperature = 0 };

  // #################################
  // get or set temperature parameters
  // #################################

  [[nodiscard]] const double& getTemperature() const;

  void setTemperature(const double& temperature);

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

  [[nodiscard]] QString getDisplayTemperature(
      const UnitType& type, const int& digits,
      const bool& showDiff = false) const override;

 private:
  // ######################
  // temperature attributes
  // ######################

  double temperature{};
};

}  // namespace oi

#endif  // SCALARENTITYTEMPERATURE_H
