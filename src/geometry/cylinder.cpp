#include "cylinder.h"

#include "featurewrapper.h"

using namespace oi;
using namespace oi::math;

/*!
 * \brief Cylinder::Cylinder
 * \param parent
 */
Cylinder::Cylinder(QObject* parent) : Geometry(parent) {
  // set up feature wrapper
  if (!this->selfFeature.isNull()) {
    this->selfFeature->setCylinder(this);
  }
}

/*!
 * \brief Cylinder::Cylinder
 * \param axisPoint
 * \param axis
 * \param radius
 * \param parent
 */
Cylinder::Cylinder(const Position& axisPoint, const Direction& axis,
                   const Radius& radius, QObject* parent)
    : Geometry(parent) {
  // set up feature wrapper
  if (!this->selfFeature.isNull()) {
    this->selfFeature->setCylinder(this);
  }

  this->setCylinder(axisPoint, axis, radius);
}

/*!
 * \brief Cylinder::Cylinder
 * \param copy
 * \param parent
 */
Cylinder::Cylinder(const Cylinder& copy, QObject* parent)
    : Geometry(copy, parent) {
  // set up feature wrapper
  if (!this->selfFeature.isNull()) {
    this->selfFeature->setCylinder(this);
  }

  this->xyz = copy.xyz;
  this->ijk = copy.ijk;
  this->radius = copy.radius;
}

/*!
 * \brief Cylinder::operator =
 * \param copy
 * \return
 */
Cylinder& Cylinder::operator=(const Cylinder& copy) {
  // set up feature wrapper
  if (!this->selfFeature.isNull()) {
    this->selfFeature->setCylinder(this);
  }

  this->xyz = copy.xyz;
  this->ijk = copy.ijk;
  this->radius = copy.radius;

  return *this;
}

/*!
 * \brief Cylinder::~Cylinder
 */
Cylinder::~Cylinder() {}

/*!
 * \brief Cylinder::hasDirection
 * \return
 */
bool Cylinder::hasDirection() const { return true; }

/*!
 * \brief Cylinder::hasPosition
 * \return
 */
bool Cylinder::hasPosition() const { return true; }

/*!
 * \brief Cylinder::hasRadius
 * \return
 */
bool Cylinder::hasRadius() const { return true; }

/*!
 * \brief Cylinder::setCylinder
 * \param axisPoint
 * \param axis
 * \param radius
 */
void Cylinder::setCylinder(const Position& axisPoint, const Direction& axis,
                           const Radius& radius) {
  // set the given parameters
  this->xyz = axisPoint;
  this->ijk = axis;
  this->radius = radius;

  emit this->geomParametersChanged(this->id);
}

/*!
 * \brief Cylinder::getUnknownParameters
 * \param displayUnits
 * \param displayDigits
 * \return
 */
QMap<GeometryParameters, QString> Cylinder::getUnknownParameters(
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

  return parameters;
}

/*!
 * \brief Cylinder::applyUnknownParameters
 * \param parameters
 */
void Cylinder::applyUnknownParameters(
    const QMap<GeometryParameters, double>& parameters) {
  // get current parameters
  OiVec position = this->xyz.getVector();
  OiVec direction = this->ijk.getVector();
  double radius = this->radius.getRadius();

  // update parameters
  QList<GeometryParameters> keys = parameters.keys();
  foreach (const GeometryParameters& key, keys) {
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
        radius = parameters.value(GeometryParameters::eUnknownRadiusA);
        break;
      default:
        break;
    }
  }

  // update cylinder definition
  direction.normalize();
  Position cylinderPosition(position);
  Direction cylinderDirection(direction);
  Radius cylinderRadius(radius);
  this->setCylinder(cylinderPosition, cylinderDirection, cylinderRadius);
}

/*!
 * \brief Cylinder::recalc
 */
void Cylinder::recalc() {
  Geometry::recalc();

  // reset cylinder definition if not solved and no nominal
  if (!this->isSolved && this->getOwnerDescription().has_value()) {
    this->xyz.setVector(0.0, 0.0, 0.0);
    this->ijk.setVector(0.0, 0.0, 0.0);
    this->radius.setRadius(0.0);
  }
}

/*!
 * \brief Cylinder::toOpenIndyXML
 * \param xmlDoc
 * \return
 */
QDomElement Cylinder::toOpenIndyXML(QDomDocument& xmlDoc) const {
  QDomElement cylinder = Geometry::toOpenIndyXML(xmlDoc);

  if (cylinder.isNull()) {
    return cylinder;
  }

  cylinder.setAttribute("type",
                        getGeometryTypeName(GeometryTypes::eCylinderGeometry));

  return cylinder;
}

/*!
 * \brief Cylinder::fromOpenIndyXML
 * \param xmlElem
 * \return
 */
bool Cylinder::fromOpenIndyXML(QDomElement& xmlElem) {
  bool result = Geometry::fromOpenIndyXML(xmlElem);

  if (result) {
    // set cylinder attributes
    QDomElement radius = xmlElem.firstChildElement("radius");
    QDomElement axis = xmlElem.firstChildElement("spatialDirection");
    QDomElement axisPoint = xmlElem.firstChildElement("coordinates");

    if (radius.isNull() || axis.isNull() || axisPoint.isNull() ||
        !radius.hasAttribute("value") || !axis.hasAttribute("i") ||
        !axis.hasAttribute("j") || !axis.hasAttribute("k") ||
        !axisPoint.hasAttribute("x") || !axisPoint.hasAttribute("y") ||
        !axisPoint.hasAttribute("z")) {
      return false;
    }

    this->radius.setRadius(radius.attribute("value").toDouble());
    this->ijk.setVector(axis.attribute("i").toDouble(),
                        axis.attribute("j").toDouble(),
                        axis.attribute("k").toDouble());
    this->xyz.setVector(axisPoint.attribute("x").toDouble(),
                        axisPoint.attribute("y").toDouble(),
                        axisPoint.attribute("z").toDouble());
  }

  return result;
}
