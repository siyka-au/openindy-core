#include "oijob.h"

#include <utility>

#include "bundleadjustment.h"
#include "snapshot.h"
using namespace oi;

/*!
 * \brief The feature observer installed on a station's own coordinate system.
 * That system is not a feature of the job, so (as the old signal connections
 * did) only its observations, transformation parameters and active state are
 * passed on - nothing else, in particular not its deletion.
 */
class OiJob::StationSystemObserver : public FeatureObserver {
 public:
  explicit StationSystemObserver(OiJob* job) : job(job) {}

  void onObservationsChanged(const int& coordId, const int& obsId) override {
    this->job->setSystemObservations(coordId, obsId);
  }
  void onTransformationParametersChanged(const int& coordId) override {
    this->job->setSystemTrafoParams(coordId);
  }
  void onActiveCoordinateSystemChanged(const int& coordId) override {
    this->job->setActiveCoordinateSystem(coordId);
  }

 private:
  OiJob* job;
};

/*!
 * \brief OiJob::OiJob
 * \param parent
 */
OiJob::OiJob(QObject* parent)
    : QObject(parent),
      stationSystemObserver(std::make_unique<StationSystemObserver>(this)),
      activeGroup("All Groups") {}

/*!
 * \brief OiJob::~OiJob
 */
OiJob::~OiJob() {
  // tell the observers first, while the job is still intact, then forget
  // them: nothing below reports to anyone any more
  this->notifyObservers(&JobObserver::onJobDestroyed, this);
  {
    const QMutexLocker locker(&this->observersMutex);
    this->observers.clear();
  }

  // no feature may keep a pointer to this job (or to the station system
  // observer) past this point
  for (const QPointer<FeatureWrapper>& feature :
       oi::snapshot(this->featureContainer.getFeaturesList())) {
    this->detachFeature(feature);
  }

  // remove all features
  this->featureContainer.removeAll();

  // delete device pointer
  if (!this->jobDevice.isNull()) {
    delete this->jobDevice.data();
  }
}

/*!
 * \brief OiJob::addObserver
 * Registers a (non-owning) observer; registering it twice has no effect.
 */
bool OiJob::isObserver(const JobObserver* observer) const {
  const QMutexLocker locker(&this->observersMutex);
  return std::find(this->observers.begin(), this->observers.end(), observer) !=
         this->observers.end();
}

void OiJob::addObserver(JobObserver* observer) {
  const QMutexLocker locker(&this->observersMutex);
  if (observer == nullptr ||
      std::find(this->observers.begin(), this->observers.end(), observer) !=
          this->observers.end()) {
    return;
  }
  this->observers.push_back(observer);
}

/*!
 * \brief OiJob::removeObserver
 */
void OiJob::removeObserver(JobObserver* observer) {
  const QMutexLocker locker(&this->observersMutex);
  this->observers.erase(
      std::remove(this->observers.begin(), this->observers.end(), observer),
      this->observers.end());
}

/*!
 * \brief OiJob::sendMessage
 * Reports a job issue to the observers.
 */
void OiJob::sendMessage(const QString& msg, const MessageTypes& msgType,
                        const MessageDestinations& msgDest) const {
  // notifying does not change the job's observable state
  const_cast<OiJob*>(this)->notifyObservers(&JobObserver::onSendMessage, msg,
                                            msgType, msgDest);
}

void OiJob::featuresRecalculated() {
  this->notifyObservers(&JobObserver::onFeaturesRecalculated);
}

void OiJob::featureRecalculated(const int& featureId) {
  this->notifyObservers(&JobObserver::onFeatureRecalculated, featureId);
}

void OiJob::trafoParamRecalculated(const int& featureId) {
  this->notifyObservers(&JobObserver::onTrafoParamRecalculated, featureId);
}

/*!
 * \brief OiJob::getJobName
 * \return
 */
const QString& OiJob::getJobName() const { return this->jobName; }

/*!
 * \brief OiJob::setJobName
 * \param jobName
 */
void OiJob::setJobName(const QString& jobName) { this->jobName = jobName; }

/*!
 * \brief OiJob::getJobDevice
 * \return
 */
const QPointer<QFileDevice>& OiJob::getJobDevice() const {
  return this->jobDevice;
}

/*!
 * \brief OiJob::setJobDevice
 * \param jobDevice
 */
void OiJob::setJobDevice(const QPointer<QFileDevice>& jobDevice) {
  this->jobDevice = jobDevice;
}

/*!
 * \return the last calculated digest of the job content
 */
const QString& OiJob::getDigest() const { return this->digest; }

/*!
 * \brief set the digest of the job content
 */
void OiJob::setDigest(const QString& digest) { this->digest = digest; }

/*!
 * \return true if the project file version compatible with this program version
 */
CompatibilyCheckResult OiJob::checkCompatibilty() const {
  QStringList jobParts = loadedVersion.split(".");
  if (jobParts.length() < 2) {
    return CompatibilyCheckResult::eCheckResult_job_wo_valid_version;
  }

  int jobMajor = jobParts.at(0).toInt();
  int jobMinor = jobParts.at(1).toInt();

  QStringList oiParts = QString(OPENINDY_VERSION).split(".");
  if (oiParts.length() < 2) {
    return CompatibilyCheckResult::eCheckResult_oi_wo_valid_version;
  }

  int oiMajor = oiParts.at(0).toInt();
  int oiMinor = oiParts.at(1).toInt();

  if (jobMajor <= 22 && jobMinor <= 1) {
    return CompatibilyCheckResult::eCheckResult_job_lt_oi_22_1;
  } else if (oiMajor == jobMajor) {
    if (oiMinor == jobMinor) {
      return CompatibilyCheckResult::eCheckResult_match;
    } else if (oiMinor > jobMinor) {
      return CompatibilyCheckResult::eCheckResult_oi_gt_job;
    } else {
      return CompatibilyCheckResult::eCheckResult_oi_lt_job;
    }
  } else if (oiMajor > jobMajor) {
    return CompatibilyCheckResult::eCheckResult_oi_gt_job;
  } else {
    return CompatibilyCheckResult::eCheckResult_oi_lt_job;
  }
}

void OiJob::setLoadedProjectVersion(const QString& loadedVersion) {
  this->loadedVersion = loadedVersion;
}

/*!
 * \brief OiJob::generateUniqueId
 * \return
 */
int OiJob::generateUniqueId() {
  this->nextId++;
  return (this->nextId - 1);
}

/*!
 * \brief OiJob::validateFeatureName
 * A name must be unique among features of the same type. Nominal and
 * actual are no longer distinct namespaces here (D17: they are two
 * ordinary features of the same type, distinguished structurally by
 * their producer, not by a flag), so this no longer branches on
 * nominal/actual at all.
 * \param name
 * \param type
 * \return
 */
bool OiJob::validateFeatureName(const QString& name, const FeatureTypes& type) {
  // do not accept empty names
  if (name.compare("") == 0) {
    return false;
  }

  // get a list of features with name name
  QList<QPointer<FeatureWrapper> > features =
      this->featureContainer.getFeaturesByName(name);

  // accept name if no other feature with that name exists
  if (features.size() == 0) {
    return true;
  }

  if (!getIsGeometry(type)) {  // non-geometry feature
    return false;
  }

  // all equal-name features have to be geometries, and none of them may share
  // this type
  for (const QPointer<FeatureWrapper>& feature : std::as_const(features)) {
    if (feature.isNull()) {
      continue;
    }

    if (feature->getGeometry().isNull()) {
      return false;
    }

    if (feature->getFeatureTypeEnum() == type) {
      return false;
    }
  }

  return true;
}

/*!
 * \brief OiJob::getFeatureCount
 * \return
 */
int OiJob::getFeatureCount() const {
  return this->featureContainer.getFeatureCount();
}

/*!
 * \brief OiJob::getGeometryCount
 * \return
 */
int OiJob::getGeometryCount() const {
  return this->featureContainer.getGeometryCount();
}

/*!
 * \brief OiJob::getFeatureCount
 * \param type
 * \return
 */
int OiJob::getFeatureCount(const FeatureTypes& type) const {
  return this->featureContainer.getFeatureCount(type);
}

/*!
 * \brief OiJob::getFeatureCount
 * \param group
 * \return
 */
int OiJob::getFeatureCount(const QString& group) const {
  return this->featureContainer.getFeatureCount(group);
}

/*!
 * \brief OiJob::getFeatureIdList
 * \return
 */
const QList<int>& OiJob::getFeatureIdList() const {
  return this->featureContainer.getFeatureIdList();
}

/*!
 * \brief OiJob::getFeatureNameList
 * \return
 */
const QStringList& OiJob::getFeatureNameList() const {
  return this->featureContainer.getFeatureNameList();
}

/*!
 * \brief OiJob::getFeatureGroupList
 * \return
 */
const QStringList& OiJob::getFeatureGroupList() const {
  return this->featureContainer.getFeatureGroupList();
}

/*!
 * \brief OiJob::getUsedMeasurementConfigs
 * \return
 */
const QList<MeasurementConfigKey>& OiJob::getUsedMeasurementConfigs() const {
  return this->featureContainer.getUsedMeasurementConfigs();
}

/*!
 * \brief OiJob::getFeaturesList
 * \return
 */
const QList<QPointer<FeatureWrapper> >& OiJob::getFeaturesList() const {
  return this->featureContainer.getFeaturesList();
}

/*!
 * \brief OiJob::getCoordinateSystemsList
 * Returns a list of all nominal systems
 * \return
 */
const QList<QPointer<CoordinateSystem> >& OiJob::getCoordinateSystemsList()
    const {
  return this->featureContainer.getCoordinateSystemsList();
}

/*!
 * \brief OiJob::getStationSystems
 * \return
 */
QList<QPointer<CoordinateSystem> > OiJob::getStationSystemsList() const {
  return this->featureContainer.getStationSystemsList();
}

/*!
 * \brief OiJob::getBundleSystemList
 * \return
 */
/*
QList<QPointer<CoordinateSystem> > OiJob::getBundleSystemList() const{ // TODO
OI-982 remove return this->featureContainer.getBundleSystemList();
}
*/

/*!
 * \brief OiJob::getStationsList
 * \return
 */
const QList<QPointer<Station> >& OiJob::getStationsList() const {
  return this->featureContainer.getStationsList();
}

/*!
 * \brief OiJob::getTransformationParametersList
 * \return
 */
const QList<QPointer<TrafoParam> >& OiJob::getTransformationParametersList()
    const {
  return this->featureContainer.getTransformationParametersList();
}

/*!
 * \brief OiJob::getGeometriesList
 * \return
 */
const QList<QPointer<FeatureWrapper> >& OiJob::getGeometriesList() const {
  return this->featureContainer.getGeometriesList();
}

/*!
 * \brief OiJob::getFeatureById
 * \param featureId
 * \return
 */
QPointer<FeatureWrapper> OiJob::getFeatureById(const int& featureId) const {
  return this->featureContainer.getFeatureById(featureId);
}

/*!
 * \brief OiJob::getFeaturesByName
 * \param name
 * \param startWith use name as "start with" pattern
 * \return
 */
QList<QPointer<FeatureWrapper> > OiJob::getFeaturesByName(
    const QString& name, const bool startWith) const {
  return this->featureContainer.getFeaturesByName(name, startWith);
}

/*!
 * \brief OiJob::getFeaturesByGroup
 * \param group
 * \return
 */
QList<QPointer<FeatureWrapper> > OiJob::getFeaturesByGroup(
    const QString& group) const {
  return this->featureContainer.getFeaturesByGroup(group);
}

/*!
 * \brief OiJob::getFeaturesByType
 * \param type
 * \return
 */
QList<QPointer<FeatureWrapper> > OiJob::getFeaturesByType(
    const FeatureTypes& type) const {
  return this->featureContainer.getFeaturesByType(type);
}

/*!
 * \brief OiJob::getGeometriesByMConfig
 * \param mConfig
 * \return
 */
QList<QPointer<Geometry> > OiJob::getGeometriesByMConfig(
    const MeasurementConfigKey& key) const {
  return this->featureContainer.getGeometriesByMConfig(key);
}

/*!
 * \brief OiJob::getActiveFeature
 * \return
 */
const QPointer<FeatureWrapper>& OiJob::getActiveFeature() const {
  return this->activeFeature;
}

/*!
 * \brief OiJob::getActiveStation
 * \return
 */
const QPointer<Station>& OiJob::getActiveStation() const {
  return this->activeStation;
}

const QPointer<CoordinateSystem>& OiJob::getActiveBundleSystem() const {
  return this->activeBundleSystem;
}

const QPointer<oi::BundleAdjustment>& OiJob::getActiveBundleAdjustment() const {
  static const QPointer<oi::BundleAdjustment> nullBundleAdjustment;
  if (this->activeBundleSystem.isNull()) {
    return nullBundleAdjustment;
  }
  return this->activeBundleSystem->getBundlePlugin();
}

const QJsonObject OiJob::getActiveBundleTemplate() const {
  if (this->activeBundleSystem.isNull()) {
    return QJsonObject();
  }
  return this->activeBundleSystem->getBundleTemplate();
}

/*!
 * \brief OiJob::getActiveCoordinateSystem
 * \return
 */
const QPointer<CoordinateSystem>& OiJob::getActiveCoordinateSystem() const {
  return this->activeCoordinateSystem;
}

/*!
 * \brief OiJob::getActiveGroup
 * \return
 */
const QString& OiJob::getActiveGroup() const { return this->activeGroup; }

/*!
 * \brief OiJob::setActiveGroup
 * \param group
 * \return
 */
bool OiJob::setActiveGroup(const QString& group) {
  if (this->featureContainer.getFeatureGroupList().contains(group) &&
      this->activeGroup.compare(group) != 0) {
    this->activeGroup = group;
    this->notifyObservers(&JobObserver::onActiveGroupChanged);
    return true;
  } else if (group.compare("All Groups") == 0) {
    this->activeGroup = group;
    this->notifyObservers(&JobObserver::onActiveGroupChanged);
    return true;
  }
  return false;
}

/*!
 * \brief OiJob::addFeature
 * \param feature
 * \return
 */
bool OiJob::addFeature(const QPointer<FeatureWrapper>& feature) {
  // check if feature is valid
  if (feature.isNull() || feature->getFeature().isNull()) {
    return false;
  }

  // check attributes and set up relations
  if (!this->checkAndSetUpNewFeature(feature)) {
    return false;
  }

  // connect the feature's signals to slots in OiJob
  this->attachFeature(feature);

  // if a group is set for the new feature emit the group changed signal
  if (feature->getFeature()->getGroupName().compare("") != 0 &&
      !this->featureContainer.getFeatureGroupList().contains(
          feature->getFeature()->getGroupName())) {
    this->notifyObservers(&JobObserver::onAvailableGroupsChanged);
  }

  // features added signals
  this->notifyObservers(&JobObserver::onFeatureSetChanged);
  if (feature->getFeatureTypeEnum() == FeatureTypes::eCoordinateSystemFeature) {
    this->notifyObservers(&JobObserver::onCoordSystemSetChanged);
  } else if (feature->getFeatureTypeEnum() == FeatureTypes::eStationFeature) {
    this->notifyObservers(&JobObserver::onStationSetChanged);
  } else if (feature->getFeatureTypeEnum() ==
             FeatureTypes::eTrafoParamFeature) {
    this->notifyObservers(&JobObserver::onTrafoParamSetChanged);
  } else {
    this->notifyObservers(&JobObserver::onGeometrySetChanged);
  }

  return true;
}

