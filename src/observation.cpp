#include "observation.h"

#include "coordinatesystem.h"
#include "function.h"
#include "geometry.h"
#include "oijob.h"
#include "snapshot.h"
#include "station.h"

#if defined(__GNUC__) && !defined(__clang__)
#warning \
    "openindy-core#1: Geometry/Observation hold direct QPointers to each other, which triggers -Wsfinae-incomplete via AUTOMOC's aggregated moc TU; see https://github.com/siyka-au/openindy-core/issues/1"
#endif

using namespace oi;
using namespace oi::math;

/*!
 * \brief Observation::Observation
 * \param parent
 */
Observation::Observation(QObject* parent)
    : Point(parent),
      isValid(false),
      isSolved(false),
      hasDirection(false),
      xyz(4),
      originalXyz(4),
      sigmaXyz(4),
      originalSigmaXyz(4),
      ijk(4),
      originalIjk(4),
      sigmaIjk(4),
      originalSigmaIjk(4),
      isDummyPoint(false) {}

/*!
 * \brief Observation::Observation
 * \param copy
 * \param parent
 */
Observation::Observation(const Observation& copy, QObject* parent)
    : Point(copy, parent),
      hasDirection(copy.hasDirection),
      ijk(copy.ijk),
      isDummyPoint(copy.isDummyPoint),
      isSolved(copy.isSolved),
      isValid(copy.isValid),
      originalIjk(copy.originalIjk),
      originalSigmaIjk(copy.originalSigmaIjk),
      originalSigmaXyz(copy.originalSigmaXyz),
      originalXyz(copy.originalXyz),
      reading(copy.reading),
      sigmaIjk(copy.sigmaIjk),
      sigmaXyz(copy.sigmaXyz),
      station(copy.station),
      xyz(copy.xyz) {
  // copy attributes

  // this->isActiveCoordSys = copy.isActiveCoordSys;

  // copy reading and station
}

/*!
 * \brief Observation::Observation
 * \param xyz
 * \param isValid
 * \param parent
 */
Observation::Observation(const OiVec& xyz, bool isValid, QObject* parent)
    : Point(parent),
      isValid(isValid),
      isSolved(false),
      hasDirection(false),
      xyz(4),
      originalXyz(4),
      sigmaXyz(4),
      originalSigmaXyz(4),
      ijk(4),
      originalIjk(4),
      sigmaIjk(4),
      originalSigmaIjk(4),
      isDummyPoint(false) {
  if (xyz.getSize() == this->xyz.getSize()) {
    this->xyz = xyz;
    this->originalXyz = xyz;
    this->isSolved = true;
  }
}

/*!
 * \brief Observation::Observation
 * \param xyz
 * \param observation id
 * \param isValid
 * \param parent
 */
Observation::Observation(const OiVec& xyz, int id, bool isValid,
                         QObject* parent)
    : Point(parent),
      isValid(isValid),
      isSolved(false),
      hasDirection(false),
      xyz(4),
      originalXyz(4),
      sigmaXyz(4),
      originalSigmaXyz(4),
      ijk(4),
      originalIjk(4),
      sigmaIjk(4),
      originalSigmaIjk(4),
      isDummyPoint(false) {
  this->id = id;

  if (xyz.getSize() == this->xyz.getSize()) {
    this->xyz = xyz;
    this->originalXyz = xyz;
    this->isSolved = true;
  }
}
/*!
 * \brief Observation::operator =
 * \param copy
 * \return
 */
Observation& Observation::operator=(const Observation& copy) {
  // copy attributes
  this->isValid = copy.isValid;
  this->isSolved = copy.isSolved;
  this->hasDirection = copy.hasDirection;
  // this->isActiveCoordSys = copy.isActiveCoordSys;
  this->xyz = copy.xyz;
  this->originalXyz = copy.originalXyz;
  this->sigmaXyz = copy.sigmaXyz;
  this->originalSigmaXyz = copy.originalSigmaXyz;
  this->ijk = copy.ijk;
  this->originalIjk = copy.originalIjk;
  this->sigmaIjk = copy.sigmaIjk;
  this->originalSigmaIjk = copy.originalSigmaIjk;
  this->isDummyPoint = copy.isDummyPoint;

  // copy reading and station
  this->reading = copy.reading;
  this->station = copy.station;

  return *this;
}

/*!
 * \brief Observation::~Observation
 */
