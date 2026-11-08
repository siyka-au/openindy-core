#include "coordinate_system.h"

#include <iterator>
#include <vector>

#include "bundle_adjustment.h"
#include "containers.h"
#include "feature_wrapper.h"
#include "geometry.h"
#include "observation.h"
#include "oi_job.h"
#include "snapshot.h"
#include "string_util.h"
#include "trafo_param.h"

using namespace oi;
using namespace oi::math;

/*!
 * \brief CoordinateSystem::CoordinateSystem
 */
CoordinateSystem::CoordinateSystem()
    : Feature(),
      isStationSystem(false),
      isBundleSystem(false),
      isActiveCoordinateSystem(false) {
  // set up feature wrapper
  if (!this->selfFeature.isNull()) {
    this->selfFeature->setCoordinateSystem(this);
  }
}

/*!
 * \brief CoordinateSystem::CoordinateSystem
 * \param isBundleSystem
 */
CoordinateSystem::CoordinateSystem(bool isBundleSystem)
    : Feature(),
      isStationSystem(false),
      isBundleSystem(isBundleSystem),
      isActiveCoordinateSystem(false) {
  // set up feature wrapper
  if (!this->selfFeature.isNull()) {
    this->selfFeature->setCoordinateSystem(this);
  }
}

/*!
 * \brief CoordinateSystem::CoordinateSystem
 * \param station
 */
CoordinateSystem::CoordinateSystem(const oi::Guarded<Station>& station)
    : Feature(),
      isStationSystem(false),
      isBundleSystem(false),
      isActiveCoordinateSystem(false) {
  // set up feature wrapper
  if (!this->selfFeature.isNull()) {
    this->selfFeature->setCoordinateSystem(this);
  }

  // set up corresponding station
  if (!station.isNull()) {
    this->isStationSystem = true;
    this->station = station;
  }
}

/*!
 * \brief CoordinateSystem::CoordinateSystem
 * \param copy
 */
CoordinateSystem::CoordinateSystem(const CoordinateSystem& copy) : Feature() {
  // set up feature wrapper
  if (!this->selfFeature.isNull()) {
    this->selfFeature->setCoordinateSystem(this);
  }

  // copy attributes
  this->name = copy.name;
  this->id = copy.id;
  this->origin = copy.origin;
  this->xAxis = copy.xAxis;
  this->yAxis = copy.yAxis;
  this->zAxis = copy.zAxis;
  this->isStationSystem = copy.isStationSystem;
  this->isBundleSystem = copy.isBundleSystem;
  this->isActiveCoordinateSystem = copy.isActiveCoordinateSystem;
  this->expansionOrigin = copy.expansionOrigin;
}

/*!
 * \brief CoordinateSystem::operator =
 * \param copy
 * \return
 */
CoordinateSystem& CoordinateSystem::operator=(const CoordinateSystem& copy) {
  // set up feature wrapper
  if (!this->selfFeature.isNull()) {
    this->selfFeature->setCoordinateSystem(this);
  }

  // copy attributes
  this->name = copy.name;
  this->id = copy.id;
  this->origin = copy.origin;
  this->xAxis = copy.xAxis;
  this->yAxis = copy.yAxis;
  this->zAxis = copy.zAxis;
  this->isStationSystem = copy.isStationSystem;
  this->isBundleSystem = copy.isBundleSystem;
  this->isActiveCoordinateSystem = copy.isActiveCoordinateSystem;
  this->expansionOrigin = copy.expansionOrigin;

  return *this;
}

/*!
 * \brief CoordinateSystem::~CoordinateSystem
 */
CoordinateSystem::~CoordinateSystem() {
  // delete all observations made from this station coordinate system (only if
  // this is a station system)
  while (!this->ownedObservations.empty()) {
    std::unique_ptr<Observation> doomed =
        std::move(this->ownedObservations.back());
    this->ownedObservations.pop_back();
    doomed.reset();  // detaches itself from the lists below
  }

  // The transformation parameter sets and the nominals referenced from here
  // are features of the job: the job's FeatureContainer owns them, and
  // OiJob::removeFeature removes a system's parameter sets with it.

  // the bundle adjustment is freed by ownedBundle
}

