#include "cone.h"

#include "featurewrapper.h"
#include "snapshot.h"

using namespace oi;
using namespace oi::math;

/*!
 * \brief Cone::Cone
 * \param parent
 */
Cone::Cone(QObject* parent) : Geometry(parent) {
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
 * \param parent
 */
Cone::Cone(const Position& apex, const Direction& axis, const double& aperture,
           QObject* parent)
    : Geometry(parent) {
  // set up feature wrapper
  if (!this->selfFeature.isNull()) {
    this->selfFeature->setCone(this);
  }

  this->setCone(apex, axis, aperture);
}

/*!
 * \brief Cone::Cone
 * \param copy
 * \param parent
 */
Cone::Cone(const Cone& copy, QObject* parent) : Geometry(copy, parent) {
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

  emit this->geomParametersChanged(this->id);
}

/*!
 * \brief Cone::getUnknownParameters
 * \param displayUnits
 * \param displayDigits
 * \return
 */
QMap<GeometryParameters, QString> Cone::getUnknownParameters(
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
      GeometryParameters::eUnknownAperture,
      this->getDisplayAperture(
          displayUnits.value(DimensionType::eAngular, UnitType::eUnitRadiant),
          displayDigits.value(DimensionType::eAngular, 0)));

  return parameters;
}

/*!
 * \brief Cone::applyUnknownParameters
 * \param parameters
 */
void Cone::applyUnknownParameters(
    const QMap<GeometryParameters, double>& parameters) {
  // get current parameters
  OiVec position = this->xyz.getVector();
  OiVec direction = this->ijk.getVector();
  double aperture = this->getAperture();

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
      case GeometryParameters::eUnknownAperture:
        aperture = parameters.value(GeometryParameters::eUnknownAperture);
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