Observation::~Observation() {
  // check the corresponding reading
  if (this->getReading().isNull()) {
    return;
  }

  QList<QPointer<Geometry> > targetGeometries;

  // remove observation and reading from its target geometries
  for (const QPointer<Geometry>& geom :
       oi::snapshot(this->getTargetGeometries())) {
    // check target geometry
    if (geom.isNull()) {
      continue;
    }
    targetGeometries.append(geom);

    // remove observation and reading from functions of the targte geometry
    for (const QPointer<Function>& function :
         oi::snapshot(geom->getFunctions())) {
      // check function and remove elements
      if (!function.isNull()) {
        function->blockSignals(true);
        function->removeInputElement(this->getId());
        function->removeInputElement(this->getReading()->getId());
        function->blockSignals(false);
      }
    }
  }
  for (const QPointer<Geometry>& geom : oi::snapshot(targetGeometries)) {
    geom->blockSignals(true);
    this->removeTargetGeometry(geom->getId());
    geom->blockSignals(false);
  }

  // remove observation and reading from the station and station system
  if (!this->getStation().isNull() &&
      !this->getStation()->getCoordinateSystem().isNull()) {
    this->getStation()->blockSignals(true);
    this->getStation()->getCoordinateSystem()->blockSignals(true);
    this->getStation()->removeReading(this->getReading());
    this->getStation()->getCoordinateSystem()->removeObservation(this);
    this->getStation()->blockSignals(false);
    this->getStation()->getCoordinateSystem()->blockSignals(false);
  }

  // delete the corresponding reading
  delete this->getReading().data();
}

/*!
 * \brief Observation::getOwnerDescription
 * D19/D20: always station-owned, always read-only - unconditionally,
 * regardless of functionList (an Observation never has a producer
 * function) so this does not defer to Geometry::getOwnerDescription at
 * all. See the class comment in observation.h.
 * \return
 */
std::optional<QString> Observation::getOwnerDescription() const {
  if (!this->station.isNull()) {
    return QString("its station \"%1\"").arg(this->station->getFeatureName());
  }
  return QString("its station");
}

/*!
 * \brief Observation::getPosition
 * Overrides Point::getPosition() (ultimately Geometry::getPosition()) so
 * that reaching this object through any Point-typed pointer sees the same
 * position getXYZ() does, computed from Observation's own xyz - see the
 * class comment in observation.h for why Observation keeps its own
 * xyz/sigma rather than writing through the inherited Position member.
 * \return
 */
const Position& Observation::getPosition() const {
  if (this->xyz.getSize() >= 3) {
    this->positionCache.setVector(this->xyz.getAt(0), this->xyz.getAt(1),
                                  this->xyz.getAt(2));
  }
  return this->positionCache;
}

/*!
 * \brief Observation::getXYZ
 * \return
 */
const OiVec& Observation::getXYZ() const { return this->xyz; }

/*!
 * \brief Observation::setXYZ
 * \param xyz
 */
void Observation::setXYZ(const OiVec& xyz) {
  // check vector
  if (xyz.getSize() == this->xyz.getSize()) {
    this->xyz = xyz;
  }
}

/*!
 * \brief Observation::getOriginalXYZ
 * \return
 */
const OiVec& Observation::getOriginalXYZ() const { return this->originalXyz; }

/*!
 * \brief Observation::getSigmaXYZ
 * \return
 */
const OiVec& Observation::getSigmaXYZ() const { return this->sigmaXyz; }

/*!
 * \brief Observation::setSigmaXyz
 * \param sigmaXyz
 */
void Observation::setSigmaXyz(const OiVec& sigmaXyz) {
  // check vector
  if (sigmaXyz.getSize() == this->sigmaXyz.getSize()) {
    this->sigmaXyz = sigmaXyz;
  }
}

/*!
 * \brief Observation::getOriginalSigmaXyz
 * \return
 */
const OiVec& Observation::getOriginalSigmaXyz() const {
  return this->originalSigmaXyz;
}

/*!
 * \brief Observation::getIJK
 * \return
 */
const OiVec& Observation::getIJK() const { return this->ijk; }

/*!
 * \brief Observation::setIJK
 * \param ijk
 */
void Observation::setIJK(const OiVec& ijk) {
  // check vector
  if (ijk.getSize() == this->ijk.getSize()) {
    this->ijk = ijk;
  }
}

/*!
 * \brief Observation::getOriginalIJK
 * \return
 */
const OiVec& Observation::getOriginalIJK() const { return this->originalIjk; }

/*!
 * \brief Observation::getSigmaIJK
 * \return
 */