/*!
 * \brief OiJob::addFeatures
 * \param fAttr
 * \return
 */
QList<QPointer<FeatureWrapper> > OiJob::addFeatures(
    const FeatureAttributes& fAttr) {
  QList<QPointer<FeatureWrapper> > result;

  // at least one feature should be created
  if (fAttr.count <= 0) {
    return result;
  }

  // D17: a geometry is no longer flagged nominal/actual up front - both are
  // ordinary features distinguished structurally by their producer function
  // once attached (Controller::addFunctionsAndMConfigs). If the caller named
  // a nominal system it must resolve; if not, this geometry simply has none
  //(the ordinary case for a fit-produced actual).
  QPointer<CoordinateSystem> nominalSystem;
  if (getIsGeometry(fAttr.typeOfFeature) && !fAttr.nominalSystem.isEmpty()) {
    QList<QPointer<FeatureWrapper> > features =
        this->featureContainer.getFeaturesByName(fAttr.nominalSystem);
    if (features.size() != 1 || features.at(0).isNull() ||
        features.at(0)->getCoordinateSystem().isNull() ||
        features.at(0)->getCoordinateSystem()->getIsStationSystem()) {
      this->sendMessage("No valid nominal system specified",
                        MessageTypes::eErrorMessage);
      return result;
    }
    nominalSystem = features.at(0)->getCoordinateSystem();
  }

  // if type is a TrafoParam then start and destination system have to be in the
  // job
  QPointer<CoordinateSystem> startSystem, destSystem;
  if (fAttr.typeOfFeature == FeatureTypes::eTrafoParamFeature) {
    QList<QPointer<FeatureWrapper> > startSystems =
        this->featureContainer.getFeaturesByName(fAttr.startSystem);
    QList<QPointer<FeatureWrapper> > destinationSystems =
        this->featureContainer.getFeaturesByName(fAttr.destinationSystem);
    if (startSystems.size() != 1 || destinationSystems.size() != 1 ||
        startSystems.at(0).isNull() || destinationSystems.at(0).isNull()) {
      this->sendMessage("No valid start and/or destination system specified",
                        MessageTypes::eErrorMessage);
      return result;
    }
    if (!startSystems.at(0)->getCoordinateSystem().isNull()) {
      startSystem = startSystems.at(0)->getCoordinateSystem();
    } else if (!startSystems.at(0)->getStation().isNull() &&
               !startSystems.at(0)
                    ->getStation()
                    ->getCoordinateSystem()
                    .isNull()) {
      startSystem = startSystems.at(0)->getStation()->getCoordinateSystem();
    }
    if (!destinationSystems.at(0)->getCoordinateSystem().isNull()) {
      destSystem = destinationSystems.at(0)->getCoordinateSystem();
    } else if (!destinationSystems.at(0)->getStation().isNull() &&
               !destinationSystems.at(0)
                    ->getStation()
                    ->getCoordinateSystem()
                    .isNull()) {
      destSystem =
          destinationSystems.at(0)->getStation()->getCoordinateSystem();
    }

    if (startSystem.isNull() || destSystem.isNull()) {
      this->sendMessage("No valid start and/or destination system specified",
                        MessageTypes::eErrorMessage);
      return result;
    }
  }

  // create and validate feature names
  QStringList featureNames = this->createFeatureNames(fAttr.name, fAttr.count);
  for (const QString& name : oi::snapshot(featureNames)) {
    if (!validateFeatureName(name, fAttr.typeOfFeature)) {
      this->sendMessage("No valid feature name specified",
                        MessageTypes::eErrorMessage);
      return result;
    }
  }

  // check if the group is a new group
  bool isNewGroup = false;
  if (fAttr.group.compare("") != 0 &&
      !this->featureContainer.getFeatureGroupList().contains(fAttr.group)) {
    isNewGroup = true;
  }

  // create the features and add them to OpenIndy
  for (const QString& name : oi::snapshot(featureNames)) {
    // create geometry (D17: a single ordinary feature - whether it ends
    // up nominal or actual falls out of the producer function attached
    // to it afterwards by Controller::addFeatures, not out of a flag set
    // here)
    if (getIsGeometry(fAttr.typeOfFeature)) {
      // create and check feature
      QPointer<FeatureWrapper> feature =
          this->createFeatureWrapper(fAttr.typeOfFeature);
      if (feature.isNull() || feature->getGeometry().isNull()) {
        continue;
      }

      // pass the job to the feature
      feature->getFeature()->setJob(this);

      // set feature attributes
      feature->getFeature()->name = name;
      feature->getFeature()->group = fAttr.group;
      feature->getGeometry()->isCommon = fAttr.isCommon;

      // set nominal system, if one was named
      if (!nominalSystem.isNull()) {
        feature->getGeometry()->nominalSystem = nominalSystem;
        nominalSystem->nominalsList.append(feature);
        nominalSystem->nominalsMap.insert(feature->getGeometry()->getId(),
                                          feature);

        // set solved state
        if (nominalSystem->getIsActiveCoordinateSystem()) {
          feature->getGeometry()->setIsSolved(true);
        }
      }

      // add and connect feature
      this->featureContainer.addFeature(feature);
      this->attachFeature(feature);

      // add feature to result list
      result.append(feature);
    }

    // create non-geometry feature
    if (!getIsGeometry(fAttr.typeOfFeature)) {
      // create and check feature
      QPointer<FeatureWrapper> feature =
          this->createFeatureWrapper(fAttr.typeOfFeature);
      if (feature.isNull() || feature->getFeature().isNull()) {
        continue;
      }

      // pass the job to the feature
      feature->getFeature()->setJob(this);

      // set feature attributes
      feature->getFeature()->setFeatureName(name);
      feature->getFeature()->group = fAttr.group;

      // if type is trafo param set start and destination system
      if (fAttr.typeOfFeature == FeatureTypes::eTrafoParamFeature) {
        feature->getTrafoParam()->from = startSystem;
        feature->getTrafoParam()->to = destSystem;
        startSystem->trafoParams.append(feature->getTrafoParam());
        destSystem->trafoParams.append(feature->getTrafoParam());

        // check if one system is station and one system is coordinate system,
        // then set datum to true
        if ((startSystem->getIsStationSystem() ||
             startSystem->getIsBundleSystem()) &&
            (!destSystem->getIsBundleSystem()) &&
            !destSystem->getIsStationSystem()) {
          feature->getTrafoParam()->setIsDatumTrafo(true);
        } else if ((destSystem->getIsStationSystem() ||
                    destSystem->getIsBundleSystem()) &&
                   (!startSystem->getIsBundleSystem() &&
                    !startSystem->getIsStationSystem())) {
          feature->getTrafoParam()->setIsDatumTrafo(true);
        } else {
          feature->getTrafoParam()->setIsDatumTrafo(false);
        }

        // set used-state to "use" as default
        feature->getTrafoParam()->setIsUsed(true);
      }

      // if type of feature is a coordinate system
      if (fAttr.typeOfFeature == FeatureTypes::eCoordinateSystemFeature) {
        feature->getCoordinateSystem()->isBundleSystem = fAttr.isBundleSystem;
        if (feature->getCoordinateSystem()->isBundleSystem) {
          this->activeBundleSystem = feature->getCoordinateSystem();
        }
      }

      // add and connect feature
      this->featureContainer.addFeature(feature);
      this->attachFeature(feature);

      // add feature to result list
      result.append(feature);
    }
  }

  // emit signals
  if (isNewGroup) {
    this->notifyObservers(&JobObserver::onAvailableGroupsChanged);
  }
  this->notifyObservers(&JobObserver::onFeatureSetChanged);
  if (fAttr.typeOfFeature == FeatureTypes::eCoordinateSystemFeature) {
    this->notifyObservers(&JobObserver::onCoordSystemSetChanged);
  } else if (fAttr.typeOfFeature == FeatureTypes::eStationFeature) {
    this->notifyObservers(&JobObserver::onStationSetChanged);
  } else if (fAttr.typeOfFeature == FeatureTypes::eTrafoParamFeature) {
    this->notifyObservers(&JobObserver::onTrafoParamSetChanged);
  } else {
    this->notifyObservers(&JobObserver::onGeometrySetChanged);
  }

  return result;
}

/*!
 * \brief OiJob::addFeatures
 * Add one or more features of various types which are not related to each other
 * \param features
 * \return
 */
bool OiJob::addFeatures(const QList<QPointer<FeatureWrapper> >& features,
                        bool overwrite) {
  QList<FeatureTypes> addedFeatureTypes;

  for (const QPointer<FeatureWrapper>& feature : std::as_const(features)) {
    // check if feature is valid
    if (feature.isNull() || feature->getFeature().isNull()) {
      continue;
    }

    // check attributes and set up relations
    if (!this->checkAndSetUpNewFeature(feature, overwrite)) {
      continue;
    }

    // connect the feature's signals to slots in OiJob
    this->attachFeature(feature);

    // add feature type to list of added feature types
    if (!addedFeatureTypes.contains(feature->getFeatureTypeEnum())) {
      addedFeatureTypes.append(feature->getFeatureTypeEnum());
    }
  }

  // features added signals
  this->notifyObservers(&JobObserver::onFeatureSetChanged);
  if (addedFeatureTypes.contains(FeatureTypes::eCoordinateSystemFeature)) {
    this->notifyObservers(&JobObserver::onCoordSystemSetChanged);
    addedFeatureTypes.removeOne(FeatureTypes::eCoordinateSystemFeature);
  } else if (addedFeatureTypes.contains(FeatureTypes::eStationFeature)) {
    this->notifyObservers(&JobObserver::onStationSetChanged);
    addedFeatureTypes.removeOne(FeatureTypes::eStationFeature);
  } else if (addedFeatureTypes.contains(FeatureTypes::eTrafoParamFeature)) {
    this->notifyObservers(&JobObserver::onTrafoParamSetChanged);
    addedFeatureTypes.removeOne(FeatureTypes::eTrafoParamFeature);
  } else if (addedFeatureTypes.size() > 0) {
    this->notifyObservers(&JobObserver::onGeometrySetChanged);
  }

  return true;
}

/*!
 * \brief OiJob::removeFeature
 * \param featureId
 * \return
 */
bool OiJob::removeFeature(const int& featureId) {
  // get the feature by id
  QPointer<FeatureWrapper> feature =
      this->featureContainer.getFeatureById(featureId);
  if (feature.isNull() || feature->getFeature().isNull()) {
    return false;
  }

  // check wether the feature could be deleted
  if (!this->canRemoveFeature(feature)) {
    this->sendMessage(QString("Cannot remove feature %1")
                          .arg(feature->getFeature()->getFeatureName()),
                      MessageTypes::eWarningMessage);
    return false;
  }

  // D19 applied to functions (Stage 7h): deleting the owner deletes what
  // it owns, so it cannot dangle
  this->removeOwnedSubGeometries(feature);

  // possibly reset active group
  if (feature->getFeature()->getGroupName().compare("") != 0 &&
      feature->getFeature()->getGroupName().compare(this->activeGroup) == 0 &&
      this->featureContainer
              .getFeaturesByGroup(feature->getFeature()->getGroupName())
              .size() == 1) {
    this->activeGroup = "";
    this->notifyObservers(&JobObserver::onActiveGroupChanged);
  }

  this->detachFeature(feature);

  bool success = this->featureContainer.removeFeature(featureId);

  this->notifyObservers(&JobObserver::onFeatureSetChanged);

  this->notifyObservers(&JobObserver::onRecalcFeatureSetRequested);

  return success;
}

/*!
 * \brief OiJob::removeFeature
 * \param feature
 * \return
 */
bool OiJob::removeFeature(const QPointer<FeatureWrapper>& feature) {
  // check feature
  if (feature.isNull() || feature->getFeature().isNull()) {
    return false;
  }

  // check wether the feature could be deleted
  if (!this->canRemoveFeature(feature)) {
    this->sendMessage(QString("Cannot remove feature %1")
                          .arg(feature->getFeature()->getFeatureName()),
                      MessageTypes::eWarningMessage);
    return false;
  }

  // D19 applied to functions (Stage 7h): deleting the owner deletes what
  // it owns, so it cannot dangle
  this->removeOwnedSubGeometries(feature);

  // possibly reset active group
  if (feature->getFeature()->getGroupName().compare("") != 0 &&
      feature->getFeature()->getGroupName().compare(this->activeGroup) == 0 &&
      this->featureContainer
              .getFeaturesByGroup(feature->getFeature()->getGroupName())
              .size() == 1) {
    this->activeGroup = "";
    this->notifyObservers(&JobObserver::onActiveGroupChanged);
  }

  this->detachFeature(feature);

  bool success =
      this->featureContainer.removeFeature(feature->getFeature()->getId());

  this->notifyObservers(&JobObserver::onFeatureSetChanged);

  this->notifyObservers(&JobObserver::onRecalcFeatureSetRequested);

  return success;
}

/*!
 * \brief OiJob::removeFeatures
 * \param featureIds
 * \return
 */
bool OiJob::removeFeatures(const QSet<int>& featureIds) {
  bool success = true;

  // check all features wether they could be removed
  for (const int& id : std::as_const(featureIds)) {
    // get the feature by id
    QPointer<FeatureWrapper> feature =
        this->featureContainer.getFeatureById(id);
    if (feature.isNull() || feature->getFeature().isNull()) {
      continue;
    }

    // check wether the feature could be removed
    if (!this->canRemoveFeature(feature)) {
      continue;
    }
  }

  // remove features
  this->blockSignals(true);
  for (const int& id : oi::snapshot(featureIds)) {
    if (!this->removeFeature(id)) {
      success = false;
    }
  }
  this->blockSignals(false);

  this->notifyObservers(&JobObserver::onActiveGroupChanged);
  this->notifyObservers(&JobObserver::onFeatureSetChanged);

  this->notifyObservers(&JobObserver::onRecalcFeatureSetRequested);

  return success;
}

/*!
 * \brief OiJob::removeFeatures
 * \param features
 * \return
 */
bool OiJob::removeFeatures(const QList<QPointer<FeatureWrapper> >& features) {
  bool success = true;

  // check all features wether they could be removed
  for (const QPointer<FeatureWrapper>& feature : std::as_const(features)) {
    // check feature
    if (feature.isNull() || feature->getFeature().isNull()) {
      continue;
    }

    // check wether the feature could be removed
    if (!this->canRemoveFeature(feature)) {
      continue;
    }
  }

  // remove features
  this->blockSignals(true);
  for (const QPointer<FeatureWrapper>& feature : oi::snapshot(features)) {
    if (!this->removeFeature(feature)) {
      success = false;
    }
  }
  this->blockSignals(false);

  this->notifyObservers(&JobObserver::onActiveGroupChanged);
  this->notifyObservers(&JobObserver::onFeatureSetChanged);

  this->notifyObservers(&JobObserver::onRecalcFeatureSetRequested);

  return success;
}