/*!
 * \brief CoordinateSystem::getIsActiveCoordinateSystem
 * \return
 */
const bool& CoordinateSystem::getIsActiveCoordinateSystem() const {
  return this->isActiveCoordinateSystem;
}

/*!
 * \brief CoordinateSystem::setActiveCoordinateSystemState
 * \param isActiveCoordinateSystem
 */
void CoordinateSystem::setActiveCoordinateSystemState(
    const bool& isActiveCoordinateSystem) {
  if (this->isActiveCoordinateSystem != isActiveCoordinateSystem) {
    this->isActiveCoordinateSystem = isActiveCoordinateSystem;
    this->notify(&FeatureObserver::onActiveCoordinateSystemChanged, this->id);
  }
}

/*!
 * \brief CoordinateSystem::getIsStationSystem
 * \return
 */
const bool& CoordinateSystem::getIsStationSystem() const {
  return this->isStationSystem;
}

/*!
 * \brief CoordinateSystem::getStation
 * \return
 */
const oi::Guarded<Station>& CoordinateSystem::getStation() const {
  return this->station;
}

/*!
 * \brief CoordinateSystem::getIsBundleSystem
 * \return
 */
const bool& CoordinateSystem::getIsBundleSystem() const {
  return this->isBundleSystem;
}

/*!
 * \brief CoordinateSystem::getOrigin
 * \return
 */
const Position& CoordinateSystem::getOrigin() const { return this->origin; }

/*!
 * \brief CoordinateSystem::getXAxis
 * \return
 */
const Direction& CoordinateSystem::getXAxis() const { return this->xAxis; }

/*!
 * \brief CoordinateSystem::getYAxis
 * \return
 */
const Direction& CoordinateSystem::getYAxis() const { return this->yAxis; }

/*!
 * \brief CoordinateSystem::getZAxis
 * \return
 */
const Direction& CoordinateSystem::getZAxis() const { return this->zAxis; }

/*!
 * \brief CoordinateSystem::setCoordinateSystem
 * \param origin
 * \param xAxis
 * \param yAxis
 * \param zAxis
 */
void CoordinateSystem::setCoordinateSystem(const Position& origin,
                                           const Direction& xAxis,
                                           const Direction& yAxis,
                                           const Direction& zAxis) {
  this->origin = origin;
  this->xAxis = xAxis;
  this->yAxis = yAxis;
  this->zAxis = zAxis;

  if (!this->station.isNull()) {
    this->station->setCoordinateSystem(origin, xAxis, yAxis, zAxis);
  }
}

/*!
 * \brief CoordinateSystem::getExpansionOrigin
 * \return
 */
const Position& CoordinateSystem::getExpansionOrigin() const {
  return this->expansionOrigin;
}

/*!
 * \brief CoordinateSystem::setExpansionOrigin
 * \param expansionOrigin
 */
void CoordinateSystem::setExpansionOrigin(const Position& expansionOrigin) {
  this->expansionOrigin = expansionOrigin;
}

/*!
 * \brief CoordinateSystem::getObservations
 * \return
 */
const std::vector<oi::Guarded<Observation> >&
CoordinateSystem::getObservations() const {
  return this->observationsList;
}

/*!
 * \brief CoordinateSystem::getObservation
 * \param observationId
 * \return
 */
oi::Guarded<Observation> CoordinateSystem::getObservation(
    const int& observationId) const {
  return oi::value(this->observationsMap, observationId,
                   oi::Guarded<Observation>());
}

/*!
 * \brief CoordinateSystem::addObservation
 * \param observation
 * \return
 */
