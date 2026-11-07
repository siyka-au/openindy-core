#include "scalarentitytemperature.h"

#include <utility>

#include "featurewrapper.h"

using namespace oi;

/*!
 * \brief ScalarEntityTemperature::ScalarEntityTemperature
 * \param parent
 */
ScalarEntityTemperature::ScalarEntityTemperature(QObject* parent)
    : Geometry(parent) {
  // set up feature wrapper
  if (!this->selfFeature.isNull()) {
    this->selfFeature->setScalarEntityTemperature(this);
  }
}

/*!
 * \brief ScalarEntityTemperature::ScalarEntityTemperature
 * \param temperature
 * \param parent
 */
ScalarEntityTemperature::ScalarEntityTemperature(const double& temperature,
                                                 QObject* parent)
    : Geometry(parent) {
  // set up feature wrapper
  if (!this->selfFeature.isNull()) {
    this->selfFeature->setScalarEntityTemperature(this);
  }

  this->setTemperature(temperature);
}

/*!
 * \brief ScalarEntityTemperature::ScalarEntityTemperature
 * \param copy
 * \param parent
 */
ScalarEntityTemperature::ScalarEntityTemperature(
    const ScalarEntityTemperature& copy, QObject* parent)
    : Geometry(copy, parent) {
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
QMap<GeometryParameters, QString> ScalarEntityTemperature::getUnknownParameters(
    const QMap<DimensionType, UnitType>& displayUnits,
    const QMap<DimensionType, int>& displayDigits) const {
  QMap<GeometryParameters, QString> parameters;

  parameters.insert(
      GeometryParameters::eUnknownTemperature,
      this->getDisplayTemperature(
          displayUnits.value(DimensionType::eTemperature, UnitType::eUnitGrad),
          displayDigits.value(DimensionType::eTemperature, 0)));

  return parameters;
}

/*!
 * \brief ScalarEntityTemperature::applyUnknownParameters
 * \param parameters
 */
void ScalarEntityTemperature::applyUnknownParameters(
    const QMap<GeometryParameters, double>& parameters) {
  // get current parameters
  double temperature = this->temperature;

  // update parameters
  QList<GeometryParameters> keys = parameters.keys();
  for (const GeometryParameters& key : std::as_const(keys)) {
    switch (key) {
      case GeometryParameters::eUnknownTemperature:
        temperature = parameters.value(GeometryParameters::eUnknownTemperature);
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
QString ScalarEntityTemperature::getDisplayTemperature(
    const UnitType& type, const int& digits, const bool& /*showDiff*/) const {
  if (this->isSolved) {
    return QString::number(convertFromDefault(this->temperature, type), 'f',
                           digits);
  }
  return QString("");
}
