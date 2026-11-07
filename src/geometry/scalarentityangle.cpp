#include "scalarentityangle.h"

#include <utility>

#include "featurewrapper.h"

using namespace oi;

/*!
 * \brief ScalarEntityAngle::ScalarEntityAngle
 */
ScalarEntityAngle::ScalarEntityAngle() : Geometry() {
  // set up feature wrapper
  if (!this->selfFeature.isNull()) {
    this->selfFeature->setScalarEntityAngle(this);
  }
}

/*!
 * \brief ScalarEntityAngle::ScalarEntityAngle
 * \param angle
 */
ScalarEntityAngle::ScalarEntityAngle(const double& angle) : Geometry() {
  // set up feature wrapper
  if (!this->selfFeature.isNull()) {
    this->selfFeature->setScalarEntityAngle(this);
  }

  this->setAngle(angle);
}

/*!
 * \brief ScalarEntityAngle::ScalarEntityAngle
 * \param copy
 */
ScalarEntityAngle::ScalarEntityAngle(const ScalarEntityAngle& copy)
    : Geometry(copy) {
  // set up feature wrapper
  if (!this->selfFeature.isNull()) {
    this->selfFeature->setScalarEntityAngle(this);
  }

  this->angle = copy.angle;
}

/*!
 * \brief ScalarEntityAngle::operator =
 * \param copy
 * \return
 */
ScalarEntityAngle& ScalarEntityAngle::operator=(const ScalarEntityAngle& copy) {
  // set up feature wrapper
  if (!this->selfFeature.isNull()) {
    this->selfFeature->setScalarEntityAngle(this);
  }

  this->angle = copy.angle;

  return *this;
}

/*!
 * \brief ScalarEntityAngle::~ScalarEntityAngle
 */
ScalarEntityAngle::~ScalarEntityAngle() {}

/*!
 * \brief ScalarEntityAngle::getAngle
 * \return
 */
const double& ScalarEntityAngle::getAngle() const { return this->angle; }

/*!
 * \brief ScalarEntityAngle::setAngle
 * \param angle
 */
void ScalarEntityAngle::setAngle(const double& angle) {
  this->angle = angle;

  this->notify(&FeatureObserver::onGeomParametersChanged, this->id);
}

/*!
 * \brief ScalarEntityAngle::getUnknownParameters
 * \param displayUnits
 * \param displayDigits
 * \return
 */
QMap<GeometryParameters, QString> ScalarEntityAngle::getUnknownParameters(
    const QMap<DimensionType, UnitType>& displayUnits,
    const QMap<DimensionType, int>& displayDigits) const {
  QMap<GeometryParameters, QString> parameters;

  parameters.insert(
      GeometryParameters::eUnknownAngle,
      this->getDisplayAngle(
          displayUnits.value(DimensionType::eAngular, UnitType::eUnitRadiant),
          displayDigits.value(DimensionType::eAngular, 0)));

  return parameters;
}

/*!
 * \brief ScalarEntityAngle::applyUnknownParameters
 * \param parameters
 */
void ScalarEntityAngle::applyUnknownParameters(
    const QMap<GeometryParameters, double>& parameters) {
  // get current parameters
  double angle = this->angle;

  // update parameters
  QList<GeometryParameters> keys = parameters.keys();
  for (const GeometryParameters& key : std::as_const(keys)) {
    switch (key) {
      case GeometryParameters::eUnknownAngle:
        angle = parameters.value(GeometryParameters::eUnknownAngle);
        break;
      default:
        break;
    }
  }

  // update angle definition
  this->setAngle(angle);
}

/*!
 * \brief ScalarEntityAngle::recalc
 */
void ScalarEntityAngle::recalc() {
  Geometry::recalc();

  // reset angle definition if not solved and no nominal
  if (!this->isSolved && this->getOwnerDescription().has_value()) {
    this->angle = 0.0;
  }
}

/*!
 * \brief ScalarEntityAngle::getDisplayAngle
 * \param type
 * \param digits
 * \param showDiff
 * \return
 */
QString ScalarEntityAngle::getDisplayAngle(const UnitType& type,
                                           const int& digits,
                                           const bool& /*showDiff*/) const {
  if (this->isSolved) {
    return QString::number(convertFromDefault(this->angle, type), 'f', digits);
  }
  return QString("");
}
