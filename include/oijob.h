#ifndef OIJOB_H
#define OIJOB_H
#include <QString>
#include <algorithm>
#include <filesystem>
#include <memory>
#include <mutex>
#include <set>
#include <vector>

#include "core_export.h"
#include "executor.h"
#include "featureattributes.h"
#include "featurecontainer.h"
#include "featureobserver.h"
#include "function.h"
#include "guarded.h"
#include "jobobserver.h"
#include "messagesink.h"
#include "types.h"
#include "util.h"

class ProjectExchanger;

namespace oi {

enum class CompatibilyCheckResult {
  eCheckResult_match = 0,
  eCheckResult_job_wo_valid_version,
  eCheckResult_oi_wo_valid_version,
  eCheckResult_oi_lt_job,
  eCheckResult_oi_gt_job,
  eCheckResult_job_lt_oi_22_1
};

/*!
 * \brief The OiJob class
 * Represents an OpenIndy job (holds all features and active states)
 */
class OI_CORE_EXPORT OiJob : public oi::Trackable,
                             public FeatureObserver,
                             public MessageSink {
  friend class ::ProjectExchanger;
  friend struct codec::Access;  // the protobuf codec (see src/codec)

 public:
  explicit OiJob();

  ~OiJob() override;

  // ####################################
  // get or set meta information of a job
  // ####################################

  [[nodiscard]] const QString& getJobName() const;
  void setJobName(const QString& jobName);

  // the file the job is saved to (empty: not saved yet)
  [[nodiscard]] const std::filesystem::path& getJobFile() const;
  void setJobFile(std::filesystem::path jobFile);

  void setDigest(const QString&);
  [[nodiscard]] const QString& getDigest() const;

  [[nodiscard]] CompatibilyCheckResult checkCompatibilty() const;
  void setLoadedProjectVersion(const QString& loadedVersion);

  int generateUniqueId();

  bool validateFeatureName(const QString& name, const FeatureTypes& type);

  // ###########################################
  // methods to access the current feature state
  // ###########################################

  // number of features
  [[nodiscard]] int getFeatureCount() const;
  [[nodiscard]] int getGeometryCount() const;
  [[nodiscard]] int getFeatureCount(const FeatureTypes& type) const;
  [[nodiscard]] int getFeatureCount(const QString& group) const;

  // access a list of all feature ids and names
  [[nodiscard]] const std::vector<int>& getFeatureIdList() const;
  [[nodiscard]] const std::vector<QString>& getFeatureNameList() const;
  [[nodiscard]] const std::vector<QString>& getFeatureGroupList() const;
  [[nodiscard]] const std::vector<MeasurementConfigKey>&
  getUsedMeasurementConfigs() const;

  // access feature lists
  [[nodiscard]] const std::vector<oi::Guarded<FeatureWrapper>>&
  getFeaturesList() const;
  [[nodiscard]] const std::vector<oi::Guarded<CoordinateSystem>>&
  getCoordinateSystemsList() const;
  [[nodiscard]] std::vector<oi::Guarded<CoordinateSystem>>
  getStationSystemsList() const;
  [[nodiscard]] std::vector<oi::Guarded<CoordinateSystem>> getBundleSystemList()
      const;
  [[nodiscard]] const std::vector<oi::Guarded<Station>>& getStationsList()
      const;
  [[nodiscard]] const std::vector<oi::Guarded<TrafoParam>>&
  getTransformationParametersList() const;
  [[nodiscard]] const std::vector<oi::Guarded<FeatureWrapper>>&
  getGeometriesList() const;

  // access specific features
  [[nodiscard]] oi::Guarded<FeatureWrapper> getFeatureById(
      const int& featureId) const;
  [[nodiscard]] std::vector<oi::Guarded<FeatureWrapper>> getFeaturesByName(
      const QString& name, const bool startWith = false) const;
  [[nodiscard]] std::vector<oi::Guarded<FeatureWrapper>> getFeaturesByGroup(
      const QString& group) const;
  [[nodiscard]] std::vector<oi::Guarded<FeatureWrapper>> getFeaturesByType(
      const FeatureTypes& type) const;
  [[nodiscard]] std::vector<oi::Guarded<Geometry>> getGeometriesByMConfig(
      const MeasurementConfigKey& key) const;

  // access active features
  [[nodiscard]] const oi::Guarded<FeatureWrapper>& getActiveFeature() const;
  [[nodiscard]] const oi::Guarded<Station>& getActiveStation() const;
  [[nodiscard]] const oi::Guarded<CoordinateSystem>& getActiveCoordinateSystem()
      const;
  [[nodiscard]] const oi::Guarded<CoordinateSystem>& getActiveBundleSystem()
      const;
  [[nodiscard]] const oi::Guarded<oi::BundleAdjustment>&
  getActiveBundleAdjustment() const;
  [[nodiscard]] const QJsonObject getActiveBundleTemplate() const;

  // get or set feature groups
  [[nodiscard]] const QString& getActiveGroup() const;
  bool setActiveGroup(const QString& group);

  // ######################
  // add or remove features
  // ######################

  // add features
  bool addFeature(const oi::Guarded<FeatureWrapper>& feature);
  std::vector<oi::Guarded<FeatureWrapper>> addFeatures(
      const FeatureAttributes& fAttr);
  bool addFeatures(const std::vector<oi::Guarded<FeatureWrapper>>& features,
                   bool overwrite = false);

  // remove features
  bool removeFeature(const int& featureId);
  bool removeFeature(const oi::Guarded<FeatureWrapper>& feature);
  bool removeFeatures(const std::set<int>& featureIds);
  bool removeFeatures(const std::vector<oi::Guarded<FeatureWrapper>>& features);
  void removeAll();

  // ##########################################
  // add or remove functions and input elements
  // ##########################################

  // add or remove functions
  void addFunction(const oi::Guarded<Function>& function);
  void removeFunction(const int& functionIndex);

  // #############################################################
  // D19 applied to functions (Stage 7h): generate function-owned
  // sub-geometry
  // #############################################################

  // Attaches `producerFunction` (an ordinary ConstructFunction, e.g.
  // LineFromPlaneNormal - the same class 7c-i's independent, editable
  // feature uses) to `subGeometry`, binds `owner` as its input at
  // neededElementsIndex, and records `owner` as subGeometry's owner
  //(Geometry::setOwnerFeature) so it is read-only and regenerates on
  // the owner's recompute. `subGeometry` must already be added to this
  // job (e.g. via addFeatures) with no function attached yet. Reuses
  // the ordinary dependency-wiring (addInputFeature) and function-attach
  //(Feature::addFunction) machinery - not a parallel path. Generic
  // across geometry type: this is the one primitive Haiku's replication
  // pass for other owner/accessor pairs reuses unchanged.
  bool addOwnedSubGeometry(const oi::Guarded<FeatureWrapper>& owner,
                           const oi::Guarded<FeatureWrapper>& subGeometry,
                           const oi::Guarded<Function>& producerFunction,
                           const int& neededElementsIndex = 0);

  // add input elements to a feature's function
  void addInputObservation(const oi::Guarded<FeatureWrapper>& target,
                           const int& functionPosition,
                           const int& neededElementsIndex,
                           const oi::Guarded<Observation>& observation);
  void addInputReading(const oi::Guarded<FeatureWrapper>& target,
                       const int& functionPosition,
                       const int& neededElementsIndex,
                       const oi::Guarded<Reading>& reading);
  void addInputFeature(const oi::Guarded<FeatureWrapper>& target,
                       const int& functionPosition,
                       const int& neededElementsIndex,
                       const oi::Guarded<FeatureWrapper>& feature);
  void addInputPoint(const oi::Guarded<FeatureWrapper>& target,
                     const int& functionPosition,
                     const int& neededElementsIndex,
                     const oi::Guarded<Point>& point);

  // D12: stamp a tag on an existing point (hand-allocation, e.g.
  // dissecting a batch in the office) and feed any function that
  // declares a matching role - the same resolution addMeasurementResults
  // uses at acquisition time, so hand-tagging and field acquisition are
  // the same operation (domain-model.md S3).
  void tagPoint(const oi::Guarded<Point>& point, int featureId,
                const QString& roleName);

  // remove input elements from a feature's function
  void removeInputElement(const oi::Guarded<FeatureWrapper>& target,
                          const int& functionPosition,
                          const int& neededElementsIndex, const int& elementId);

  // ##########################
  // add or remove observations
  // ##########################

  // add new observations
  void addMeasurementResults(const int& geomId,
                             const std::vector<oi::Guarded<Reading>>& readings);

  // remove existing observations
  void removeObservations(const int& featureId);
  void removeObservations(const int& featureId,
                          const std::vector<int> selectedIds);
  void removeAllObservations();
  void enableObservations(const int& featureId);
  void disableObservations(const int& featureId);

  // ###############################
  // set use state of input elements
  // ###############################

  void setShouldBeUsed(const oi::Guarded<FeatureWrapper>& target,
                       const int& functionIndex, const int& neededElementIndex,
                       const int& elementId, const bool& use,
                       const bool& recalc);

  // #################################################
  // declare a NeededElement's filter pipeline (D16, domain-model.md S5)
  // #################################################

  // replace the whole declared pipeline for one NeededElement position -
  // the callable entry point a UI would use for "define the filter for
  // this role". Same shape as setShouldBeUsed above: mutate, then
  // optionally force the recalc that makes it take effect.
  void setPipeline(const oi::Guarded<FeatureWrapper>& target,
                   const int& functionIndex, const int& neededElementIndex,
                   const std::vector<PipelineStage>& stages,
                   const bool& recalc);

  // convenience for the single most common declared stage - D16's
  // hand-picking case: reject one specific element with a reason,
  // without disturbing whatever pipeline already exists for this
  // position. Appends an eExcludeStage rather than replacing the
  // pipeline, so it composes with an already-declared
  // eResidualOutlierRejectionStage instead of clobbering it.
  void excludeElement(const oi::Guarded<FeatureWrapper>& target,
                      const int& functionIndex, const int& neededElementIndex,
                      const int& elementId, const QString& reason,
                      const bool& recalc);

  void createTemplateFromJob();

 public:
  // ###################################################################
  // report job issues and recalculation results (used to be signals)
  // ###################################################################

  void sendMessage(const QString& msg, const MessageTypes& msgType,
                   const MessageDestinations& msgDest =
                       MessageDestinations::eConsoleMessage) const;

  // forwarded from FeatureUpdater
  void featuresRecalculated();
  void featureRecalculated(const int& featureId);
  void trafoParamRecalculated(const int& featureId);

 public:
  // ##########################################################
  // observers: receive the job's change notifications (see
  // JobObserver). Non-owning; each observer is registered once.
  // ##########################################################

  void addObserver(JobObserver* observer);
  void removeObserver(JobObserver* observer);

  // ###########################################################
  // where the stations' sensor results are delivered (executor.h):
  // handed to every station of the job, now and when it is added.
  // Non-owning; it has to outlive the job. The default delivers
  // inline, on the sensors' worker threads.
  // ###########################################################

  void setExecutor(Executor* executor);
  [[nodiscard]] Executor* getExecutor() const;

 private:
  // ###########################################################################
  // internal handlers that are called when a feature changes (or is
  // added/removed)
  // ###########################################################################

  // active feature changes
  void setActiveFeature(const int& featureId);
  void setActiveStation(const int& featureId);
  void setActiveCoordinateSystem(const int& featureId);

  // feature specific attributes changed
  void setFeatureName(const int& featureId, const QString& oldName);
  void setFeatureGroup(const int& featureId, const QString& oldGroup);
  void setFeatureComment(const int& featureId);
  void setFeatureIsUpdated(const int& featureId);  // currently not connected
  void setFeatureIsSolved(const int& featureId);
  void setFeatureFunctions(const int& featureId);
  void setFeatureUsedFor(const int& featureId);
  void setFeaturePreviouslyNeeded(const int& featureId);

  // geometry specific attributes changed
  void setGeometryIsCommon(const int& featureId);
  void setGeometryRealises(const int& featureId);
  void setGeometryObservations(const int& featureId);
  void setGroupMembers(const int& featureId);
  void setGeometryNominalSystem(const int& featureId);
  void setGeometryStatistic(const int& featureId);
  void setGeometrySimulationData(const int& featureId);
  void setGeometryMeasurementConfig(const int& featureId,
                                    const QString& oldMConfig,
                                    const MeasurementConfigKey oldKey);

  // coordinate system specific attributes changed
  void setSystemObservations(const int& featureId, const int& obsId);
  void setSystemTrafoParams(const int& featureId);
  void setSystemsNominals(const int& featureId);

  // station specific attributes changed
  void setStationSensor(const int& featureId);

  // transformation parameter specific attributes changed
  void setTrafoParamParameters(const int& featureId);
  void setTrafoParamSystems(const int& featureId);
  void setTrafoParamIsUsed(const int& featureId);
  // void setTrafoParamValidTime(const int &featureId);
  // void setTrafoParamIsMovement(const int &featureId);
  void setTrafoParamIsDatum(const int& featureId);

  // element deleted
  void elementAboutToBeDeleted(const int& elementId, const QString& name,
                               const QString& group, const FeatureTypes& type);

 private:
  // calls `callback` on every registered observer. Iterates over a copy and
  // skips observers that were removed meanwhile, so observers may add or
  // remove themselves from within a callback.
  template <typename Callback, typename... Args>
  void notifyObservers(Callback callback, const Args&... args) {
    std::vector<JobObserver*> snapshot;
    {
      const std::lock_guard<std::mutex> locker(this->observersMutex);
      snapshot = this->observers;
    }
    for (JobObserver* observer : snapshot) {
      if (this->isObserver(observer)) {
        (observer->*callback)(args...);
      }
    }
  }

  [[nodiscard]] bool isObserver(const JobObserver* observer) const;

  // guarded: changes may be reported from worker threads (as the signals
  // they replace could be) while observers are added or removed
  mutable std::mutex observersMutex;
  std::vector<JobObserver*> observers;

  Executor* executor{&InlineExecutor::instance()};

  // ##############################
  // connect or disconnect features
  // ##############################

  void attachFeature(const oi::Guarded<FeatureWrapper>& feature);
  void detachFeature(const oi::Guarded<FeatureWrapper>& feature);

  // make this job the message sink of a function it owns (and take it back);
  // a function must never be left pointing at a job that is gone
  void adoptFunction(Function* function);
  void releaseFunction(Function* function);

  // MessageSink: messages of the functions and bundle adjustments of this job
  // are reported like the job's own
  void onMessage(const QString& msg, const MessageTypes& type,
                 const MessageDestinations& dest) override;

  // ##########################################################
  // FeatureObserver: relay the feature model's notifications
  // into the job's handlers (and from there into its signals)
  // ##########################################################

  void onElementAboutToBeDeleted(const int& elementId, const QString& name,
                                 const QString& group,
                                 const FeatureTypes& type) override;
  void onFeatureNameChanged(const int& featureId,
                            const QString& oldName) override;
  void onFeatureGroupChanged(const int& featureId,
                             const QString& oldGroup) override;
  void onFeatureCommentChanged(const int& featureId) override;
  void onFeatureIsSolvedChanged(const int& featureId) override;
  void onFeatureFunctionListChanged(const int& featureId) override;
  void onFeatureUsedForChanged(const int& featureId) override;
  void onFeaturePreviouslyNeededChanged(const int& featureId) override;
  void onFeatureIsActiveChanged(const int& featureId) override;
  void onGeomIsCommonChanged(const int& featureId) override;
  void onGeomRealisesChanged(const int& featureId) override;
  void onGeomObservationsChanged(const int& featureId) override;
  void onGeomNominalSystemChanged(const int& featureId) override;
  void onGeomStatisticChanged(const int& featureId) override;
  void onGeomSimulationDataChanged(const int& featureId) override;
  void onGeomMeasurementConfigChanged(
      const int& featureId, const QString& oldMConfig,
      const MeasurementConfigKey oldKey) override;
  void onGroupMembersChanged(const int& featureId) override;
  void onTransformationParameterChanged(const int& tpId) override;
  void onCoordinateSystemsChanged(const int& tpId) override;
  void onIsUsedChanged(const int& tpId) override;
  void onIsDatumTrafoChanged(const int& tpId) override;
  void onTransformationParametersChanged(const int& coordId) override;
  void onNominalsChanged(const int& coordId) override;
  void onActiveCoordinateSystemChanged(const int& coordId) override;
  void onActiveStationChanged(const int& stationId) override;
  void onSensorChanged(const int& stationId) override;

  class StationSystemObserver;
  std::unique_ptr<StationSystemObserver> stationSystemObserver;

  // ##############
  // helper methods
  // ##############

  [[nodiscard]] std::vector<QString> createFeatureNames(const QString& name,
                                                        const int& count) const;
  [[nodiscard]] oi::Guarded<FeatureWrapper> createFeatureWrapper(
      const FeatureTypes& type) const;
  bool checkAndSetUpNewFeature(const oi::Guarded<FeatureWrapper>& feature,
                               bool overwrite = false);

  [[nodiscard]] bool canRemoveFeature(
      const oi::Guarded<FeatureWrapper>& feature) const;

  // D19 applied to functions (Stage 7h): "deleting the owner deletes its
  // owned sub-geometry" - called from removeFeature() before the feature
  // itself is removed. Copies the owner's getOwnedSubGeometries() list
  // before iterating since removeFeature mutates it (via ~Geometry's
  // cleanup), and recurses through the ordinary removeFeature(int) path
  // rather than deleting Feature objects directly, so every owned child
  // gets the normal disconnect/container-removal/signal bookkeeping.
  void removeOwnedSubGeometries(const oi::Guarded<FeatureWrapper>& feature);

  bool checkCircleWarning(const oi::Guarded<Feature>& activeFeature,
                          const oi::Guarded<Feature>& usedForActiveFeature);

  void setUpDependencies(const InputElement& element,
                         const oi::Guarded<Feature>& feature);
  void resetDependencies(const InputElement& element,
                         const oi::Guarded<Feature>& feature);

  // D12: feeds any function of the tagged feature that declares a
  // matching ePointElement role - the query behind tag resolution. Used
  // by both addMeasurementResults (acquisition) and tagPoint (hand
  // allocation), which is what makes them the same operation.
  void resolvePointIntent(const oi::Guarded<Point>& point,
                          const MeasurementIntent& intent);

  // ###################################################################################
  // add features when a project has been loaded (only accessible from
  // ProjectExchanger)
  // ###################################################################################

  void addFeaturesFromXml(
      const std::vector<oi::Guarded<FeatureWrapper>>& features);

 private:
  // #########################
  // meta information of a job
  // #########################

  QString jobName;                // the name under that the job is saved
  std::filesystem::path jobFile;  // the file where the job is saved

  // the last calculated digest of the job content
  QString digest;

  // ################################
  // active features and active group
  // ################################

  // active features
  oi::Guarded<FeatureWrapper> activeFeature;
  oi::Guarded<Station> activeStation;
  oi::Guarded<CoordinateSystem> activeCoordinateSystem;
  oi::Guarded<CoordinateSystem> activeBundleSystem;

  // active group
  QString activeGroup;

  // ################################
  // feature container and current id
  // ################################

  FeatureContainer featureContainer;  // all features of this job

  int nextId{1};  // the next free id an element of this job could get

  void enableOrDisableObservations(const int& featureId, bool enable);
  void enableOrDisableStationObservations(oi::Guarded<Station> station,
                                          bool enable);
  void enableOrDisableGeometryObservations(const int& featureId, bool enable,
                                           oi::Guarded<Station> station);

  QString loadedVersion;
};

}  // namespace oi

#endif  // OIJOB_H