/*!
 * \brief OiJob::removeAll
 */
void OiJob::removeAll() { this->featureContainer.removeAll(); }

/*!
 * \brief OiJob::addFunction
 * \param function
 */
void OiJob::addFunction(const QPointer<Function>& function) {
  // check active feature
  if (this->activeFeature.isNull() ||
      this->activeFeature->getFeature().isNull()) {
    this->sendMessage("No active feature", MessageTypes::eErrorMessage);
    return;
  }

  // check function
  if (function.isNull()) {
    return;
  }

  // connect the function
  QObject::connect(function.data(), &Function::sendMessage, this,
                   &OiJob::sendMessage, Qt::AutoConnection);

  // add the function to the active feature
  this->activeFeature->getFeature()->addFunction(function);

  // set up dependencies
  QMap<int, QList<InputElement> > inputElements = function->getInputElements();
  for (int key : inputElements.keys()) {
    for (const InputElement& element : oi::snapshot(inputElements[key])) {
      this->setUpDependencies(element, this->activeFeature->getFeature());
    }
  }
}

/*!
 * \brief OiJob::addOwnedSubGeometry
 * D19 applied to functions (Stage 7h) - see the declaration's comment.
 * \param owner
 * \param subGeometry
 * \param producerFunction
 * \param neededElementsIndex
 * \return
 */
bool OiJob::addOwnedSubGeometry(const QPointer<FeatureWrapper>& owner,
                                const QPointer<FeatureWrapper>& subGeometry,
                                const QPointer<Function>& producerFunction,
                                const int& neededElementsIndex) {
  // check owner
  if (owner.isNull() || owner->getGeometry().isNull()) {
    this->sendMessage("No valid owner feature for owned sub-geometry",
                      MessageTypes::eErrorMessage);
    return false;
  }
  if (this->featureContainer.getFeatureById(owner->getGeometry()->getId())
          .isNull()) {
    this->sendMessage("Owner feature is not part of this job",
                      MessageTypes::eErrorMessage);
    return false;
  }

  // check sub-geometry: must already be registered in this job, with no
  // function attached yet (this call is the one and only time an owned
  // accessor's producer gets attached)
  if (subGeometry.isNull() || subGeometry->getGeometry().isNull()) {
    this->sendMessage("No valid feature to generate as owned sub-geometry",
                      MessageTypes::eErrorMessage);
    return false;
  }
  if (this->featureContainer.getFeatureById(subGeometry->getGeometry()->getId())
          .isNull()) {
    this->sendMessage("Sub-geometry feature is not part of this job",
                      MessageTypes::eErrorMessage);
    return false;
  }
  if (!subGeometry->getGeometry()->getFunctions().isEmpty()) {
    this->sendMessage("Feature already has a producer function",
                      MessageTypes::eErrorMessage);
    return false;
  }

  // check function
  if (producerFunction.isNull()) {
    this->sendMessage("No valid function for owned sub-geometry",
                      MessageTypes::eErrorMessage);
    return false;
  }

  // connect the function the same way OiJob::addFunction does
  QObject::connect(producerFunction.data(), &Function::sendMessage, this,
                   &OiJob::sendMessage, Qt::AutoConnection);

  // attach the function - mints its id, appends to functionList
  subGeometry->getGeometry()->addFunction(producerFunction);

  // bind the owner as this function's input and wire the ordinary
  // dependency edges (previouslyNeeded/usedFor), exactly as if the user
  // had picked this function and this input by hand (7c-i's path)
  this->addInputFeature(subGeometry, 0, neededElementsIndex, owner);

  // record ownership - makes the feature read-only (getOwnerDescription)
  // and lets the owner find/regenerate/cascade-delete it
  subGeometry->getGeometry()->setOwnerFeature(owner->getGeometry());

  // trigger the initial computation through the ordinary recalc path -
  // not a second recalculation mechanism, the same signal
  // OiJob::removeFeature and friends already emit
  this->notifyObservers(&JobObserver::onRecalcFeatureSetRequested);

  return true;
}

/*!
 * \brief OiJob::removeFunction
 * \param functionIndex
 */
void OiJob::removeFunction(const int& functionIndex) {
  // check active feature
  if (this->activeFeature.isNull() ||
      this->activeFeature->getFeature().isNull()) {
    this->sendMessage("No active feature", MessageTypes::eErrorMessage);
    return;
  }

  // get and check the function at the specified index
  if (functionIndex < 0 ||
      functionIndex >=
          this->activeFeature->getFeature()->getFunctions().size()) {
    return;
  }
  QPointer<Function> function =
      this->activeFeature->getFeature()->getFunctions().at(functionIndex);
  if (function.isNull()) {
    return;
  }

  // get a list of remaining function
  QList<QPointer<Function> > remainingFunctions;
  for (const QPointer<Function>& f :
       oi::snapshot(this->activeFeature->getFeature()->getFunctions())) {
    if (f.isNull() || f == function) {
      continue;
    }
    remainingFunctions.append(f);
  }

  // get a list of input elements and reset dependencies
  QMap<int, QList<InputElement> > inputElements = function->getInputElements();
  for (int key : inputElements.keys()) {
    for (const InputElement& element : oi::snapshot(inputElements[key])) {
      // check if element is contained by any other function of the feature
      bool isStillIncluded = false;
      for (const QPointer<Function>& f : std::as_const(remainingFunctions)) {
        if (f->hasInputElement(element.id)) {
          isStillIncluded = true;
          break;
        }
      }

      // reset dependencies
      if (!isStillIncluded) {
        this->resetDependencies(element, this->activeFeature->getFeature());
      }
    }
  }

  // remove and delete the function
  this->activeFeature->getFeature()->removeFunction(functionIndex);
  delete function.data();
}

/*!
 * \brief OiJob::addInputObservation
 * Adds an existing observation the the specified function of the target feature
 * \param target
 * \param functionPosition
 * \param neededElementsIndex
 * \param observation
 */
void OiJob::addInputObservation(const QPointer<FeatureWrapper>& target,
                                const int& functionPosition,
                                const int& neededElementsIndex,
                                const QPointer<Observation>& observation) {
  // check target feature and observation
  if (target.isNull() || target->getFeature().isNull() ||
      observation.isNull()) {
    return;
  }

  // check function position
  if (functionPosition < 0 ||
      functionPosition >= target->getFeature()->getFunctions().size() ||
      target->getFeature()->getFunctions().at(functionPosition).isNull()) {
    return;
  }

  // create and add input element
  InputElement element(observation->getId());
  element.typeOfElement = ElementTypes::eObservationElement;
  element.observation = observation;
  target->getFeature()
      ->getFunctions()
      .at(functionPosition)
      ->addInputElement(element, neededElementsIndex);

  // create dependencies
  if (!target->getGeometry().isNull()) {
    observation->addTargetGeometry(target->getGeometry());
  }
}

/*!
 * \brief OiJob::addInputPoint
 * Adds an existing point to the specified function of the target feature -
 * the ePointElement counterpart of addInputObservation, and what tag
 * resolution (D12) calls once it has matched a Point's MeasurementIntent
 * to a NeededElement::roleName.
 * \param target
 * \param functionPosition
 * \param neededElementsIndex
 * \param point
 */
void OiJob::addInputPoint(const QPointer<FeatureWrapper>& target,
                          const int& functionPosition,
                          const int& neededElementsIndex,
                          const QPointer<Point>& point) {
  // check target feature and point
  if (target.isNull() || target->getFeature().isNull() || point.isNull()) {
    return;
  }

  // check function position
  if (functionPosition < 0 ||
      functionPosition >= target->getFeature()->getFunctions().size() ||
      target->getFeature()->getFunctions().at(functionPosition).isNull()) {
    return;
  }

  QPointer<Function> function =
      target->getFeature()->getFunctions().at(functionPosition);

  // do not add the same point twice at the same position
  if (function->hasInputElement(point->getId())) {
    return;
  }

  // create and add input element
  InputElement element(point->getId());
  element.typeOfElement = ElementTypes::ePointElement;
  element.point = point;
  element.geometry = point;
  function->addInputElement(element, neededElementsIndex);

  // record the dependency edge (recalculation order, delete-cascade) the
  // same way OiJob::addFunction does for a function's inputs declared up
  // front - this is the incremental-add counterpart, since tag
  // resolution adds points to a function's inputs one at a time, after
  // the function already exists
  this->setUpDependencies(element, target->getFeature());
}

/*!
 * \brief OiJob::resolvePointIntent
 * \param point
 * \param intent
 */
void OiJob::resolvePointIntent(const QPointer<Point>& point,
                               const MeasurementIntent& intent) {
  if (point.isNull()) {
    return;
  }

  const QPointer<FeatureWrapper> target =
      this->featureContainer.getFeatureById(intent.featureId);
  if (target.isNull() || target->getFeature().isNull()) {
    return;
  }

  const QList<QPointer<Function> >& functions =
      target->getFeature()->getFunctions();
  for (int functionPosition = 0; functionPosition < functions.size();
       ++functionPosition) {
    const QPointer<Function>& function = functions.at(functionPosition);
    if (function.isNull()) {
      continue;
    }

    const QList<NeededElement>& needed = function->getNeededElements();
    for (int neededElementsIndex = 0; neededElementsIndex < needed.size();
         ++neededElementsIndex) {
      const NeededElement& ne = needed.at(neededElementsIndex);
      if (ne.typeOfElement == ElementTypes::ePointElement &&
          ne.roleName == intent.roleName) {
        this->addInputPoint(target, functionPosition, neededElementsIndex,
                            point);
      }
    }
  }
}

/*!
 * \brief OiJob::tagPoint
 * D12: stamps a MeasurementIntent on an existing point and feeds it into
 * any function that declares a matching role - the retroactive-tagging
 * path (dissecting a batch in the office). Shares resolvePointIntent with
 * addMeasurementResults so field acquisition and hand allocation are the
 * same operation (domain-model.md S3).
 * \param point
 * \param featureId
 * \param roleName
 */
void OiJob::tagPoint(const QPointer<Point>& point, int featureId,
                     const QString& roleName) {
  if (point.isNull()) {
    return;
  }

  const MeasurementIntent intent(featureId, roleName);
  point->addIntent(intent);
  this->resolvePointIntent(point, intent);
}

/*!
 * \brief OiJob::addInputReading
 * Adds an existing reading to the specified function of the target feature
 * \param target
 * \param functionPosition
 * \param neededElementsIndex
 * \param reading
 */
void OiJob::addInputReading(const QPointer<FeatureWrapper>& target,
                            const int& functionPosition,
                            const int& neededElementsIndex,
                            const QPointer<Reading>& reading) {
  // check target feature and reading
  if (target.isNull() || target->getFeature().isNull() || reading.isNull()) {
    return;
  }

  // check function position
  if (functionPosition < 0 ||
      functionPosition >= target->getFeature()->getFunctions().size() ||
      target->getFeature()->getFunctions().at(functionPosition).isNull()) {
    return;
  }

  // create and add input element
  InputElement element(reading->getId());
  element.typeOfElement = getElementTypeEnum(reading->getTypeOfReading());

  switch (reading->getTypeOfReading()) {
    case ReadingTypes::eDistanceReading:
      element.distanceReading = reading;
      break;
    case ReadingTypes::eCartesianReading:
      element.cartesianReading = reading;
      break;
    case ReadingTypes::ePolarReading:
      element.polarReading = reading;
      break;
    case ReadingTypes::eDirectionReading:
      element.directionReading = reading;
      break;
    case ReadingTypes::eTemperatureReading:
      element.temperatureReading = reading;
      break;
    case ReadingTypes::eLevelReading:
      element.levelReading = reading;
      break;
    default:
      break;
  }

  target->getFeature()
      ->getFunctions()
      .at(functionPosition)
      ->addInputElement(element, neededElementsIndex);
}

/*!
 * \brief OiJob::addInputFeature
 * Adds an existing feature to the specified function of the target feature
 * \param target
 * \param functionPosition
 * \param neededElementsIndex
 * \param feature
 */
