#include "cylinder.h"

#include <map>
#include <vector>

#include "containers.h"
#include "featurewrapper.h"
#include "snapshot.h"

using namespace oi;
using namespace oi::math;

/*!
 * \brief Cylinder::Cylinder
 */
Cylinder::Cylinder() : Geometry() {
  // set up feature wrapper
  if (!this->selfFeature.isNull()) {
    this->selfFeature->setCylinder(this);
  }
}

/*!
 * \brief Cylinder::Cylinder
 * \param axisPoint
 * \param axis
 * \param radius
 */
Cylinder::Cylinder(const Position& axisPoint, const Direction& axis,
                   const Radius& radius)
    : Geometry() {
  // set up feature wrapper
  if (!this->selfFeature.isNull()) {
    this->selfFeature->setCylinder(this);
  }

  this->setCylinder(axisPoint, axis, radius);
}

/*!
 * \brief Cylinder::Cylinder
 * \param copy
 */
Cylinder::Cylinder(const Cylinder& copy) : Geometry(copy) {
  // set up feature wrapper
  if (!this->selfFeature.isNull()) {
    this->selfFeature->setCylinder(this);
  }

  this->xyz = copy.xyz;
  this->ijk = copy.ijk;
  this->radius = copy.radius;
}

/*!
 * \brief Cylinder::operator =
 * \param copy
 * \return
 */
Cylinder& Cylinder::operator=(const Cylinder& copy) {
  // set up feature wrapper
  if (!this->selfFeature.isNull()) {
    this->selfFeature->setCylinder(this);
  }

  this->xyz = copy.xyz;
  this->ijk = copy.ijk;
  this->radius = copy.radius;

  return *this;
}

/*!
 * \brief Cylinder::~Cylinder
 */
Cylinder::~Cylinder() {}

/*!
 * \brief Cylinder::hasDirection
 * \return
 */
bool Cylinder::hasDirection() const { return true; }

/*!
 * \brief Cylinder::hasPosition
 * \return
 */
bool Cylinder::hasPosition() const { return true; }

/*!
 * \brief Cylinder::hasRadius
 * \return
 */
bool Cylinder::hasRadius() const { return true; }

/*!
 * \brief Cylinder::setCylinder
 * \param axisPoint
 * \param axis
 * \param radius
 */
void Cylinder::setCylinder(const Position& axisPoint, const Direction& axis,
                           const Radius& radius) {
  // set the given parameters
  this->xyz = axisPoint;
  this->ijk = axis;
  this->radius = radius;

  this->notify(&FeatureObserver::onGeomParametersChanged, this->id);
}

/*!
 * \brief Cylinder::getUnknownParameters
 * \param displayUnits
 * \param displayDigits
 * \return
 */
std::map<GeometryParameters, std::string> Cylinder::getUnknownParameters(
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
 * \brief Cylinder::applyUnknownParameters
 * \param parameters
 */
void Cylinder::applyUnknownParameters(
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

  // update cylinder definition
  direction.normalize();
  Position cylinderPosition(position);
  Direction cylinderDirection(direction);
  Radius cylinderRadius(radius);
  this->setCylinder(cylinderPosition, cylinderDirection, cylinderRadius);
}

/*!
 * \brief Cylinder::recalc
 */
void Cylinder::recalc() {
  Geometry::recalc();

  // reset cylinder definition if not solved and no nominal
  if (!this->isSolved && this->getOwnerDescription().has_value()) {
    this->xyz.setVector(0.0, 0.0, 0.0);
    this->ijk.setVector(0.0, 0.0, 0.0);
    this->radius.setRadius(0.0);
  }
}