bool CoordinateSystem::addObservation(std::unique_ptr<Observation> owned) {
  // check if this is a station system (the observation is freed otherwise)
  if (!this->isStationSystem || this->station.isNull()) {
    return false;
  }

  if (owned && !owned->getReading().isNull() && owned->getStation().isNull()) {
    const oi::Guarded<Observation> observation(owned.get());

    // set observation's station
    observation->setStation(this->station);

    // generate unique id for the observation and its corresponding reading
    if (!this->job.isNull()) {
      observation->id = this->job->generateUniqueId();
      observation->getReading()->id = this->job->generateUniqueId();

      // D19/D20: an Observation is now itself a Feature (Point) but
      // is never added through OiJob::addFeature/setJob() the
      // ordinary way - it needs a job pointer anyway, for
      // getTargetGeometries()'s id lookups and for
      // rejectIfNotEditable()'s message, so set it directly here
      //(friend access) rather than inventing a second path.
      observation->job = this->job;
    }

    // add observation to the coordinate system
    this->observationsList.push_back(observation);
    this->observationsMap.insert_or_assign(observation->getId(), observation);
    this->ownedObservations.push_back(std::move(owned));

    this->notify(&FeatureObserver::onObservationsChanged, this->id,
                 observation->getId());

    return true;
  }

  return false;
}

/*!
 * \brief CoordinateSystem::deleteObservation
 * Destroys the observation if this system owns it.
 * \param obs
 */
void CoordinateSystem::deleteObservation(const oi::Guarded<Observation>& obs) {
  if (obs.isNull()) {
    return;
  }
  const auto it = std::find_if(this->ownedObservations.begin(),
                               this->ownedObservations.end(),
                               [&obs](const std::unique_ptr<Observation>& o) {
                                 return o.get() == obs.data();
                               });
  if (it == this->ownedObservations.end()) {
    return;
  }

  // move it out first: its destructor detaches itself from this system
  std::unique_ptr<Observation> doomed = std::move(*it);
  this->ownedObservations.erase(it);
  doomed.reset();
}

/*!
 * \brief CoordinateSystem::getTransformationParameters
 * \return
 */
const std::vector<oi::Guarded<TrafoParam> >&
CoordinateSystem::getTransformationParameters() const {
  return this->trafoParams;
}

/*!
 * \brief CoordinateSystem::getTransformationParameters
 * \param to
 * \return
 */
const std::vector<oi::Guarded<TrafoParam> >
CoordinateSystem::getTransformationParameters(
    const oi::Guarded<CoordinateSystem>& to) const {
  std::vector<oi::Guarded<TrafoParam> > result;

  // check the destination system
  if (to.isNull()) {
    return result;
  }

  // run through all trafo params and select those whose destination system
  // equals "to"
  for (const oi::Guarded<TrafoParam>& trafo : oi::snapshot(this->trafoParams)) {
    if (!trafo.isNull() && !trafo->getDestinationSystem().isNull() &&
        !trafo->getStartSystem().isNull()) {
      if (trafo->getDestinationSystem() == to ||
          trafo->getStartSystem() == to) {
        result.push_back(trafo);
      }
    }
  }

  return result;
}

/*!
 * \brief CoordinateSystem::addTransformationParameter
 * \param trafoParam
 * \return
 */
bool CoordinateSystem::addTransformationParameter(
    const oi::Guarded<TrafoParam>& trafoParam) {
  if (!trafoParam.isNull() && !trafoParam->getStartSystem().isNull() &&
      !trafoParam->getDestinationSystem().isNull()) {
    // check if the trafo param is already contained
    if (oi::contains(this->trafoParams, trafoParam)) {
      return false;
    }

    // check if the trafoParam is in the same job
    if (!this->job.isNull() &&
        !oi::contains(this->job->getTransformationParametersList(),
                      trafoParam)) {
      return false;
    }

    // check if this system is either start or destination system
    if (trafoParam->getStartSystem() != this &&
        trafoParam->getDestinationSystem() != this) {
      return false;
    }

    this->trafoParams.push_back(trafoParam);

    this->notify(&FeatureObserver::onTransformationParametersChanged, this->id);

    return true;
  }

  return false;
}

