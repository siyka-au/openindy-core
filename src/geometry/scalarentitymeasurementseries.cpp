#include "scalarentitymeasurementseries.h"

#include <map>
#include <utility>
#include <vector>

#include "containers.h"
#include "featurewrapper.h"
#include "stringutil.h"

using namespace oi;

/*!
 * \brief ScalarEntityMeasurementSeries::ScalarEntityMeasurementSeries
 */
ScalarEntityMeasurementSeries::ScalarEntityMeasurementSeries() : Geometry() {
  // set up feature wrapper
  if (!this->selfFeature.isNull()) {
    this->selfFeature->setScalarEntityMeasurementSeries(this);
  }
}

/*!
 * \brief ScalarEntityMeasurementSeries::ScalarEntityMeasurementSeries
 * \param seriesValue
 */
ScalarEntityMeasurementSeries::ScalarEntityMeasurementSeries(
    const double& seriesValue)
    : Geometry() {
  // set up feature wrapper
  if (!this->selfFeature.isNull()) {
    this->selfFeature->setScalarEntityMeasurementSeries(this);
  }

  this->setSeriesValue(seriesValue);
}

/*!
 * \brief ScalarEntityMeasurementSeries::ScalarEntityMeasurementSeries
 * \param copy
 */
ScalarEntityMeasurementSeries::ScalarEntityMeasurementSeries(
    const ScalarEntityMeasurementSeries& copy)
    : Geometry(copy) {
  // set up feature wrapper
  if (!this->selfFeature.isNull()) {
    this->selfFeature->setScalarEntityMeasurementSeries(this);
  }

  this->seriesValue = copy.seriesValue;
}

/*!
 * \brief ScalarEntityMeasurementSeries::operator =
 * \param copy
 * \return
 */
ScalarEntityMeasurementSeries& ScalarEntityMeasurementSeries::operator=(
    const ScalarEntityMeasurementSeries& copy) {
  // set up feature wrapper
  if (!this->selfFeature.isNull()) {
    this->selfFeature->setScalarEntityMeasurementSeries(this);
  }

  this->seriesValue = copy.seriesValue;

  return *this;
}

/*!
 * \brief ScalarEntityMeasurementSeries::~ScalarEntityMeasurementSeries
 */
ScalarEntityMeasurementSeries::~ScalarEntityMeasurementSeries() {}

/*!
 * \brief ScalarEntityMeasurementSeries::getSeriesValue
 * \return
 */
const double& ScalarEntityMeasurementSeries::getSeriesValue() const {
  return this->seriesValue;
}

/*!
 * \brief ScalarEntityMeasurementSeries::setSeriesValue
 * \param seriesValue
 */
void ScalarEntityMeasurementSeries::setSeriesValue(const double& seriesValue) {
  this->seriesValue = seriesValue;

  this->notify(&FeatureObserver::onGeomParametersChanged, this->id);
}

/*!
 * \brief ScalarEntityMeasurementSeries::getUnknownParameters
 * \param displayUnits
 * \param displayDigits
 * \return
 */
std::map<GeometryParameters, std::string>
ScalarEntityMeasurementSeries::getUnknownParameters(
    const std::map<DimensionType, UnitType>& displayUnits,
    const std::map<DimensionType, int>& displayDigits) const {
  std::map<GeometryParameters, std::string> parameters;

  parameters.insert_or_assign(
      GeometryParameters::eUnknownMeasurementSeries,
      this->getDisplayMeasurementSeries(
          oi::value(displayUnits, DimensionType::eMetric, UnitType::eUnitMeter),
          oi::value(displayDigits, DimensionType::eMetric, 0)));

  return parameters;
}

/*!
 * \brief ScalarEntityMeasurementSeries::applyUnknownParameters
 * \param parameters
 */
void ScalarEntityMeasurementSeries::applyUnknownParameters(
    const std::map<GeometryParameters, double>& parameters) {
  // get current parameters
  double series = this->seriesValue;

  // update parameters
  std::vector<GeometryParameters> keys = oi::keys(parameters);
  for (const GeometryParameters& key : std::as_const(keys)) {
    switch (key) {
      case GeometryParameters::eUnknownMeasurementSeries:
        series = oi::value(parameters,
                           GeometryParameters::eUnknownMeasurementSeries);
        break;
      default:
        break;
    }
  }

  // update measurement series definition
  this->setSeriesValue(series);
}

/*!
 * \brief ScalarEntityMeasurementSeries::recalc
 */
void ScalarEntityMeasurementSeries::recalc() {
  Geometry::recalc();

  // reset measurement series definition if not solved and no nominal
  if (!this->isSolved && this->getOwnerDescription().has_value()) {
    this->seriesValue = 0.0;
  }
}

/*!
 * \brief ScalarEntityMeasurementSeries::getDisplayMeasurementSeries
 * \param type
 * \param digits
 * \param showDiff
 * \return
 */
std::string ScalarEntityMeasurementSeries::getDisplayMeasurementSeries(
    const UnitType& type, const int& digits, const bool& /*showDiff*/) const {
  if (this->isSolved) {
    return oi::number(convertFromDefault(this->seriesValue, type), 'f', digits);
  }
  return std::string("");
}
