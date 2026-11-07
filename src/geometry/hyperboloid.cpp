#include "hyperboloid.h"

#include <map>
#include <vector>

#include "containers.h"
#include "featurewrapper.h"
#include "snapshot.h"

using namespace oi;
using namespace oi::math;

/*!
 * \brief Hyperboloid::Hyperboloid
 */
Hyperboloid::Hyperboloid() : Geometry() {
  // set up feature wrapper
  if (!this->selfFeature.isNull()) {
    this->selfFeature->setHyperboloid(this);
  }
}

/*!
 * \brief Hyperboloid::Hyperboloid
 * \param center
 * \param axis
 * \param a
 * \param c
 */
Hyperboloid::Hyperboloid(const Position& center, const Direction& axis,
                         const double& a, const double& c)
    : Geometry() {
  // set up feature wrapper
  if (!this->selfFeature.isNull()) {
    this->selfFeature->setHyperboloid(this);
  }

  this->setHyperboloid(center, axis, a, c);
}

/*!
 * \brief Hyperboloid::Hyperboloid
 * \param copy
 */
Hyperboloid::Hyperboloid(const Hyperboloid& copy) : Geometry(copy) {
  // set up feature wrapper
  if (!this->selfFeature.isNull()) {
    this->selfFeature->setHyperboloid(this);
  }

  this->xyz = copy.xyz;
  this->ijk = copy.ijk;
  this->a = copy.a;
  this->c = copy.c;
}

/*!
 * \brief Hyperboloid::operator =
 * \param copy
 * \return
 */
Hyperboloid& Hyperboloid::operator=(const Hyperboloid& copy) {
  // set up feature wrapper
  if (!this->selfFeature.isNull()) {
    this->selfFeature->setHyperboloid(this);
  }

  this->xyz = copy.xyz;
  this->ijk = copy.ijk;
  this->a = copy.a;
  this->c = copy.c;

  return *this;
}

/*!
 * \brief Hyperboloid::~Hyperboloid
 */
Hyperboloid::~Hyperboloid() {}

/*!
 * \brief Hyperboloid::hasDirection
 * \return
 */
bool Hyperboloid::hasDirection() const { return true; }

/*!
 * \brief Hyperboloid::hasPosition
 * \return
 */
bool Hyperboloid::hasPosition() const { return true; }

/*!
 * \brief Hyperboloid::getA
 * \return
 */
const double& Hyperboloid::getA() const { return this->a; }

/*!
 * \brief Hyperboloid::getC
 * \return
 */
const double& Hyperboloid::getC() const { return this->c; }

/*!
 * \brief Hyperboloid::setHyperboloid
 * \param center
 * \param axis
 * \param a
 * \param c
 */
void Hyperboloid::setHyperboloid(const Position& center, const Direction& axis,
                                 const double& a, const double& c) {
  // set the given parameters
  this->xyz = center;
  this->ijk = axis;
  this->a = a;
  this->c = c;

  this->notify(&FeatureObserver::onGeomParametersChanged, this->id);
}

/*!
 * \brief Hyperboloid::getUnknownParameters
 * \param displayUnits
 * \param displayDigits
 * \return
 */
std::map<GeometryParameters, QString> Hyperboloid::getUnknownParameters(
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
      GeometryParameters::eUnknownA,
      this->getDisplayA(
          oi::value(displayUnits, DimensionType::eMetric, UnitType::eUnitMeter),
          oi::value(displayDigits, DimensionType::eMetric, 0)));
  parameters.insert_or_assign(
      GeometryParameters::eUnknownC,
      this->getDisplayC(
          oi::value(displayUnits, DimensionType::eMetric, UnitType::eUnitMeter),
          oi::value(displayDigits, DimensionType::eMetric, 0)));

  return parameters;
}

/*!
 * \brief Hyperboloid::applyUnknownParameters
 * \param parameters
 */
void Hyperboloid::applyUnknownParameters(
    const std::map<GeometryParameters, double>& parameters) {
  // get current parameters
  OiVec position = this->xyz.getVector();
  OiVec direction = this->ijk.getVector();
  double a = this->a;
  double c = this->c;

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
      case GeometryParameters::eUnknownC:
        c = oi::value(parameters, GeometryParameters::eUnknownC);
        break;
      default:
        break;
    }
  }

  // update hyperboloid definition
  direction.normalize();
  Position hyperboloidPosition(position);
  Direction hyperboloidDirection(direction);
  this->setHyperboloid(hyperboloidPosition, hyperboloidDirection, a, c);
}

/*!
 * \brief Hyperboloid::recalc
 */
void Hyperboloid::recalc() {
  Geometry::recalc();

  // reset hyperboloid definition if not solved and no nominal
  if (!this->isSolved && this->getOwnerDescription().has_value()) {
    this->xyz.setVector(0.0, 0.0, 0.0);
    this->ijk.setVector(0.0, 0.0, 0.0);
    this->a = 0.0;
    this->c = 0.0;
  }
}

/*!
 * \brief Hyperboloid::getDisplayA
 * \param type
 * \param digits
 * \param showDiff
 * \return
 */
QString Hyperboloid::getDisplayA(const UnitType& type, const int& digits,
                                 const bool& /*showDiff*/) const {
  if (this->isSolved) {
    return QString::number(convertFromDefault(this->a, type), 'f', digits);
  }
  return QString("");
}

/*!
 * \brief Hyperboloid::getDisplayC
 * \param type
 * \param digits
 * \param showDiff
 * \return
 */
QString Hyperboloid::getDisplayC(const UnitType& type, const int& digits,
                                 const bool& /*showDiff*/) const {
  if (this->isSolved) {
    return QString::number(convertFromDefault(this->c, type), 'f', digits);
  }
  return QString("");
}