void OiJob::addInputFeature(const QPointer<FeatureWrapper>& target,
                            const int& functionPosition,
                            const int& neededElementsIndex,
                            const QPointer<FeatureWrapper>& feature) {
  // check target and input features
  if (target.isNull() || target->getFeature().isNull() || feature.isNull() ||
      feature->getFeature().isNull()) {
    return;
  }

  // check function position
  if (functionPosition < 0 ||
      functionPosition >= target->getFeature()->getFunctions().size()) {
    return;
  }

  QPointer<Function> function =
      target->getFeature()->getFunctions().at(functionPosition);
  if (function.isNull()) {
    return;
  }

  // check circular reference
  if (this->checkCircleWarning(target->getFeature(), feature->getFeature())) {
    this->sendMessage("Cannot add input element: circular reference error",
                      MessageTypes::eErrorMessage,
                      MessageDestinations::eMessageBoxMessage);
    return;
  }

  // create and add input element
  // D12: position is always the NeededElement's real list index now (no
  // more InputElementKey::eDummyPoint sentinel doubling as a map key -
  // see function.h/fitfunction.h), so this is always a direct index
  // lookup.
  if (neededElementsIndex < 0 ||
      neededElementsIndex >= function->getNeededElements().size()) {
    return;
  }
  InputElement element(feature->getFeature()->getId());
  element.typeOfElement =
      function->getNeededElements().at(neededElementsIndex).typeOfElement;

  switch (element.typeOfElement) {
    case ElementTypes::eCircleElement: {
      // check circle
      if (feature->getGeometry().isNull() || feature->getCircle().isNull()) {
        return;
      }

      // set up input element
      element.circle = feature->getCircle();
      element.geometry = feature->getGeometry();

      break;
    }
    case ElementTypes::eConeElement: {
      // check cone
      if (feature->getGeometry().isNull() || feature->getCone().isNull()) {
        return;
      }

      // set up input element
      element.cone = feature->getCone();
      element.geometry = feature->getGeometry();

      break;
    }
    case ElementTypes::eCylinderElement: {
      // check cylinder
      if (feature->getGeometry().isNull() || feature->getCylinder().isNull()) {
        return;
      }

      // set up input element
      element.cylinder = feature->getCylinder();
      element.geometry = feature->getGeometry();

      break;
    }
    case ElementTypes::eEllipseElement: {
      // check ellipse
      if (feature->getGeometry().isNull() || feature->getEllipse().isNull()) {
        return;
      }

      // set up input element
      element.ellipse = feature->getEllipse();
      element.geometry = feature->getGeometry();

      break;
    }
    case ElementTypes::eEllipsoidElement: {
      // check ellipsoid
      if (feature->getGeometry().isNull() || feature->getEllipsoid().isNull()) {
        return;
      }

      // set up input element
      element.ellipsoid = feature->getEllipsoid();
      element.geometry = feature->getGeometry();

      break;
    }
    case ElementTypes::eHyperboloidElement: {
      // check hyperboloid
      if (feature->getGeometry().isNull() ||
          feature->getHyperboloid().isNull()) {
        return;
      }

      // set up input element
      element.hyperboloid = feature->getHyperboloid();
      element.geometry = feature->getGeometry();

      break;
    }
    case ElementTypes::eLineElement: {
      // check line
      if (feature->getGeometry().isNull() || feature->getLine().isNull()) {
        return;
      }

      // set up input element
      element.line = feature->getLine();
      element.geometry = feature->getGeometry();

      break;
    }
    case ElementTypes::eNurbsElement: {
      // check nurbs
      if (feature->getGeometry().isNull() || feature->getNurbs().isNull()) {
        return;
      }

      // set up input element
      element.nurbs = feature->getNurbs();
      element.geometry = feature->getGeometry();

      break;
    }
    case ElementTypes::eParaboloidElement: {
      // check paraboloid
      if (feature->getGeometry().isNull() ||
          feature->getParaboloid().isNull()) {
        return;
      }

      // set up input element
      element.paraboloid = feature->getParaboloid();
      element.geometry = feature->getGeometry();

      break;
    }
    case ElementTypes::ePlaneElement: {
      // check plane
      if (feature->getGeometry().isNull() || feature->getPlane().isNull()) {
        return;
      }

      // set up input element
      element.plane = feature->getPlane();
      element.geometry = feature->getGeometry();

      break;
    }
    case ElementTypes::ePointElement: {
      // check point
      if (feature->getGeometry().isNull() || feature->getPoint().isNull()) {
        return;
      }

      // set up input element
      element.point = feature->getPoint();
      element.geometry = feature->getGeometry();

      break;
    }
    case ElementTypes::ePointCloudElement: {
      // check point cloud
      if (feature->getGeometry().isNull() ||
          feature->getPointCloud().isNull()) {
        return;
      }

      // set up input element
      element.pointCloud = feature->getPointCloud();
      element.geometry = feature->getGeometry();

      break;
    }
    case ElementTypes::eScalarEntityAngleElement: {
      // check angle
      if (feature->getGeometry().isNull() ||
          feature->getScalarEntityAngle().isNull()) {
        return;
      }

      // set up input element
      element.scalarEntityAngle = feature->getScalarEntityAngle();
      element.geometry = feature->getGeometry();

      break;
    }
    case ElementTypes::eScalarEntityDistanceElement: {
      // check distance
      if (feature->getGeometry().isNull() ||
          feature->getScalarEntityDistance().isNull()) {
        return;
      }

      // set up input element
      element.scalarEntityDistance = feature->getScalarEntityDistance();
      element.geometry = feature->getGeometry();

      break;
    }
    case ElementTypes::eScalarEntityMeasurementSeriesElement: {
      // check series
      if (feature->getGeometry().isNull() ||
          feature->getScalarEntityMeasurementSeries().isNull()) {
        return;
      }

      // set up input element
      element.scalarEntityMeasurementSeries =
          feature->getScalarEntityMeasurementSeries();
      element.geometry = feature->getGeometry();

      break;
    }
    case ElementTypes::eScalarEntityTemperatureElement: {
      // check temperature
      if (feature->getGeometry().isNull() ||
          feature->getScalarEntityTemperature().isNull()) {
        return;
      }

      // set up input element
      element.scalarEntityTemperature = feature->getScalarEntityTemperature();
      element.geometry = feature->getGeometry();

      break;
    }
    case ElementTypes::eSlottedHoleElement: {
      // check slotted hole
      if (feature->getGeometry().isNull() ||
          feature->getSlottedHole().isNull()) {
        return;
      }

      // set up input element
      element.slottedHole = feature->getSlottedHole();
      element.geometry = feature->getGeometry();

      break;
    }
    case ElementTypes::eSphereElement: {
      // check sphere
      if (feature->getGeometry().isNull() || feature->getSphere().isNull()) {
        return;
      }

      // set up input element
      element.sphere = feature->getSphere();
      element.geometry = feature->getGeometry();

      break;
    }
    case ElementTypes::eTorusElement: {
      // check torus
      if (feature->getGeometry().isNull() || feature->getTorus().isNull()) {
        return;
      }

      // set up input element
      element.torus = feature->getTorus();
      element.geometry = feature->getGeometry();

      break;
    }
    case ElementTypes::eDirectionElement: {
      // check direction
      if (feature->getGeometry().isNull() ||
          !feature->getGeometry()->hasDirection()) {
        return;
      }

      // set up input element
      element.geometry = feature->getGeometry();

      break;
    }
    case ElementTypes::ePositionElement: {
      // check position
      if (feature->getGeometry().isNull() ||
          !feature->getGeometry()->hasPosition()) {
        return;
      }

      // set up input element
      element.geometry = feature->getGeometry();

      break;
    }
    case ElementTypes::eRadiusElement: {
      // check radius
      if (feature->getGeometry().isNull() ||
          !feature->getGeometry()->hasRadius()) {
        return;
      }

      // set up input element
      element.geometry = feature->getGeometry();

      break;
    }
    case ElementTypes::eCoordinateSystemElement: {
      // check coordinate system
      if (feature->getCoordinateSystem().isNull()) {
        return;
      }

      // set up input element
      element.coordSystem = feature->getCoordinateSystem();

      break;
    }
    case ElementTypes::eStationElement: {
      // check station
      if (feature->getStation().isNull()) {
        return;
      }

      // set up input element
      element.station = feature->getStation();

      break;
    }
    case ElementTypes::eTrafoParamElement: {
      // check trafo param
      if (feature->getTrafoParam().isNull()) {
        return;
      }

      // set up input element
      element.trafoParam = feature->getTrafoParam();

      break;
    }
    default: {
      return;
    }
  }

  // add the input element to the function
  function->addInputElement(element, neededElementsIndex);

  // create dependencies
  target->getFeature()->addPreviouslyNeeded(feature);
}

/*!
 * \brief OiJob::removeInputElement
 * Removes an input element from the specified function
 * \param target
 * \param functionPosition
 * \param neededElementsIndex
 * \param elementId
 */
void OiJob::removeInputElement(const QPointer<FeatureWrapper>& target,
                               const int& functionPosition,
                               const int& neededElementsIndex,
                               const int& elementId) {
  // check target
  if (target.isNull() || target->getFeature().isNull()) {
    return;
  }

  // check function position
  if (functionPosition < 0 ||
      functionPosition >= target->getFeature()->getFunctions().size() ||
      target->getFeature()->getFunctions().at(functionPosition).isNull()) {
    return;
  }
  Function* function =
      target->getFeature()->getFunctions().at(functionPosition);

  // check and get input elements
  if (!function->hasInputElement(elementId)) {
    return;
  }
  InputElement inputElement = function->getInputElement(elementId);

  // remove the input element with the specified id
  function->removeInputElement(elementId, neededElementsIndex);

  // check wether any function of the feature still contains the element
  bool isStillIncluded = false;
  for (const QPointer<Function>& function :
       oi::snapshot(target->getFeature()->getFunctions())) {
    if (function.isNull()) {
      continue;
    }
    if (function->hasInputElement(elementId)) {
      isStillIncluded = true;
      break;
    }
  }

  // reset dependencies if necessary
  if (!isStillIncluded) {
    this->resetDependencies(inputElement, target->getFeature());
  }
}

/*!
 * \brief OiJob::addMeasurementResults
 * D12/D15a/D20: always creates exactly one new, tagged Point from the
 * given readings - "a Point is one physical target as captured in one
 * acquisition, holding however many readings the instrument took"
 * (domain-model.md S2). This replaces the old eObservationElement special
 * case (binding a raw Observation straight into the target feature's
 * first function, only when that function happened to declare
 * eObservationElement) - deleted outright rather than reworked, per the
 * stage scope: core never invents a group or feeds a legacy
 * eObservationElement fit directly any more, it only ever produces a
 * tagged Point and lets tag resolution (resolvePointIntent) feed whatever
 * declares a matching ePointElement role. A fit still declaring
 * eObservationElement (BestFitPlane and siblings, pending Stage 7c-ii)
 * simply is not fed by this path any more - a known, sequenced gap, not an
 * oversight (plan-domain-model-rework.md, Stage 7f/7c-ii).
 * \param geomId the feature this acquisition was for
 * \param readings
 */
void OiJob::addMeasurementResults(const int& geomId,
                                  const QList<QPointer<Reading> >& readings) {
  // check active station
  QPointer<Station> activeStation = this->activeStation;
  if (activeStation.isNull() || activeStation->getCoordinateSystem().isNull()) {
    this->sendMessage(
        "Measurement rejected: no active station with a coordinate system",
        MessageTypes::eErrorMessage, MessageDestinations::eMessageBoxMessage);
    for (const QPointer<Reading>& reading : oi::snapshot(readings)) {
      if (!reading.isNull()) {
        delete reading.data();
      }
    }
    return;
  }

  // get and check feature with the id geomId - this is the acquisition's
  //*target*, not necessarily a fit function's input any more; it is
  // what the new Point gets tagged for
  QPointer<FeatureWrapper> targetFeature =
      this->featureContainer.getFeatureById(geomId);
  if (targetFeature.isNull() || targetFeature->getGeometry().isNull()) {
    this->sendMessage(
        QString("Measurement rejected: target feature %1 could not be resolved")
            .arg(geomId),
        MessageTypes::eErrorMessage, MessageDestinations::eMessageBoxMessage);
    for (const QPointer<Reading>& reading : oi::snapshot(readings)) {
      if (!reading.isNull()) {
        delete reading.data();
      }
    }
    return;
  }

  // create one station-owned Observation per reading, unchanged from
  // before - these hold the raw shots and are what makes the new Point
  //"a measurement" (Point::getOwnerDescription, point.cpp)
  QList<QPointer<Observation> > observations;
  bool isDummyPoint = false;
  for (const QPointer<Reading>& reading : oi::snapshot(readings)) {
    if (reading.isNull()) {
      continue;
    }

    QPointer<Observation> observation = new Observation();
    reading->setObservation(observation);
    activeStation->getCoordinateSystem()->addObservation(observation);
    observations.append(observation);
    isDummyPoint = isDummyPoint || observation->getIsDummyPoint();
  }

  if (observations.isEmpty()) {
    return;
  }

  // D15a: the aggregate position for this acquisition is computed by
  // Point::recalc() (point.cpp), NOT here. This is not a stylistic
  // choice - it is required for correctness: an Observation's xyz is not
  // yet meaningful at this point in the call chain (Reading::setObservation
  // only ever sets originalXyz; the transform into the job's active
  // coordinate system is app-level machinery - FeatureUpdater/
  // TrafoController - that core cannot and must not depend on). Computing
  // a centroid here would silently average zeros. Point::recalc() runs
  //*after* FeatureUpdater::recalcFeature has transformed this point's
  // own observations, via the ordinary dependency graph - which is
  // exactly why the point is registered as a dependency below before
  // anything is recalculated.
  QPointer<FeatureWrapper> pointFeature =
      this->createFeatureWrapper(FeatureTypes::ePointFeature);
  if (pointFeature.isNull() || pointFeature->getPoint().isNull()) {
    this->sendMessage(
        "Measurement rejected: could not create the resulting point",
        MessageTypes::eErrorMessage, MessageDestinations::eMessageBoxMessage);
    return;
  }

  pointFeature->getFeature()->setJob(this);
  pointFeature->getFeature()->name =
      QString("P%1").arg(pointFeature->getFeature()->getId());

  this->featureContainer.addFeature(pointFeature);
  this->attachFeature(pointFeature);

  QPointer<Point> point = pointFeature->getPoint();
  point->blockSignals(true);
  for (const QPointer<Observation>& observation : oi::snapshot(observations)) {
    point->addObservation(observation);
  }
  point->blockSignals(false);
  point->announceObservationsChanged();

  // D12: stamp the tag and feed any function that already declares a
  // matching role - "no binding step" (Stage 7f acceptance criteria).
  // eDummyPoint's old positional special-casing becomes the role name
  //"dummyPoint" (function.h); every other measurement is untagged-role
  //"default". resolvePointIntent also wires the dependency edge (via
  // addInputPoint -> setUpDependencies) that makes the recalc below
  // reach this point's own transform+average before the function that
  // consumes it runs.
  const QString roleName =
      isDummyPoint ? QStringLiteral("dummyPoint") : QStringLiteral("default");
  const MeasurementIntent intent(geomId, roleName);
  point->addIntent(intent);
  this->resolvePointIntent(point, intent);

  // recalculate the new point itself first - this is what actually
  // transforms its observations and computes its position/isSolved
  //(Point::recalc()); then recalculate the target feature, which by now
  // either already updated via the point's own usedFor cascade (if tag
  // resolution fed a function) or simply has nothing new to do (if
  // nothing consumes this role yet - a known, sequenced gap for
  // functions still declaring eObservationElement, see the class
  // comment above).
  this->notifyObservers(&JobObserver::onRecalcFeatureRequested,
                        pointFeature->getFeature());
  this->notifyObservers(&JobObserver::onRecalcFeatureRequested,
                        targetFeature->getFeature());
}

/*!
 * \brief OiJob::removeObservations
 * \param featureId
 */
void OiJob::removeObservations(const int& featureId) {
  // get and check feature with the id featureId
  QPointer<FeatureWrapper> feature =
      this->featureContainer.getFeatureById(featureId);
  if (feature.isNull() || feature->getGeometry().isNull()) {
    return;
  }

  // run through all observations of the feature
  QList<QPointer<Observation> > observations =
      feature->getGeometry()->getObservations();
  for (const QPointer<Observation>& obs : oi::snapshot(observations)) {
    delete obs.data();
  }

  // recalculate the feature
  this->notifyObservers(&JobObserver::onRecalcFeatureRequested,
                        feature->getFeature());

  this->notifyObservers(&JobObserver::onGeometryObservationsChanged,
                        feature->getFeature()->getId());
}

void OiJob::enableOrDisableObservations(const int& featureId, bool enable) {
  QPointer<FeatureWrapper> feature =
      this->featureContainer.getFeatureById(featureId);
  if (feature.isNull()) {
    this->sendMessage(
        QString("Cannot enable or disable observations from %1")
            .arg(feature.isNull() ? QString("feature id: %1").arg(featureId)
                                  : feature->getFeature()->getFeatureName()),
        MessageTypes::eWarningMessage);
    return;
  }

  QPointer<Station> station = feature->getStation();
  if (!station.isNull()) {  // is station
    enableOrDisableStationObservations(station, enable);
  } else {
    enableOrDisableGeometryObservations(featureId, enable, QPointer<Station>());
  }
}

void OiJob::enableOrDisableStationObservations(QPointer<Station> station,
                                               bool enable) {
  for (int featureId :
       oi::snapshot(this->featureContainer.getFeatureIdList())) {
    enableOrDisableGeometryObservations(featureId, enable, station);
  }
}

