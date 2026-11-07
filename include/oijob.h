#ifndef OIJOB_H
#define OIJOB_H

#include <QIODevice>
#include <QList>
#include <QMap>
#include <QMultiMap>
#include <QObject>
#include <QPointer>
#include <QSet>
#include <QString>
#include <QStringList>
#include <memory>

#include "core_export.h"
#include "featureattributes.h"
#include "featurecontainer.h"
#include "featureobserver.h"
#include "function.h"
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
class OI_CORE_EXPORT OiJob : public QObject, public FeatureObserver {
  friend class ::ProjectExchanger;
  friend struct codec::Access;  // the protobuf codec (see src/codec)
  Q_OBJECT
  Q_DISABLE_COPY_MOVE(OiJob)

 public:
  explicit OiJob(QObject* parent = nullptr);

  ~OiJob() override;

  // ####################################
  // get or set meta information of a job
  // ####################################

  [[nodiscard]] const QString& getJobName() const;
  void setJobName(const QString& jobName);

  [[nodiscard]] const QPointer<QFileDevice>& getJobDevice() const;
  void setJobDevice(const QPointer<QFileDevice>& jobDevice);

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
  [[nodiscard]] const QList<int>& getFeatureIdList() const;
  [[nodiscard]] const QStringList& getFeatureNameList() const;
  [[nodiscard]] const QStringList& getFeatureGroupList() const;
  [[nodiscard]] const QList<MeasurementConfigKey>& getUsedMeasurementConfigs()
      const;

  // access feature lists
  [[nodiscard]] const QList<QPointer<FeatureWrapper>>& getFeaturesList() const;
  [[nodiscard]] const QList<QPointer<CoordinateSystem>>&
  getCoordinateSystemsList() const;
  [[nodiscard]] QList<QPointer<CoordinateSystem>> getStationSystemsList() const;
  [[nodiscard]] QList<QPointer<CoordinateSystem>> getBundleSystemList() const;
  [[nodiscard]] const QList<QPointer<Station>>& getStationsList() const;
  [[nodiscard]] const QList<QPointer<TrafoParam>>&
  getTransformationParametersList() const;
  [[nodiscard]] const QList<QPointer<FeatureWrapper>>& getGeometriesList()
      const;

  // access specific features
  [[nodiscard]] QPointer<FeatureWrapper> getFeatureById(
      const int& featureId) const;
  [[nodiscard]] QList<QPointer<FeatureWrapper>> getFeaturesByName(
      const QString& name, const bool startWith = false) const;
  [[nodiscard]] QList<QPointer<FeatureWrapper>> getFeaturesByGroup(
      const QString& group) const;
  [[nodiscard]] QList<QPointer<FeatureWrapper>> getFeaturesByType(
      const FeatureTypes& type) const;
  [[nodiscard]] QList<QPointer<Geometry>> getGeometriesByMConfig(
      const MeasurementConfigKey& key) const;

  // access active features
  [[nodiscard]] const QPointer<FeatureWrapper>& getActiveFeature() const;
  [[nodiscard]] const QPointer<Station>& getActiveStation() const;
  [[nodiscard]] const QPointer<CoordinateSystem>& getActiveCoordinateSystem()
      const;
  [[nodiscard]] const QPointer<CoordinateSystem>& getActiveBundleSystem() const;
  [[nodiscard]] const QPointer<oi::BundleAdjustment>&
  getActiveBundleAdjustment() const;
  [[nodiscard]] const QJsonObject getActiveBundleTemplate() const;

  // get or set feature groups
  [[nodiscard]] const QString& getActiveGroup() const;
  bool setActiveGroup(const QString& group);

  // ######################
  // add or remove features
  // ######################

  // add features
  bool addFeature(const QPointer<FeatureWrapper>& feature);
  QList<QPointer<FeatureWrapper>> addFeatures(const FeatureAttributes& fAttr);
  bool addFeatures(const QList<QPointer<FeatureWrapper>>& features,
                   bool overwrite = false);

  // remove features
  bool removeFeature(const int& featureId);
  bool removeFeature(const QPointer<FeatureWrapper>& feature);
  bool removeFeatures(const QSet<int>& featureIds);
  bool removeFeatures(const QList<QPointer<FeatureWrapper>>& features);
  void removeAll();

  // ##########################################
  // add or remove functions and input elements
  // ##########################################

  // add or remove functions
  void addFunction(const QPointer<Function>& function);
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
  bool addOwnedSubGeometry(const QPointer<FeatureWrapper>& owner,
                           const QPointer<FeatureWrapper>& subGeometry,
                           const QPointer<Function>& producerFunction,
                           const int& neededElementsIndex = 0);

