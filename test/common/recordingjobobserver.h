#ifndef RECORDINGJOBOBSERVER_H
#define RECORDINGJOBOBSERVER_H

#include <QString>
#include <QStringList>

#include "feature.h"
#include "jobobserver.h"
#include "oijob.h"

// Test helper: remembers every JobObserver notification as
// "name[:arg...]" (without the leading "on", first letter lower-cased) in
// arrival order.
class RecordingJobObserver : public oi::JobObserver {
 public:
  QStringList calls;
  QList<oi::OiJob*> destroyed;

  RecordingJobObserver() = default;
  ~RecordingJobObserver() override {
    for (oi::OiJob* job : this->attached) {
      job->removeObserver(this);
    }
  }

  // registers with the job and unregisters again on destruction (unless the
  // job died first)
  void attach(oi::OiJob* job) {
    this->attached << job;
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
    this->destroyed << job;
    this->attached.removeAll(job);
  }

  void onActiveFeatureChanged() override {
    this->calls << QString("activeFeatureChanged");
  }
  void onActiveStationChanged() override {
    this->calls << QString("activeStationChanged");
  }
  void onActiveCoordinateSystemChanged() override {
    this->calls << QString("activeCoordinateSystemChanged");
  }
  void onFeatureSetChanged() override {
    this->calls << QString("featureSetChanged");
  }
  void onCoordSystemSetChanged() override {
    this->calls << QString("coordSystemSetChanged");
  }
  void onStationSetChanged() override {
    this->calls << QString("stationSetChanged");
  }
  void onTrafoParamSetChanged() override {
    this->calls << QString("trafoParamSetChanged");
  }
  void onGeometrySetChanged() override {
    this->calls << QString("geometrySetChanged");
  }
  void onAvailableGroupsChanged() override {
    this->calls << QString("availableGroupsChanged");
  }
  void onActiveGroupChanged() override {
    this->calls << QString("activeGroupChanged");
  }
  void onFeatureAttributesChanged() override {
    this->calls << QString("featureAttributesChanged");
  }
  void onFeatureNameChanged(const int& featureId,
                            const QString& oldName) override {
    this->calls
        << QString("featureNameChanged:%1:%2").arg(featureId).arg(oldName);
  }
  void onFeatureGroupChanged(const int& featureId,
                             const QString& oldGroup) override {
    this->calls
        << QString("featureGroupChanged:%1:%2").arg(featureId).arg(oldGroup);
  }
  void onFeatureCommentChanged(const int& featureId) override {
    this->calls << QString("featureCommentChanged:%1").arg(featureId);
  }
  void onFeatureIsUpdatedChanged(const int& featureId) override {
    this->calls << QString("featureIsUpdatedChanged:%1").arg(featureId);
  }
  void onFeatureIsSolvedChanged(const int& featureId) override {
    this->calls << QString("featureIsSolvedChanged:%1").arg(featureId);
  }
  void onFeatureFunctionsChanged(const int& featureId) override {
    this->calls << QString("featureFunctionsChanged:%1").arg(featureId);
  }
  void onFeatureUsedForChanged(const int& featureId) override {
    this->calls << QString("featureUsedForChanged:%1").arg(featureId);
  }
  void onFeaturePreviouslyNeededChanged(const int& featureId) override {
    this->calls << QString("featurePreviouslyNeededChanged:%1").arg(featureId);
  }
  void onGeometryIsCommonChanged(const int& featureId) override {
    this->calls << QString("geometryIsCommonChanged:%1").arg(featureId);
  }
  void onGeometryRealisesChanged(const int& featureId) override {
    this->calls << QString("geometryRealisesChanged:%1").arg(featureId);
  }
  void onGeometryObservationsChanged(const int& featureId) override {
    this->calls << QString("geometryObservationsChanged:%1").arg(featureId);
  }
  void onGroupMembersChanged(const int& featureId) override {
    this->calls << QString("groupMembersChanged:%1").arg(featureId);
  }
  void onGeometryNominalSystemChanged(const int& featureId) override {
    this->calls << QString("geometryNominalSystemChanged:%1").arg(featureId);
  }
  void onGeometryStatisticChanged(const int& featureId) override {
    this->calls << QString("geometryStatisticChanged:%1").arg(featureId);
  }
  void onGeometrySimulationDataChanged(const int& featureId) override {
    this->calls << QString("geometrySimulationDataChanged:%1").arg(featureId);
  }
  void onGeometryMeasurementConfigChanged(
      const int& featureId, const QString& oldMConfig,
      const oi::MeasurementConfigKey oldKey) override {
    this->calls << QString("geometryMeasurementConfigChanged:%1:%2:%3")
                       .arg(featureId)
                       .arg(oldMConfig)
                       .arg(oldKey.getName());
  }
  void onSystemObservationsChanged(const int& featureId,
                                   const int& obsId) override {
    this->calls
        << QString("systemObservationsChanged:%1:%2").arg(featureId).arg(obsId);
  }
  void onSystemTrafoParamsChanged(const int& featureId) override {
    this->calls << QString("systemTrafoParamsChanged:%1").arg(featureId);
  }
  void onSystemsNominalsChanged(const int& featureId) override {
    this->calls << QString("systemsNominalsChanged:%1").arg(featureId);
  }
  void onStationSensorChanged(const int& featureId) override {
    this->calls << QString("stationSensorChanged:%1").arg(featureId);
  }
  void onTrafoParamParametersChanged(const int& featureId) override {
    this->calls << QString("trafoParamParametersChanged:%1").arg(featureId);
  }
  void onTrafoParamSystemsChanged(const int& featureId) override {
    this->calls << QString("trafoParamSystemsChanged:%1").arg(featureId);
  }
  void onTrafoParamIsUsedChanged(const int& featureId) override {
    this->calls << QString("trafoParamIsUsedChanged:%1").arg(featureId);
  }
  void onTrafoParamIsDatumChanged(const int& featureId) override {
    this->calls << QString("trafoParamIsDatumChanged:%1").arg(featureId);
  }
  void onSendMessage(const QString& msg, const oi::MessageTypes& msgType,
                     const oi::MessageDestinations& msgDest) override {
    this->calls << QString("sendMessage:%1:%2:%3")
                       .arg(msg)
                       .arg(static_cast<int>(msgType))
                       .arg(static_cast<int>(msgDest));
  }
  void onRecalcFeatureRequested(const QPointer<oi::Feature>& feature) override {
    this->calls << QString("recalcFeature:%1").arg(feature->getId());
  }
  void onRecalcFeatureSetRequested() override {
    this->calls << QString("recalcFeatureSet");
  }
  void onFeaturesRecalculated() override {
    this->calls << QString("featuresRecalculated");
  }
  void onFeatureRecalculated(const int& featureId) override {
    this->calls << QString("featureRecalculated:%1").arg(featureId);
  }
  void onTrafoParamRecalculated(const int& featureId) override {
    this->calls << QString("trafoParamRecalculated:%1").arg(featureId);
  }

 private:
  QList<oi::OiJob*> attached;
};

#endif  // RECORDINGJOBOBSERVER_H
