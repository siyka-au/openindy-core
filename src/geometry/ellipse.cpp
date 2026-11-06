#include "ellipse.h"

#include "featurewrapper.h"
#include "snapshot.h"

using namespace oi;
using namespace oi::math;

/*!
 * \brief Ellipse::Ellipse
 * \param parent
 */
Ellipse::Ellipse(QObject* parent) : Geometry(parent) {
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
 * \param parent
 */
Ellipse::Ellipse(const Position& center, const Direction& normal,
                 const double& a, const double& b,
                 const Direction& semiMajorAxis, QObject* parent)
    : Geometry(parent) {
  // set up feature wrapper
  if (!this->selfFeature.isNull()) {
    this->selfFeature->setEllipse(this);
  }

  this->setEllipse(center, normal, a, b, semiMajorAxis);
}

/*!
 * \brief Ellipse::Ellipse
 * \param copy
 * \param parent
 */
Ellipse::Ellipse(const Ellipse& copy, QObject* parent)
    : Geometry(copy, parent) {
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

  emit this->geomParametersChanged(this->id);
}

/*!
 * \brief Ellipse::getUnknownParameters
 * \param displayUnits
 * \param displayDigits
 * \return
 */
QMap<GeometryParameters, QString> Ellipse::getUnknownParameters(
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
  parameters.insert(GeometryParameters::eUnknownSecondaryI,
                    this->getDisplaySecondaryI(
                        displayDigits.value(DimensionType::eDimensionless, 0)));
  parameters.insert(GeometryParameters::eUnknownSecondaryJ,
                    this->getDisplaySecondaryJ(
                        displayDigits.value(DimensionType::eDimensionless, 0)));
  parameters.insert(GeometryParameters::eUnknownSecondaryK,
                    this->getDisplaySecondaryK(
                        displayDigits.value(DimensionType::eDimensionless, 0)));

  return parameters;
}

/*!
 * \brief Ellipse::applyUnknownParameters
 * \param parameters
 */
void Ellipse::applyUnknownParameters(
    const QMap<GeometryParameters, double>& parameters) {
  // get current parameters
  OiVec position = this->xyz.getVector();
  OiVec directionA = this->ijk.getVector();
  OiVec directionB = this->semiMajorAxis.getVector();
  double a = this->a;
  double b = this->b;

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
        directionA.setAt(
            0, parameters.value(GeometryParameters::eUnknownPrimaryI));
        break;
      case GeometryParameters::eUnknownPrimaryJ:
        directionA.setAt(
            1, parameters.value(GeometryParameters::eUnknownPrimaryJ));
        break;
      case GeometryParameters::eUnknownPrimaryK:
        directionA.setAt(
            2, parameters.value(GeometryParameters::eUnknownPrimaryK));
        break;
      case GeometryParameters::eUnknownA:
        a = parameters.value(GeometryParameters::eUnknownA);
        break;
      case GeometryParameters::eUnknownB:
        b = parameters.value(GeometryParameters::eUnknownB);
        break;
      case GeometryParameters::eUnknownSecondaryI:
        directionB.setAt(
            0, parameters.value(GeometryParameters::eUnknownSecondaryI));
        break;
      case GeometryParameters::eUnknownSecondaryJ:
        directionB.setAt(
            1, parameters.value(GeometryParameters::eUnknownSecondaryJ));
        break;
      case GeometryParameters::eUnknownSecondaryK:
        directionB.setAt(
            2, parameters.value(GeometryParameters::eUnknownSecondaryK));
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
 * \brief Ellipse::toOpenIndyXML
 * \param xmlDoc
 * \return
 */
QDomElement Ellipse::toOpenIndyXML(QDomDocument& xmlDoc) const {
  QDomElement ellipse = Geometry::toOpenIndyXML(xmlDoc);

  if (ellipse.isNull()) {
    return ellipse;
  }

  ellipse.setAttribute("type",
                       getGeometryTypeName(GeometryTypes::eEllipseGeometry));

  // set semi-major axis length
  QDomElement a = xmlDoc.createElement("a");
  if (this->isSolved || !this->getOwnerDescription().has_value()) {
    a.setAttribute("value", this->a);
  } else {
    a.setAttribute("value", 0.0);
  }
  ellipse.appendChild(a);

  // set semi-minor axis length
  QDomElement b = xmlDoc.createElement("b");
  if (this->isSolved || !this->getOwnerDescription().has_value()) {
    b.setAttribute("value", this->b);
  } else {
    b.setAttribute("value", 0.0);
  }
  ellipse.appendChild(b);

  // set semi-major axis direction
  QDomElement semiMajorAxis = xmlDoc.createElement("semiMajorAxis");
  if (this->isSolved || !this->getOwnerDescription().has_value()) {
    semiMajorAxis.setAttribute("i", this->semiMajorAxis.getVector().getAt(0));
    semiMajorAxis.setAttribute("j", this->semiMajorAxis.getVector().getAt(1));
    semiMajorAxis.setAttribute("k", this->semiMajorAxis.getVector().getAt(2));
  } else {
    semiMajorAxis.setAttribute("i", 0.0);
    semiMajorAxis.setAttribute("j", 0.0);
    semiMajorAxis.setAttribute("k", 0.0);
  }
  ellipse.appendChild(semiMajorAxis);

  return ellipse;
}

/*!
 * \brief Ellipse::fromOpenIndyXML
 * \param xmlElem
 * \return
 */
bool Ellipse::fromOpenIndyXML(QDomElement& xmlElem) {
  bool result = Geometry::fromOpenIndyXML(xmlElem);

  if (result) {
    // set ellipse attributes
    QDomElement a = xmlElem.firstChildElement("a");
    QDomElement b = xmlElem.firstChildElement("b");
    QDomElement semiMajorAxis = xmlElem.firstChildElement("semiMajorAxis");
    QDomElement normal = xmlElem.firstChildElement("spatialDirection");
    QDomElement center = xmlElem.firstChildElement("coordinates");

    if (a.isNull() || b.isNull() || semiMajorAxis.isNull() || normal.isNull() ||
        center.isNull() || !a.hasAttribute("value") ||
        !b.hasAttribute("value") || semiMajorAxis.hasAttribute("i") ||
        !semiMajorAxis.hasAttribute("j") || !semiMajorAxis.hasAttribute("k") ||
        !normal.hasAttribute("i") || normal.hasAttribute("j") ||
        normal.hasAttribute("k") || !center.hasAttribute("x") ||
        !center.hasAttribute("y") || !center.hasAttribute("z")) {
      return false;
    }

    this->a = a.attribute("value").toDouble();
    this->b = b.attribute("value").toDouble();
    this->semiMajorAxis.setVector(semiMajorAxis.attribute("i").toDouble(),
                                  semiMajorAxis.attribute("j").toDouble(),
                                  semiMajorAxis.attribute("k").toDouble());
    this->ijk.setVector(normal.attribute("i").toDouble(),
                        normal.attribute("j").toDouble(),
                        normal.attribute("k").toDouble());
    this->xyz.setVector(center.attribute("x").toDouble(),
                        center.attribute("y").toDouble(),
                        center.attribute("z").toDouble());
  }

  return result;
}

/*!
 * \brief Ellipse::getDisplaySecondaryI
 * \param digits
 * \param showDiff
 * \return
 */
QString Ellipse::getDisplaySecondaryI(const int& digits,
                                      const bool& /*showDiff*/) const {
  if (this->isSolved) {
    return QString::number(this->semiMajorAxis.getVector().getAt(0), 'f',
                           digits);
  }
  return QString("");
}

/*!
 * \brief Ellipse::getDisplaySecondaryJ
 * \param digits
 * \param showDiff
 * \return
 */
QString Ellipse::getDisplaySecondaryJ(const int& digits,
                                      const bool& /*showDiff*/) const {
  if (this->isSolved) {
    return QString::number(this->semiMajorAxis.getVector().getAt(1), 'f',
                           digits);
  }
  return QString("");
}

/*!
 * \brief Ellipse::getDisplaySecondaryK
 * \param digits
 * \param showDiff
 * \return
 */
QString Ellipse::getDisplaySecondaryK(const int& digits,
                                      const bool& /*showDiff*/) const {
  if (this->isSolved) {
    return QString::number(this->semiMajorAxis.getVector().getAt(2), 'f',
                           digits);
  }
  return QString("");
}

/*!
 * \brief Ellipse::getDisplayA
 * \param type
 * \param digits
 * \param showDiff
 * \return
 */
QString Ellipse::getDisplayA(const UnitType& type, const int& digits,
                             const bool& /*showDiff*/) const {
  if (this->isSolved) {
    return QString::number(convertFromDefault(this->a, type), 'f', digits);
  }
  return QString("");
}

/*!
 * \brief Ellipse::getDisplayB
 * \param type
 * \param digits
 * \param showDiff
 * \return
 */
QString Ellipse::getDisplayB(const UnitType& type, const int& digits,
                             const bool& /*showDiff*/) const {
  if (this->isSolved) {
    return QString::number(convertFromDefault(this->b, type), 'f', digits);
  }
  return QString("");
}