void OiJob::enableOrDisableGeometryObservations(const int& featureId,
                                                bool enable,
                                                QPointer<Station> station) {
  int functionIndex = 0;
  int neededElementIndex = 0;
  // get and check feature with the id featureId
  QPointer<FeatureWrapper> feature =
      this->featureContainer.getFeatureById(featureId);
  if (feature.isNull() || feature->getGeometry().isNull()) {
    this->sendMessage(
        QString("Cannot enable or disable observations from %1")
            .arg(feature.isNull() ? QString("feature id: %1").arg(featureId)
                                  : feature->getFeature()->getFeatureName()),
        MessageTypes::eWarningMessage);
    return;
  }

  // run through all observations of the feature
  QList<QPointer<Observation> > observations =
      feature->getGeometry()->getObservations();
  for (const QPointer<Observation>& obs : oi::snapshot(observations)) {
    if ((!station.isNull() && station->getId() != obs->getStation()->getId()) ||
        functionIndex >= feature->getFeature()->getFunctions().size()) {
      continue;  // skip
    }
    int elementId = obs->getId();

    // check if the current use state already equals the new use state
    if (feature->getFeature()->getFunctions()[functionIndex]->getShouldBeUsed(
            neededElementIndex, elementId) == enable) {
      continue;
    }

    // set should use
    feature->getFeature()->getFunctions()[functionIndex]->setShouldBeUsed(
        neededElementIndex, elementId, enable);
  }

  // recalculate the feature
  this->notifyObservers(&JobObserver::onRecalcFeatureRequested,
                        feature->getFeature());
}

void OiJob::enableObservations(const int& featureId) {
  enableOrDisableObservations(featureId, true);
}
void OiJob::disableObservations(const int& featureId) {
  enableOrDisableObservations(featureId, false);
}
/*!
 * \brief OiJob::removeObservations
 * \param featureId
 * \param selectedIds
 */
void OiJob::removeObservations(const int& featureId,
                               const QList<int> selectedIds) {
  // get and check feature with the id featureId
  QPointer<FeatureWrapper> feature =
      this->featureContainer.getFeatureById(featureId);
  if (feature.isNull() || feature->getGeometry().isNull()) {
    return;
  }

  // run through all observations of the feature
  QList<QPointer<Observation> > observations =
      feature->getGeometry()->getObservations();
  for (const QPointer<Observation>& obs : oi::snapshot(observations)) {
    if (selectedIds.contains(obs->getId())) {
      delete obs.data();
    }
  }

  // recalculate the feature
  this->notifyObservers(&JobObserver::onRecalcFeatureRequested,
                        feature->getFeature());

  this->notifyObservers(&JobObserver::onGeometryObservationsChanged,
                        feature->getFeature()->getId());
}

/*!
 * \brief OiJob::removeAllObservations
 */
void OiJob::removeAllObservations() {
  // get a list of all geometries
  QList<QPointer<FeatureWrapper> > geometries =
      this->featureContainer.getGeometriesList();

  // remove all observations
  for (const QPointer<FeatureWrapper>& geom : oi::snapshot(geometries)) {
    // check geometry
    if (geom.isNull() || geom->getGeometry().isNull()) {
      continue;
    }

    // remove all observations of the current geometry
    this->removeObservations(geom->getGeometry()->getId());
  }
}

/*!
 * \brief OiJob::setShouldBeUsed
 * \param target
 * \param functionIndex
 * \param neededElementIndex
 * \param elementId
 * \param use
 * \param recalc
 */
void OiJob::setShouldBeUsed(const QPointer<FeatureWrapper>& target,
                            const int& functionIndex,
                            const int& neededElementIndex, const int& elementId,
                            const bool& use, const bool& recalc) {
  // check target feature
  if (target.isNull() || target->getFeature().isNull()) {
    return;
  }

  // check function position
  if (functionIndex < 0 ||
      functionIndex >= target->getFeature()->getFunctions().size() ||
      target->getFeature()->getFunctions().at(functionIndex).isNull()) {
    return;
  }

  // check if the current use state already equals the new use state
  if (target->getFeature()->getFunctions()[functionIndex]->getShouldBeUsed(
          neededElementIndex, elementId) == use) {
    return;
  }

  // set should use
  target->getFeature()->getFunctions()[functionIndex]->setShouldBeUsed(
      neededElementIndex, elementId, use);

  // force recalculation if requested
  if (recalc) {
    this->notifyObservers(&JobObserver::onRecalcFeatureRequested,
                          target->getFeature());
  }
}

/*!
 * \brief OiJob::setPipeline
 * D16/domain-model.md S5: replace the declared pipeline for one
 * NeededElement position. Mirrors setShouldBeUsed's shape - mutate the
 * declaration, then optionally trigger the ordinary recalc that
 * evaluates it (Function::exec(), not a second recalculation path).
 * \param target
 * \param functionIndex
 * \param neededElementIndex
 * \param stages
 * \param recalc
 */
void OiJob::setPipeline(const QPointer<FeatureWrapper>& target,
                        const int& functionIndex, const int& neededElementIndex,
                        const QList<PipelineStage>& stages,
                        const bool& recalc) {
  if (target.isNull() || target->getFeature().isNull()) {
    return;
  }

  if (functionIndex < 0 ||
      functionIndex >= target->getFeature()->getFunctions().size() ||
      target->getFeature()->getFunctions().at(functionIndex).isNull()) {
    return;
  }

  target->getFeature()->getFunctions()[functionIndex]->setPipeline(
      neededElementIndex, stages);

  if (recalc) {
    this->notifyObservers(&JobObserver::onRecalcFeatureRequested,
                          target->getFeature());
  }
}

/*!
 * \brief OiJob::excludeElement
 * D16: the hand-picking entry point - reject one element with a reason
 * without replacing whatever pipeline already exists at this position.
 * \param target
 * \param functionIndex
 * \param neededElementIndex
 * \param elementId
 * \param reason
 * \param recalc
 */
void OiJob::excludeElement(const QPointer<FeatureWrapper>& target,
                           const int& functionIndex,
                           const int& neededElementIndex, const int& elementId,
                           const QString& reason, const bool& recalc) {
  if (target.isNull() || target->getFeature().isNull()) {
    return;
  }

  if (functionIndex < 0 ||
      functionIndex >= target->getFeature()->getFunctions().size() ||
      target->getFeature()->getFunctions().at(functionIndex).isNull()) {
    return;
  }

  target->getFeature()->getFunctions()[functionIndex]->addPipelineStage(
      neededElementIndex, PipelineStage::exclude(elementId, reason));

  if (recalc) {
    this->notifyObservers(&JobObserver::onRecalcFeatureRequested,
                          target->getFeature());
  }
}

/*!
 * \brief OiJob::setActiveFeature
 * \param featureId
 */
void OiJob::setActiveFeature(const int& featureId) {
  // get and check the feature
  QPointer<FeatureWrapper> feature =
      this->featureContainer.getFeatureById(featureId);
  if (feature.isNull() || feature->getFeature().isNull()) {
    return;
  }

  if (feature->getFeature()
          ->getIsActiveFeature()) {  // if the feature was activated

    // check if feature already is the active feature
    if (!this->activeFeature.isNull() && this->activeFeature == feature) {
      return;
    }

    // get pointer to the old active feature
    QPointer<FeatureWrapper> oldActiveFeature = this->activeFeature;

    // save feature as active feature
    this->activeFeature = feature;

    // set currently active feature to deactive
    if (!oldActiveFeature.isNull() &&
        !oldActiveFeature->getFeature().isNull()) {
      oldActiveFeature->getFeature()->setActiveFeatureState(false);
    }

    this->notifyObservers(&JobObserver::onActiveFeatureChanged);

  } else {  // if the feature was deactivated

    // if the feature was deactivated without specifying a new active feature
    if (!this->activeFeature.isNull() && this->activeFeature == feature) {
      this->activeFeature = QPointer<FeatureWrapper>(nullptr);
      this->notifyObservers(&JobObserver::onActiveFeatureChanged);
      return;
    }

    // if there is a new active feature
    if (!this->activeFeature.isNull() &&
        !this->activeFeature->getFeature().isNull()) {
      return;
    }
  }
}

/*!
 * \brief OiJob::setActiveStation
 * \param featureId
 */
void OiJob::setActiveStation(const int& featureId) {
  // get and check the feature
  QPointer<FeatureWrapper> feature =
      this->featureContainer.getFeatureById(featureId);
  if (feature.isNull() || feature->getStation().isNull()) {
    return;
  }

  if (feature->getStation()
          ->getIsActiveStation()) {  // if the station was activated

    // check if station already is the active station
    if (!this->activeStation.isNull() &&
        this->activeStation == feature->getStation()) {
      return;
    }

    // set currently active station to deactive
    if (!this->activeStation.isNull()) {
      this->activeStation->blockSignals(true);
      this->activeStation->setActiveStationState(false);
      this->activeStation->blockSignals(false);
    }

    // set sensor of the new station and reset sensor of old one
    if (!this->activeStation.isNull()) {
      // take sensor from the active station
      QPointer<Sensor> sensor(nullptr);
      if (this->activeStation->getIsSensorSet()) {
        sensor = this->activeStation->takeSensor();
      }

      // save station as active station
      this->activeStation = feature->getStation();

      // set sensor of activated station
      if (!sensor.isNull()) {
        this->activeStation->setSensor(sensor);
      }

    } else {
      // save station as active station
      this->activeStation = feature->getStation();
    }

    this->notifyObservers(&JobObserver::onActiveStationChanged);

  } else {  // if the station was deactivated

    // if the station was deactivated without specifying a new active station
    if (!this->activeStation.isNull() &&
        this->activeStation == feature->getStation()) {
      // reset sensor
      this->activeStation->resetSensor();

      // reset active station
      this->activeStation = QPointer<Station>(nullptr);
      this->notifyObservers(&JobObserver::onActiveStationChanged);
      return;
    }
  }
}

/*!
 * \brief OiJob::setActiveCoordinateSystem
 * \param featureId
 */
void OiJob::setActiveCoordinateSystem(const int& featureId) {
  // get and check the feature
  QPointer<FeatureWrapper> feature =
      this->featureContainer.getFeatureById(featureId);
  if (feature.isNull() || feature->getCoordinateSystem().isNull()) {
    return;
  }

  if (feature->getCoordinateSystem()
          ->getIsActiveCoordinateSystem()) {  // if the system was activated

    // check if system already is the active system
    if (!this->activeCoordinateSystem.isNull() &&
        this->activeCoordinateSystem == feature->getCoordinateSystem()) {
      return;
    }

    QPointer<CoordinateSystem> oldSystem = this->activeCoordinateSystem;

    // save system as active system
    this->activeCoordinateSystem = feature->getCoordinateSystem();

    // set currently active system to deactive
    if (!oldSystem.isNull()) {
      oldSystem->setActiveCoordinateSystemState(false);
    }

    this->notifyObservers(&JobObserver::onActiveCoordinateSystemChanged);

  } else {  // if the system was deactivated

    // if the system was deactivated without specifying a new active system
    if (!this->activeCoordinateSystem.isNull() &&
        this->activeCoordinateSystem == feature->getCoordinateSystem()) {
      this->activeCoordinateSystem = QPointer<CoordinateSystem>(nullptr);
      this->notifyObservers(&JobObserver::onActiveCoordinateSystemChanged);
      return;
    }
  }
}

/*!
 * \brief OiJob::setFeatureName
 * \param featureId
 * \param oldName
 */
void OiJob::setFeatureName(const int& featureId, const QString& oldName) {
  // get and check the feature
  QPointer<FeatureWrapper> feature =
      this->featureContainer.getFeatureById(featureId);
  if (feature.isNull() || feature->getFeature().isNull()) {
    return;
  }

  // D17: nominal and actual are no longer linked by matching names (that
  // was the mechanism this method used to maintain on rename), so there
  // are no cross-feature relations left to fix up here - a rename only
  // ever affects this one feature.

  // update feature container
  this->featureContainer.featureNameChanged(featureId, oldName);

  this->notifyObservers(&JobObserver::onFeatureAttributesChanged);
  this->notifyObservers(&JobObserver::onFeatureNameChanged, featureId, oldName);
}

/*!
 * \brief OiJob::setFeatureGroup
 * \param featureId
 * \param oldGroup
 */
void OiJob::setFeatureGroup(const int& featureId, const QString& oldGroup) {
  // get and check the feature
  QPointer<FeatureWrapper> feature =
      this->featureContainer.getFeatureById(featureId);
  if (feature.isNull() || feature->getFeature().isNull()) {
    return;
  }

  // check if the group is a new group
  bool isNewGroup = false;
  if (!this->featureContainer.getFeatureGroupList().contains(
          feature->getFeature()->getGroupName())) {
    isNewGroup = true;
  }

  // update feature container
  this->featureContainer.featureGroupChanged(featureId, oldGroup);

  // check if there is another feature with the group oldGroup
  bool wasLastGroup = false;
  if (!this->featureContainer.getFeatureGroupList().contains(oldGroup)) {
    wasLastGroup = true;
  }

  this->notifyObservers(&JobObserver::onFeatureAttributesChanged);
  this->notifyObservers(&JobObserver::onFeatureGroupChanged, featureId,
                        oldGroup);
  if (isNewGroup || wasLastGroup) {
    this->notifyObservers(&JobObserver::onAvailableGroupsChanged);
  }
}

/*!
 * \brief OiJob::setFeatureComment
 * \param featureId
 */
void OiJob::setFeatureComment(const int& featureId) {
  this->notifyObservers(&JobObserver::onFeatureAttributesChanged);
  this->notifyObservers(&JobObserver::onFeatureCommentChanged, featureId);
}

/*!
 * \brief OiJob::setFeatureIsUpdated
 * \param featureId
 */
void OiJob::setFeatureIsUpdated(const int& featureId) {
  this->notifyObservers(&JobObserver::onFeatureAttributesChanged);
  this->notifyObservers(&JobObserver::onFeatureIsUpdatedChanged, featureId);
}

/*!
 * \brief OiJob::setFeatureIsSolved
 * \param featureId
 */
void OiJob::setFeatureIsSolved(const int& featureId) {
  this->notifyObservers(&JobObserver::onFeatureAttributesChanged);
  this->notifyObservers(&JobObserver::onFeatureIsSolvedChanged, featureId);
}

/*!
 * \brief OiJob::setFeatureFunctions
 * \param featureId
 */
void OiJob::setFeatureFunctions(const int& featureId) {
  this->notifyObservers(&JobObserver::onFeatureAttributesChanged);
  this->notifyObservers(&JobObserver::onFeatureFunctionsChanged, featureId);
}

/*!
 * \brief OiJob::setFeatureUsedFor
 * \param featureId
 */
void OiJob::setFeatureUsedFor(const int& featureId) {
  this->notifyObservers(&JobObserver::onFeatureAttributesChanged);
  this->notifyObservers(&JobObserver::onFeatureUsedForChanged, featureId);
}

/*!
 * \brief OiJob::setFeaturePreviouslyNeeded
 * \param featureId
 */
void OiJob::setFeaturePreviouslyNeeded(const int& featureId) {
  this->notifyObservers(&JobObserver::onFeatureAttributesChanged);
  this->notifyObservers(&JobObserver::onFeaturePreviouslyNeededChanged,
                        featureId);
}

/*!
 * \brief OiJob::setGeometryIsCommon
 * \param featureId
 */
void OiJob::setGeometryIsCommon(const int& featureId) {
  this->notifyObservers(&JobObserver::onGeometryIsCommonChanged, featureId);
}

/*!
 * \brief OiJob::setGeometryRealises
 * \param featureId
 */