/*!
 * \brief CoordinateSystem::removeTransformationParameter
 * \param trafoParam
 * \return
 */
bool CoordinateSystem::removeTransformationParameter(
    const oi::Guarded<TrafoParam>& trafoParam) {
  if (!trafoParam.isNull()) {
    bool result = oi::removeOne(this->trafoParams, trafoParam);
    this->notify(&FeatureObserver::onTransformationParametersChanged, this->id);
    return result;
  }
  return false;
}

/*!
 * \brief CoordinateSystem::getNominals
 * \return
 */
const std::vector<oi::Guarded<FeatureWrapper> >& CoordinateSystem::getNominals()
    const {
  return this->nominalsList;
}

/*!
 * \brief CoordinateSystem::addNominal
 * \param nominal
 * \return
 */
bool CoordinateSystem::addNominal(const oi::Guarded<FeatureWrapper>& nominal) {
  // check if this is a nominal system
  if (this->isStationSystem) {
    return false;
  }

  // only a user-owned geometry can be a nominal in this system (D19) -
  // a fit-owned actual has no business here
  if (!nominal.isNull() && !nominal->getGeometry().isNull() &&
      !nominal->getGeometry()->getOwnerDescription().has_value()) {
    // check if the nominal has already been added
    if (this->nominalsMap.contains(nominal->getGeometry()->getId())) {
      return false;
    }

    // check if the nominal is in the same job
    if (!this->job.isNull()) {
      oi::Guarded<FeatureWrapper> jobFeature =
          this->job->getFeatureById(nominal->getGeometry()->getId());
      if (jobFeature.isNull() || jobFeature->getGeometry().isNull() ||
          jobFeature->getGeometry() != nominal->getGeometry()) {
        return false;
      }
    }

    nominal->getGeometry()->setNominalSystem(this);
    this->nominalsList.push_back(nominal);
    this->nominalsMap.insert_or_assign(nominal->getGeometry()->getId(),
                                       nominal);

    this->notify(&FeatureObserver::onNominalsChanged, this->id);

    return true;
  }

  return false;
}

/*!
 * \brief CoordinateSystem::addNominals
 * \param nominals
 * \return
 */
bool CoordinateSystem::addNominals(
    const std::vector<oi::Guarded<FeatureWrapper> >& nominals) {
  if (nominals.size() > 0) {
    int oldSize = this->nominalsList.size();

    // add all nominals without emitting the corresponding signals
    this->blockSignals(true);
    for (const oi::Guarded<FeatureWrapper>& geom : oi::snapshot(nominals)) {
      this->addNominal(geom);
    }
    this->blockSignals(false);

    if (std::ssize(this->nominalsList) > oldSize) {
      this->notify(&FeatureObserver::onNominalsChanged, this->id);
      return true;
    }
  }

  return false;
}

/*!
 * \brief CoordinateSystem::removeNominal
 * \param nominal
 * \return
 */
bool CoordinateSystem::removeNominal(
    const oi::Guarded<FeatureWrapper>& nominal) {
  if (!nominal.isNull() && !nominal->getGeometry().isNull()) {
    // get nominal pointer to remove
    oi::Guarded<FeatureWrapper> removeNominal = nominal;
    if (this->nominalsMap.contains(nominal->getGeometry()->getId())) {
      removeNominal = this->nominalsMap[nominal->getGeometry()->getId()];
    }

    if (this->nominalsMap.erase(nominal->getGeometry()->getId()) == 1) {
      oi::removeOne(this->nominalsList, removeNominal);
      this->notify(&FeatureObserver::onNominalsChanged, this->id);
      return true;
    }
  }

  return false;
}

/*!
 * \brief CoordinateSystem::removeNominal
 * \param featureId
 * \return
 */
