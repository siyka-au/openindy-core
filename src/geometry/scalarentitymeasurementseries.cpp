#include "scalarentitymeasurementseries.h"

#include <utility>

#include "featurewrapper.h"

using namespace oi;

/*!
 * \brief ScalarEntityMeasurementSeries::ScalarEntityMeasurementSeries
 * \param parent
 */
ScalarEntityMeasurementSeries::ScalarEntityMeasurementSeries(QObject* parent)
    : Geometry(parent) {
  // set up feature wrapper
  if (!this->selfFeature.isNull()) {
    this->selfFeature->setScalarEntityMeasurementSeries(this);
  }
}

/*!
 * \brief ScalarEntityMeasurementSeries::ScalarEntityMeasurementSeries
 * \param seriesValue
 * \param parent
 */
ScalarEntityMeasurementSeries::ScalarEntityMeasurementSeries(
    const double& seriesValue, QObject* parent)
    : Geometry(parent) {
  // set up feature wrapper
  if (!this->selfFeature.isNull()) {
    this->selfFeature->setScalarEntityMeasurementSeries(this);
  }

  this->setSeriesValue(seriesValue);
}

/*!
 * \brief ScalarEntityMeasurementSeries::ScalarEntityMeasurementSeries
 * \param copy
 * \param parent
 */
ScalarEntityMeasurementSeries::ScalarEntityMeasurementSeries(
    const ScalarEntityMeasurementSeries& copy, QObject* parent)
    : Geometry(copy, parent) {
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
QMap<GeometryParameters, QString>
ScalarEntityMeasurementSeries::getUnknownParameters(
    const QMap<DimensionType, UnitType>& displayUnits,
    const QMap<DimensionType, int>& displayDigits) const {
  QMap<GeometryParameters, QString> parameters;

  parameters.insert(
      GeometryParameters::eUnknownMeasurementSeries,
      this->getDisplayMeasurementSeries(
          displayUnits.value(DimensionType::eMetric, UnitType::eUnitMeter),
          displayDigits.value(DimensionType::eMetric, 0)));

  return parameters;
}

/*!
 * \brief ScalarEntityMeasurementSeries::applyUnknownParameters
 * \param parameters
 */
void ScalarEntityMeasurementSeries::applyUnknownParameters(
    const QMap<GeometryParameters, double>& parameters) {
  // get current parameters
  double series = this->seriesValue;

  // update parameters
  QList<GeometryParameters> keys = parameters.keys();
  for (const GeometryParameters& key : std::as_const(keys)) {
    switch (key) {
      case GeometryParameters::eUnknownMeasurementSeries:
        series =
            parameters.value(GeometryParameters::eUnknownMeasurementSeries);
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
QString ScalarEntityMeasurementSeries::getDisplayMeasurementSeries(
    const UnitType& type, const int& digits, const bool& /*showDiff*/) const {
  if (this->isSolved) {
    return QString::number(convertFromDefault(this->seriesValue, type), 'f',
                           digits);
  }
  return QString("");
}
