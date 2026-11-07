#include "scalarentitydistance.h"

#include <utility>

#include "featurewrapper.h"

using namespace oi;

/*!
 * \brief ScalarEntityDistance::ScalarEntityDistance
 * \param parent
 */
ScalarEntityDistance::ScalarEntityDistance(QObject* parent) : Geometry(parent) {
  // set up feature wrapper
  if (!this->selfFeature.isNull()) {
    this->selfFeature->setScalarEntityDistance(this);
  }
}

/*!
 * \brief ScalarEntityDistance::ScalarEntityDistance
 * \param distance
 * \param parent
 */
ScalarEntityDistance::ScalarEntityDistance(const double& distance,
                                           QObject* parent)
    : Geometry(parent) {
  // set up feature wrapper
  if (!this->selfFeature.isNull()) {
    this->selfFeature->setScalarEntityDistance(this);
  }

  this->setDistance(distance);
}

/*!
 * \brief ScalarEntityDistance::ScalarEntityDistance
 * \param copy
 * \param parent
 */
ScalarEntityDistance::ScalarEntityDistance(const ScalarEntityDistance& copy,
                                           QObject* parent)
    : Geometry(copy, parent) {
  // set up feature wrapper
  if (!this->selfFeature.isNull()) {
    this->selfFeature->setScalarEntityDistance(this);
  }

  this->distance = copy.distance;
}

/*!
 * \brief ScalarEntityDistance::operator =
 * \param copy
 * \return
 */
ScalarEntityDistance& ScalarEntityDistance::operator=(
    const ScalarEntityDistance& copy) {
  // set up feature wrapper
  if (!this->selfFeature.isNull()) {
    this->selfFeature->setScalarEntityDistance(this);
  }

  this->distance = copy.distance;

  return *this;
}

/*!
 * \brief ScalarEntityDistance::~ScalarEntityDistance
 */
ScalarEntityDistance::~ScalarEntityDistance() {}

/*!
 * \brief ScalarEntityDistance::getDistance
 * \return
 */
const double& ScalarEntityDistance::getDistance() const {
  return this->distance;
}

/*!
 * \brief ScalarEntityDistance::setDistance
 * \param distance
 */
void ScalarEntityDistance::setDistance(const double& distance) {
  this->distance = distance;

  this->notify(&FeatureObserver::onGeomParametersChanged, this->id);
}

/*!
 * \brief ScalarEntityDistance::getUnknownParameters
 * \param displayUnits
 * \param displayDigits
 * \return
 */
QMap<GeometryParameters, QString> ScalarEntityDistance::getUnknownParameters(
    const QMap<DimensionType, UnitType>& displayUnits,
    const QMap<DimensionType, int>& displayDigits) const {
  QMap<GeometryParameters, QString> parameters;

  parameters.insert(
      GeometryParameters::eUnknownDistance,
      this->getDisplayDistance(
          displayUnits.value(DimensionType::eMetric, UnitType::eUnitMeter),
          displayDigits.value(DimensionType::eMetric, 0)));

  return parameters;
}

/*!
 * \brief ScalarEntityDistance::applyUnknownParameters
 * \param parameters
 */
void ScalarEntityDistance::applyUnknownParameters(
    const QMap<GeometryParameters, double>& parameters) {
  // get current parameters
  double distance = this->distance;

  // update parameters
  QList<GeometryParameters> keys = parameters.keys();
  for (const GeometryParameters& key : std::as_const(keys)) {
    switch (key) {
      case GeometryParameters::eUnknownDistance:
        distance = parameters.value(GeometryParameters::eUnknownDistance);
        break;
      default:
        break;
    }
  }

  // update distance definition
  this->setDistance(distance);
}

/*!
 * \brief ScalarEntityDistance::recalc
 */
void ScalarEntityDistance::recalc() {
  Geometry::recalc();

  // reset distance definition if not solved
  if (!this->isSolved) {
    this->distance = 0.0;
  }
}

/*!
 * \brief ScalarEntityDistance::getDisplayDistance
 * \param type
 * \param digits
 * \param showDiff
 * \return
 */
QString ScalarEntityDistance::getDisplayDistance(
    const UnitType& type, const int& digits, const bool& /*showDiff*/) const {
  if (this->isSolved) {
    return QString::number(convertFromDefault(this->distance, type), 'f',
                           digits);
  }
  return QString("");
}
