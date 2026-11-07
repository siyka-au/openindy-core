#include "paraboloid.h"

#include "featurewrapper.h"
#include "snapshot.h"

using namespace oi;
using namespace oi::math;

/*!
 * \brief Paraboloid::Paraboloid
 */
Paraboloid::Paraboloid() : Geometry() {
  // set up feature wrapper
  if (!this->selfFeature.isNull()) {
    this->selfFeature->setParaboloid(this);
  }
}

/*!
 * \brief Paraboloid::Paraboloid
 * \param apex
 * \param axis
 * \param a
 */
Paraboloid::Paraboloid(const Position& apex, const Direction& axis,
                       const double& a)
    : Geometry() {
  // set up feature wrapper
  if (!this->selfFeature.isNull()) {
    this->selfFeature->setParaboloid(this);
  }

  this->setParaboloid(apex, axis, a);
}

/*!
 * \brief Paraboloid::Paraboloid
 * \param copy
 */
Paraboloid::Paraboloid(const Paraboloid& copy) : Geometry(copy) {
  // set up feature wrapper
  if (!this->selfFeature.isNull()) {
    this->selfFeature->setParaboloid(this);
  }

  this->xyz = copy.xyz;
  this->ijk = copy.ijk;
}

/*!
 * \brief Paraboloid::operator =
 * \param copy
 * \return
 */
Paraboloid& Paraboloid::operator=(const Paraboloid& copy) {
  // set up feature wrapper
  if (!this->selfFeature.isNull()) {
    this->selfFeature->setParaboloid(this);
  }

  this->xyz = copy.xyz;
  this->ijk = copy.ijk;

  return *this;
}

/*!
 * \brief Paraboloid::~Paraboloid
 */
Paraboloid::~Paraboloid() {}

/*!
 * \brief Paraboloid::hasDirection
 * \return
 */
bool Paraboloid::hasDirection() const { return true; }

/*!
 * \brief Paraboloid::hasPosition
 * \return
 */
bool Paraboloid::hasPosition() const { return true; }

/*!
 * \brief Paraboloid::getA
 * Returns the compression or streatch parameter
 * \return
 */
const double& Paraboloid::getA() const { return this->a; }

/*!
 * \brief Paraboloid::setParaboloid
 * \param apex
 * \param axis
 * \param a
 */
void Paraboloid::setParaboloid(const Position& apex, const Direction& axis,
                               const double& a) {
  // set the given parameters
  this->xyz = apex;
  this->ijk = axis;
  this->a = a;

  this->notify(&FeatureObserver::onGeomParametersChanged, this->id);
}

/*!
 * \brief Paraboloid::getUnknownParameters
 * \param displayUnits
 * \param displayDigits
 * \return
 */
QMap<GeometryParameters, QString> Paraboloid::getUnknownParameters(
    const QMap<DimensionType, UnitType>& displayUnits,
    const QMap<DimensionType, int>& displayDigits) const {
  QMap<GeometryParameters, QString> parameters;

  parameters.insert(
      GeometryParameters::eUnknownX,
      this->getDisplayX(
          displayUnits.value(DimensionType::eMetric, UnitType::eUnitMeter),
          displayDigits.value(DimensionType::eMetric), 0));
  parameters.insert(
      GeometryParameters::eUnknownY,
      this->getDisplayY(
          displayUnits.value(DimensionType::eMetric, UnitType::eUnitMeter),
          displayDigits.value(DimensionType::eMetric), 0));
  parameters.insert(
      GeometryParameters::eUnknownZ,
      this->getDisplayZ(
          displayUnits.value(DimensionType::eMetric, UnitType::eUnitMeter),
          displayDigits.value(DimensionType::eMetric), 0));
  parameters.insert(GeometryParameters::eUnknownPrimaryI,
                    this->getDisplayPrimaryI(
                        displayDigits.value(DimensionType::eDimensionless, 0)));
  parameters.insert(GeometryParameters::eUnknownPrimaryJ,
                    this->getDisplayPrimaryJ(
                        displayDigits.value(DimensionType::eDimensionless, 0)));
  parameters.insert(GeometryParameters::eUnknownPrimaryK,
                    this->getDisplayPrimaryK(
                        displayDigits.value(DimensionType::eDimensionless, 0)));
  parameters.insert(
      GeometryParameters::eUnknownA,
      this->getDisplayRadiusA(
          displayUnits.value(DimensionType::eMetric, UnitType::eUnitMeter),
          displayDigits.value(DimensionType::eMetric, 0)));

  return parameters;
}

/*!
 * \brief Paraboloid::applyUnknownParameters
 * \param parameters
 */
void Paraboloid::applyUnknownParameters(
    const QMap<GeometryParameters, double>& parameters) {
  // get current parameters
  OiVec position = this->xyz.getVector();
  OiVec direction = this->ijk.getVector();
  double a = this->a;

  // update parameters
  QList<GeometryParameters> keys = parameters.keys();
  for (const GeometryParameters& key : oi::snapshot(keys)) {
    switch (key) {
      case GeometryParameters::eUnknownX:
        position.setAt(0, parameters.value(GeometryParameters::eUnknownX));
        break;
      case GeometryParameters::eUnknownY:
        position.setAt(1, parameters.value(GeometryParameters::eUnknownY));
        break;
      case GeometryParameters::eUnknownZ:
        position.setAt(2, parameters.value(GeometryParameters::eUnknownZ));
        break;
      case GeometryParameters::eUnknownPrimaryI:
        direction.setAt(0,
                        parameters.value(GeometryParameters::eUnknownPrimaryI));
        break;
      case GeometryParameters::eUnknownPrimaryJ:
        direction.setAt(1,
                        parameters.value(GeometryParameters::eUnknownPrimaryJ));
        break;
      case GeometryParameters::eUnknownPrimaryK:
        direction.setAt(2,
                        parameters.value(GeometryParameters::eUnknownPrimaryK));
        break;
      case GeometryParameters::eUnknownA:
        a = parameters.value(GeometryParameters::eUnknownA);
        break;
      default:
        break;
    }
  }

  // update paraboloid definition
  direction.normalize();
  Position paraboloidPosition(position);
  Direction paraboloidDirection(direction);
  this->setParaboloid(paraboloidPosition, paraboloidDirection, a);
}

/*!
 * \brief Paraboloid::recalc
 */
void Paraboloid::recalc() {
  Geometry::recalc();

  // reset paraboloid definition if not solved and no nominal
  if (!this->isSolved && this->getOwnerDescription().has_value()) {
    this->xyz.setVector(0.0, 0.0, 0.0);
    this->ijk.setVector(0.0, 0.0, 0.0);
    this->a = 0.0;
  }
}

/*!
 * \brief Paraboloid::getDisplayA
 * \param type
 * \param digits
 * \param showDiff
 * \return
 */
QString Paraboloid::getDisplayA(const UnitType& type, const int& digits,
                                const bool& /*showDiff*/) const {
  if (this->isSolved) {
    return QString::number(convertFromDefault(this->a, type), 'f', digits);
  }
  return QString("");
}