const OiVec& Observation::getSigmaIJK() const { return this->sigmaIjk; }

/*!
 * \brief Observation::setSigmaIjk
 * \param sigmaIjk
 */
void Observation::setSigmaIjk(const OiVec& sigmaIjk) {
  // check vector
  if (sigmaIjk.getSize() == this->sigmaIjk.getSize()) {
    this->sigmaIjk = sigmaIjk;
  }
}

/*!
 * \brief Observation::getOriginalSigmaIjk
 * \return
 */
const OiVec& Observation::getOriginalSigmaIjk() const {
  return this->originalSigmaIjk;
}

/*!
 * \brief Observation::getStation
 * \return
 */
const QPointer<Station>& Observation::getStation() const {
  return this->station;
}

/*!
 * \brief Observation::setStation
 * \param station
 */
void Observation::setStation(const QPointer<Station>& station) {
  if (this->station.isNull() && !station.isNull()) {
    this->station = station;
  }
}

/*!
 * \brief Observation::getTargetGeometries
 * Resolves the inherited Point::intents (D12) back into live Geometry
 * pointers via this observation's job, for the pre-existing callers that
 * want "what geometry is this for" rather than a bare feature id. Returned
 * by value (rather than the old by-reference QList<QPointer<Geometry>>&)
 * since there is no single stored list left to bind a reference to - a
 * const& binding a caller takes to the returned temporary is still safe
 * (lifetime extension), so this did not require touching call sites.
 * \return
 */
QList<QPointer<Geometry> > Observation::getTargetGeometries() const {
  QList<QPointer<Geometry> > result;

  if (this->job.isNull()) {
    return result;
  }

  for (const MeasurementIntent& intent : oi::snapshot(this->getIntents())) {
    const QPointer<FeatureWrapper> feature =
        this->job->getFeatureById(intent.featureId);
    if (!feature.isNull() && !feature->getGeometry().isNull() &&
        !result.contains(feature->getGeometry())) {
      result.append(feature->getGeometry());
    }
  }

  return result;
}

/*!
 * \brief Observation::addTargetGeometry
 * Stamps an untagged (roleName empty) MeasurementIntent for the given
 * geometry - the pre-7f callers of this method never distinguished roles,
 * so an empty role preserves their behaviour exactly.
 * \param targetGeometry
 */
void Observation::addTargetGeometry(const QPointer<Geometry>& targetGeometry) {
  if (!targetGeometry.isNull()) {
    const MeasurementIntent intent(targetGeometry->getId(), QString());

    // check if target geometry is already included
    if (this->hasIntent(intent.featureId, intent.roleName)) {
      return;
    }

    this->addIntent(intent);

    targetGeometry->addObservation(this);

    if (this->measuredTargetGeometry.isNull()) {
      this->measuredTargetGeometry = targetGeometry;
    }
  }
}

/*!
 * \brief Observation::removeTargetGeometry
 * Removes every intent tagging this observation for targetGeometry,
 * regardless of role - matches the old semantics of "this observation is
 * no longer for that geometry at all". Takes the pointer directly (no job
 * lookup needed) so this keeps working even on a bare, job-less
 * Observation.
 * \param targetGeometry
 */
void Observation::removeTargetGeometry(
    const QPointer<Geometry>& targetGeometry) {
  if (targetGeometry.isNull()) {
    return;
  }

  QList<MeasurementIntent> matching;
  for (const MeasurementIntent& intent : oi::snapshot(this->getIntents())) {
    if (intent.featureId == targetGeometry->getId()) {
      matching.append(intent);
    }
  }
  if (matching.isEmpty()) {
    return;
  }
  for (const MeasurementIntent& intent : oi::snapshot(matching)) {
    this->removeIntent(intent);
  }

  targetGeometry->removeObservation(this);
}

/*!
 * \brief Observation::removeTargetGeometry
 * \param geomId
 */
void Observation::removeTargetGeometry(const int& geomId) {
  if (!this->job.isNull()) {
    const QPointer<FeatureWrapper> feature = this->job->getFeatureById(geomId);
    if (!feature.isNull() && !feature->getGeometry().isNull()) {
      this->removeTargetGeometry(feature->getGeometry());
    }
  }

  const QList<QPointer<Geometry> > remaining = this->getTargetGeometries();
  if (this->measuredTargetGeometry.isNull() && remaining.size() == 1) {
    this->measuredTargetGeometry = remaining.at(0);
  }
}

