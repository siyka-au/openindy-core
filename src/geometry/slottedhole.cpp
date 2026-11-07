#include "slottedhole.h"

#include <map>
#include <vector>

#include "containers.h"
#include "featurewrapper.h"
#include "snapshot.h"
#include "stringutil.h"

using namespace oi;
using namespace oi::math;

/*!
 * \brief SlottedHole::SlottedHole
 */
SlottedHole::SlottedHole() : Geometry() {
  // set up feature wrapper
  if (!this->selfFeature.isNull()) {
    this->selfFeature->setSlottedHole(this);
  }
}

/*!
 * \brief SlottedHole::SlottedHole
 * \param center
 * \param normal
 * \param radius
 * \param length
 * \param holeAxis
 */
SlottedHole::SlottedHole(const Position& center, const Direction& normal,
                         const Radius& radius, const double& length,
                         const Direction& holeAxis)
    : Geometry() {
  // set up feature wrapper
  if (!this->selfFeature.isNull()) {
    this->selfFeature->setSlottedHole(this);
  }

  this->setSlottedHole(center, normal, radius, length, holeAxis);
}

/*!
 * \brief SlottedHole::SlottedHole
 * \param circleCenterA
 * \param circleCenterB
 * \param normal
 * \param radius
 */
SlottedHole::SlottedHole(const Position& circleCenterA,
                         const Position& circleCenterB, const Direction& normal,
                         const Radius& radius)
    : Geometry() {
  // set up feature wrapper
  if (!this->selfFeature.isNull()) {
    this->selfFeature->setSlottedHole(this);
  }

  this->setSlottedHole(circleCenterA, circleCenterB, normal, radius);
}

/*!
 * \brief SlottedHole::SlottedHole
 * \param copy
 */
SlottedHole::SlottedHole(const SlottedHole& copy) : Geometry(copy) {
  // set up feature wrapper
  if (!this->selfFeature.isNull()) {
    this->selfFeature->setSlottedHole(this);
  }

  this->center = copy.center;
  this->normal = copy.normal;
  this->radius = copy.radius;
  this->length = copy.length;
  this->holeAxis = copy.holeAxis;
  this->circleCenterA = copy.circleCenterA;
  this->circleCenterB = copy.circleCenterB;
}

/*!
 * \brief SlottedHole::operator =
 * \param copy
 * \return
 */
SlottedHole& SlottedHole::operator=(const SlottedHole& copy) {
  // set up feature wrapper
  if (!this->selfFeature.isNull()) {
    this->selfFeature->setSlottedHole(this);
  }

  this->center = copy.center;
  this->normal = copy.normal;
  this->radius = copy.radius;
  this->length = copy.length;
  this->holeAxis = copy.holeAxis;
  this->circleCenterA = copy.circleCenterA;
  this->circleCenterB = copy.circleCenterB;

  return *this;
}

/*!
 * \brief SlottedHole::~SlottedHole
 */
SlottedHole::~SlottedHole() {}

/*!
 * \brief SlottedHole::hasDirection
 * \return
 */
bool SlottedHole::hasDirection() const { return true; }

/*!
 * \brief SlottedHole::hasPosition
 * \return
 */
bool SlottedHole::hasPosition() const { return true; }

/*!
 * \brief SlottedHole::hasRadius
 * \return
 */
bool SlottedHole::hasRadius() const { return true; }

/*!
 * \brief SlottedHole::getRadius
 * Returns the radius of the outer circles
 * \return
 */
const Radius& SlottedHole::getRadius() const { return this->radius; }

/*!
 * \brief SlottedHole::getDirection
 * Returns the normal vector of the slotted hole
 * \return
 */
const Direction& SlottedHole::getDirection() const { return this->normal; }

/*!
 * \brief SlottedHole::getPosition
 * Returns the center of the slotted hole
 * \return
 */
const Position& SlottedHole::getPosition() const { return this->center; }

/*!
 * \brief SlottedHole::getCircleCenterA
 * Returns the center of one circle
 * \return
 */
const Position& SlottedHole::getCircleCenterA() const {
  return this->circleCenterA;
}

/*!
 * \brief SlottedHole::getCircleCenterB
 * Returns the center of the other circle
 * \return
 */
const Position& SlottedHole::getCircleCenterB() const {
  return this->circleCenterB;
}

/*!
 * \brief SlottedHole::getHoleAxis
 * Returns a vector pointing from circle center A to B
 * \return
 */
const Direction& SlottedHole::getHoleAxis() const { return this->holeAxis; }

/*!
 * \brief SlottedHole::getLength
 * \return
 */
const double& SlottedHole::getLength() const { return this->length; }

/*!
 * \brief SlottedHole::setSlottedHole
 * \param center
 * \param normal
 * \param radius
 * \param length
 * \param holeAxis
 */
