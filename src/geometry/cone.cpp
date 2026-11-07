#include "cone.h"

#include <map>
#include <vector>

#include "containers.h"
#include "featurewrapper.h"
#include "snapshot.h"

using namespace oi;
using namespace oi::math;

/*!
 * \brief Cone::Cone
 */
Cone::Cone() : Geometry() {
  // set up feature wrapper
  if (!this->selfFeature.isNull()) {
    this->selfFeature->setCone(this);
  }
}

/*!
 * \brief Cone::Cone
 * \param apex
 * \param axis
 * \param aperture
 */
Cone::Cone(const Position& apex, const Direction& axis, const double& aperture)
    : Geometry() {
  // set up feature wrapper
  if (!this->selfFeature.isNull()) {
    this->selfFeature->setCone(this);
  }

  this->setCone(apex, axis, aperture);
}

/*!
 * \brief Cone::Cone
 * \param copy
 */
Cone::Cone(const Cone& copy) : Geometry(copy) {
  // set up feature wrapper
  if (!this->selfFeature.isNull()) {
    this->selfFeature->setCone(this);
  }

  this->xyz = copy.xyz;
  this->ijk = copy.ijk;
  this->aperture = copy.aperture;
}

/*!
 * \brief Cone::operator =
 * \param copy
 * \return
 */
Cone& Cone::operator=(const Cone& copy) {
  // set up feature wrapper
  if (!this->selfFeature.isNull()) {
    this->selfFeature->setCone(this);
  }

  this->xyz = copy.xyz;
  this->ijk = copy.ijk;
  this->aperture = copy.aperture;

  return *this;
}

/*!
 * \brief Cone::~Cone
 */
Cone::~Cone() {}

/*!
 * \brief Cone::hasDirection
 * \return
 */
bool Cone::hasDirection() const { return true; }

/*!
 * \brief Cone::hasPosition
 * \return
 */
bool Cone::hasPosition() const { return true; }

/*!
 * \brief Cone::getAperture
 * Returns the aperture of the cone
 * \return
 */
const double& Cone::getAperture() const { return this->aperture; }

/*!
 * \brief Cone::setCone
 * \param apex
 * \param axis
 * \param aperture
 */
void Cone::setCone(const Position& apex, const Direction& axis,
                   const double& aperture) {
  // set the given parameters
  this->xyz = apex;
  this->ijk = axis;
  this->aperture = aperture;

  this->notify(&FeatureObserver::onGeomParametersChanged, this->id);
}

/*!
 * \brief Cone::getUnknownParameters
 * \param displayUnits
 * \param displayDigits
 * \return
 */
std::map<GeometryParameters, QString> Cone::getUnknownParameters(
    const std::map<DimensionType, UnitType>& displayUnits,
    const std::map<DimensionType, int>& displayDigits) const {
  std::map<GeometryParameters, QString> parameters;

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
      GeometryParameters::eUnknownAperture,
      this->getDisplayAperture(
          oi::value(displayUnits, DimensionType::eAngular,
                    UnitType::eUnitRadiant),
          oi::value(displayDigits, DimensionType::eAngular, 0)));

  return parameters;
}

/*!
 * \brief Cone::applyUnknownParameters
 * \param parameters
 */
void Cone::applyUnknownParameters(
    const std::map<GeometryParameters, double>& parameters) {
  // get current parameters
  OiVec position = this->xyz.getVector();
  OiVec direction = this->ijk.getVector();
  double aperture = this->getAperture();

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
      case GeometryParameters::eUnknownAperture:
        aperture = oi::value(parameters, GeometryParameters::eUnknownAperture);
        break;
      default:
        break;
    }
  }

  // update cone definition
  direction.normalize();
  Position conePosition(position);
  Direction coneDirection(direction);
  this->setCone(conePosition, coneDirection, aperture);
}

/*!
 * \brief Cone::recalc
 */
void Cone::recalc() {
  Geometry::recalc();

  // reset cone definition if not solved and no nominal
  if (!this->isSolved && this->getOwnerDescription().has_value()) {
    this->xyz.setVector(0.0, 0.0, 0.0);
    this->ijk.setVector(0.0, 0.0, 0.0);
    this->aperture = 0.0;
  }
}
