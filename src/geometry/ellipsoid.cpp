#include "ellipsoid.h"

#include "featurewrapper.h"
#include "snapshot.h"

using namespace oi;
using namespace oi::math;

/*!
 * \brief Ellipsoid::Ellipsoid
 */
Ellipsoid::Ellipsoid() : Geometry() {
  // set up feature wrapper
  if (!this->selfFeature.isNull()) {
    this->selfFeature->setEllipsoid(this);
  }
}

/*!
 * \brief Ellipsoid::Ellipsoid
 * \param center
 * \param majorAxis
 * \param a
 * \param b
 * \param c
 * \param minorAxis
 */
Ellipsoid::Ellipsoid(const Position& center, const Direction& majorAxis,
                     const double& a, const double& b)
    : Geometry() {
  // set up feature wrapper
  if (!this->selfFeature.isNull()) {
    this->selfFeature->setEllipsoid(this);
  }

  this->setEllipsoid(center, majorAxis, a, b);
}

/*!
 * \brief Ellipsoid::Ellipsoid
 * \param copy
 */
Ellipsoid::Ellipsoid(const Ellipsoid& copy) : Geometry(copy) {
  // set up feature wrapper
  if (!this->selfFeature.isNull()) {
    this->selfFeature->setEllipsoid(this);
  }

  this->xyz = copy.xyz;
  this->ijk = copy.ijk;
  this->a = copy.a;
  this->b = copy.b;
}

/*!
 * \brief Ellipsoid::operator =
 * \param copy
 * \return
 */
Ellipsoid& Ellipsoid::operator=(const Ellipsoid& copy) {
  // set up feature wrapper
  if (!this->selfFeature.isNull()) {
    this->selfFeature->setEllipsoid(this);
  }

  this->xyz = copy.xyz;
  this->ijk = copy.ijk;
  this->a = copy.a;
  this->b = copy.b;

  return *this;
}

/*!
 * \brief Ellipsoid::~Ellipsoid
 */
Ellipsoid::~Ellipsoid() {}

/*!
 * \brief Ellipsoid::hasDirection
 * \return
 */
bool Ellipsoid::hasDirection() const { return true; }

/*!
 * \brief Ellipsoid::hasPosition
 * \return
 */
bool Ellipsoid::hasPosition() const { return true; }

/*!
 * \brief Ellipsoid::getA
 * Returns the semi-major axis of the ellipsoid
 * \return
 */
const double& Ellipsoid::getA() const { return this->a; }

/*!
 * \brief Ellipsoid::getB
 * Returns the semi-minor axis of the ellipsoid
 * \return
 */
const double& Ellipsoid::getB() const { return this->b; }

/*!
 * \brief Ellipsoid::setEllipsoid
 * \param center
 * \param majorAxis
 * \param a
 * \param b
 * \param c
 * \param minorAxis
 */
void Ellipsoid::setEllipsoid(const Position& center, const Direction& majorAxis,
                             const double& a, const double& b) {
  // set the given parameters
  this->xyz = center;
  this->ijk = majorAxis;
  this->a = a;
  this->b = b;

  this->notify(&FeatureObserver::onGeomParametersChanged, this->id);
}

/*!
 * \brief Ellipsoid::getUnknownParameters
 * \param displayUnits
 * \param displayDigits
 * \return
 */
QMap<GeometryParameters, QString> Ellipsoid::getUnknownParameters(
    const QMap<DimensionType, UnitType>& displayUnits,
    const QMap<DimensionType, int>& displayDigits) const {
  QMap<GeometryParameters, QString> parameters;

  parameters.insert(
      GeometryParameters::eUnknownX,
      this->getDisplayX(
          displayUnits.value(DimensionType::eMetric, UnitType::eUnitMeter),
          displayDigits.value(DimensionType::eMetric, 0)));
  parameters.insert(
      GeometryParameters::eUnknownY,
      this->getDisplayY(
          displayUnits.value(DimensionType::eMetric, UnitType::eUnitMeter),
          displayDigits.value(DimensionType::eMetric, 0)));
  parameters.insert(
      GeometryParameters::eUnknownZ,
      this->getDisplayZ(
          displayUnits.value(DimensionType::eMetric, UnitType::eUnitMeter),
          displayDigits.value(DimensionType::eMetric, 0)));
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
      this->getDisplayA(
          displayUnits.value(DimensionType::eMetric, UnitType::eUnitMeter),
          displayDigits.value(DimensionType::eMetric, 0)));
  parameters.insert(
      GeometryParameters::eUnknownB,
      this->getDisplayB(
          displayUnits.value(DimensionType::eMetric, UnitType::eUnitMeter),
          displayDigits.value(DimensionType::eMetric, 0)));

  return parameters;
}

/*!
 * \brief Ellipsoid::applyUnknownParameters
 * \param parameters
 */
void Ellipsoid::applyUnknownParameters(
    const QMap<GeometryParameters, double>& parameters) {
  // get current parameters
  OiVec position = this->xyz.getVector();
  OiVec direction = this->ijk.getVector();
  double a = this->a;
  double b = this->b;

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
      case GeometryParameters::eUnknownB:
        b = parameters.value(GeometryParameters::eUnknownB);
        break;
      default:
        break;
    }
  }

  // update ellipsoid definition
  direction.normalize();
  Position ellipsoidPosition(position);
  Direction ellipsoidDirection(direction);
  this->setEllipsoid(ellipsoidPosition, ellipsoidDirection, a, b);
}

/*!
 * \brief Ellipsoid::recalc
 */
void Ellipsoid::recalc() {
  Geometry::recalc();

  // reset ellipsoid definition if not solved and no nominal
  if (!this->isSolved && this->getOwnerDescription().has_value()) {
    this->xyz.setVector(0.0, 0.0, 0.0);
    this->ijk.setVector(0.0, 0.0, 0.0);
    this->a = 0.0;
    this->b = 0.0;
  }
}

/*!
 * \brief Ellipsoid::getDisplayA
 * \param type
 * \param digits
 * \param showDiff
 * \return
 */
QString Ellipsoid::getDisplayA(const UnitType& type, const int& digits,
                               const bool& /*showDiff*/) const {
  if (this->isSolved) {
    return QString::number(convertFromDefault(this->a, type), 'f', digits);
  }
  return QString("");
}

/*!
 * \brief Ellipsoid::getDisplayB
 * \param type
 * \param digits
 * \param showDiff
 * \return
 */
QString Ellipsoid::getDisplayB(const UnitType& type, const int& digits,
                               const bool& /*showDiff*/) const {
  if (this->isSolved) {
    return QString::number(convertFromDefault(this->b, type), 'f', digits);
  }
  return QString("");
}
