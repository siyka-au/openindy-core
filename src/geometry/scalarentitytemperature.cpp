#include "scalarentitytemperature.h"

#include <map>
#include <utility>
#include <vector>

#include "containers.h"
#include "featurewrapper.h"
#include "stringutil.h"

using namespace oi;

/*!
 * \brief ScalarEntityTemperature::ScalarEntityTemperature
 */
ScalarEntityTemperature::ScalarEntityTemperature() : Geometry() {
  // set up feature wrapper
  if (!this->selfFeature.isNull()) {
    this->selfFeature->setScalarEntityTemperature(this);
  }
}

/*!
 * \brief ScalarEntityTemperature::ScalarEntityTemperature
 * \param temperature
 */
ScalarEntityTemperature::ScalarEntityTemperature(const double& temperature)
    : Geometry() {
  // set up feature wrapper
  if (!this->selfFeature.isNull()) {
    this->selfFeature->setScalarEntityTemperature(this);
  }

  this->setTemperature(temperature);
}

/*!
 * \brief ScalarEntityTemperature::ScalarEntityTemperature
 * \param copy
 */
ScalarEntityTemperature::ScalarEntityTemperature(
    const ScalarEntityTemperature& copy)
    : Geometry(copy) {
  // set up feature wrapper
  if (!this->selfFeature.isNull()) {
    this->selfFeature->setScalarEntityTemperature(this);
  }

  this->temperature = copy.temperature;
}

/*!
 * \brief ScalarEntityTemperature::operator =
 * \param copy
 * \return
 */
ScalarEntityTemperature& ScalarEntityTemperature::operator=(
    const ScalarEntityTemperature& copy) {
  // set up feature wrapper
  if (!this->selfFeature.isNull()) {
    this->selfFeature->setScalarEntityTemperature(this);
  }

  this->temperature = copy.temperature;

  return *this;
}

/*!
 * \brief ScalarEntityTemperature::~ScalarEntityTemperature
 */
ScalarEntityTemperature::~ScalarEntityTemperature() {}

/*!
 * \brief ScalarEntityTemperature::getTemperature
 * \return
 */
const double& ScalarEntityTemperature::getTemperature() const {
  return this->temperature;
}

/*!
 * \brief ScalarEntityTemperature::setTemperature
 * \param temperature
 */
void ScalarEntityTemperature::setTemperature(const double& temperature) {
  this->temperature = temperature;

  this->notify(&FeatureObserver::onGeomParametersChanged, this->id);
}

/*!
 * \brief ScalarEntityTemperature::getUnknownParameters
 * \param displayUnits
 * \param displayDigits
 * \return
 */
std::map<GeometryParameters, std::string>
ScalarEntityTemperature::getUnknownParameters(
    const std::map<DimensionType, UnitType>& displayUnits,
    const std::map<DimensionType, int>& displayDigits) const {
  std::map<GeometryParameters, std::string> parameters;

  parameters.insert_or_assign(
      GeometryParameters::eUnknownTemperature,
      this->getDisplayTemperature(
          oi::value(displayUnits, DimensionType::eTemperature,
                    UnitType::eUnitGrad),
          oi::value(displayDigits, DimensionType::eTemperature, 0)));

  return parameters;
}

/*!
 * \brief ScalarEntityTemperature::applyUnknownParameters
 * \param parameters
 */
void ScalarEntityTemperature::applyUnknownParameters(
    const std::map<GeometryParameters, double>& parameters) {
  // get current parameters
  double temperature = this->temperature;

  // update parameters
  std::vector<GeometryParameters> keys = oi::keys(parameters);
  for (const GeometryParameters& key : std::as_const(keys)) {
    switch (key) {
      case GeometryParameters::eUnknownTemperature:
        temperature =
            oi::value(parameters, GeometryParameters::eUnknownTemperature);
        break;
      default:
        break;
    }
  }

  // update temperature definition
  this->setTemperature(temperature);
}

/*!
 * \brief ScalarEntityTemperature::recalc
 */
void ScalarEntityTemperature::recalc() {
  Geometry::recalc();

  // reset temperature definition if not solved and no nominal
  if (!this->isSolved && this->getOwnerDescription().has_value()) {
    this->temperature = 0.0;
  }
}

/*!
 * \brief ScalarEntityTemperature::getDisplayTemperature
 * \param type
 * \param digits
 * \param showDiff
 * \return
 */
std::string ScalarEntityTemperature::getDisplayTemperature(
    const UnitType& type, const int& digits, const bool& /*showDiff*/) const {
  if (this->isSolved) {
    return oi::number(convertFromDefault(this->temperature, type), 'f', digits);
  }
  return std::string("");
}