bool CoordinateSystem::removeNominal(const int& featureId) {
  oi::Guarded<FeatureWrapper> nominal =
      oi::value(this->nominalsMap, featureId, oi::Guarded<FeatureWrapper>());

  if (!nominal.isNull()) {
    this->nominalsMap.erase(featureId);
    oi::removeOne(this->nominalsList, nominal);
    this->notify(&FeatureObserver::onNominalsChanged, this->id);
    return true;
  }

  return false;
}

/*!
 * \brief CoordinateSystem::getBundleTemplate
 * \return
 */
const std::string& CoordinateSystem::getBundleTemplate() const {
  return this->bundleTemplate;
}

/*!
 * \brief CoordinateSystem::setBundleTemplate
 * \param bundleTemplate
 */
void CoordinateSystem::setBundleTemplate(const std::string& bundleTemplate) {
  this->bundleTemplate = bundleTemplate;
}

/*!
 * \brief CoordinateSystem::getBundlePlugin
 * \return
 */
const oi::Guarded<BundleAdjustment>& CoordinateSystem::getBundlePlugin() const {
  return this->bundlePlugin;
}

/*!
 * \brief CoordinateSystem::setBundlePlugin
 * \param bundle
 */
bool CoordinateSystem::setBundlePlugin(
    std::unique_ptr<BundleAdjustment> bundle) {
  // check if the system is a bundle system
  if (!this->isBundleSystem) {
    return false;  // the bundle is freed
  }

  // check bundle plugin
  if (!bundle) {
    return false;
  }

  // pass coordinate system to bundle plugin
  bundle->setBundleSystem(this);

  // set bundle (the previous one, if any, is freed)
  this->bundlePlugin = bundle.get();
  this->ownedBundle = std::move(bundle);
  this->notify(&FeatureObserver::onBundlePluginChanged,
               this->id);  // TODO OI-932 not used
  return true;
}

/*!
 * \brief CoordinateSystem::recalc
 */
void CoordinateSystem::recalc() { Feature::recalc(); }

/*!
 * \brief CoordinateSystem::getDisplayX
 * \param type
 * \param digits
 * \param showDiff
 * \return
 */
std::string CoordinateSystem::getDisplayX(const UnitType& type,
                                          const int& digits,
                                          const bool& /*showDiff*/) const {
  if (this->getIsActiveCoordinateSystem()) {
    return oi::number(
        convertFromDefault(this->origin.getVector().getAt(0), type), 'f',
        digits);
  }
  for (oi::Guarded<TrafoParam> trafoP : oi::snapshot(this->trafoParams)) {
    if (trafoP->getIsUsed()) {
      return oi::number(
          convertFromDefault(this->origin.getVector().getAt(0), type), 'f',
          digits);
    }
  }
  return "";
}

/*!
 * \brief CoordinateSystem::getDisplayY
 * \param type
 * \param digits
 * \param showDiff
 * \return
 */
std::string CoordinateSystem::getDisplayY(const UnitType& type,
                                          const int& digits,
                                          const bool& /*showDiff*/) const {
  if (this->getIsActiveCoordinateSystem()) {
    return oi::number(
        convertFromDefault(this->origin.getVector().getAt(1), type), 'f',
        digits);
  }
  for (oi::Guarded<TrafoParam> trafoP : oi::snapshot(this->trafoParams)) {
    if (trafoP->getIsUsed()) {
      return oi::number(
          convertFromDefault(this->origin.getVector().getAt(1), type), 'f',
          digits);
    }
  }
  return "";
}

/*!
 * \brief CoordinateSystem::getDisplayZ
 * \param type
 * \param digits
 * \param showDiff
 * \return
 */
std::string CoordinateSystem::getDisplayZ(const UnitType& type,
                                          const int& digits,
                                          const bool& /*showDiff*/) const {
  if (this->getIsActiveCoordinateSystem()) {
    return oi::number(
        convertFromDefault(this->origin.getVector().getAt(2), type), 'f',
        digits);
  }
  for (oi::Guarded<TrafoParam> trafoP : oi::snapshot(this->trafoParams)) {
    if (trafoP->getIsUsed()) {
      return oi::number(
          convertFromDefault(this->origin.getVector().getAt(2), type), 'f',
          digits);
    }
  }
  return "";
}

