#ifndef RECORDINGJOBOBSERVER_H
#define RECORDINGJOBOBSERVER_H
#include <QString>
#include <vector>

#include "containers.h"
#include "feature.h"
#include "jobobserver.h"
#include "oijob.h"

// Test helper: remembers every JobObserver notification as
// "name[:arg...]" (without the leading "on", first letter lower-cased) in
// arrival order.
class RecordingJobObserver : public oi::JobObserver {
 public:
  std::vector<QString> calls;
  std::vector<oi::OiJob*> destroyed;

  RecordingJobObserver() = default;

  //! all calls joined with `sep`, for readable failure messages and substring
  //! checks
  [[nodiscard]] QString joined(const QString& sep = ";") const {
    QString out;
    for (const QString& call : this->calls) {
      if (!out.isEmpty()) {
        out += sep;
      }
      out += call;
    }
    return out;
  }

  ~RecordingJobObserver() override {
    for (oi::OiJob* job : this->attached) {
      job->removeObserver(this);
    }
  }

  // registers with the job and unregisters again on destruction (unless the
  // job died first)
  void attach(oi::OiJob* job) {
    this->attached.push_back(job);
    job->addObserver(this);
  }

  [[nodiscard]] int count(const QString& name) const {
    int n = 0;
    for (const QString& c : this->calls) {
      if (c == name || c.startsWith(name + ":")) {
        ++n;
      }
    }
    return n;
  }

  void onJobDestroyed(oi::OiJob* job) override {
    this->destroyed.push_back(job);
    oi::removeAll(this->attached, job);
  }