/*!
 * \brief Observation::getMeasuredTargetGeometry
 * \return
 */
const QPointer<Geometry>& Observation::getMeasuredTargetGeometry() const {
  return this->measuredTargetGeometry;
}

/*!
 * \brief Observation::setMeasuredTargetGeometry
 * \param measuredTargetGeom
 */
void Observation::setMeasuredTargetGeometry(
    const QPointer<Geometry>& measuredTargetGeom) {
  if (!measuredTargetGeom.isNull()) {
    this->measuredTargetGeometry = measuredTargetGeom;
  }
}

/*!
 * \brief Observation::getReading
 * \return
 */
const QPointer<Reading>& Observation::getReading() const {
  return this->reading;
}

/*!
 * \brief Observation::setReading
 * \param reading
 */
/*void Observation::setReading(const QPointer<Reading> &reading){
    if(!reading.isNull()){
        this->reading = reading;
    }
}*/

/*!
 * \brief Observation::setIsValid
 * \param isValid
 */
/*void Observation::setIsValid(const bool &isValid){
    this->isValid = isValid;
}*/

/*!
 * \brief Observation::getIsValid
 * \return
 */
const bool& Observation::getIsValid() const { return this->isValid; }

/*!
 * \brief Observation::setIsSolved
 * \param isSolved
 */
void Observation::setIsSolved(const bool& isSolved) {
  this->isSolved = isSolved;

  // Observation::isSolved ("valid in current coordinate system") shadows
  // the inherited Feature::isSolved ("solved in display system", see
  // feature.h) - they answer different questions historically, but keep
  // the inherited one in sync too so a caller that reaches this object
  // through a Point*/Feature* (Feature::getIsSolved is not virtual) does
  // not see a permanently-false default.
  this->Feature::setIsSolved(isSolved);
}

/*!
 * \brief Observation::getIsSolved
 * \return
 */
const bool& Observation::getIsSolved() const { return this->isSolved; }

/*!
 * \brief Observation::getHasDirection
 * \return
 */
const bool& Observation::getHasDirection() const { return this->hasDirection; }

const bool& Observation::getIsDummyPoint() const { return this->isDummyPoint; }

/*!
 * \brief Observation::getDisplayId
 * \return
 */
QString Observation::getDisplayId() const { return QString::number(this->id); }

/*!
 * \brief Observation::getDisplayStation
 * \return
 */
QString Observation::getDisplayStation() const {
  if (this->station.isNull()) {
    return "";
  }
  return this->station->getFeatureName();
}

/*!
 * \brief Observation::getDisplayTargetGeometries
 * \return
 */
QString Observation::getDisplayTargetGeometries() const {
  QString result = "";
  for (const QPointer<Geometry>& geometry :
       oi::snapshot(this->getTargetGeometries())) {
    if (geometry.isNull()) {
      continue;
    }

    if (result.compare("") == 0) {
      result.append(geometry->getFeatureName());
    } else {
      result.append(QString(", %1").arg(geometry->getFeatureName()));
    }
  }
  return result;
}

/*!
 * \brief Observation::getDisplayX
 * \param type
 * \param digits
 * \return
 */
QString Observation::getDisplayX(const UnitType& type, const int& digits,
                                 const bool& /*showDiff*/) const {
  if (this->isValid && this->isSolved) {
    return QString::number(convertFromDefault(this->xyz.getAt(0), type), 'f',
                           digits);
  }
  return QString("");
}

/*!
 * \brief Observation::getDisplayY
 * \param type
 * \param digits
 * \return
 */
QString Observation::getDisplayY(const UnitType& type, const int& digits,
                                 const bool& /*showDiff*/) const {
  if (this->isValid && this->isSolved) {
    return QString::number(convertFromDefault(this->xyz.getAt(1), type), 'f',
                           digits);
  }
  return QString("");
}

/*!
 * \brief Observation::getDisplayZ
 * \param type
 * \param digits
 * \return
 */
QString Observation::getDisplayZ(const UnitType& type, const int& digits,
                                 const bool& /*showDiff*/) const {
  if (this->isValid && this->isSolved) {
    return QString::number(convertFromDefault(this->xyz.getAt(2), type), 'f',
                           digits);
  }
  return QString("");
}

/*!
 * \brief Observation::getDisplaySigmaX
 * \param type
 * \param digits
 * \return
 */
