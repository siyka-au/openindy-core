#include "sphere.h"

#include "featurewrapper.h"
#include "snapshot.h"

using namespace oi;
using namespace oi::math;

/*!
 * \brief Sphere::Sphere
 * \param parent
 */
Sphere::Sphere(QObject* parent) : Geometry(parent) {
  // set up feature wrapper
  if (!this->selfFeature.isNull()) {
    this->selfFeature->setSphere(this);
  }
}

/*!
 * \brief Sphere::Sphere
 * \param center
 * \param radius
 * \param parent
 */
Sphere::Sphere(const Position& center, const Radius& radius, QObject* parent)
    : Geometry(parent) {
  // set up feature wrapper
  if (!this->selfFeature.isNull()) {
    this->selfFeature->setSphere(this);
  }

  this->setSphere(center, radius);
}

/*!
 * \brief Sphere::Sphere
 * \param copy
 * \param parent
 */
Sphere::Sphere(const Sphere& copy, QObject* parent) : Geometry(copy, parent) {
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

  emit this->geomParametersChanged(this->id);
}

/*!
 * \brief Sphere::getUnknownParameters
 * \param displayUnits
 * \param displayDigits
 * \return
 */
QMap<GeometryParameters, QString> Sphere::getUnknownParameters(
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
  parameters.insert(
      GeometryParameters::eUnknownRadiusA,
      this->getDisplayRadiusA(
          displayUnits.value(DimensionType::eMetric, UnitType::eUnitMeter),
          displayDigits.value(DimensionType::eMetric, 0)));

  return parameters;
}

/*!
 * \brief Sphere::applyUnknownParameters
 * \param parameters
 */
void Sphere::applyUnknownParameters(
    const QMap<GeometryParameters, double>& parameters) {
  // get current parameters
  OiVec position = this->xyz.getVector();
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
      case GeometryParameters::eUnknownRadiusA:
        radius = parameters.value(GeometryParameters::eUnknownRadiusA);
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
