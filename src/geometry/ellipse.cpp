#include "ellipse.h"

#include <map>
#include <vector>

#include "containers.h"
#include "featurewrapper.h"
#include "snapshot.h"
#include "stringutil.h"

using namespace oi;
using namespace oi::math;

/*!
 * \brief Ellipse::Ellipse
 */
Ellipse::Ellipse() : Geometry() {
  // set up feature wrapper
  if (!this->selfFeature.isNull()) {
    this->selfFeature->setEllipse(this);
  }
}

/*!
 * \brief Ellipse::Ellipse
 * \param center
 * \param normal
 * \param a
 * \param b
 * \param semiMajorAxis
 */
Ellipse::Ellipse(const Position& center, const Direction& normal,
                 const double& a, const double& b,
                 const Direction& semiMajorAxis)
    : Geometry() {
  // set up feature wrapper
  if (!this->selfFeature.isNull()) {
    this->selfFeature->setEllipse(this);
  }

  this->setEllipse(center, normal, a, b, semiMajorAxis);
}

/*!
 * \brief Ellipse::Ellipse
 * \param copy
 */
Ellipse::Ellipse(const Ellipse& copy) : Geometry(copy) {
  // set up feature wrapper
  if (!this->selfFeature.isNull()) {
    this->selfFeature->setEllipse(this);
  }

  this->xyz = copy.xyz;
  this->ijk = copy.ijk;
  this->a = copy.a;
  this->b = copy.b;
  this->semiMajorAxis = copy.semiMajorAxis;
}

/*!
 * \brief Ellipse::operator =
 * \param copy
 * \return
 */
Ellipse& Ellipse::operator=(const Ellipse& copy) {
  // set up feature wrapper
  if (!this->selfFeature.isNull()) {
    this->selfFeature->setEllipse(this);
  }

  this->xyz = copy.xyz;
  this->ijk = copy.ijk;
  this->a = copy.a;
  this->b = copy.b;
  this->semiMajorAxis = copy.semiMajorAxis;

  return *this;
}

/*!
 * \brief Ellipse::~Ellipse
 */
Ellipse::~Ellipse() {}

/*!
 * \brief Ellipse::hasDirection
 * \return
 */
bool Ellipse::hasDirection() const { return true; }

/*!
 * \brief Ellipse::hasPosition
 * \return
 */
bool Ellipse::hasPosition() const { return true; }

/*!
 * \brief Ellipse::getA
 * Returns the semi-major axis of the ellipse
 * \return
 */
const double& Ellipse::getA() const { return this->a; }

/*!
 * \brief Ellipse::getB
 * Returns the semi-minor axis of the ellipse
 * \return
 */
const double& Ellipse::getB() const { return this->b; }

/*!
 * \brief Ellipse::getSemiMajorAxisDirection
 * Returns the direction of the semi-major axis
 * \return
 */
const Direction& Ellipse::getSemiMajorAxisDirection() const {
  return this->semiMajorAxis;
}

/*!
 * \brief Ellipse::setEllipse
 * \param center
 * \param normal
 * \param a
 * \param b
 * \param semiMajorAxis
 */
void Ellipse::setEllipse(const Position& center, const Direction& normal,
                         const double& a, const double& b,
                         const Direction& semiMajorAxis) {
  // set the given parameters
  this->xyz = center;
  this->ijk = normal;
  this->a = a;
  this->b = b;
  this->semiMajorAxis = semiMajorAxis;

  this->notify(&FeatureObserver::onGeomParametersChanged, this->id);
}

/*!
 * \brief Ellipse::getUnknownParameters
 * \param displayUnits
 * \param displayDigits
 * \return
 */
std::map<GeometryParameters, std::string> Ellipse::getUnknownParameters(
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
      GeometryParameters::eUnknownA,
      this->getDisplayA(
          oi::value(displayUnits, DimensionType::eMetric, UnitType::eUnitMeter),
          oi::value(displayDigits, DimensionType::eMetric, 0)));
  parameters.insert_or_assign(
      GeometryParameters::eUnknownB,
      this->getDisplayB(
          oi::value(displayUnits, DimensionType::eMetric, UnitType::eUnitMeter),
          oi::value(displayDigits, DimensionType::eMetric, 0)));
  parameters.insert_or_assign(
      GeometryParameters::eUnknownSecondaryI,
      this->getDisplaySecondaryI(
          oi::value(displayDigits, DimensionType::eDimensionless, 0)));
  parameters.insert_or_assign(
      GeometryParameters::eUnknownSecondaryJ,
      this->getDisplaySecondaryJ(
          oi::value(displayDigits, DimensionType::eDimensionless, 0)));
  parameters.insert_or_assign(
      GeometryParameters::eUnknownSecondaryK,
      this->getDisplaySecondaryK(
          oi::value(displayDigits, DimensionType::eDimensionless, 0)));

  return parameters;
}

