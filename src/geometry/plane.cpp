#include "plane.h"

#include "featurewrapper.h"
#include "snapshot.h"

using namespace oi;
using namespace oi::math;

/*!
 * \brief Plane::Plane
 * \param parent
 */
Plane::Plane(QObject* parent) : Geometry(parent) {
  // set up feature wrapper
  if (!this->selfFeature.isNull()) {
    this->selfFeature->setPlane(this);
  }
}

/*!
 * \brief Plane::Plane
 * \param xyz
 * \param normal
 * \param parent
 */
Plane::Plane(const Position& xyz, const Direction& normal, QObject* parent)
    : Geometry(parent) {
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
 * \param parent
 */
Plane::Plane(const Direction& normal, const double& d, QObject* parent)
    : Geometry(parent) {
  // set up feature wrapper
  if (!this->selfFeature.isNull()) {
    this->selfFeature->setPlane(this);
  }

  this->setPlane(normal, d);
}

/*!
 * \brief Plane::Plane
 * \param copy
 * \param parent
 */
Plane::Plane(const Plane& copy, QObject* parent) : Geometry(copy, parent) {
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

  emit this->geomParametersChanged(this->id);
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

  emit this->geomParametersChanged(this->id);
}

/*!
 * \brief Plane::getUnknownParameters
 * \param displayUnits
 * \param displayDigits
 * \return
 */
QMap<GeometryParameters, QString> Plane::getUnknownParameters(
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

  return parameters;
}

/*!
 * \brief Plane::applyUnknownParameters
 * \param parameters
 */
void Plane::applyUnknownParameters(
    const QMap<GeometryParameters, double>& parameters) {
  // get current parameters
  OiVec position = this->xyz.getVector();
  OiVec direction = this->ijk.getVector();

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
