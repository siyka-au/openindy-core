#include "sphere.h"

#include <map>
#include <vector>

#include "containers.h"
#include "feature_wrapper.h"
#include "snapshot.h"

using namespace oi;
using namespace oi::math;

/*!
 * \brief Sphere::Sphere
 */
Sphere::Sphere() : Geometry() {
  // set up feature wrapper
  if (!this->selfFeature.isNull()) {
    this->selfFeature->setSphere(this);
  }
}

/*!
 * \brief Sphere::Sphere
 * \param center
 * \param radius
 */
Sphere::Sphere(const Position& center, const Radius& radius) : Geometry() {
  // set up feature wrapper
  if (!this->selfFeature.isNull()) {
    this->selfFeature->setSphere(this);
  }

  this->setSphere(center, radius);
}

/*!
 * \brief Sphere::Sphere
 * \param copy
 */
Sphere::Sphere(const Sphere& copy) : Geometry(copy) {
  // set up feature wrapper
  if (!this->selfFeature.isNull()) {
    this->selfFeature->setSphere(this);
  }

  this->xyz = copy.xyz;
  this->radius = copy.radius;
}

/*!
 * \brief Sphere::operator =
 * \param copy
 * \return
 */
Sphere& Sphere::operator=(const Sphere& copy) {
  // set up feature wrapper
  if (!this->selfFeature.isNull()) {
    this->selfFeature->setSphere(this);
  }

  this->xyz = copy.xyz;
  this->radius = copy.radius;

  return *this;
}

/*!
 * \brief Sphere::~Sphere
 */
Sphere::~Sphere() {}

/*!
 * \brief Sphere::hasPosition
 * \return
 */
bool Sphere::hasPosition() const { return true; }

/*!
 * \brief Sphere::hasRadius
 * \return
 */
bool Sphere::hasRadius() const { return true; }

/*!
 * \brief Sphere::setSphere
 * \param center
 * \param radius
 */
void Sphere::setSphere(const Position& center, const Radius& radius) {
  // set the given parameters
  this->xyz = center;
  this->radius = radius;

  this->notify(&FeatureObserver::onGeomParametersChanged, this->id);
}

/*!
 * \brief Sphere::getUnknownParameters
 * \param displayUnits
 * \param displayDigits
 * \return
 */
std::map<GeometryParameters, std::string> Sphere::getUnknownParameters(
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
      GeometryParameters::eUnknownRadiusA,
      this->getDisplayRadiusA(
          oi::value(displayUnits, DimensionType::eMetric, UnitType::eUnitMeter),
          oi::value(displayDigits, DimensionType::eMetric, 0)));

  return parameters;
}

/*!
 * \brief Sphere::applyUnknownParameters
 * \param parameters
 */
void Sphere::applyUnknownParameters(
    const std::map<GeometryParameters, double>& parameters) {
  // get current parameters
  OiVec position = this->xyz.getVector();
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
      case GeometryParameters::eUnknownRadiusA:
        radius = oi::value(parameters, GeometryParameters::eUnknownRadiusA);
        break;
      default:
        break;
    }
  }

  // update sphere definition
  Position spherePosition(position);
  Radius sphereRadius(radius);
  this->setSphere(spherePosition, sphereRadius);
}

/*!
 * \brief Sphere::recalc
 */
void Sphere::recalc() {
  Geometry::recalc();

  // reset sphere definition if not solved and no nominal
  if (!this->isSolved && this->getOwnerDescription().has_value()) {
    this->xyz.setVector(0.0, 0.0, 0.0);
    this->radius.setRadius(0.0);
  }
}
