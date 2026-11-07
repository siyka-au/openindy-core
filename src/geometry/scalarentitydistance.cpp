#include "scalarentitydistance.h"

#include <map>
#include <utility>
#include <vector>

#include "containers.h"
#include "featurewrapper.h"

using namespace oi;

/*!
 * \brief ScalarEntityDistance::ScalarEntityDistance
 */
ScalarEntityDistance::ScalarEntityDistance() : Geometry() {
  // set up feature wrapper
  if (!this->selfFeature.isNull()) {
    this->selfFeature->setScalarEntityDistance(this);
  }
}

/*!
 * \brief ScalarEntityDistance::ScalarEntityDistance
 * \param distance
 */
ScalarEntityDistance::ScalarEntityDistance(const double& distance)
    : Geometry() {
  // set up feature wrapper
  if (!this->selfFeature.isNull()) {
    this->selfFeature->setScalarEntityDistance(this);
  }

  this->setDistance(distance);
}

/*!
 * \brief ScalarEntityDistance::ScalarEntityDistance
 * \param copy
 */
ScalarEntityDistance::ScalarEntityDistance(const ScalarEntityDistance& copy)
    : Geometry(copy) {
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
std::map<GeometryParameters, QString>
ScalarEntityDistance::getUnknownParameters(
    const std::map<DimensionType, UnitType>& displayUnits,
    const std::map<DimensionType, int>& displayDigits) const {
  std::map<GeometryParameters, QString> parameters;

  parameters.insert_or_assign(
      GeometryParameters::eUnknownDistance,
      this->getDisplayDistance(
          oi::value(displayUnits, DimensionType::eMetric, UnitType::eUnitMeter),
          oi::value(displayDigits, DimensionType::eMetric, 0)));

  return parameters;
}

/*!
 * \brief ScalarEntityDistance::applyUnknownParameters
 * \param parameters
 */
void ScalarEntityDistance::applyUnknownParameters(
    const std::map<GeometryParameters, double>& parameters) {
  // get current parameters
  double distance = this->distance;

  // update parameters
  std::vector<GeometryParameters> keys = oi::keys(parameters);
  for (const GeometryParameters& key : std::as_const(keys)) {
    switch (key) {
      case GeometryParameters::eUnknownDistance:
        distance = oi::value(parameters, GeometryParameters::eUnknownDistance);
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
