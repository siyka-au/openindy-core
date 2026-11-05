#include "ellipsoid.h"

#include "featurewrapper.h"

using namespace oi;
using namespace oi::math;

/*!
 * \brief Ellipsoid::Ellipsoid
 * \param parent
 */
Ellipsoid::Ellipsoid(QObject* parent) : Geometry(parent) {
  // set up feature wrapper
  if (!this->selfFeature.isNull()) {
    this->selfFeature->setEllipsoid(this);
  }
}

/*!
 * \brief Ellipsoid::Ellipsoid
 * \param center
 * \param majorAxis
 * \param a
 * \param b
 * \param c
 * \param minorAxis
 * \param parent
 */
Ellipsoid::Ellipsoid(const Position& center, const Direction& majorAxis,
                     const double& a, const double& b, QObject* parent)
    : Geometry(parent) {
  // set up feature wrapper
  if (!this->selfFeature.isNull()) {
    this->selfFeature->setEllipsoid(this);
  }

  this->setEllipsoid(center, majorAxis, a, b);
}

/*!
 * \brief Ellipsoid::Ellipsoid
 * \param copy
 * \param parent
 */
Ellipsoid::Ellipsoid(const Ellipsoid& copy, QObject* parent)
    : Geometry(copy, parent) {
  // set up feature wrapper
  if (!this->selfFeature.isNull()) {
    this->selfFeature->setEllipsoid(this);
  }

  this->xyz = copy.xyz;
  this->ijk = copy.ijk;
  this->a = copy.a;
  this->b = copy.b;
}

/*!
 * \brief Ellipsoid::operator =
 * \param copy
 * \return
 */
Ellipsoid& Ellipsoid::operator=(const Ellipsoid& copy) {
  // set up feature wrapper
  if (!this->selfFeature.isNull()) {
    this->selfFeature->setEllipsoid(this);
  }

  this->xyz = copy.xyz;
  this->ijk = copy.ijk;
  this->a = copy.a;
  this->b = copy.b;

  return *this;
}

/*!
 * \brief Ellipsoid::~Ellipsoid
 */
Ellipsoid::~Ellipsoid() {}

/*!
 * \brief Ellipsoid::hasDirection
 * \return
 */
bool Ellipsoid::hasDirection() const { return true; }

/*!
 * \brief Ellipsoid::hasPosition
 * \return
 */
bool Ellipsoid::hasPosition() const { return true; }

/*!
 * \brief Ellipsoid::getA
 * Returns the semi-major axis of the ellipsoid
 * \return
 */
const double& Ellipsoid::getA() const { return this->a; }

/*!
 * \brief Ellipsoid::getB
 * Returns the semi-minor axis of the ellipsoid
 * \return
 */
const double& Ellipsoid::getB() const { return this->b; }

/*!
 * \brief Ellipsoid::setEllipsoid
 * \param center
 * \param majorAxis
 * \param a
 * \param b
 * \param c
 * \param minorAxis
 */
void Ellipsoid::setEllipsoid(const Position& center, const Direction& majorAxis,
                             const double& a, const double& b) {
  // set the given parameters
  this->xyz = center;
  this->ijk = majorAxis;
  this->a = a;
  this->b = b;

  emit this->geomParametersChanged(this->id);
}

/*!
 * \brief Ellipsoid::getUnknownParameters
 * \param displayUnits
 * \param displayDigits
 * \return
 */
QMap<GeometryParameters, QString> Ellipsoid::getUnknownParameters(
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
      GeometryParameters::eUnknownA,
      this->getDisplayA(
          displayUnits.value(DimensionType::eMetric, UnitType::eUnitMeter),
          displayDigits.value(DimensionType::eMetric, 0)));
  parameters.insert(
      GeometryParameters::eUnknownB,
      this->getDisplayB(
          displayUnits.value(DimensionType::eMetric, UnitType::eUnitMeter),
          displayDigits.value(DimensionType::eMetric, 0)));

  return parameters;
}

/*!
 * \brief Ellipsoid::applyUnknownParameters
 * \param parameters
 */