void OiJob::setGeometryRealises(const int& featureId) {
  this->notifyObservers(&JobObserver::onGeometryRealisesChanged, featureId);
}

/*!
 * \brief OiJob::setGeometryObservations
 * \param featureId
 */
void OiJob::setGeometryObservations(const int& featureId) {
  this->notifyObservers(&JobObserver::onGeometryObservationsChanged, featureId);
}

/*!
 * \brief OiJob::setGroupMembers
 * A group's membership changed, so anything computed from its contents is
 * stale. This is what makes bulk acquisition work: a driver adds a
 * thousand points to a group and every fit bound to that group re-solves,
 * without the fit's own inputs ever being touched.
 * \param featureId
 */
void OiJob::setGroupMembers(const int& featureId) {
  this->notifyObservers(&JobObserver::onGroupMembersChanged, featureId);

  // recalculate only what actually consumes this group. The dependency
  // edges already exist - OiJob::setUpDependencies records them when a
  // function's input element references another feature.
  QPointer<FeatureWrapper> group = this->getFeatureById(featureId);
  if (group.isNull() || group->getFeature().isNull()) {
    return;
  }

  for (const QPointer<FeatureWrapper>& dependent :
       oi::snapshot(group->getFeature()->getUsedFor())) {
    if (dependent.isNull() || dependent->getFeature().isNull()) {
      continue;
    }
    this->notifyObservers(&JobObserver::onRecalcFeatureRequested,
                          dependent->getFeature());
  }
}

/*!
 * \brief OiJob::setGeometryNominalSystem
 * \param featureId
 */
void OiJob::setGeometryNominalSystem(const int& featureId) {
  this->notifyObservers(&JobObserver::onGeometryNominalSystemChanged,
                        featureId);
}

/*!
 * \brief OiJob::setGeometryStatistic
 * \param featureId
 */
void OiJob::setGeometryStatistic(const int& featureId) {
  this->notifyObservers(&JobObserver::onGeometryStatisticChanged, featureId);
}

/*!
 * \brief OiJob::setGeometrySimulationData
 * \param featureId
 */
void OiJob::setGeometrySimulationData(const int& featureId) {
  this->notifyObservers(&JobObserver::onGeometrySimulationDataChanged,
                        featureId);
}

/*!
 * \brief OiJob::setGeometryMeasurementConfig
 * \param featureId
 * \param oldMConfig
 * \param oldIsSaved
 */
void OiJob::setGeometryMeasurementConfig(const int& featureId,
                                         const QString& oldMConfig,
                                         const MeasurementConfigKey oldKey) {
  // update feature container
  this->featureContainer.geometryMeasurementConfigChanged(featureId, oldMConfig,
                                                          oldKey);

  this->notifyObservers(&JobObserver::onGeometryMeasurementConfigChanged,
                        featureId, oldMConfig, oldKey);
}

/*!
 * \brief OiJob::setSystemObservations
 * \param featureId
 * \param obsId
 */
void OiJob::setSystemObservations(const int& featureId, const int& obsId) {
  this->notifyObservers(&JobObserver::onSystemObservationsChanged, featureId,
                        obsId);
}

/*!
 * \brief OiJob::setSystemTrafoParams
 * \param featureId
 */
void OiJob::setSystemTrafoParams(const int& featureId) {
  this->notifyObservers(&JobObserver::onSystemTrafoParamsChanged, featureId);
}

/*!
 * \brief OiJob::setSystemsNominals
 * \param featureId
 */
void OiJob::setSystemsNominals(const int& featureId) {
  this->notifyObservers(&JobObserver::onSystemsNominalsChanged, featureId);
}

/*!
 * \brief OiJob::setStationSensor
 * \param featureId
 */
void OiJob::setStationSensor(const int& featureId) {
  this->notifyObservers(&JobObserver::onStationSensorChanged, featureId);
}

/*!
 * \brief OiJob::setTrafoParamParameters
 * \param featureId
 */
void OiJob::setTrafoParamParameters(const int& featureId) {
  this->notifyObservers(&JobObserver::onTrafoParamParametersChanged, featureId);
}

/*!
 * \brief OiJob::setTrafoParamSystems
 * \param featureId
 */
void OiJob::setTrafoParamSystems(const int& featureId) {
  this->notifyObservers(&JobObserver::onTrafoParamSystemsChanged, featureId);
}

/*!
 * \brief OiJob::setTrafoParamIsUsed
 * \param featureId
 */
void OiJob::setTrafoParamIsUsed(const int& featureId) {
  this->notifyObservers(&JobObserver::onTrafoParamIsUsedChanged, featureId);
}

/*!
 * \brief OiJob::setTrafoParamIsDatum
 * \param featureId
 */
void OiJob::setTrafoParamIsDatum(const int& featureId) {
  this->notifyObservers(&JobObserver::onTrafoParamIsDatumChanged, featureId);
}

/*!
 * \brief OiJob::setTrafoParamValidTime
 * \param featureId
 */
/*void OiJob::setTrafoParamValidTime(const int &featureId){
    emit this->trafoParamValidTimeChanged(featureId);
}*/

/*!
 * \brief OiJob::setTrafoParamIsMovement
 * \param featureId
 */
/*void OiJob::setTrafoParamIsMovement(const int &featureId){
    emit this->trafoParamIsMovementChanged(featureId);
}*/

/*!
 * \brief OiJob::elementAboutToBeDeleted
 * \param elementId
 * \param name
 * \param group
 * \param type
 */
void OiJob::elementAboutToBeDeleted(const int& elementId, const QString& name,
                                    const QString& group,
                                    const FeatureTypes& type) {
  QPointer<FeatureWrapper> feature =
      this->featureContainer.getFeatureById(elementId);
  this->detachFeature(feature);
  this->featureContainer.checkAndClean(elementId, name, group, type);
}

/*!
 * \brief OiJob::attachFeature
 * Make the job the observer of the feature (and of the station system a
 * station owns), and connect the station's own signals.
 * \param feature
 */
void OiJob::attachFeature(const QPointer<FeatureWrapper>& feature) {
  // check feature
  if (feature.isNull() || feature->getFeature().isNull()) {
    return;
  }

  feature->getFeature()->setObserver(this);

  // station connects (sensor/station signals are not feature-model signals)
  if (feature->getFeatureTypeEnum() == FeatureTypes::eStationFeature) {
    QObject::connect(feature->getStation().data(),
                     &Station::activeStationChanged, this,
                     &OiJob::setActiveStation, Qt::AutoConnection);
    QObject::connect(feature->getStation().data(), &Station::sensorChanged,
                     this, &OiJob::setStationSensor, Qt::AutoConnection);
    if (!feature->getStation()->getCoordinateSystem().isNull()) {
      feature->getStation()->getCoordinateSystem()->setObserver(
          this->stationSystemObserver.get());
    }
  }
}

/*!
 * \brief OiJob::detachFeature
 * Undo attachFeature: the feature (and station system) forgets the job.
 * \param feature
 */
void OiJob::detachFeature(const QPointer<FeatureWrapper>& feature) {
  // check feature
  if (feature.isNull() || feature->getFeature().isNull()) {
    return;
  }

  if (feature->getFeature()->getObserver() == this) {
    feature->getFeature()->setObserver(nullptr);
  }

  // station connects
  if (feature->getFeatureTypeEnum() == FeatureTypes::eStationFeature) {
    QObject::disconnect(feature->getStation().data(),
                        &Station::activeStationChanged, this,
                        &OiJob::setActiveStation);
    QObject::disconnect(feature->getStation().data(), &Station::sensorChanged,
                        this, &OiJob::setStationSensor);
    if (!feature->getStation()->getCoordinateSystem().isNull() &&
        feature->getStation()->getCoordinateSystem()->getObserver() ==
            this->stationSystemObserver.get()) {
      feature->getStation()->getCoordinateSystem()->setObserver(nullptr);
    }
  }
}

// ###########################################################################
// FeatureObserver: the feature model reports to the job, which relays it into
// its own signals exactly as the former signal/slot connections did
// ###########################################################################

void OiJob::onElementAboutToBeDeleted(const int& elementId, const QString& name,
                                      const QString& group,
                                      const FeatureTypes& type) {
  this->elementAboutToBeDeleted(elementId, name, group, type);
}

void OiJob::onFeatureNameChanged(const int& featureId, const QString& oldName) {
  this->setFeatureName(featureId, oldName);
}

void OiJob::onFeatureGroupChanged(const int& featureId,
                                  const QString& oldGroup) {
  this->setFeatureGroup(featureId, oldGroup);
}

void OiJob::onFeatureCommentChanged(const int& featureId) {
  this->setFeatureComment(featureId);
}

// the one notification that has always been delivered later, through the
// event loop (temporary: goes away with the signals themselves)
void OiJob::onFeatureIsSolvedChanged(const int& featureId) {
  QMetaObject::invokeMethod(
      this, [this, featureId]() { this->setFeatureIsSolved(featureId); },
      Qt::QueuedConnection);
}

void OiJob::onFeatureFunctionListChanged(const int& featureId) {
  this->setFeatureFunctions(featureId);
}

void OiJob::onFeatureUsedForChanged(const int& featureId) {
  this->setFeatureUsedFor(featureId);
}

void OiJob::onFeaturePreviouslyNeededChanged(const int& featureId) {
  this->setFeaturePreviouslyNeeded(featureId);
}

void OiJob::onFeatureIsActiveChanged(const int& featureId) {
  this->setActiveFeature(featureId);
}

void OiJob::onGeomIsCommonChanged(const int& featureId) {
  this->setGeometryIsCommon(featureId);
}

void OiJob::onGeomRealisesChanged(const int& featureId) {
  this->setGeometryRealises(featureId);
}

void OiJob::onGeomObservationsChanged(const int& featureId) {
  this->setGeometryObservations(featureId);
}

void OiJob::onGeomNominalSystemChanged(const int& featureId) {
  this->setGeometryNominalSystem(featureId);
}

void OiJob::onGeomStatisticChanged(const int& featureId) {
  this->setGeometryStatistic(featureId);
}

void OiJob::onGeomSimulationDataChanged(const int& featureId) {
  this->setGeometrySimulationData(featureId);
}

void OiJob::onGeomMeasurementConfigChanged(const int& featureId,
                                           const QString& oldMConfig,
                                           const MeasurementConfigKey oldKey) {
  this->setGeometryMeasurementConfig(featureId, oldMConfig, oldKey);
}

void OiJob::onGroupMembersChanged(const int& featureId) {
  this->setGroupMembers(featureId);
}

void OiJob::onTransformationParameterChanged(const int& tpId) {
  this->setTrafoParamParameters(tpId);
}

void OiJob::onCoordinateSystemsChanged(const int& tpId) {
  this->setTrafoParamSystems(tpId);
}

void OiJob::onIsUsedChanged(const int& tpId) {
  this->setTrafoParamIsUsed(tpId);
}

void OiJob::onIsDatumTrafoChanged(const int& tpId) {
  this->setTrafoParamIsDatum(tpId);
}

void OiJob::onTransformationParametersChanged(const int& coordId) {
  this->setSystemTrafoParams(coordId);
}

void OiJob::onNominalsChanged(const int& coordId) {
  this->setSystemsNominals(coordId);
}

void OiJob::onActiveCoordinateSystemChanged(const int& coordId) {
  this->setActiveCoordinateSystem(coordId);
}

/*!
 * \brief OiJob::createFeatureNames
 * \param name
 * \return
 */
QStringList OiJob::createFeatureNames(const QString& name,
                                      const int& count) const {
  QStringList result;

  // if only one feature shall be created take the hole name
  if (count == 1) {
    result.append(name);
    return result;
  }

  // split base name from postfix
  QString baseName = name;
  QString postFix;

  // the postfix is the run of ASCII digits at the end of the name
  qsizetype digitsStart = baseName.size();
  while (digitsStart > 0 && baseName.at(digitsStart - 1) >= QChar('0') &&
         baseName.at(digitsStart - 1) <= QChar('9')) {
    --digitsStart;
  }
  postFix = baseName.mid(digitsStart);
  baseName.truncate(digitsStart);

  // add leading zero
  int numLeadingZeros = 0;
  while (postFix.size() > 1 && postFix.at(0) == '0') {
    numLeadingZeros++;
    postFix.remove(0, 1);
  }

  numLeadingZeros = numLeadingZeros + postFix.length();

  int postFixInt = postFix.toInt();
  if (postFixInt == 0) {
    postFixInt++;
  }
  QString leadZeros = "";

  for (int i = 0; i < count; i++) {
    leadZeros.clear();

    postFix = QString::number(postFixInt + i);

    int diff = numLeadingZeros - postFix.length();

    if (diff > 0) {
      for (int j = 0; j < diff; j++) {
        leadZeros.append("0");
      }
    }

    if (leadZeros.compare("") == 0 && postFix.toInt() < 10) {
      leadZeros.append("0");
    }

    result.append(QString("%1%2%3").arg(baseName).arg(leadZeros).arg(postFix));
  }

  return result;
}

/*!
 * \brief OiJob::createFeatureWrapper
 * Create a feature wrapper containing a feature of the given type
 * \param type
 * \return
 */