QString Observation::getDisplaySigmaX(const UnitType& type,
                                      const int& digits) const {
  if (this->isValid && this->isSolved) {
    return QString::number(convertFromDefault(this->sigmaXyz.getAt(0), type),
                           'f', digits);
  }
  return QString("");
}

/*!
 * \brief Observation::getDisplaySigmaY
 * \param type
 * \param digits
 * \return
 */
QString Observation::getDisplaySigmaY(const UnitType& type,
                                      const int& digits) const {
  if (this->isValid && this->isSolved) {
    return QString::number(convertFromDefault(this->sigmaXyz.getAt(1), type),
                           'f', digits);
  }
  return QString("");
}

/*!
 * \brief Observation::getDisplaySigmaZ
 * \param type
 * \param digits
 * \return
 */
QString Observation::getDisplaySigmaZ(const UnitType& type,
                                      const int& digits) const {
  if (this->isValid && this->isSolved) {
    return QString::number(convertFromDefault(this->sigmaXyz.getAt(2), type),
                           'f', digits);
  }
  return QString("");
}

/*!
 * \brief Observation::getDisplayI
 * \param digits
 * \return
 */
QString Observation::getDisplayI(const int& digits) const {
  if (this->isValid && this->isSolved) {
    return QString::number(this->ijk.getAt(0), 'f', digits);
  }
  return QString("");
}

/*!
 * \brief Observation::getDisplayJ
 * \param digits
 * \return
 */
QString Observation::getDisplayJ(const int& digits) const {
  if (this->isValid && this->isSolved) {
    return QString::number(this->ijk.getAt(1), 'f', digits);
  }
  return QString("");
}

/*!
 * \brief Observation::getDisplayK
 * \param digits
 * \return
 */
QString Observation::getDisplayK(const int& digits) const {
  if (this->isValid && this->isSolved) {
    return QString::number(this->ijk.getAt(2), 'f', digits);
  }
  return QString("");
}

/*!
 * \brief Observation::getDisplaySigmaI
 * \param type
 * \param digits
 * \return
 */
QString Observation::getDisplaySigmaI(const int& digits) const {
  if (this->isValid && this->isSolved) {
    return QString::number(this->sigmaIjk.getAt(0), 'f', digits);
  }
  return QString("");
}

/*!
 * \brief Observation::getDisplaySigmaJ
 * \param digits
 * \return
 */
QString Observation::getDisplaySigmaJ(const int& digits) const {
  if (this->isValid && this->isSolved) {
    return QString::number(this->sigmaIjk.getAt(1), 'f', digits);
  }
  return QString("");
}

/*!
 * \brief Observation::getDisplaySigmaK
 * \param digits
 * \return
 */
QString Observation::getDisplaySigmaK(const int& digits) const {
  if (this->isValid && this->isSolved) {
    return QString::number(this->sigmaIjk.getAt(2), 'f', digits);
  }
  return QString("");
}

/*!
 * \brief Observation::getDisplayIsValid
 * \return
 */
QString Observation::getDisplayIsValid() const {
  return this->isValid ? "true" : "false";
}

/*!
 * \brief Observation::getDisplayIsSolved
 * \return
 */
QString Observation::getDisplayIsSolved() const {
  return this->isSolved ? "true" : "false";
}

/*!
 * \brief Observation::toOpenIndyXML
 * \param xmlDoc
 * \return
 */
