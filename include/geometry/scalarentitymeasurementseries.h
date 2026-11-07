#ifndef SCALARENTITYMEASUREMENTSERIES_H
#define SCALARENTITYMEASUREMENTSERIES_H
#include <map>
#include <string>

#include "core_export.h"
#include "geometry.h"

namespace oi {

/*!
 * \brief The ScalarEntityMeasurementSeries class
 */
class OI_CORE_EXPORT ScalarEntityMeasurementSeries : public Geometry {
  friend struct codec::Access;  // the protobuf codec (see src/codec)

 public:
  explicit ScalarEntityMeasurementSeries();
  explicit ScalarEntityMeasurementSeries(const double& seriesValue);

  ScalarEntityMeasurementSeries(const ScalarEntityMeasurementSeries& copy);

  ScalarEntityMeasurementSeries& operator=(
      const ScalarEntityMeasurementSeries& copy);

  ~ScalarEntityMeasurementSeries() override;

  // ########################################
  // order of unknown parameters (Qxx-matrix)
  // ########################################

  enum MeasurementSeriesUnknowns { unknownMeasurementSeries = 0 };

  // ########################################
  // get or set measurement series parameters
  // ########################################

  [[nodiscard]] const double& getSeriesValue() const;

  void setSeriesValue(const double& seriesValue);

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

  [[nodiscard]] std::string getDisplayMeasurementSeries(
      const UnitType& type, const int& digits,
      const bool& showDiff = false) const override;

 private:
  // #############################
  // measurement series attributes
  // #############################

  double seriesValue{};
};

}  // namespace oi

#endif  // SCALARENTITYMEASUREMENTSERIES_H