QPointer<FeatureWrapper> OiJob::createFeatureWrapper(
    const FeatureTypes& type) const {
  QPointer<FeatureWrapper> feature = new FeatureWrapper();

  switch (type) {
    case FeatureTypes::eStationFeature: {
      Station* station = new Station();
      feature->setStation(station);
      break;
    }
    case FeatureTypes::eTrafoParamFeature: {
      TrafoParam* trafoParam = new TrafoParam();
      feature->setTrafoParam(trafoParam);
      break;
    }
    case FeatureTypes::eCoordinateSystemFeature: {
      CoordinateSystem* coordSystem = new CoordinateSystem();
      feature->setCoordinateSystem(coordSystem);
      break;
    }
    case FeatureTypes::eCircleFeature: {
      Circle* circle = new Circle();
      feature->setCircle(circle);
      break;
    }
    case FeatureTypes::eConeFeature: {
      Cone* cone = new Cone();
      feature->setCone(cone);
      break;
    }
    case FeatureTypes::eCylinderFeature: {
      Cylinder* cylinder = new Cylinder();
      feature->setCylinder(cylinder);
      break;
    }
    case FeatureTypes::eEllipseFeature: {
      Ellipse* ellipse = new Ellipse();
      feature->setEllipse(ellipse);
      break;
    }
    case FeatureTypes::eEllipsoidFeature: {
      Ellipsoid* ellipsoid = new Ellipsoid();
      feature->setEllipsoid(ellipsoid);
      break;
    }
    case FeatureTypes::eHyperboloidFeature: {
      Hyperboloid* hyperboloid = new Hyperboloid();
      feature->setHyperboloid(hyperboloid);
      break;
    }
    case FeatureTypes::eLineFeature: {
      Line* line = new Line();
      feature->setLine(line);
      break;
    }
    case FeatureTypes::eNurbsFeature: {
      Nurbs* nurbs = new Nurbs();
      feature->setNurbs(nurbs);
      break;
    }
    case FeatureTypes::eParaboloidFeature: {
      Paraboloid* paraboloid = new Paraboloid();
      feature->setParaboloid(paraboloid);
      break;
    }
    case FeatureTypes::ePlaneFeature: {
      Plane* plane = new Plane();
      feature->setPlane(plane);
      break;
    }
    case FeatureTypes::ePointFeature: {
      Point* point = new Point();
      feature->setPoint(point);
      break;
    }
    case FeatureTypes::ePointCloudFeature: {
      PointCloud* pointCloud = new PointCloud();
      feature->setPointCloud(pointCloud);
      break;
    }
    case FeatureTypes::eGroupFeature: {
      // organisational, never nominal: a group holds references and is
      // neither measured nor solved, so isNominal has no meaning for it
      Group* group = new Group();
      feature->setGroup(group);
      break;
    }
    case FeatureTypes::ePointGroupFeature: {
      PointGroup* pointGroup = new PointGroup();
      feature->setPointGroup(pointGroup);
      break;
    }
    case FeatureTypes::eScalarEntityAngleFeature: {
      ScalarEntityAngle* angle = new ScalarEntityAngle();
      feature->setScalarEntityAngle(angle);
      break;
    }
    case FeatureTypes::eScalarEntityDistanceFeature: {
      ScalarEntityDistance* distance = new ScalarEntityDistance();
      feature->setScalarEntityDistance(distance);
      break;
    }
    case FeatureTypes::eScalarEntityMeasurementSeriesFeature: {
      ScalarEntityMeasurementSeries* measurementSeries =
          new ScalarEntityMeasurementSeries();
      feature->setScalarEntityMeasurementSeries(measurementSeries);
      break;
    }
    case FeatureTypes::eScalarEntityTemperatureFeature: {
      ScalarEntityTemperature* temperature = new ScalarEntityTemperature();
      feature->setScalarEntityTemperature(temperature);
      break;
    }
    case FeatureTypes::eSlottedHoleFeature: {
      SlottedHole* slottedHole = new SlottedHole();
      feature->setSlottedHole(slottedHole);
      break;
    }
    case FeatureTypes::eSphereFeature: {
      Sphere* sphere = new Sphere();
      feature->setSphere(sphere);
      break;
    }
    case FeatureTypes::eTorusFeature: {
      Torus* torus = new Torus();
      feature->setTorus(torus);
      break;
    }
    default: {
      delete feature.data();
    }
  }

  return feature;
}

/*!
 * \brief OiJob::checkAndSetUpNewFeature
 * Check the attributes of the given feature and set up relations
 * \param feature
 * \return
 */
bool OiJob::checkAndSetUpNewFeature(const QPointer<FeatureWrapper>& feature,
                                    bool overwrite) {
  // pass the job to the feature
  feature->getFeature()->setJob(this);

  // check feature name
  QPointer<CoordinateSystem> nominalSystem =
      (!feature->getGeometry().isNull())
          ? feature->getGeometry()->getNominalSystem()
          : QPointer<CoordinateSystem>(nullptr);
  if (!this->validateFeatureName(feature->getFeature()->getFeatureName(),
                                 feature->getFeatureTypeEnum())) {
    // check if feature should be overwritten
    if (overwrite &&
        feature->getFeatureTypeEnum() == FeatureTypes::ePointFeature) {
      // get existing feature and set new values
      // get a list of features with name name
      QList<QPointer<FeatureWrapper> > features =
          this->featureContainer.getFeaturesByName(
              feature->getFeature()->getFeatureName());

      for (QPointer<FeatureWrapper> fw : oi::snapshot(features)) {
        if (!fw->getGeometry()->getNominalSystem().isNull() &&
            fw->getGeometry()->getNominalSystem() ==
                feature->getGeometry()->getNominalSystem()) {
          // overwrite existing position with new position
          fw->getPoint()->setPoint(feature->getPoint()->getPosition());
          this->sendMessage("changed values of existing features",
                            MessageTypes::eWarningMessage);
          return false;
        }
      }
    } else {
      this->sendMessage("feature name already exists",
                        MessageTypes::eWarningMessage);
      return false;
    }
  }

  // new feature shall not be active
  feature->getFeature()->setActiveFeatureState(false);

  // check feature relations
  if (feature->getFeatureTypeEnum() == FeatureTypes::eCoordinateSystemFeature) {
    if (!feature->getCoordinateSystem().isNull() &&
        feature->getCoordinateSystem()->getIsBundleSystem()) {
      this->activeBundleSystem = feature->getCoordinateSystem();
    }

    // check if the system is a nominal system
    if (feature->getCoordinateSystem()->getIsStationSystem()) {
      return false;
    }

    // new coordinate system shall not be active
    feature->getCoordinateSystem()->setActiveCoordinateSystemState(false);

    // a nominal system cannot have observations
    if (feature->getCoordinateSystem()->getObservations().size() > 0) {
      return false;
    }

    // a new system cannot have trafo params
    if (feature->getCoordinateSystem()->getTransformationParameters().size() >
        0) {
      return false;
    }

    // a new system cannot have nominals
    if (feature->getCoordinateSystem()->getNominals().size() > 0) {
      return true;
    }

  } else if (feature->getFeatureTypeEnum() == FeatureTypes::eStationFeature) {
    // new station shall not be active
    feature->getStation()->setActiveStationState(false);

  } else if (feature->getFeatureTypeEnum() ==
             FeatureTypes::eTrafoParamFeature) {
    // start and destination system have to exist in this job
    if (feature->getTrafoParam()->getStartSystem().isNull() ||
        feature->getTrafoParam()->getDestinationSystem().isNull()) {
      return false;
    }
    QPointer<FeatureWrapper> jobFrom = this->getFeatureById(
        feature->getTrafoParam()->getStartSystem()->getId());
    QPointer<FeatureWrapper> jobTo = this->getFeatureById(
        feature->getTrafoParam()->getDestinationSystem()->getId());
    if (jobFrom.isNull() || jobTo.isNull() ||
        jobFrom->getCoordinateSystem().isNull() ||
        jobTo->getCoordinateSystem().isNull() ||
        jobFrom->getCoordinateSystem() !=
            feature->getTrafoParam()->getStartSystem() ||
        jobTo->getCoordinateSystem() !=
            feature->getTrafoParam()->getDestinationSystem()) {
      return false;
    }

  } else if (getIsGeometry(feature->getFeatureTypeEnum())) {
    // an incoming geometry never carries observations - those are
    // owned by a station in this job (D19) and a copy/paste or a
    // freshly-loaded feature cannot come with someone else's already
    // bound to it
    if (feature->getGeometry()->getObservations().size() > 0) {
      return false;
    }

    // if this geometry names a nominal system, it must already be
    // represented in this job - D17 no longer requires every geometry
    // to have one (only nominals do; a fit-produced actual ordinarily
    // has none)
    if (!feature->getGeometry()->getNominalSystem().isNull() &&
        !this->getCoordinateSystemsList().contains(
            feature->getGeometry()->getNominalSystem())) {
      return false;
    }

    // if this geometry realises a nominal, that nominal must already
    // be represented in this job. Many actuals may reference the same
    // nominal (re-inspection over time, D17) so there is nothing else
    // to enforce here.
    if (feature->getGeometry()->getRealises().has_value()) {
      const QPointer<Geometry> nominal =
          feature->getGeometry()->getRealises().value();
      QPointer<FeatureWrapper> jobNominal =
          nominal.isNull() ? QPointer<FeatureWrapper>(nullptr)
                           : this->getFeatureById(nominal->getId());
      if (jobNominal.isNull() || jobNominal->getGeometry().isNull() ||
          jobNominal->getGeometry() != nominal) {
        return false;
      }
    }
  }

  // check if the group is a new group
  bool isNewGroup = false;
  if (feature->getFeature()->getGroupName().compare("") != 0 &&
      !this->featureContainer.getFeatureGroupList().contains(
          feature->getFeature()->getGroupName())) {
    isNewGroup = true;
  }

  // add the feature to the internal lists and maps
  this->featureContainer.addFeature(feature);

  // add to the nominal system's own list, if it named one
  if (!nominalSystem.isNull()) {
    nominalSystem->addNominal(feature);
  }

  // if a group is set for the new feature emit the group changed signal
  if (isNewGroup) {
    this->notifyObservers(&JobObserver::onAvailableGroupsChanged);
  }

  return true;
}

/*!
 * \brief OiJob::canRemoveFeature
 * Check wether a feature could be deleted or not
 * \param feature
 * \return
 */
bool OiJob::canRemoveFeature(const QPointer<FeatureWrapper>& feature) const {
  // do not remove active station or active coordinate system
  if (!feature->getStation().isNull() &&
      feature->getStation()->getIsActiveStation()) {
    this->sendMessage("Cannot delete the active station",
                      MessageTypes::eWarningMessage);
    return false;
  }
  if ((!feature->getCoordinateSystem().isNull() &&
       feature->getCoordinateSystem()->getIsActiveCoordinateSystem()) ||
      (!feature->getStation().isNull() &&
       !feature->getStation()->getCoordinateSystem().isNull() &&
       feature->getStation()
           ->getCoordinateSystem()
           ->getIsActiveCoordinateSystem())) {
    this->sendMessage("Cannot delete the active coordinate system",
                      MessageTypes::eWarningMessage);
    return false;
  }

  // do not delete a station system (without deleting the station)
  if (!feature->getCoordinateSystem().isNull() &&
      feature->getCoordinateSystem()->getIsStationSystem()) {
    this->sendMessage(
        "Cannot delete a station system without the corresponding station",
        MessageTypes::eWarningMessage);
    return false;
  }

  // do not delete coordinate systems with observations
  if (!feature->getStation().isNull() &&
      !feature->getStation()->getCoordinateSystem().isNull() &&
      feature->getStation()->getCoordinateSystem()->getObservations().size() >
          0) {
    this->sendMessage(
        "Cannot delete a station which contains one or more observations",
        MessageTypes::eWarningMessage);
    return false;
  }

  // do not delete coordinate systems with nominals
  if (!feature->getCoordinateSystem().isNull() &&
      feature->getCoordinateSystem()->getNominals().size() > 0) {
    this->sendMessage(
        "Cannot delete a coordinate system which contains one or more nominals",
        MessageTypes::eWarningMessage);
    return false;
  }

  return true;
}

/*!
 * \brief OiJob::removeOwnedSubGeometries
 * D19 applied to functions (Stage 7h) - see the declaration's comment.
 * \param feature
 */
void OiJob::removeOwnedSubGeometries(const QPointer<FeatureWrapper>& feature) {
  if (feature.isNull() || feature->getGeometry().isNull()) {
    return;
  }

  // copy first: removeFeature (via ~Geometry) mutates the owner's
  // ownedSubGeometries list as each child is actually deleted
  const QList<QPointer<Geometry> > owned =
      feature->getGeometry()->getOwnedSubGeometries();

  for (const QPointer<Geometry>& child : oi::snapshot(owned)) {
    if (!child.isNull()) {
      this->removeFeature(child->getId());
    }
  }
}

/*!
 * \brief OiJob::checkCircleWarning
 * \param activeFeature
 * \param usedForActiveFeature
 * \return
 */
bool OiJob::checkCircleWarning(const QPointer<Feature>& activeFeature,
                               const QPointer<Feature>& usedForActiveFeature) {
  // check features
  if (activeFeature.isNull() || usedForActiveFeature.isNull() ||
      activeFeature->getId() == usedForActiveFeature->getId()) {
    return true;
  }

  // check if active feature is in the list of previously needed features of
  // usedForActiveFeature
  for (const QPointer<FeatureWrapper>& feature :
       oi::snapshot(usedForActiveFeature->getPreviouslyNeeded())) {
    // check feature
    if (feature.isNull() || feature->getFeature().isNull()) {
      continue;
    }

    if (feature->getFeature()->getId() == activeFeature->getId()) {
      return true;
    } else if (feature->getFeature()->getPreviouslyNeeded().size() > 0 &&
               this->checkCircleWarning(activeFeature, feature->getFeature())) {
      return true;
    }
  }

  return false;
}

/*!
 * \brief OiJob::setUpDependencies
 * \param element
 * \param feature
 */
