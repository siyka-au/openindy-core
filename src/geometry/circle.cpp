#include "circle.h"

#include <map>
#include <vector>

#include "containers.h"
#include "feature_wrapper.h"
#include "snapshot.h"

using namespace oi;
using namespace oi::math;

/*!
 * \brief Circle::Circle
 */
Circle::Circle() : Geometry() {
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
 */
Circle::Circle(const Position& center, const Direction& normal,
               const Radius& radius)
    : Geometry() {
  // set up feature wrapper
  if (!this->selfFeature.isNull()) {
    this->selfFeature->setCircle(this);
  }

  this->setCircle(center, normal, radius);
}

/*!
 * \brief Circle::Circle
 * \param copy
 */
Circle::Circle(const Circle& copy) : Geometry(copy) {
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
std::map<GeometryParameters, std::string> Circle::getUnknownParameters(
    const std::map<DimensionType, UnitType>& displayUnits,
    const std::map<DimensionType, int>& displayDigits) const {
  std::map<GeometryParameters, std::string> parameters;

  parameters.insert_or_assign(
      GeometryParameters::eUnknownX,
      this->getDisplayX(
          oi::value(displayUnits, DimensionType::eMetric, UnitType::eUnitMeter),
          oi::value(displayDigits, DimensionType::eMetric, 0)));
  parameters.insert_or_assign(
      GeometryParameters::eUnknownY,
      this->getDisplayY(
          oi::value(displayUnits, DimensionType::eMetric, UnitType::eUnitMeter),
          oi::value(displayDigits, DimensionType::eMetric, 0)));
  parameters.insert_or_assign(
      GeometryParameters::eUnknownZ,
      this->getDisplayZ(
          oi::value(displayUnits, DimensionType::eMetric, UnitType::eUnitMeter),
          oi::value(displayDigits, DimensionType::eMetric, 0)));
  parameters.insert_or_assign(
      GeometryParameters::eUnknownPrimaryI,
      this->getDisplayPrimaryI(
          oi::value(displayDigits, DimensionType::eDimensionless, 0)));
  parameters.insert_or_assign(
      GeometryParameters::eUnknownPrimaryJ,
      this->getDisplayPrimaryJ(
          oi::value(displayDigits, DimensionType::eDimensionless, 0)));
  parameters.insert_or_assign(
      GeometryParameters::eUnknownPrimaryK,
      this->getDisplayPrimaryK(
          oi::value(displayDigits, DimensionType::eDimensionless, 0)));
  parameters.insert_or_assign(
      GeometryParameters::eUnknownRadiusA,
      this->getDisplayRadiusA(
          oi::value(displayUnits, DimensionType::eMetric, UnitType::eUnitMeter),
          oi::value(displayDigits, DimensionType::eMetric, 0)));

  return parameters;
}

/*!
 * \brief Circle::applyUnknownParameters
 * \param parameters
 */
void Circle::applyUnknownParameters(
    const std::map<GeometryParameters, double>& parameters) {
  // get current parameters
  OiVec position = this->xyz.getVector();
  OiVec direction = this->ijk.getVector();
  double radius = this->radius.getRadius();

  // update parameters
  std::vector<GeometryParameters> keys = oi::keys(parameters);
  for (const GeometryParameters& key : oi::snapshot(keys)) {
    switch (key) {
      case GeometryParameters::eUnknownX:
        position.setAt(0, oi::value(parameters, GeometryParameters::eUnknownX));
        break;
      case GeometryParameters::eUnknownY:
        position.setAt(1, oi::value(parameters, GeometryParameters::eUnknownY));
        break;
      case GeometryParameters::eUnknownZ:
        position.setAt(2, oi::value(parameters, GeometryParameters::eUnknownZ));
        break;
      case GeometryParameters::eUnknownPrimaryI:
        direction.setAt(
            0, oi::value(parameters, GeometryParameters::eUnknownPrimaryI));
        break;
      case GeometryParameters::eUnknownPrimaryJ:
        direction.setAt(
            1, oi::value(parameters, GeometryParameters::eUnknownPrimaryJ));
        break;
      case GeometryParameters::eUnknownPrimaryK:
        direction.setAt(
            2, oi::value(parameters, GeometryParameters::eUnknownPrimaryK));
        break;
      case GeometryParameters::eUnknownRadiusA:
        radius = oi::value(parameters, GeometryParameters::eUnknownRadiusA);
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