void Ellipsoid::applyUnknownParameters(
    const QMap<GeometryParameters, double>& parameters) {
  // get current parameters
  OiVec position = this->xyz.getVector();
  OiVec direction = this->ijk.getVector();
  double a = this->a;
  double b = this->b;

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
      case GeometryParameters::eUnknownA:
        a = parameters.value(GeometryParameters::eUnknownA);
        break;
      case GeometryParameters::eUnknownB:
        b = parameters.value(GeometryParameters::eUnknownB);
        break;
      default:
        break;
    }
  }

  // update ellipsoid definition
  direction.normalize();
  Position ellipsoidPosition(position);
  Direction ellipsoidDirection(direction);
  this->setEllipsoid(ellipsoidPosition, ellipsoidDirection, a, b);
}

/*!
 * \brief Ellipsoid::recalc
 */
void Ellipsoid::recalc() {
  Geometry::recalc();

  // reset ellipsoid definition if not solved and no nominal
  if (!this->isSolved && this->getOwnerDescription().has_value()) {
    this->xyz.setVector(0.0, 0.0, 0.0);
    this->ijk.setVector(0.0, 0.0, 0.0);
    this->a = 0.0;
    this->b = 0.0;
  }
}

/*!
 * \brief Ellipsoid::toOpenIndyXML
 * \param xmlDoc
 * \return
 */
QDomElement Ellipsoid::toOpenIndyXML(QDomDocument& xmlDoc) const {
  QDomElement ellipsoid = Geometry::toOpenIndyXML(xmlDoc);

  if (ellipsoid.isNull()) {
    return ellipsoid;
  }

  ellipsoid.setAttribute(
      "type", getGeometryTypeName(GeometryTypes::eEllipsoidGeometry));

  // set semi-major axis length
  QDomElement a = xmlDoc.createElement("a");
  if (this->isSolved || !this->getOwnerDescription().has_value()) {
    a.setAttribute("value", this->a);
  } else {
    a.setAttribute("value", 0.0);
  }
  ellipsoid.appendChild(a);

  // set semi-minor axis length
  QDomElement b = xmlDoc.createElement("b");
  if (this->isSolved || !this->getOwnerDescription().has_value()) {
    b.setAttribute("value", this->b);
  } else {
    b.setAttribute("value", 0.0);
  }
  ellipsoid.appendChild(b);

  return ellipsoid;
}

/*!
 * \brief Ellipsoid::fromOpenIndyXML
 * \param xmlElem
 * \return
 */
bool Ellipsoid::fromOpenIndyXML(QDomElement& xmlElem) {
  bool result = Geometry::fromOpenIndyXML(xmlElem);

  if (result) {
    // set ellipsoid attributes
    QDomElement a = xmlElem.firstChildElement("a");
    QDomElement b = xmlElem.firstChildElement("b");
    QDomElement majorAxis = xmlElem.firstChildElement("spatialDirection");
    QDomElement center = xmlElem.firstChildElement("coordinates");

    if (a.isNull() || b.isNull() || majorAxis.isNull() || center.isNull() ||
        !a.hasAttribute("value") || !b.hasAttribute("value") ||
        majorAxis.hasAttribute("i") || !majorAxis.hasAttribute("j") ||
        !majorAxis.hasAttribute("k") || !center.hasAttribute("x") ||
        !center.hasAttribute("y") || !center.hasAttribute("z")) {
      return false;
    }

    this->a = a.attribute("value").toDouble();
    this->b = b.attribute("value").toDouble();
    this->ijk.setVector(majorAxis.attribute("i").toDouble(),
                        majorAxis.attribute("j").toDouble(),
                        majorAxis.attribute("k").toDouble());
    this->xyz.setVector(center.attribute("x").toDouble(),
                        center.attribute("y").toDouble(),
                        center.attribute("z").toDouble());
  }

  return result;
}

/*!
 * \brief Ellipsoid::getDisplayA
 * \param type
 * \param digits
 * \param showDiff
 * \return
 */
QString Ellipsoid::getDisplayA(const UnitType& type, const int& digits,
                               const bool& /*showDiff*/) const {
  if (this->isSolved) {
    return QString::number(convertFromDefault(this->a, type), 'f', digits);
  }
  return QString("");
}

/*!
 * \brief Ellipsoid::getDisplayB
 * \param type
 * \param digits
 * \param showDiff
 * \return
 */
QString Ellipsoid::getDisplayB(const UnitType& type, const int& digits,
                               const bool& /*showDiff*/) const {
  if (this->isSolved) {
    return QString::number(convertFromDefault(this->b, type), 'f', digits);
  }
  return QString("");
}
