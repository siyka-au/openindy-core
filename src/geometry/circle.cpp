#include "circle.h"

#include "featurewrapper.h"
#include "snapshot.h"

using namespace oi;
using namespace oi::math;

/*!
 * \brief Circle::Circle
 * \param parent
 */
Circle::Circle(QObject* parent) : Geometry(parent) {
  // set up feature wrapper
  if (!this->selfFeature.isNull()) {
    this->selfFeature->setCircle(this);
  }
}

/*!
 * \brief Circle::Circle
 * \param center
 * \param normal
 * \param radius
 * \param parent
 */
Circle::Circle(const Position& center, const Direction& normal,
               const Radius& radius, QObject* parent)
    : Geometry(parent) {
  // set up feature wrapper
  if (!this->selfFeature.isNull()) {
    this->selfFeature->setCircle(this);
  }

  this->setCircle(center, normal, radius);
}

/*!
 * \brief Circle::Circle
 * \param copy
 * \param parent
 */
Circle::Circle(const Circle& copy, QObject* parent) : Geometry(copy, parent) {
  // set up feature wrapper
  if (!this->selfFeature.isNull()) {
    this->selfFeature->setCircle(this);
  }

  this->xyz = copy.xyz;
  this->radius = copy.radius;
  this->ijk = copy.ijk;
}

/*!
 * \brief Circle::operator =
 * \param copy
 * \return
 */
Circle& Circle::operator=(const Circle& copy) {
  // set up feature wrapper
  if (!this->selfFeature.isNull()) {
    this->selfFeature->setCircle(this);
  }

  this->xyz = copy.xyz;
  this->radius = copy.radius;
  this->ijk = copy.ijk;

  return *this;
}

Circle::~Circle() {}

/*!
 * \brief Circle::hasDirection
 * \return
 */
bool Circle::hasDirection() const { return true; }

/*!
 * \brief Circle::hasPosition
 * \return
 */
bool Circle::hasPosition() const { return true; }

/*!
 * \brief Circle::hasRadius
 * \return
 */
bool Circle::hasRadius() const { return true; }

/*!
 * \brief Circle::setCircle
 * \param center
 * \param normal
 * \param radius
 */
void Circle::setCircle(const Position& center, const Direction& normal,
                       const Radius& radius) {
  // set the given parameters
  this->xyz = center;
  this->ijk = normal;
  this->radius = radius;

  this->notify(&FeatureObserver::onGeomParametersChanged, this->id);
}

/*!
 * \brief Circle::getUnknownParameters
 * \param displayUnits
 * \param displayDigits
 * \return
 */
QMap<GeometryParameters, QString> Circle::getUnknownParameters(
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
      GeometryParameters::eUnknownRadiusA,
      this->getDisplayRadiusA(
          displayUnits.value(DimensionType::eMetric, UnitType::eUnitMeter),
          displayDigits.value(DimensionType::eMetric, 0)));

  return parameters;
}

/*!
 * \brief Circle::applyUnknownParameters
 * \param parameters
 */
void Circle::applyUnknownParameters(
    const QMap<GeometryParameters, double>& parameters) {
  // get current parameters
  OiVec position = this->xyz.getVector();
  OiVec direction = this->ijk.getVector();
  double radius = this->radius.getRadius();

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
      case GeometryParameters::eUnknownRadiusA:
        radius = parameters.value(GeometryParameters::eUnknownRadiusA);
        break;
      default:
        break;
    }
  }

  // update circle definition
  direction.normalize();
  Position circlePosition(position);
  Direction circleDirection(direction);
  Radius circleRadius(radius);
  this->setCircle(circlePosition, circleDirection, circleRadius);
}

/*!
 * \brief Circle::recalc
 */
void Circle::recalc() {
  Geometry::recalc();

  // reset circle definition if not solved and no nominal
  if (!this->isSolved && this->getOwnerDescription().has_value()) {
    this->xyz.setVector(0.0, 0.0, 0.0);
    this->ijk.setVector(0.0, 0.0, 0.0);
    this->radius.setRadius(0.0);
  }
}