  // add input elements to a feature's function
  void addInputObservation(const QPointer<FeatureWrapper>& target,
                           const int& functionPosition,
                           const int& neededElementsIndex,
                           const QPointer<Observation>& observation);
  void addInputReading(const QPointer<FeatureWrapper>& target,
                       const int& functionPosition,
                       const int& neededElementsIndex,
                       const QPointer<Reading>& reading);
  void addInputFeature(const QPointer<FeatureWrapper>& target,
                       const int& functionPosition,
                       const int& neededElementsIndex,
                       const QPointer<FeatureWrapper>& feature);
  void addInputPoint(const QPointer<FeatureWrapper>& target,
                     const int& functionPosition,
                     const int& neededElementsIndex,
                     const QPointer<Point>& point);

  // D12: stamp a tag on an existing point (hand-allocation, e.g.
  // dissecting a batch in the office) and feed any function that
  // declares a matching role - the same resolution addMeasurementResults
  // uses at acquisition time, so hand-tagging and field acquisition are
  // the same operation (domain-model.md S3).
  void tagPoint(const QPointer<Point>& point, int featureId,
                const QString& roleName);

  // remove input elements from a feature's function
  void removeInputElement(const QPointer<FeatureWrapper>& target,
                          const int& functionPosition,
                          const int& neededElementsIndex, const int& elementId);

  // ##########################
  // add or remove observations
  // ##########################

  // add new observations
  void addMeasurementResults(const int& geomId,
                             const QList<QPointer<Reading>>& readings);

  // remove existing observations
  void removeObservations(const int& featureId);
  void removeObservations(const int& featureId, const QList<int> selectedIds);
  void removeAllObservations();
  void enableObservations(const int& featureId);
  void disableObservations(const int& featureId);

  // ###############################
  // set use state of input elements
  // ###############################

