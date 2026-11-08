#include "torus.h"

#include <map>
#include <vector>

#include "containers.h"
#include "feature_wrapper.h"
#include "snapshot.h"
#include "string_util.h"

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
std::map<GeometryParameters, std::string> Torus::getUnknownParameters(
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
  parameters.insert_or_assign(
      GeometryParameters::eUnknownRadiusA,
      this->getDisplayRadiusA(
          oi::value(displayUnits, DimensionType::eMetric, UnitType::eUnitMeter),
          oi::value(displayDigits, DimensionType::eMetric, 0)));

  return parameters;
}

/*!
 * \brief Torus::applyUnknownParameters
 * \param parameters
 */
void Torus::applyUnknownParameters(
    const std::map<GeometryParameters, double>& parameters) {
  // get current parameters
  OiVec position = this->xyz.getVector();
  OiVec direction = this->ijk.getVector();
  double radiusA = this->radius.getRadius();
  double radiusB = this->radiusB.getRadius();

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
        radiusA = oi::value(parameters, GeometryParameters::eUnknownRadiusA);
        break;
      case GeometryParameters::eUnknownRadiusB:
        radiusB = oi::value(parameters, GeometryParameters::eUnknownRadiusA);
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
std::string Torus::getDisplayRadiusB(const UnitType& type, const int& digits,
                                     const bool& /*showDiff*/) const {
  if (this->isSolved) {
    return oi::number(convertFromDefault(this->radiusB.getRadius(), type), 'f',
                      digits);
  }
  return std::string("");
}
