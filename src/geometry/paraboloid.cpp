#include "paraboloid.h"

#include <map>
#include <vector>

#include "containers.h"
#include "feature_wrapper.h"
#include "snapshot.h"
#include "string_util.h"

using namespace oi;
using namespace oi::math;

/*!
 * \brief Paraboloid::Paraboloid
 */
Paraboloid::Paraboloid() : Geometry() {
  // set up feature wrapper
  if (!this->selfFeature.isNull()) {
    this->selfFeature->setParaboloid(this);
  }
}

/*!
 * \brief Paraboloid::Paraboloid
 * \param apex
 * \param axis
 * \param a
 */
Paraboloid::Paraboloid(const Position& apex, const Direction& axis,
                       const double& a)
    : Geometry() {
  // set up feature wrapper
  if (!this->selfFeature.isNull()) {
    this->selfFeature->setParaboloid(this);
  }

  this->setParaboloid(apex, axis, a);
}

/*!
 * \brief Paraboloid::Paraboloid
 * \param copy
 */
Paraboloid::Paraboloid(const Paraboloid& copy) : Geometry(copy) {
  // set up feature wrapper
  if (!this->selfFeature.isNull()) {
    this->selfFeature->setParaboloid(this);
  }

  this->xyz = copy.xyz;
  this->ijk = copy.ijk;
}

/*!
 * \brief Paraboloid::operator =
 * \param copy
 * \return
 */
Paraboloid& Paraboloid::operator=(const Paraboloid& copy) {
  // set up feature wrapper
  if (!this->selfFeature.isNull()) {
    this->selfFeature->setParaboloid(this);
  }

  this->xyz = copy.xyz;
  this->ijk = copy.ijk;

  return *this;
}

/*!
 * \brief Paraboloid::~Paraboloid
 */
Paraboloid::~Paraboloid() {}

/*!
 * \brief Paraboloid::hasDirection
 * \return
 */
bool Paraboloid::hasDirection() const { return true; }

/*!
 * \brief Paraboloid::hasPosition
 * \return
 */
bool Paraboloid::hasPosition() const { return true; }

/*!
 * \brief Paraboloid::getA
 * Returns the compression or streatch parameter
 * \return
 */
const double& Paraboloid::getA() const { return this->a; }

/*!
 * \brief Paraboloid::setParaboloid
 * \param apex
 * \param axis
 * \param a
 */
void Paraboloid::setParaboloid(const Position& apex, const Direction& axis,
                               const double& a) {
  // set the given parameters
  this->xyz = apex;
  this->ijk = axis;
  this->a = a;

  this->notify(&FeatureObserver::onGeomParametersChanged, this->id);
}

/*!
 * \brief Paraboloid::getUnknownParameters
 * \param displayUnits
 * \param displayDigits
 * \return
 */
std::map<GeometryParameters, std::string> Paraboloid::getUnknownParameters(
    const std::map<DimensionType, UnitType>& displayUnits,
    const std::map<DimensionType, int>& displayDigits) const {
  std::map<GeometryParameters, std::string> parameters;

  parameters.insert_or_assign(
      GeometryParameters::eUnknownX,
      this->getDisplayX(
          oi::value(displayUnits, DimensionType::eMetric, UnitType::eUnitMeter),
          oi::value(displayDigits, DimensionType::eMetric), 0));
  parameters.insert_or_assign(
      GeometryParameters::eUnknownY,
      this->getDisplayY(
          oi::value(displayUnits, DimensionType::eMetric, UnitType::eUnitMeter),
          oi::value(displayDigits, DimensionType::eMetric), 0));
  parameters.insert_or_assign(
      GeometryParameters::eUnknownZ,
      this->getDisplayZ(
          oi::value(displayUnits, DimensionType::eMetric, UnitType::eUnitMeter),
          oi::value(displayDigits, DimensionType::eMetric), 0));
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
      GeometryParameters::eUnknownA,
      this->getDisplayRadiusA(
          oi::value(displayUnits, DimensionType::eMetric, UnitType::eUnitMeter),
          oi::value(displayDigits, DimensionType::eMetric, 0)));

  return parameters;
}

/*!
 * \brief Paraboloid::applyUnknownParameters
 * \param parameters
 */
void Paraboloid::applyUnknownParameters(
    const std::map<GeometryParameters, double>& parameters) {
  // get current parameters
  OiVec position = this->xyz.getVector();
  OiVec direction = this->ijk.getVector();
  double a = this->a;

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
      case GeometryParameters::eUnknownA:
        a = oi::value(parameters, GeometryParameters::eUnknownA);
        break;
      default:
        break;
    }
  }

  // update paraboloid definition
  direction.normalize();
  Position paraboloidPosition(position);
  Direction paraboloidDirection(direction);
  this->setParaboloid(paraboloidPosition, paraboloidDirection, a);
}

/*!
 * \brief Paraboloid::recalc
 */
void Paraboloid::recalc() {
  Geometry::recalc();

  // reset paraboloid definition if not solved and no nominal
  if (!this->isSolved && this->getOwnerDescription().has_value()) {
    this->xyz.setVector(0.0, 0.0, 0.0);
    this->ijk.setVector(0.0, 0.0, 0.0);
    this->a = 0.0;
  }
}

/*!
 * \brief Paraboloid::getDisplayA
 * \param type
 * \param digits
 * \param showDiff
 * \return
 */
std::string Paraboloid::getDisplayA(const UnitType& type, const int& digits,
                                    const bool& /*showDiff*/) const {
  if (this->isSolved) {
    return oi::number(convertFromDefault(this->a, type), 'f', digits);
  }
  return std::string("");
}