  void setShouldBeUsed(const QPointer<FeatureWrapper>& target,
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
  void setPipeline(const QPointer<FeatureWrapper>& target,
                   const int& functionIndex, const int& neededElementIndex,
                   const QList<PipelineStage>& stages, const bool& recalc);

  // convenience for the single most common declared stage - D16's
  // hand-picking case: reject one specific element with a reason,
  // without disturbing whatever pipeline already exists for this
  // position. Appends an eExcludeStage rather than replacing the
  // pipeline, so it composes with an already-declared
  // eResidualOutlierRejectionStage instead of clobbering it.
  void excludeElement(const QPointer<FeatureWrapper>& target,
                      const int& functionIndex, const int& neededElementIndex,
                      const int& elementId, const QString& reason,
                      const bool& recalc);

  void createTemplateFromJob();

 signals:

  // #########################################
  // signals to inform about job state changes
  // #########################################

  // active feature changes
  void activeFeatureChanged();
  void activeStationChanged();
  void activeCoordinateSystemChanged();

  // feature(s) added or removed
  void featureSetChanged();
  void coordSystemSetChanged();
  void stationSetChanged();
  void trafoParamSetChanged();
  void geometrySetChanged();

  // group(s) added or removed
  void availableGroupsChanged();
  void activeGroupChanged();

  // general feature attributes changed
  void featureAttributesChanged();

  // feature specific attributes changed
  void featureNameChanged(const int& featureId, const QString& oldName);
  void featureGroupChanged(const int& featureId, const QString& oldGroup);
  void featureCommentChanged(const int& featureId);
  void featureIsUpdatedChanged(const int& featureId);  // currently not
                                                       // connected
  void featureIsSolvedChanged(const int& featureId);
  void featureFunctionsChanged(const int& featureId);
  void featureUsedForChanged(const int& featureId);
  void featurePreviouslyNeededChanged(const int& featureId);

  // geometry specific attributes changed
  void geometryIsCommonChanged(const int& featureId);
  void geometryRealisesChanged(const int& featureId);
  void geometryObservationsChanged(const int& featureId);
  void groupMembersChanged(const int& featureId);
  void geometryNominalSystemChanged(const int& featureId);
  void geometryStatisticChanged(const int& featureId);
  void geometrySimulationDataChanged(const int& featureId);
  void geometryMeasurementConfigChanged(const int& featureId,
                                        const QString& oldMConfig,
                                        const MeasurementConfigKey oldKey);

  // coordinate system specific attributes changed
  void systemObservationsChanged(const int& featureId, const int& obsId);
  void systemTrafoParamsChanged(const int& featureId);
  void systemsNominalsChanged(const int& featureId);

  // station specific attributes changed
  void stationSensorChanged(const int& featureId);

  // transformation parameter specific attributes changed
  void trafoParamParametersChanged(const int& featureId);
  void trafoParamSystemsChanged(const int& featureId);
  void trafoParamIsUsedChanged(const int& featureId);
  // void trafoParamValidTimeChanged(const int &featureId);
  // void trafoParamIsMovementChanged(const int &featureId);
  void trafoParamIsDatumChanged(const int& featureId);

  // ###################################################
  // general signals to inform OpenIndy about job issues
  // ###################################################

  void sendMessage(const QString& msg, const MessageTypes& msgType,
                   const MessageDestinations& msgDest =
                       MessageDestinations::eConsoleMessage) const;

  // ##############################
  // signals to force recalculation
  // ##############################

  void recalcFeature(const QPointer<Feature>& feature);
  void recalcFeatureSet();

  // #####################################
  // signals forwarded from FeatureUpdater
  // #####################################

  void featuresRecalculated();
  void featureRecalculated(const int& featureId);
  void trafoParamRecalculated(const int& featureId);

 private slots:

  // ###########################################################################
  // internal slots that are called when a feature changes (or is added/removed)
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
  // ##############################
  // connect or disconnect features
  // ##############################

  void attachFeature(const QPointer<FeatureWrapper>& feature);
  void detachFeature(const QPointer<FeatureWrapper>& feature);

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

  class StationSystemObserver;
  std::unique_ptr<StationSystemObserver> stationSystemObserver;

  // ##############
  // helper methods
  // ##############

  [[nodiscard]] QStringList createFeatureNames(const QString& name,
                                               const int& count) const;
  [[nodiscard]] QPointer<FeatureWrapper> createFeatureWrapper(
      const FeatureTypes& type) const;
  bool checkAndSetUpNewFeature(const QPointer<FeatureWrapper>& feature,
                               bool overwrite = false);

  [[nodiscard]] bool canRemoveFeature(
      const QPointer<FeatureWrapper>& feature) const;

  // D19 applied to functions (Stage 7h): "deleting the owner deletes its
  // owned sub-geometry" - called from removeFeature() before the feature
  // itself is removed. Copies the owner's getOwnedSubGeometries() list
  // before iterating since removeFeature mutates it (via ~Geometry's
  // cleanup), and recurses through the ordinary removeFeature(int) path
  // rather than deleting Feature objects directly, so every owned child
  // gets the normal disconnect/container-removal/signal bookkeeping.
  void removeOwnedSubGeometries(const QPointer<FeatureWrapper>& feature);

  bool checkCircleWarning(const QPointer<Feature>& activeFeature,
                          const QPointer<Feature>& usedForActiveFeature);

  void setUpDependencies(const InputElement& element,
                         const QPointer<Feature>& feature);
  void resetDependencies(const InputElement& element,
                         const QPointer<Feature>& feature);

  // D12: feeds any function of the tagged feature that declares a
  // matching ePointElement role - the query behind tag resolution. Used
  // by both addMeasurementResults (acquisition) and tagPoint (hand
  // allocation), which is what makes them the same operation.
  void resolvePointIntent(const QPointer<Point>& point,
                          const MeasurementIntent& intent);

  // ###################################################################################
  // add features when a project has been loaded (only accessible from
  // ProjectExchanger)
  // ###################################################################################

  void addFeaturesFromXml(const QList<QPointer<FeatureWrapper>>& features);

 private:
  // #########################
  // meta information of a job
  // #########################

  QString jobName;                  // the name under that the job is saved
  QPointer<QFileDevice> jobDevice;  // the device where the job is saved

  // the last calculated digest of the job content
  QString digest;

  // ################################
  // active features and active group
  // ################################

  // active features
  QPointer<FeatureWrapper> activeFeature;
  QPointer<Station> activeStation;
  QPointer<CoordinateSystem> activeCoordinateSystem;
  QPointer<CoordinateSystem> activeBundleSystem;

  // active group
  QString activeGroup;

  // ################################
  // feature container and current id
  // ################################

  FeatureContainer featureContainer;  // all features of this job

  int nextId{1};  // the next free id an element of this job could get

  void enableOrDisableObservations(const int& featureId, bool enable);
  void enableOrDisableStationObservations(QPointer<Station> station,
                                          bool enable);
  void enableOrDisableGeometryObservations(const int& featureId, bool enable,
                                           QPointer<Station> station);

  QString loadedVersion;
};

}  // namespace oi

#endif  // OIJOB_H
