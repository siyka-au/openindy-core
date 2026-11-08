#include "plane.h"

#include <map>
#include <vector>

#include "containers.h"
#include "feature_wrapper.h"
#include "snapshot.h"

using namespace oi;
using namespace oi::math;

/*!
 * \brief Plane::Plane
 */
Plane::Plane() : Geometry() {
  // set up feature wrapper
  if (!this->selfFeature.isNull()) {
    this->selfFeature->setPlane(this);
  }
}

/*!
 * \brief Plane::Plane
 * \param xyz
 * \param normal
 */
Plane::Plane(const Position& xyz, const Direction& normal) : Geometry() {
  // set up feature wrapper
  if (!this->selfFeature.isNull()) {
    this->selfFeature->setPlane(this);
  }

  this->setPlane(xyz, normal);
}

/*!
 * \brief Plane::Plane
 * \param normal
 * \param d
 */
Plane::Plane(const Direction& normal, const double& d) : Geometry() {
  // set up feature wrapper
  if (!this->selfFeature.isNull()) {
    this->selfFeature->setPlane(this);
  }

  this->setPlane(normal, d);
}

/*!
 * \brief Plane::Plane
 * \param copy
 */
Plane::Plane(const Plane& copy) : Geometry(copy) {
  // set up feature wrapper
  if (!this->selfFeature.isNull()) {
    this->selfFeature->setPlane(this);
  }

  this->xyz = copy.xyz;
  this->ijk = copy.ijk;
  this->d = copy.d;
}

/*!
 * \brief Plane::operator =
 * \param copy
 * \return
 */
Plane& Plane::operator=(const Plane& copy) {
  // set up feature wrapper
  if (!this->selfFeature.isNull()) {
    this->selfFeature->setPlane(this);
  }

  this->xyz = copy.xyz;
  this->ijk = copy.ijk;
  this->d = copy.d;

  return *this;
}

/*!
 * \brief Plane::~Plane
 */
Plane::~Plane() {}

/*!
 * \brief Plane::hasDirection
 * \return
 */
bool Plane::hasDirection() const { return true; }

/*!
 * \brief Plane::hasPosition
 * \return
 */
bool Plane::hasPosition() const { return true; }

/*!
 * \brief Plane::getDistanceFromOrigin
 * Returns the smallest vertical distance of the plane from the origin
 * \return
 */
const double& Plane::getDistanceFromOrigin() const { return this->d; }

/*!
 * \brief Plane::setPlane
 * \param xyz
 * \param normal
 */
void Plane::setPlane(const Position& xyz, const Direction& normal) {
  // set the given parameters
  this->xyz.setVector(xyz.getVector());
  this->ijk = normal;

  // calculate the other parameters
  OiVec n0 = normal.getVector();
  n0.normalize();
  OiVec::dot(this->d, this->xyz.getVector(), n0);

  this->notify(&FeatureObserver::onGeomParametersChanged, this->id);
}

/*!
 * \brief Plane::setPlane
 * \param normal
 * \param d
 */
void Plane::setPlane(const Direction& /*normal*/, const double& d) {
  // set the given parameters
  this->ijk = ijk;
  this->d = d;

  // calculate the other parameters
  this->xyz.setVector(this->d * this->ijk.getVector());

  this->notify(&FeatureObserver::onGeomParametersChanged, this->id);
}

/*!
 * \brief Plane::getUnknownParameters
 * \param displayUnits
 * \param displayDigits
 * \return
 */
std::map<GeometryParameters, std::string> Plane::getUnknownParameters(
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

  return parameters;
}

/*!
 * \brief Plane::applyUnknownParameters
 * \param parameters
 */
void Plane::applyUnknownParameters(
    const std::map<GeometryParameters, double>& parameters) {
  // get current parameters
  OiVec position = this->xyz.getVector();
  OiVec direction = this->ijk.getVector();

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
      default:
        break;
    }
  }

  // update plane definition
  direction.normalize();
  Position planePosition(position);
  Direction planeDirection(direction);
  this->setPlane(planePosition, planeDirection);
}

/*!
 * \brief Plane::recalc
 */
void Plane::recalc() {
  Geometry::recalc();

  // reset plane definition if not solved and no nominal
  if (!this->isSolved && this->getOwnerDescription().has_value()) {
    this->xyz.setVector(0.0, 0.0, 0.0);
    this->ijk.setVector(0.0, 0.0, 0.0);
    this->d = 0.0;
  }
}