/*!
 * \brief Plane::getDisplayPrimaryI
 * \param digits
 * \param showDiff
 * \return
 */
std::string CoordinateSystem::getDisplayPrimaryI(
    const int& digits, const bool& /*showDiff*/) const {
  return oi::number(this->zAxis.getVector().getAt(0), 'f', digits);
}

/*!
 * \brief Plane::getDisplayPrimaryJ
 * \param digits
 * \param showDiff
 * \return
 */
std::string CoordinateSystem::getDisplayPrimaryJ(
    const int& digits, const bool& /*showDiff*/) const {
  return oi::number(this->zAxis.getVector().getAt(1), 'f', digits);
}

/*!
 * \brief Plane::getDisplayPrimaryK
 * \param digits
 * \param showDiff
 * \return
 */
std::string CoordinateSystem::getDisplayPrimaryK(
    const int& digits, const bool& /*showDiff*/) const {
  return oi::number(this->zAxis.getVector().getAt(2), 'f', digits);
}

/*!
 * \brief CoordinateSystem::getDisplayExpansionOriginX
 * \param type
 * \param digits
 * \return
 */
std::string CoordinateSystem::getDisplayExpansionOriginX(
    const UnitType& type, const int& digits) const {
  return oi::number(
      convertFromDefault(this->expansionOrigin.getVector().getAt(0), type), 'f',
      digits);
}

/*!
 * \brief CoordinateSystem::getDisplayExpansionOriginY
 * \param type
 * \param digits
 * \return
 */
std::string CoordinateSystem::getDisplayExpansionOriginY(
    const UnitType& type, const int& digits) const {
  return oi::number(
      convertFromDefault(this->expansionOrigin.getVector().getAt(1), type), 'f',
      digits);
}

/*!
 * \brief CoordinateSystem::getDisplayExpansionOriginZ
 * \param type
 * \param digits
 * \return
 */
std::string CoordinateSystem::getDisplayExpansionOriginZ(
    const UnitType& type, const int& digits) const {
  return oi::number(
      convertFromDefault(this->expansionOrigin.getVector().getAt(2), type), 'f',
      digits);
}

/*!
 * \brief CoordinateSystem::getDisplayXAxisI
 * \param digits
 * \return
 */
std::string CoordinateSystem::getDisplayXAxisI(const int& digits) const {
  return oi::number(this->xAxis.getVector().getAt(0), 'f', digits);
}

/*!
 * \brief CoordinateSystem::getDisplayXAxisJ
 * \param digits
 * \return
 */
std::string CoordinateSystem::getDisplayXAxisJ(const int& digits) const {
  return oi::number(this->xAxis.getVector().getAt(1), 'f', digits);
}

/*!
 * \brief CoordinateSystem::getDisplayXAxisK
 * \param digits
 * \return
 */
std::string CoordinateSystem::getDisplayXAxisK(const int& digits) const {
  return oi::number(this->xAxis.getVector().getAt(2), 'f', digits);
}

/*!
 * \brief CoordinateSystem::getDisplayYAxisI
 * \param digits
 * \return
 */
std::string CoordinateSystem::getDisplayYAxisI(const int& digits) const {
  return oi::number(this->yAxis.getVector().getAt(0), 'f', digits);
}

/*!
 * \brief CoordinateSystem::getDisplayYAxisJ
 * \param digits
 * \return
 */
std::string CoordinateSystem::getDisplayYAxisJ(const int& digits) const {
  return oi::number(this->yAxis.getVector().getAt(1), 'f', digits);
}

/*!
 * \brief CoordinateSystem::getDisplayYAxisK
 * \param digits
 * \return
 */
std::string CoordinateSystem::getDisplayYAxisK(const int& digits) const {
  return oi::number(this->yAxis.getVector().getAt(2), 'f', digits);
}