void SlottedHole::setSlottedHole(const Position& center,
                                 const Direction& normal, const Radius& radius,
                                 const double& length,
                                 const Direction& holeAxis) {
  // set the given parameters
  this->center = center;
  this->normal = normal;
  this->radius = radius;
  this->length = length;
  this->holeAxis = holeAxis;

  // calculate the other parameters
  OiVec axis = this->holeAxis.getVector();
  axis.normalize();
  this->circleCenterA.setVector(this->center.getVector() +
                                (this->length / 2 - this->radius.getRadius()) *
                                    axis);
  this->circleCenterB.setVector(this->center.getVector() -
                                (this->length / 2 - this->radius.getRadius()) *
                                    axis);

  this->notify(&FeatureObserver::onGeomParametersChanged, this->id);
}

/*!
 * \brief SlottedHole::setSlottedHole
 * \param circleCenterA
 * \param circleCenterB
 * \param normal
 * \param radius
 */
void SlottedHole::setSlottedHole(const Position& circleCenterA,
                                 const Position& circleCenterB,
                                 const Direction& normal,
                                 const Radius& radius) {
  // set the given parameters
  this->circleCenterA = circleCenterA;
  this->circleCenterB = circleCenterB;
  this->normal = normal;
  this->radius = radius;

  // calculate the other parameters
  this->center.setVector(this->circleCenterA.getVector() +
                         0.5 * (this->circleCenterB.getVector() -
                                this->circleCenterA.getVector()));
  OiVec axis =
      this->circleCenterB.getVector() - this->circleCenterA.getVector();
  OiVec::dot(this->length, axis, axis);
  axis.normalize();
  this->holeAxis.setVector(axis);

  this->notify(&FeatureObserver::onGeomParametersChanged, this->id);
}

/*!
 * \brief SlottedHole::getUnknownParameters
 * \param displayUnits
 * \param displayDigits
 * \return
 */
std::map<GeometryParameters, std::string> SlottedHole::getUnknownParameters(
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
          oi::value(displayDigits, DimensionType::eDimensionless)));
  parameters.insert_or_assign(
      GeometryParameters::eUnknownPrimaryJ,
      this->getDisplayPrimaryJ(
          oi::value(displayDigits, DimensionType::eDimensionless)));
  parameters.insert_or_assign(
      GeometryParameters::eUnknownPrimaryK,
      this->getDisplayPrimaryK(
          oi::value(displayDigits, DimensionType::eDimensionless)));
  parameters.insert_or_assign(
      GeometryParameters::eUnknownRadiusA,
      this->getDisplayRadiusA(
          oi::value(displayUnits, DimensionType::eMetric, UnitType::eUnitMeter),
          oi::value(displayDigits, DimensionType::eMetric, 0)));
  parameters.insert_or_assign(
      GeometryParameters::eUnknownLength,
      this->getDisplayLength(
          oi::value(displayUnits, DimensionType::eMetric, UnitType::eUnitMeter),
          oi::value(displayDigits, DimensionType::eMetric, 0)));
  parameters.insert_or_assign(
      GeometryParameters::eUnknownSecondaryI,
      this->getDisplaySecondaryI(
          oi::value(displayDigits, DimensionType::eDimensionless)));
  parameters.insert_or_assign(
      GeometryParameters::eUnknownSecondaryJ,
      this->getDisplaySecondaryJ(
          oi::value(displayDigits, DimensionType::eDimensionless)));
  parameters.insert_or_assign(
      GeometryParameters::eUnknownSecondaryK,
      this->getDisplaySecondaryK(
          oi::value(displayDigits, DimensionType::eDimensionless)));

  return parameters;
}

/*!
 * \brief SlottedHole::applyUnknownParameters
 * \param parameters
 */
void SlottedHole::applyUnknownParameters(
    const std::map<GeometryParameters, double>& parameters) {
  // get current parameters
  OiVec position = this->center.getVector();
  OiVec directionA = this->normal.getVector();
  OiVec directionB = this->holeAxis.getVector();
  double radius = this->radius.getRadius();
  double length = this->length;

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
      case GeometryParameters::eUnknownRadiusA:
        radius = oi::value(parameters, GeometryParameters::eUnknownRadiusA);
        break;
      case GeometryParameters::eUnknownLength:
        length = oi::value(parameters, GeometryParameters::eUnknownLength);
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

  // update slotted hole definition
  directionA.normalize();
  directionB.normalize();
  Position holePosition(position);
  Direction holeDirectionA(directionA);
  Direction holeDirectionB(directionB);
  Radius holeRadius(radius);
  this->setSlottedHole(holePosition, holeDirectionA, holeRadius, length,
                       holeDirectionB);
}

/*!
 * \brief SlottedHole::recalc
 */