void OiJob::setUpDependencies(const InputElement& element,
                              const QPointer<Feature>& feature) {
  if (getIsFeature(element.typeOfElement)) {
    switch (element.typeOfElement) {
      case ElementTypes::eCircleElement: {
        if (!element.geometry.isNull() &&
            !element.geometry->getFeatureWrapper().isNull()) {
          feature->addPreviouslyNeeded(element.geometry->getFeatureWrapper());
        }
        break;
      }
      case ElementTypes::eConeElement: {
        if (!element.geometry.isNull() &&
            !element.geometry->getFeatureWrapper().isNull()) {
          feature->addPreviouslyNeeded(element.geometry->getFeatureWrapper());
        }
        break;
      }
      case ElementTypes::eCylinderElement: {
        if (!element.geometry.isNull() &&
            !element.geometry->getFeatureWrapper().isNull()) {
          feature->addPreviouslyNeeded(element.geometry->getFeatureWrapper());
        }
        break;
      }
      case ElementTypes::eEllipseElement: {
        if (!element.geometry.isNull() &&
            !element.geometry->getFeatureWrapper().isNull()) {
          feature->addPreviouslyNeeded(element.geometry->getFeatureWrapper());
        }
        break;
      }
      case ElementTypes::eEllipsoidElement: {
        if (!element.geometry.isNull() &&
            !element.geometry->getFeatureWrapper().isNull()) {
          feature->addPreviouslyNeeded(element.geometry->getFeatureWrapper());
        }
        break;
      }
      case ElementTypes::eHyperboloidElement: {
        if (!element.geometry.isNull() &&
            !element.geometry->getFeatureWrapper().isNull()) {
          feature->addPreviouslyNeeded(element.geometry->getFeatureWrapper());
        }
        break;
      }
      case ElementTypes::eLineElement: {
        if (!element.geometry.isNull() &&
            !element.geometry->getFeatureWrapper().isNull()) {
          feature->addPreviouslyNeeded(element.geometry->getFeatureWrapper());
        }
        break;
      }
      case ElementTypes::eNurbsElement: {
        if (!element.geometry.isNull() &&
            !element.geometry->getFeatureWrapper().isNull()) {
          feature->addPreviouslyNeeded(element.geometry->getFeatureWrapper());
        }
        break;
      }
      case ElementTypes::eParaboloidElement: {
        if (!element.geometry.isNull() &&
            !element.geometry->getFeatureWrapper().isNull()) {
          feature->addPreviouslyNeeded(element.geometry->getFeatureWrapper());
        }
        break;
      }
      case ElementTypes::ePlaneElement: {
        if (!element.geometry.isNull() &&
            !element.geometry->getFeatureWrapper().isNull()) {
          feature->addPreviouslyNeeded(element.geometry->getFeatureWrapper());
        }
        break;
      }
      case ElementTypes::ePointElement: {
        if (!element.geometry.isNull() &&
            !element.geometry->getFeatureWrapper().isNull()) {
          feature->addPreviouslyNeeded(element.geometry->getFeatureWrapper());
        }
        break;
      }
      case ElementTypes::ePointCloudElement: {
        if (!element.geometry.isNull() &&
            !element.geometry->getFeatureWrapper().isNull()) {
          feature->addPreviouslyNeeded(element.geometry->getFeatureWrapper());
        }
        break;
      }
      case ElementTypes::eScalarEntityAngleElement: {
        if (!element.geometry.isNull() &&
            !element.geometry->getFeatureWrapper().isNull()) {
          feature->addPreviouslyNeeded(element.geometry->getFeatureWrapper());
        }
        break;
      }
      case ElementTypes::eScalarEntityDistanceElement: {
        if (!element.geometry.isNull() &&
            !element.geometry->getFeatureWrapper().isNull()) {
          feature->addPreviouslyNeeded(element.geometry->getFeatureWrapper());
        }
        break;
      }
      case ElementTypes::eScalarEntityMeasurementSeriesElement: {
        if (!element.geometry.isNull() &&
            !element.geometry->getFeatureWrapper().isNull()) {
          feature->addPreviouslyNeeded(element.geometry->getFeatureWrapper());
        }
        break;
      }
      case ElementTypes::eScalarEntityTemperatureElement: {
        if (!element.geometry.isNull() &&
            !element.geometry->getFeatureWrapper().isNull()) {
          feature->addPreviouslyNeeded(element.geometry->getFeatureWrapper());
        }
        break;
      }
      case ElementTypes::eSlottedHoleElement: {
        if (!element.geometry.isNull() &&
            !element.geometry->getFeatureWrapper().isNull()) {
          feature->addPreviouslyNeeded(element.geometry->getFeatureWrapper());
        }
        break;
      }
      case ElementTypes::eSphereElement: {
        if (!element.geometry.isNull() &&
            !element.geometry->getFeatureWrapper().isNull()) {
          feature->addPreviouslyNeeded(element.geometry->getFeatureWrapper());
        }
        break;
      }
      case ElementTypes::eTorusElement: {
        if (!element.geometry.isNull() &&
            !element.geometry->getFeatureWrapper().isNull()) {
          feature->addPreviouslyNeeded(element.geometry->getFeatureWrapper());
        }
        break;
      }
      case ElementTypes::eCoordinateSystemElement: {
        if (!element.coordSystem.isNull() &&
            !element.coordSystem->getFeatureWrapper().isNull()) {
          feature->addPreviouslyNeeded(
              element.coordSystem->getFeatureWrapper());
        }
        break;
      }
      case ElementTypes::eStationElement: {
        if (!element.station.isNull() &&
            !element.station->getFeatureWrapper().isNull()) {
          feature->addPreviouslyNeeded(element.station->getFeatureWrapper());
        }
        break;
      }
      case ElementTypes::eTrafoParamElement: {
        if (!element.trafoParam.isNull() &&
            !element.trafoParam->getFeatureWrapper().isNull()) {
          feature->addPreviouslyNeeded(element.trafoParam->getFeatureWrapper());
        }
        break;
      }
      default:
        break;
    }

  } else if (getIsObservation(element.typeOfElement)) {
    if (!element.observation.isNull() &&
        !feature->getFeatureWrapper().isNull() &&
        !feature->getFeatureWrapper()->getGeometry().isNull()) {
      feature->getFeatureWrapper()->getGeometry()->addObservation(
          element.observation);
    }

  } else if (getIsReading(element.typeOfElement)) {
    return;

  } else if (element.typeOfElement != ElementTypes::eUndefinedElement) {
    if (!element.geometry.isNull() &&
        !element.geometry->getFeatureWrapper().isNull()) {
      feature->addPreviouslyNeeded(element.geometry->getFeatureWrapper());
    }
  }
}

/*!
 * \brief OiJob::resetDependencies
 * \param element
 * \param feature
 */
void OiJob::resetDependencies(const InputElement& element,
                              const QPointer<Feature>& feature) {
  if (getIsFeature(element.typeOfElement)) {
    switch (element.typeOfElement) {
      case ElementTypes::eCircleElement: {
        if (!element.geometry.isNull() &&
            !element.geometry->getFeatureWrapper().isNull()) {
          feature->removePreviouslyNeeded(
              element.geometry->getFeatureWrapper());
        }
        break;
      }
      case ElementTypes::eConeElement: {
        if (!element.geometry.isNull() &&
            !element.geometry->getFeatureWrapper().isNull()) {
          feature->removePreviouslyNeeded(
              element.geometry->getFeatureWrapper());
        }
        break;
      }
      case ElementTypes::eCylinderElement: {
        if (!element.geometry.isNull() &&
            !element.geometry->getFeatureWrapper().isNull()) {
          feature->removePreviouslyNeeded(
              element.geometry->getFeatureWrapper());
        }
        break;
      }
      case ElementTypes::eEllipseElement: {
        if (!element.geometry.isNull() &&
            !element.geometry->getFeatureWrapper().isNull()) {
          feature->removePreviouslyNeeded(
              element.geometry->getFeatureWrapper());
        }
        break;
      }
      case ElementTypes::eEllipsoidElement: {
        if (!element.geometry.isNull() &&
            !element.geometry->getFeatureWrapper().isNull()) {
          feature->removePreviouslyNeeded(
              element.geometry->getFeatureWrapper());
        }
        break;
      }
      case ElementTypes::eHyperboloidElement: {
        if (!element.geometry.isNull() &&
            !element.geometry->getFeatureWrapper().isNull()) {
          feature->removePreviouslyNeeded(
              element.geometry->getFeatureWrapper());
        }
        break;
      }
      case ElementTypes::eLineElement: {
        if (!element.geometry.isNull() &&
            !element.geometry->getFeatureWrapper().isNull()) {
          feature->removePreviouslyNeeded(
              element.geometry->getFeatureWrapper());
        }
        break;
      }
      case ElementTypes::eNurbsElement: {
        if (!element.geometry.isNull() &&
            !element.geometry->getFeatureWrapper().isNull()) {
          feature->removePreviouslyNeeded(
              element.geometry->getFeatureWrapper());
        }
        break;
      }
      case ElementTypes::eParaboloidElement: {
        if (!element.geometry.isNull() &&
            !element.geometry->getFeatureWrapper().isNull()) {
          feature->removePreviouslyNeeded(
              element.geometry->getFeatureWrapper());
        }
        break;
      }
      case ElementTypes::ePlaneElement: {
        if (!element.geometry.isNull() &&
            !element.geometry->getFeatureWrapper().isNull()) {
          feature->removePreviouslyNeeded(
              element.geometry->getFeatureWrapper());
        }
        break;
      }
      case ElementTypes::ePointElement: {
        if (!element.geometry.isNull() &&
            !element.geometry->getFeatureWrapper().isNull()) {
          feature->removePreviouslyNeeded(
              element.geometry->getFeatureWrapper());
        }
        break;
      }
      case ElementTypes::ePointCloudElement: {
        if (!element.geometry.isNull() &&
            !element.geometry->getFeatureWrapper().isNull()) {
          feature->removePreviouslyNeeded(
              element.geometry->getFeatureWrapper());
        }
        break;
      }
      case ElementTypes::eScalarEntityAngleElement: {
        if (!element.geometry.isNull() &&
            !element.geometry->getFeatureWrapper().isNull()) {
          feature->removePreviouslyNeeded(
              element.geometry->getFeatureWrapper());
        }
        break;
      }
      case ElementTypes::eScalarEntityDistanceElement: {
        if (!element.geometry.isNull() &&
            !element.geometry->getFeatureWrapper().isNull()) {
          feature->removePreviouslyNeeded(
              element.geometry->getFeatureWrapper());
        }
        break;
      }
      case ElementTypes::eScalarEntityMeasurementSeriesElement: {
        if (!element.geometry.isNull() &&
            !element.geometry->getFeatureWrapper().isNull()) {
          feature->removePreviouslyNeeded(
              element.geometry->getFeatureWrapper());
        }
        break;
      }
      case ElementTypes::eScalarEntityTemperatureElement: {
        if (!element.geometry.isNull() &&
            !element.geometry->getFeatureWrapper().isNull()) {
          feature->removePreviouslyNeeded(
              element.geometry->getFeatureWrapper());
        }
        break;
      }
      case ElementTypes::eSlottedHoleElement: {
        if (!element.geometry.isNull() &&
            !element.geometry->getFeatureWrapper().isNull()) {
          feature->removePreviouslyNeeded(
              element.geometry->getFeatureWrapper());
        }
        break;
      }
      case ElementTypes::eSphereElement: {
        if (!element.geometry.isNull() &&
            !element.geometry->getFeatureWrapper().isNull()) {
          feature->removePreviouslyNeeded(
              element.geometry->getFeatureWrapper());
        }
        break;
      }
      case ElementTypes::eTorusElement: {
        if (!element.geometry.isNull() &&
            !element.geometry->getFeatureWrapper().isNull()) {
          feature->removePreviouslyNeeded(
              element.geometry->getFeatureWrapper());
        }
        break;
      }
      case ElementTypes::eCoordinateSystemElement: {
        if (!element.coordSystem.isNull() &&
            !element.coordSystem->getFeatureWrapper().isNull()) {
          feature->removePreviouslyNeeded(
              element.coordSystem->getFeatureWrapper());
        }
        break;
      }
      case ElementTypes::eStationElement: {
        if (!element.station.isNull() &&
            !element.station->getFeatureWrapper().isNull()) {
          feature->removePreviouslyNeeded(element.station->getFeatureWrapper());
        }
        break;
      }
      case ElementTypes::eTrafoParamElement: {
        if (!element.trafoParam.isNull() &&
            !element.trafoParam->getFeatureWrapper().isNull()) {
          feature->removePreviouslyNeeded(
              element.trafoParam->getFeatureWrapper());
        }
        break;
      }
      default:
        break;
    }

  } else if (getIsObservation(element.typeOfElement)) {
    if (!element.observation.isNull() &&
        !feature->getFeatureWrapper().isNull() &&
        !feature->getFeatureWrapper()->getGeometry().isNull()) {
      element.observation->removeTargetGeometry(
          feature->getFeatureWrapper()->getGeometry());
    }

  } else if (getIsReading(element.typeOfElement)) {
    return;

  } else if (element.typeOfElement != ElementTypes::eUndefinedElement) {
    if (!element.geometry.isNull() &&
        !element.geometry->getFeatureWrapper().isNull()) {
      feature->removePreviouslyNeeded(element.geometry->getFeatureWrapper());
    }
  }
}

/*!
 * \brief OiJob::addFeaturesFromXml
 * Add features when a project has been loaded (only accessible from
 * ProjectExchanger)
 * \param features
 */
void OiJob::addFeaturesFromXml(
    const QList<QPointer<FeatureWrapper> >& features) {
  for (const QPointer<FeatureWrapper>& feature : oi::snapshot(features)) {
    // check if feature is valid
    if (feature.isNull() || feature->getFeature().isNull()) {
      continue;
    }

    // check if the feature is a station system
    if (!feature->getCoordinateSystem().isNull() &&
        feature->getCoordinateSystem()->getIsStationSystem()) {
      continue;
    }

    // pass the job instance to the feature
    feature->getFeature()->job = this;
    for (QPointer<Function> function : feature->getFeature()->getFunctions()) {
      QObject::connect(function.data(), &Function::sendMessage, this,
                       &OiJob::sendMessage, Qt::AutoConnection);
    }

    if (!feature->getStation().isNull()) {
      // pass job to the station system
      if (!feature->getStation()->getCoordinateSystem().isNull()) {
        feature->getStation()->getCoordinateSystem()->job = this;
      }

      // pass the job to the station point
      if (!feature->getStation()->getPosition().isNull()) {
        feature->getStation()->getPosition()->job = this;
      }

    } else if (!feature->getCoordinateSystem().isNull()) {
      if (feature->getCoordinateSystem()->getIsBundleSystem()) {
        if (!feature->getCoordinateSystem()->getBundlePlugin().isNull()) {
          feature->getCoordinateSystem()->getBundlePlugin()->setCurrentJob(
              this);
          this->activeBundleSystem = feature->getCoordinateSystem();
        } else if (feature->getFeature()->getFeatureName() ==
                   "Bundle01") {  // default
          this->activeBundleSystem = feature->getCoordinateSystem();
        }
      }
    }

    // connect the feature's signals to slots in OiJob
    this->attachFeature(feature);

    // add the feature
    this->featureContainer.addFeature(feature);

    // save active feature
    if (feature->getFeature()->getIsActiveFeature()) {
      this->activeFeature = feature;
    }

    // save active coordinate system
    if (!feature->getCoordinateSystem().isNull() &&
        feature->getCoordinateSystem()->getIsActiveCoordinateSystem()) {
      this->activeCoordinateSystem = feature->getCoordinateSystem();
    } else if (!feature->getStation().isNull() &&
               !feature->getStation()->getCoordinateSystem().isNull() &&
               feature->getStation()
                   ->getCoordinateSystem()
                   ->getIsActiveCoordinateSystem()) {
      this->activeCoordinateSystem =
          feature->getStation()->getCoordinateSystem();
    }

    // save active station
    if (!feature->getStation().isNull() &&
        feature->getStation()->getIsActiveStation()) {
      this->activeStation = feature->getStation();
    }
  }
}

void OiJob::createTemplateFromJob() {
  this->removeAllObservations();  // from geometries

  // set first station to active station
  QList<QPointer<Station> > stations = this->getStationsList();
  std::sort(stations.begin(), stations.end(),
            [](const QPointer<Station>& lhs, const QPointer<Station>& rhs) {
              return lhs->getId() < rhs->getId();
            });

  // remove used sensors
  QList<Sensor> usedSensors;
  int i = 0;
  for (QPointer<Station> station : stations) {
    if (++i == 1) {
      blockSignals(true);  // supress copying of sensor
      stations.first()->setActiveStationState(true);
      blockSignals(false);
      stations.first()->setActiveFeatureState(true);
    } else {
      stations.first()->setActiveStationState(false);
      stations.first()->setActiveFeatureState(false);
    }
    station->resetSensor();
    // remove used sensors
    station->setUsedSensors(usedSensors);
    // remove observations from coordinate system
    QPointer<CoordinateSystem> coordianteSystem =
        station->getCoordinateSystem();
    for (QPointer<Observation> obs : coordianteSystem->getObservations()) {
      coordianteSystem->removeObservation(obs);
    }
  }

  // remove observations from coordinate system
  QList<QPointer<CoordinateSystem> > coordianteSystems =
      this->getCoordinateSystemsList();
  for (QPointer<CoordinateSystem> coordianteSystem : coordianteSystems) {
    for (QPointer<Observation> obs : coordianteSystem->getObservations()) {
      coordianteSystem->removeObservation(obs);
    }

    QPointer<BundleAdjustment> bundle = coordianteSystem->getBundlePlugin();
    if (!bundle.isNull()) {
      bundle->clearResults();
    }
  }

  // remove transformations parameter
  QList<QPointer<TrafoParam> > trafoParams =
      this->getTransformationParametersList();
  OiVec rotation(3);
  OiVec translation(3);
  OiVec scale(3);
  scale.setAt(0, 1.);
  scale.setAt(1, 1.);
  scale.setAt(2, 1.);
  for (QPointer<TrafoParam> trafoParam : trafoParams) {
    trafoParam->setTransformationParameters(rotation, translation, scale);
    trafoParam->setIsSolved(false);
  }
}