/*!
 * \brief CoordinateSystem::getDisplayZAxisI
 * \param digits
 * \return
 */
std::string CoordinateSystem::getDisplayZAxisI(const int& digits) const {
  return oi::number(this->zAxis.getVector().getAt(0), 'f', digits);
}

/*!
 * \brief CoordinateSystem::getDisplayZAxisJ
 * \param digits
 * \return
 */
std::string CoordinateSystem::getDisplayZAxisJ(const int& digits) const {
  return oi::number(this->zAxis.getVector().getAt(1), 'f', digits);
}

/*!
 * \brief CoordinateSystem::getDisplayZAxisK
 * \param digits
 * \return
 */
std::string CoordinateSystem::getDisplayZAxisK(const int& digits) const {
  return oi::number(this->zAxis.getVector().getAt(2), 'f', digits);
}

/*!
 * \brief CoordinateSystem::setUpFeatureId
 */
void CoordinateSystem::setUpFeatureId() {
  // check job
  if (this->job.isNull()) {
    return;
  }

  // generate unique ids for observations
  for (const oi::Guarded<Observation>& obs :
       oi::snapshot(this->observationsList)) {
    if (!obs.isNull()) {
      obs->id = this->job->generateUniqueId();
    }
  }

  // feature specific
  Feature::setUpFeatureId();
}

/*!
 * \brief CoordinateSystem::removeObservation
 * \param obs
 */
void CoordinateSystem::removeObservation(const oi::Guarded<Observation>& obs) {
  // check observation
  if (obs.isNull()) {
    return;
  }

  oi::removeOne(this->observationsList, obs);
  this->observationsMap.erase(obs->getId());
}

void CoordinateSystem::resetOriginAndAxis() {
  OiVec o;
  o.add(0.0);
  o.add(0.0);
  o.add(0.0);
  Position origin;
  origin.setVector(o);

  OiVec x;
  x.add(1.0);
  x.add(0.0);
  x.add(0.0);
  x.add(1.0);
  Direction xAxis;
  xAxis.setVector(x);

  OiVec y;
  y.add(0.0);
  y.add(1.0);
  y.add(0.0);
  y.add(1.0);
  Direction yAxis;
  yAxis.setVector(y);

  OiVec z;
  z.add(0.0);
  z.add(0.0);
  z.add(1.0);
  z.add(1.0);
  Direction zAxis;
  zAxis.setVector(z);

  this->setCoordinateSystem(origin, xAxis, yAxis, zAxis);
}

void CoordinateSystem::transformOriginAndAxis(OiMat trafoMat) {
  // create homogeneous rotation matrix
  OiMat rotMat = trafoMat;
  rotMat.setAt(0, 3, 0.0);
  rotMat.setAt(1, 3, 0.0);
  rotMat.setAt(2, 3, 0.0);

  OiVec o;
  o.add(0.0);
  o.add(0.0);
  o.add(0.0);
  o.add(1.0);
  OiVec ot = trafoMat * o;
  Position origin;
  origin.setVector(ot);

  OiVec x;
  x.add(1.0);
  x.add(0.0);
  x.add(0.0);
  x.add(1.0);
  OiVec xt = rotMat * x;
  xt.removeLast();
  xt.normalize();
  xt.add(1.0);
  Direction xAxis;
  xAxis.setVector(xt);

  OiVec y;
  y.add(0.0);
  y.add(1.0);
  y.add(0.0);
  y.add(1.0);
  OiVec yt = rotMat * y;
  yt.removeLast();
  yt.normalize();
  yt.add(1.0);
  Direction yAxis;
  yAxis.setVector(yt);

  OiVec z;
  z.add(0.0);
  z.add(0.0);
  z.add(1.0);
  z.add(1.0);
  OiVec zt = rotMat * z;
  zt.removeLast();
  zt.normalize();
  zt.add(1.0);
  Direction zAxis;
  zAxis.setVector(zt);

  this->setCoordinateSystem(origin, xAxis, yAxis, zAxis);
}
