#include "observation.h"

#include <format>
#include <vector>

#include "containers.h"
#include "coordinatesystem.h"
#include "function.h"
#include "geometry.h"
#include "oijob.h"
#include "snapshot.h"
#include "station.h"
#include "stringutil.h"

using namespace oi;
using namespace oi::math;

/*!
 * \brief Observation::Observation
 */
Observation::Observation()
    : Point(),
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
 */
Observation::Observation(const Observation& copy)
    : Point(copy),
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
 */
Observation::Observation(const OiVec& xyz, bool isValid)
    : Point(),
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
 */
Observation::Observation(const OiVec& xyz, int id, bool isValid)
    : Point(),
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

  std::vector<oi::Guarded<Geometry> > targetGeometries;

  // remove observation and reading from its target geometries
  for (const oi::Guarded<Geometry>& geom :
       oi::snapshot(this->getTargetGeometries())) {
    // check target geometry
    if (geom.isNull()) {
      continue;
    }
    targetGeometries.push_back(geom);

    // remove observation and reading from functions of the targte geometry
    for (const oi::Guarded<Function>& function :
         oi::snapshot(geom->getFunctions())) {
      // check function and remove elements
      if (!function.isNull()) {
        function->removeInputElement(this->getId());
        function->removeInputElement(this->getReading()->getId());
      }
    }
  }
  for (const oi::Guarded<Geometry>& geom : oi::snapshot(targetGeometries)) {
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
std::optional<std::string> Observation::getOwnerDescription() const {
  if (!this->station.isNull()) {
    return std::format("its station \"{}\"", this->station->getFeatureName());
  }
  return std::string("its station");
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
const oi::Guarded<Station>& Observation::getStation() const {
  return this->station;
}

/*!
 * \brief Observation::setStation
 * \param station
 */
void Observation::setStation(const oi::Guarded<Station>& station) {
  if (this->station.isNull() && !station.isNull()) {
    this->station = station;
  }
}

/*!
 * \brief Observation::getTargetGeometries
 * Resolves the inherited Point::intents (D12) back into live Geometry
 * pointers via this observation's job, for the pre-existing callers that
 * want "what geometry is this for" rather than a bare feature id. Returned
 * by value (rather than the old by-reference
 * std::vector<oi::Guarded<Geometry>>&) since there is no single stored list
 * left to bind a reference to - a const& binding a caller takes to the returned
 * temporary is still safe (lifetime extension), so this did not require
 * touching call sites.
 * \return
 */
std::vector<oi::Guarded<Geometry> > Observation::getTargetGeometries() const {
  std::vector<oi::Guarded<Geometry> > result;

  if (this->job.isNull()) {
    return result;
  }

  for (const MeasurementIntent& intent : oi::snapshot(this->getIntents())) {
    const oi::Guarded<FeatureWrapper> feature =
        this->job->getFeatureById(intent.featureId);
    if (!feature.isNull() && !feature->getGeometry().isNull() &&
        !oi::contains(result, feature->getGeometry())) {
      result.push_back(feature->getGeometry());
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
void Observation::addTargetGeometry(
    const oi::Guarded<Geometry>& targetGeometry) {
  if (!targetGeometry.isNull()) {
    const MeasurementIntent intent(targetGeometry->getId(), std::string());

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
    const oi::Guarded<Geometry>& targetGeometry) {
  if (targetGeometry.isNull()) {
    return;
  }

  std::vector<MeasurementIntent> matching;
  for (const MeasurementIntent& intent : oi::snapshot(this->getIntents())) {
    if (intent.featureId == targetGeometry->getId()) {
      matching.push_back(intent);
    }
  }
  if (matching.empty()) {
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
    const oi::Guarded<FeatureWrapper> feature =
        this->job->getFeatureById(geomId);
    if (!feature.isNull() && !feature->getGeometry().isNull()) {
      this->removeTargetGeometry(feature->getGeometry());
    }
  }

  const std::vector<oi::Guarded<Geometry> > remaining =
      this->getTargetGeometries();
  if (this->measuredTargetGeometry.isNull() && remaining.size() == 1) {
    this->measuredTargetGeometry = remaining.at(0);
  }
}

/*!
 * \brief Observation::getMeasuredTargetGeometry
 * \return
 */
const oi::Guarded<Geometry>& Observation::getMeasuredTargetGeometry() const {
  return this->measuredTargetGeometry;
}

/*!
 * \brief Observation::setMeasuredTargetGeometry
 * \param measuredTargetGeom
 */
void Observation::setMeasuredTargetGeometry(
    const oi::Guarded<Geometry>& measuredTargetGeom) {
  if (!measuredTargetGeom.isNull()) {
    this->measuredTargetGeometry = measuredTargetGeom;
  }
}

/*!
 * \brief Observation::getReading
 * \return
 */
const oi::Guarded<Reading>& Observation::getReading() const {
  return this->reading;
}

/*!
 * \brief Observation::setReading
 * \param reading
 */
/*void Observation::setReading(const oi::Guarded<Reading> &reading){
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
std::string Observation::getDisplayId() const { return oi::number(this->id); }

/*!
 * \brief Observation::getDisplayStation
 * \return
 */
std::string Observation::getDisplayStation() const {
  if (this->station.isNull()) {
    return "";
  }
  return this->station->getFeatureName();
}

/*!
 * \brief Observation::getDisplayTargetGeometries
 * \return
 */
std::string Observation::getDisplayTargetGeometries() const {
  std::string result = "";
  for (const oi::Guarded<Geometry>& geometry :
       oi::snapshot(this->getTargetGeometries())) {
    if (geometry.isNull()) {
      continue;
    }

    if (result.compare("") == 0) {
      result.append(geometry->getFeatureName());
    } else {
      result.append(std::format(", {}", geometry->getFeatureName()));
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
std::string Observation::getDisplayX(const UnitType& type, const int& digits,
                                     const bool& /*showDiff*/) const {
  if (this->isValid && this->isSolved) {
    return oi::number(convertFromDefault(this->xyz.getAt(0), type), 'f',
                      digits);
  }
  return std::string("");
}

/*!
 * \brief Observation::getDisplayY
 * \param type
 * \param digits
 * \return
 */
std::string Observation::getDisplayY(const UnitType& type, const int& digits,
                                     const bool& /*showDiff*/) const {
  if (this->isValid && this->isSolved) {
    return oi::number(convertFromDefault(this->xyz.getAt(1), type), 'f',
                      digits);
  }
  return std::string("");
}

/*!
 * \brief Observation::getDisplayZ
 * \param type
 * \param digits
 * \return
 */
std::string Observation::getDisplayZ(const UnitType& type, const int& digits,
                                     const bool& /*showDiff*/) const {
  if (this->isValid && this->isSolved) {
    return oi::number(convertFromDefault(this->xyz.getAt(2), type), 'f',
                      digits);
  }
  return std::string("");
}

/*!
 * \brief Observation::getDisplaySigmaX
 * \param type
 * \param digits
 * \return
 */
std::string Observation::getDisplaySigmaX(const UnitType& type,
                                          const int& digits) const {
  if (this->isValid && this->isSolved) {
    return oi::number(convertFromDefault(this->sigmaXyz.getAt(0), type), 'f',
                      digits);
  }
  return std::string("");
}

/*!
 * \brief Observation::getDisplaySigmaY
 * \param type
 * \param digits
 * \return
 */
std::string Observation::getDisplaySigmaY(const UnitType& type,
                                          const int& digits) const {
  if (this->isValid && this->isSolved) {
    return oi::number(convertFromDefault(this->sigmaXyz.getAt(1), type), 'f',
                      digits);
  }
  return std::string("");
}

/*!
 * \brief Observation::getDisplaySigmaZ
 * \param type
 * \param digits
 * \return
 */
std::string Observation::getDisplaySigmaZ(const UnitType& type,
                                          const int& digits) const {
  if (this->isValid && this->isSolved) {
    return oi::number(convertFromDefault(this->sigmaXyz.getAt(2), type), 'f',
                      digits);
  }
  return std::string("");
}

/*!
 * \brief Observation::getDisplayI
 * \param digits
 * \return
 */
std::string Observation::getDisplayI(const int& digits) const {
  if (this->isValid && this->isSolved) {
    return oi::number(this->ijk.getAt(0), 'f', digits);
  }
  return std::string("");
}

/*!
 * \brief Observation::getDisplayJ
 * \param digits
 * \return
 */
std::string Observation::getDisplayJ(const int& digits) const {
  if (this->isValid && this->isSolved) {
    return oi::number(this->ijk.getAt(1), 'f', digits);
  }
  return std::string("");
}

/*!
 * \brief Observation::getDisplayK
 * \param digits
 * \return
 */
std::string Observation::getDisplayK(const int& digits) const {
  if (this->isValid && this->isSolved) {
    return oi::number(this->ijk.getAt(2), 'f', digits);
  }
  return std::string("");
}

/*!
 * \brief Observation::getDisplaySigmaI
 * \param type
 * \param digits
 * \return
 */
std::string Observation::getDisplaySigmaI(const int& digits) const {
  if (this->isValid && this->isSolved) {
    return oi::number(this->sigmaIjk.getAt(0), 'f', digits);
  }
  return std::string("");
}

/*!
 * \brief Observation::getDisplaySigmaJ
 * \param digits
 * \return
 */
std::string Observation::getDisplaySigmaJ(const int& digits) const {
  if (this->isValid && this->isSolved) {
    return oi::number(this->sigmaIjk.getAt(1), 'f', digits);
  }
  return std::string("");
}

/*!
 * \brief Observation::getDisplaySigmaK
 * \param digits
 * \return
 */
std::string Observation::getDisplaySigmaK(const int& digits) const {
  if (this->isValid && this->isSolved) {
    return oi::number(this->sigmaIjk.getAt(2), 'f', digits);
  }
  return std::string("");
}

/*!
 * \brief Observation::getDisplayIsValid
 * \return
 */
std::string Observation::getDisplayIsValid() const {
  return this->isValid ? "true" : "false";
}

/*!
 * \brief Observation::getDisplayIsSolved
 * \return
 */
std::string Observation::getDisplayIsSolved() const {
  return this->isSolved ? "true" : "false";
}
