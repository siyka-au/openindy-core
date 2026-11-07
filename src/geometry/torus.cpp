#include "torus.h"

#include "featurewrapper.h"
#include "snapshot.h"

using namespace oi;
using namespace oi::math;

/*!
 * \brief Torus::Torus
 */
Torus::Torus() : Geometry() {
  // set up feature wrapper
  if (!this->selfFeature.isNull()) {
    this->selfFeature->setTorus(this);
  }
}

/*!
 * \brief Torus::Torus
 * \param center
 * \param normal
 * \param radiusA
 * \param radiusB
 */
Torus::Torus(const Position& center, const Direction& normal,
             const Radius& radiusA, const Radius& radiusB)
    : Geometry() {
  // set up feature wrapper
  if (!this->selfFeature.isNull()) {
    this->selfFeature->setTorus(this);
  }

  this->setTorus(center, normal, radiusA, radiusB);
}

/*!
 * \brief Torus::Torus
 * \param copy
 */
Torus::Torus(const Torus& copy) : Geometry(copy) {
  // set up feature wrapper
  if (!this->selfFeature.isNull()) {
    this->selfFeature->setTorus(this);
  }

  this->xyz = copy.xyz;
  this->ijk = copy.ijk;
  this->radius = copy.radius;
  this->radiusB = copy.radiusB;
}

/*!
 * \brief Torus::operator =
 * \param copy
 * \return
 */
Torus& Torus::operator=(const Torus& copy) {
  // set up feature wrapper
  if (!this->selfFeature.isNull()) {
    this->selfFeature->setTorus(this);
  }

  this->xyz = copy.xyz;
  this->ijk = copy.ijk;
  this->radius = copy.radius;
  this->radiusB = copy.radiusB;

  return *this;
}

/*!
 * \brief Torus::~Torus
 */
Torus::~Torus() {}

/*!
 * \brief Torus::hasDirection
 * \return
 */
bool Torus::hasDirection() const { return true; }

/*!
 * \brief Torus::hasPosition
 * \return
 */
bool Torus::hasPosition() const { return true; }

/*!
 * \brief Torus::hasRadius
 * \return
 */
bool Torus::hasRadius() const { return true; }

/*!
 * \brief Torus::getSmallRadius
 * Returns the distance of the center curve to the torus surface (radiusA >
 * radiusB)
 * \return
 */
const Radius& Torus::getSmallRadius() const { return this->radiusB; }

/*!
 * \brief Torus::setTorus
 * \param center
 * \param normal
 * \param radiusA
 * \param radiusB
 */
void Torus::setTorus(const Position& center, const Direction& normal,
                     const Radius& radiusA, const Radius& radiusB) {
  // set the given parameters
  this->xyz = center;
  this->ijk = normal;
  this->radius = radiusA;
  this->radiusB = radiusB;

  this->notify(&FeatureObserver::onGeomParametersChanged, this->id);
}

/*!
 * \brief Torus::getUnknownParameters
 * \param displayUnits
 * \param displayDigits
 * \return
 */
QMap<GeometryParameters, QString> Torus::getUnknownParameters(
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
  parameters.insert(
      GeometryParameters::eUnknownRadiusA,
      this->getDisplayRadiusA(
          displayUnits.value(DimensionType::eMetric, UnitType::eUnitMeter),
          displayDigits.value(DimensionType::eMetric, 0)));

  return parameters;
}

/*!
 * \brief Torus::applyUnknownParameters
 * \param parameters
 */
void Torus::applyUnknownParameters(
    const QMap<GeometryParameters, double>& parameters) {
  // get current parameters
  OiVec position = this->xyz.getVector();
  OiVec direction = this->ijk.getVector();
  double radiusA = this->radius.getRadius();
  double radiusB = this->radiusB.getRadius();

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
        radiusA = parameters.value(GeometryParameters::eUnknownRadiusA);
        break;
      case GeometryParameters::eUnknownRadiusB:
        radiusB = parameters.value(GeometryParameters::eUnknownRadiusA);
        break;
      default:
        break;
    }
  }

  // update torus definition
  direction.normalize();
  Position torusPosition(position);
  Direction torusDirection(direction);
  Radius torusRadiusA(radiusA);
  Radius torusRadiusB(radiusB);
  this->setTorus(torusPosition, torusDirection, torusRadiusA, torusRadiusB);
}

/*!
 * \brief Torus::recalc
 */
void Torus::recalc() {
  Geometry::recalc();

  // reset torus definition if not solved and no nominal
  if (!this->isSolved && this->getOwnerDescription().has_value()) {
    this->xyz.setVector(0.0, 0.0, 0.0);
    this->ijk.setVector(0.0, 0.0, 0.0);
    this->radius.setRadius(0.0);
    this->radiusB.setRadius(0.0);
  }
}

/*!
 * \brief Torus::getDisplayRadiusB
 * \param type
 * \param digits
 * \param showDiff
 * \return
 */
QString Torus::getDisplayRadiusB(const UnitType& type, const int& digits,
                                 const bool& /*showDiff*/) const {
  if (this->isSolved) {
    return QString::number(convertFromDefault(this->radiusB.getRadius(), type),
                           'f', digits);
  }
  return QString("");
}