/*!
 * \brief Ellipse::applyUnknownParameters
 * \param parameters
 */
void Ellipse::applyUnknownParameters(
    const std::map<GeometryParameters, double>& parameters) {
  // get current parameters
  OiVec position = this->xyz.getVector();
  OiVec directionA = this->ijk.getVector();
  OiVec directionB = this->semiMajorAxis.getVector();
  double a = this->a;
  double b = this->b;

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
        directionA.setAt(
            0, oi::value(parameters, GeometryParameters::eUnknownPrimaryI));
        break;
      case GeometryParameters::eUnknownPrimaryJ:
        directionA.setAt(
            1, oi::value(parameters, GeometryParameters::eUnknownPrimaryJ));
        break;
      case GeometryParameters::eUnknownPrimaryK:
        directionA.setAt(
            2, oi::value(parameters, GeometryParameters::eUnknownPrimaryK));
        break;
      case GeometryParameters::eUnknownA:
        a = oi::value(parameters, GeometryParameters::eUnknownA);
        break;
      case GeometryParameters::eUnknownB:
        b = oi::value(parameters, GeometryParameters::eUnknownB);
        break;
      case GeometryParameters::eUnknownSecondaryI:
        directionB.setAt(
            0, oi::value(parameters, GeometryParameters::eUnknownSecondaryI));
        break;
      case GeometryParameters::eUnknownSecondaryJ:
        directionB.setAt(
            1, oi::value(parameters, GeometryParameters::eUnknownSecondaryJ));
        break;
      case GeometryParameters::eUnknownSecondaryK:
        directionB.setAt(
            2, oi::value(parameters, GeometryParameters::eUnknownSecondaryK));
        break;
      default:
        break;
    }
  }

  // update ellipse definition
  directionA.normalize();
  directionB.normalize();
  Position ellipsePosition(position);
  Direction ellipseDirectionA(directionA);
  Direction ellipseDirectionB(directionB);
  this->setEllipse(ellipsePosition, ellipseDirectionA, a, b, ellipseDirectionB);
}

/*!
 * \brief Ellipse::recalc
 */
void Ellipse::recalc() {
  Geometry::recalc();

  // reset ellipse definition if not solved and no nominal
  if (!this->isSolved && this->getOwnerDescription().has_value()) {
    this->xyz.setVector(0.0, 0.0, 0.0);
    this->ijk.setVector(0.0, 0.0, 0.0);
    this->a = 0.0;
    this->b = 0.0;
    this->semiMajorAxis.setVector(0.0, 0.0, 0.0);
  }
}

/*!
 * \brief Ellipse::getDisplaySecondaryI
 * \param digits
 * \param showDiff
 * \return
 */
std::string Ellipse::getDisplaySecondaryI(const int& digits,
                                          const bool& /*showDiff*/) const {
  if (this->isSolved) {
    return oi::number(this->semiMajorAxis.getVector().getAt(0), 'f', digits);
  }
  return std::string("");
}

/*!
 * \brief Ellipse::getDisplaySecondaryJ
 * \param digits
 * \param showDiff
 * \return
 */
std::string Ellipse::getDisplaySecondaryJ(const int& digits,
                                          const bool& /*showDiff*/) const {
  if (this->isSolved) {
    return oi::number(this->semiMajorAxis.getVector().getAt(1), 'f', digits);
  }
  return std::string("");
}

/*!
 * \brief Ellipse::getDisplaySecondaryK
 * \param digits
 * \param showDiff
 * \return
 */
std::string Ellipse::getDisplaySecondaryK(const int& digits,
                                          const bool& /*showDiff*/) const {
  if (this->isSolved) {
    return oi::number(this->semiMajorAxis.getVector().getAt(2), 'f', digits);
  }
  return std::string("");
}

/*!
 * \brief Ellipse::getDisplayA
 * \param type
 * \param digits
 * \param showDiff
 * \return
 */
std::string Ellipse::getDisplayA(const UnitType& type, const int& digits,
                                 const bool& /*showDiff*/) const {
  if (this->isSolved) {
    return oi::number(convertFromDefault(this->a, type), 'f', digits);
  }
  return std::string("");
}

/*!
 * \brief Ellipse::getDisplayB
 * \param type
 * \param digits
 * \param showDiff
 * \return
 */
std::string Ellipse::getDisplayB(const UnitType& type, const int& digits,
                                 const bool& /*showDiff*/) const {
  if (this->isSolved) {
    return oi::number(convertFromDefault(this->b, type), 'f', digits);
  }
  return std::string("");
}