QDomElement Observation::toOpenIndyXML(QDomDocument& xmlDoc) const {
  QDomElement observation = Element::toOpenIndyXML(xmlDoc);

  if (observation.isNull()) {
    return observation;
  }

  observation.setTagName("observation");

  // add observation attributes
  if (this->xyz.getSize() >= 3) {
    observation.setAttribute("x", this->xyz.getAt(0));
    observation.setAttribute("y", this->xyz.getAt(1));
    observation.setAttribute("z", this->xyz.getAt(2));
  } else {
    observation.setAttribute("x", 0.0);
    observation.setAttribute("y", 0.0);
    observation.setAttribute("z", 0.0);
  }
  if (this->ijk.getSize() >= 3) {
    observation.setAttribute("i", this->ijk.getAt(0));
    observation.setAttribute("j", this->ijk.getAt(1));
    observation.setAttribute("k", this->ijk.getAt(2));
  } else {
    observation.setAttribute("i", 0.0);
    observation.setAttribute("j", 0.0);
    observation.setAttribute("k", 1.0);
  }
  if (this->sigmaXyz.getSize() >= 3) {
    observation.setAttribute("sigmaX", this->sigmaXyz.getAt(0));
    observation.setAttribute("sigmaY", this->sigmaXyz.getAt(1));
    observation.setAttribute("sigmaZ", this->sigmaXyz.getAt(2));
  } else {
    observation.setAttribute("sigmaX", 0.0);
    observation.setAttribute("sigmaY", 0.0);
    observation.setAttribute("sigmaZ", 0.0);
  }
  if (this->sigmaIjk.getSize() >= 3) {
    observation.setAttribute("sigmaI", this->sigmaIjk.getAt(0));
    observation.setAttribute("sigmaJ", this->sigmaIjk.getAt(1));
    observation.setAttribute("sigmaK", this->sigmaIjk.getAt(2));
  } else {
    observation.setAttribute("sigmaI", 0.0);
    observation.setAttribute("sigmaJ", 0.0);
    observation.setAttribute("sigmaK", 0.0);
  }
  /*observation.setAttribute("sigmaX", 0.0);
  observation.setAttribute("sigmaY", 0.0);
  observation.setAttribute("sigmaZ", 0.0);*/

  observation.setAttribute("isValid", this->isValid);
  observation.setAttribute("isSolved", this->isSolved);
  observation.setAttribute("isDummyPoint", this->isDummyPoint);

  // add station
  if (!this->station.isNull()) {
    QDomElement station = xmlDoc.createElement("station");
    station.setAttribute("ref", this->station->getId());
    observation.appendChild(station);
  }

  // add reading
  if (!this->reading.isNull()) {
    QDomElement reading = this->reading->toOpenIndyXML(xmlDoc);
    if (!reading.isNull()) {
      observation.appendChild(reading);
    }
  }

  // add measuredTargetGeometry
  if (!this->measuredTargetGeometry.isNull()) {
    QDomElement measuredTargetGeometry =
        xmlDoc.createElement("measuredTargetGeometry");
    measuredTargetGeometry.setAttribute("ref",
                                        this->measuredTargetGeometry->getId());
    observation.appendChild(measuredTargetGeometry);
  }

  return observation;
}

/*!
 * \brief Observation::fromOpenIndyXML
 * \param xmlElem
 * \return
 */
bool Observation::fromOpenIndyXML(QDomElement& xmlElem) {
  bool result = Element::fromOpenIndyXML(xmlElem);

  if (result) {
    // set observation attributes
    if (!xmlElem.hasAttribute("isValid") || !xmlElem.hasAttribute("isSolved") ||
        !xmlElem.hasAttribute("x") || !xmlElem.hasAttribute("y") ||
        !xmlElem.hasAttribute("z") || !xmlElem.hasAttribute("sigmaX") ||
        !xmlElem.hasAttribute("sigmaY") || !xmlElem.hasAttribute("sigmaZ")) {
      return false;
    }
    this->isValid = xmlElem.attribute("isValid").toInt();
    this->isSolved = xmlElem.attribute("isSolved").toInt();
    this->isDummyPoint = xmlElem.hasAttribute("isDummyPoint")
                             ? xmlElem.attribute("isDummyPoint").toInt()
                             : false;
    this->xyz.setAt(0, xmlElem.attribute("x").toDouble());
    this->xyz.setAt(1, xmlElem.attribute("y").toDouble());
    this->xyz.setAt(2, xmlElem.attribute("z").toDouble());
    this->xyz.setAt(3, 1.0);
    this->sigmaXyz.setAt(0, xmlElem.attribute("sigmaX").toDouble());
    this->sigmaXyz.setAt(1, xmlElem.attribute("sigmaY").toDouble());
    this->sigmaXyz.setAt(2, xmlElem.attribute("sigmaZ").toDouble());

    this->ijk.setAt(0, xmlElem.attribute("i").toDouble());
    this->ijk.setAt(1, xmlElem.attribute("j").toDouble());
    this->ijk.setAt(2, xmlElem.attribute("k").toDouble());
    this->ijk.setAt(3, 1.0);
    this->sigmaIjk.setAt(0, xmlElem.attribute("sigmaI").toDouble());
    this->sigmaIjk.setAt(1, xmlElem.attribute("sigmaJ").toDouble());
    this->sigmaIjk.setAt(2, xmlElem.attribute("sigmaK").toDouble());

    // set reading
    QDomElement reading = xmlElem.firstChildElement("reading");
    if (reading.isNull()) {
      return false;
    }
    this->reading = new Reading();
    result = this->reading->fromOpenIndyXML(reading);
    this->reading->setObservation(this);
  }

  return result;
}