  void onActiveFeatureChanged() override {
    this->calls.push_back(QString("activeFeatureChanged"));
  }
  void onActiveStationChanged() override {
    this->calls.push_back(QString("activeStationChanged"));
  }
  void onActiveCoordinateSystemChanged() override {
    this->calls.push_back(QString("activeCoordinateSystemChanged"));
  }
  void onFeatureSetChanged() override {
    this->calls.push_back(QString("featureSetChanged"));
  }
  void onCoordSystemSetChanged() override {
    this->calls.push_back(QString("coordSystemSetChanged"));
  }
  void onStationSetChanged() override {
    this->calls.push_back(QString("stationSetChanged"));
  }
  void onTrafoParamSetChanged() override {
    this->calls.push_back(QString("trafoParamSetChanged"));
  }
  void onGeometrySetChanged() override {
    this->calls.push_back(QString("geometrySetChanged"));
  }
  void onAvailableGroupsChanged() override {
    this->calls.push_back(QString("availableGroupsChanged"));
  }
  void onActiveGroupChanged() override {
    this->calls.push_back(QString("activeGroupChanged"));
  }
  void onFeatureAttributesChanged() override {
    this->calls.push_back(QString("featureAttributesChanged"));
  }
  void onFeatureNameChanged(const int& featureId,
                            const QString& oldName) override {
    this->calls.push_back(
        QString("featureNameChanged:%1:%2").arg(featureId).arg(oldName));
  }
  void onFeatureGroupChanged(const int& featureId,
                             const QString& oldGroup) override {
    this->calls.push_back(
        QString("featureGroupChanged:%1:%2").arg(featureId).arg(oldGroup));
  }
  void onFeatureCommentChanged(const int& featureId) override {
    this->calls.push_back(QString("featureCommentChanged:%1").arg(featureId));
  }
  void onFeatureIsUpdatedChanged(const int& featureId) override {
    this->calls.push_back(QString("featureIsUpdatedChanged:%1").arg(featureId));
  }
  void onFeatureIsSolvedChanged(const int& featureId) override {
    this->calls.push_back(QString("featureIsSolvedChanged:%1").arg(featureId));
  }
  void onFeatureFunctionsChanged(const int& featureId) override {
    this->calls.push_back(QString("featureFunctionsChanged:%1").arg(featureId));
  }
  void onFeatureUsedForChanged(const int& featureId) override {
    this->calls.push_back(QString("featureUsedForChanged:%1").arg(featureId));
  }
  void onFeaturePreviouslyNeededChanged(const int& featureId) override {
    this->calls.push_back(
        QString("featurePreviouslyNeededChanged:%1").arg(featureId));
  }
  void onGeometryIsCommonChanged(const int& featureId) override {
    this->calls.push_back(QString("geometryIsCommonChanged:%1").arg(featureId));
  }
  void onGeometryRealisesChanged(const int& featureId) override {
    this->calls.push_back(QString("geometryRealisesChanged:%1").arg(featureId));
  }
  void onGeometryObservationsChanged(const int& featureId) override {
    this->calls.push_back(
        QString("geometryObservationsChanged:%1").arg(featureId));
  }
  void onGroupMembersChanged(const int& featureId) override {
    this->calls.push_back(QString("groupMembersChanged:%1").arg(featureId));
  }
  void onGeometryNominalSystemChanged(const int& featureId) override {
    this->calls.push_back(
        QString("geometryNominalSystemChanged:%1").arg(featureId));
  }
  void onGeometryStatisticChanged(const int& featureId) override {
    this->calls.push_back(
        QString("geometryStatisticChanged:%1").arg(featureId));
  }
  void onGeometrySimulationDataChanged(const int& featureId) override {
    this->calls.push_back(
        QString("geometrySimulationDataChanged:%1").arg(featureId));
  }
  void onGeometryMeasurementConfigChanged(
      const int& featureId, const QString& oldMConfig,
      const oi::MeasurementConfigKey oldKey) override {
    this->calls.push_back(QString("geometryMeasurementConfigChanged:%1:%2:%3")
                              .arg(featureId)
                              .arg(oldMConfig)
                              .arg(oldKey.getName()));
  }
  void onSystemObservationsChanged(const int& featureId,
                                   const int& obsId) override {
    this->calls.push_back(
        QString("systemObservationsChanged:%1:%2").arg(featureId).arg(obsId));
  }
  void onSystemTrafoParamsChanged(const int& featureId) override {
    this->calls.push_back(
        QString("systemTrafoParamsChanged:%1").arg(featureId));
  }
  void onSystemsNominalsChanged(const int& featureId) override {
    this->calls.push_back(QString("systemsNominalsChanged:%1").arg(featureId));
  }
  void onStationSensorChanged(const int& featureId) override {
    this->calls.push_back(QString("stationSensorChanged:%1").arg(featureId));
  }
  void onTrafoParamParametersChanged(const int& featureId) override {
    this->calls.push_back(
        QString("trafoParamParametersChanged:%1").arg(featureId));
  }
  void onTrafoParamSystemsChanged(const int& featureId) override {
    this->calls.push_back(
        QString("trafoParamSystemsChanged:%1").arg(featureId));
  }
  void onTrafoParamIsUsedChanged(const int& featureId) override {
    this->calls.push_back(QString("trafoParamIsUsedChanged:%1").arg(featureId));
  }
  void onTrafoParamIsDatumChanged(const int& featureId) override {
    this->calls.push_back(
        QString("trafoParamIsDatumChanged:%1").arg(featureId));
  }
  void onSendMessage(const QString& msg, const oi::MessageTypes& msgType,
                     const oi::MessageDestinations& msgDest) override {
    this->calls.push_back(QString("sendMessage:%1:%2:%3")
                              .arg(msg)
                              .arg(static_cast<int>(msgType))
                              .arg(static_cast<int>(msgDest)));
  }
  void onRecalcFeatureRequested(
      const oi::Guarded<oi::Feature>& feature) override {
    this->calls.push_back(QString("recalcFeature:%1").arg(feature->getId()));
  }
  void onRecalcFeatureSetRequested() override {
    this->calls.push_back(QString("recalcFeatureSet"));
  }
  void onFeaturesRecalculated() override {
    this->calls.push_back(QString("featuresRecalculated"));
  }
  void onFeatureRecalculated(const int& featureId) override {
    this->calls.push_back(QString("featureRecalculated:%1").arg(featureId));
  }
  void onTrafoParamRecalculated(const int& featureId) override {
    this->calls.push_back(QString("trafoParamRecalculated:%1").arg(featureId));
  }

 private:
  std::vector<oi::OiJob*> attached;
};

#endif  // RECORDINGJOBOBSERVER_H