void SlottedHole::recalc() {
  Geometry::recalc();

  // reset slotted hole definition if not solved and no nominal
  if (!this->isSolved && this->getOwnerDescription().has_value()) {
    this->center.setVector(0.0, 0.0, 0.0);
    this->normal.setVector(0.0, 0.0, 0.0);
    this->radius.setRadius(0.0);
    this->length = 0.0;
    this->holeAxis.setVector(0.0, 0.0, 0.0);
    this->circleCenterA.setVector(0.0, 0.0, 0.0);
    this->circleCenterB.setVector(0.0, 0.0, 0.0);
  }
}

/*!
 * \brief SlottedHole::getDisplayX
 * \param type
 * \param digits
 * \param showDiff
 * \return
 */
std::string SlottedHole::getDisplayX(const UnitType& type, const int& digits,
                                     const bool& /*showDiff*/) const {
  if (this->isSolved) {
    return oi::number(
        convertFromDefault(this->center.getVector().getAt(0), type), 'f',
        digits);
  }
  return std::string("");
}

/*!
 * \brief SlottedHole::getDisplayY
 * \param type
 * \param digits
 * \param showDiff
 * \return
 */
std::string SlottedHole::getDisplayY(const UnitType& type, const int& digits,
                                     const bool& /*showDiff*/) const {
  if (this->isSolved) {
    return oi::number(
        convertFromDefault(this->center.getVector().getAt(1), type), 'f',
        digits);
  }
  return std::string("");
}

/*!
 * \brief SlottedHole::getDisplayZ
 * \param type
 * \param digits
 * \param showDiff
 * \return
 */
std::string SlottedHole::getDisplayZ(const UnitType& type, const int& digits,
                                     const bool& /*showDiff*/) const {
  if (this->isSolved) {
    return oi::number(
        convertFromDefault(this->center.getVector().getAt(2), type), 'f',
        digits);
  }
  return std::string("");
}

/*!
 * \brief SlottedHole::getDisplayPrimaryI
 * \param digits
 * \param showDiff
 * \return
 */
std::string SlottedHole::getDisplayPrimaryI(const int& digits,
                                            const bool& /*showDiff*/) const {
  if (this->isSolved) {
    return oi::number(this->normal.getVector().getAt(0), 'f', digits);
  }
  return std::string("");
}

/*!
 * \brief SlottedHole::getDisplayPrimaryJ
 * \param digits
 * \param showDiff
 * \return
 */
std::string SlottedHole::getDisplayPrimaryJ(const int& digits,
                                            const bool& /*showDiff*/) const {
  if (this->isSolved) {
    return oi::number(this->normal.getVector().getAt(1), 'f', digits);
  }
  return std::string("");
}

/*!
 * \brief SlottedHole::getDisplayPrimaryK
 * \param digits
 * \param showDiff
 * \return
 */
std::string SlottedHole::getDisplayPrimaryK(const int& digits,
                                            const bool& /*showDiff*/) const {
  if (this->isSolved) {
    return oi::number(this->normal.getVector().getAt(2), 'f', digits);
  }
  return std::string("");
}

/*!
 * \brief SlottedHole::getDisplayRadiusA
 * \param type
 * \param digits
 * \param showDiff
 * \return
 */
std::string SlottedHole::getDisplayRadiusA(const UnitType& type,
                                           const int& digits,
                                           const bool& /*showDiff*/) const {
  if (this->isSolved) {
    return oi::number(convertFromDefault(this->radius.getRadius(), type), 'f',
                      digits);
  }
  return std::string("");
}

/*!
 * \brief SlottedHole::getDisplaySecondaryI
 * \param digits
 * \param showDiff
 * \return
 */
std::string SlottedHole::getDisplaySecondaryI(const int& digits,
                                              const bool& /*showDiff*/) const {
  if (this->isSolved) {
    return oi::number(this->holeAxis.getVector().getAt(0), 'f', digits);
  }
  return std::string("");
}

/*!
 * \brief SlottedHole::getDisplaySecondaryJ
 * \param digits
 * \param showDiff
 * \return
 */
std::string SlottedHole::getDisplaySecondaryJ(const int& digits,
                                              const bool& /*showDiff*/) const {
  if (this->isSolved) {
    return oi::number(this->holeAxis.getVector().getAt(1), 'f', digits);
  }
  return std::string("");
}

/*!
 * \brief SlottedHole::getDisplaySecondaryK
 * \param digits
 * \param showDiff
 * \return
 */
std::string SlottedHole::getDisplaySecondaryK(const int& digits,
                                              const bool& /*showDiff*/) const {
  if (this->isSolved) {
    return oi::number(this->holeAxis.getVector().getAt(2), 'f', digits);
  }
  return std::string("");
}

/*!
 * \brief SlottedHole::getDisplayLength
 * \param type
 * \param digits
 * \param showDiff
 * \return
 */
std::string SlottedHole::getDisplayLength(const UnitType& type,
                                          const int& digits,
                                          const bool& /*showDiff*/) const {
  if (this->isSolved) {
    return oi::number(convertFromDefault(this->length, type), 'f', digits);
  }
  return std::string("");
}
