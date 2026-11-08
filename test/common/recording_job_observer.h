#ifndef OI_CORE_TEST_COMMON_RECORDING_JOB_OBSERVER_H_
#define OI_CORE_TEST_COMMON_RECORDING_JOB_OBSERVER_H_
#include <format>
#include <string>
#include <vector>

#include "containers.h"
#include "feature.h"
#include "job_observer.h"
#include "oi_job.h"

// Test helper: remembers every JobObserver notification as
// "name[:arg...]" (without the leading "on", first letter lower-cased) in
// arrival order.
class RecordingJobObserver : public oi::JobObserver {
 public:
  std::vector<std::string> calls;
  std::vector<oi::OiJob*> destroyed;

  RecordingJobObserver() = default;

  //! all calls joined with `sep`, for readable failure messages and substring
  //! checks
  [[nodiscard]] std::string joined(const std::string& sep = ";") const {
    std::string out;
    for (const std::string& call : this->calls) {
      if (!out.empty()) {
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

  [[nodiscard]] int count(const std::string& name) const {
    int n = 0;
    for (const std::string& c : this->calls) {
      if (c == name || c.starts_with(name + ":")) {
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
    this->calls.push_back(std::string("activeFeatureChanged"));
  }
  void onActiveStationChanged() override {
    this->calls.push_back(std::string("activeStationChanged"));
  }
  void onActiveCoordinateSystemChanged() override {
    this->calls.push_back(std::string("activeCoordinateSystemChanged"));
  }
  void onFeatureSetChanged() override {
    this->calls.push_back(std::string("featureSetChanged"));
  }
  void onCoordSystemSetChanged() override {
    this->calls.push_back(std::string("coordSystemSetChanged"));
  }
  void onStationSetChanged() override {
    this->calls.push_back(std::string("stationSetChanged"));
  }
  void onTrafoParamSetChanged() override {
    this->calls.push_back(std::string("trafoParamSetChanged"));
  }
  void onGeometrySetChanged() override {
    this->calls.push_back(std::string("geometrySetChanged"));
  }
  void onAvailableGroupsChanged() override {
    this->calls.push_back(std::string("availableGroupsChanged"));
  }
  void onActiveGroupChanged() override {
    this->calls.push_back(std::string("activeGroupChanged"));
  }
  void onFeatureAttributesChanged() override {
    this->calls.push_back(std::string("featureAttributesChanged"));
  }
  void onFeatureNameChanged(const int& featureId,
                            const std::string& oldName) override {
    this->calls.push_back(
        std::format("featureNameChanged:{}:{}", featureId, oldName));
  }
  void onFeatureGroupChanged(const int& featureId,
                             const std::string& oldGroup) override {
    this->calls.push_back(
        std::format("featureGroupChanged:{}:{}", featureId, oldGroup));
  }
  void onFeatureCommentChanged(const int& featureId) override {
    this->calls.push_back(std::format("featureCommentChanged:{}", featureId));
  }
  void onFeatureIsUpdatedChanged(const int& featureId) override {
    this->calls.push_back(std::format("featureIsUpdatedChanged:{}", featureId));
  }
  void onFeatureIsSolvedChanged(const int& featureId) override {
    this->calls.push_back(std::format("featureIsSolvedChanged:{}", featureId));
  }
  void onFeatureFunctionsChanged(const int& featureId) override {
    this->calls.push_back(std::format("featureFunctionsChanged:{}", featureId));
  }
  void onFeatureUsedForChanged(const int& featureId) override {
    this->calls.push_back(std::format("featureUsedForChanged:{}", featureId));
  }
  void onFeaturePreviouslyNeededChanged(const int& featureId) override {
    this->calls.push_back(
        std::format("featurePreviouslyNeededChanged:{}", featureId));
  }
  void onGeometryIsCommonChanged(const int& featureId) override {
    this->calls.push_back(std::format("geometryIsCommonChanged:{}", featureId));
  }
  void onGeometryRealisesChanged(const int& featureId) override {
    this->calls.push_back(std::format("geometryRealisesChanged:{}", featureId));
  }
  void onGeometryObservationsChanged(const int& featureId) override {
    this->calls.push_back(
        std::format("geometryObservationsChanged:{}", featureId));
  }
  void onGroupMembersChanged(const int& featureId) override {
    this->calls.push_back(std::format("groupMembersChanged:{}", featureId));
  }
  void onGeometryNominalSystemChanged(const int& featureId) override {
    this->calls.push_back(
        std::format("geometryNominalSystemChanged:{}", featureId));
  }
  void onGeometryStatisticChanged(const int& featureId) override {
    this->calls.push_back(
        std::format("geometryStatisticChanged:{}", featureId));
  }
  void onGeometrySimulationDataChanged(const int& featureId) override {
    this->calls.push_back(
        std::format("geometrySimulationDataChanged:{}", featureId));
  }
  void onGeometryMeasurementConfigChanged(
      const int& featureId, const std::string& oldMConfig,
      const oi::MeasurementConfigKey oldKey) override {
    this->calls.push_back(
        std::format("geometryMeasurementConfigChanged:{}:{}:{}", featureId,
                    oldMConfig, oldKey.getName()));
  }
  void onSystemObservationsChanged(const int& featureId,
                                   const int& obsId) override {
    this->calls.push_back(
        std::format("systemObservationsChanged:{}:{}", featureId, obsId));
  }
  void onSystemTrafoParamsChanged(const int& featureId) override {
    this->calls.push_back(
        std::format("systemTrafoParamsChanged:{}", featureId));
  }
  void onSystemsNominalsChanged(const int& featureId) override {
    this->calls.push_back(std::format("systemsNominalsChanged:{}", featureId));
  }
  void onStationSensorChanged(const int& featureId) override {
    this->calls.push_back(std::format("stationSensorChanged:{}", featureId));
  }
  void onTrafoParamParametersChanged(const int& featureId) override {
    this->calls.push_back(
        std::format("trafoParamParametersChanged:{}", featureId));
  }
  void onTrafoParamSystemsChanged(const int& featureId) override {
    this->calls.push_back(
        std::format("trafoParamSystemsChanged:{}", featureId));
  }
  void onTrafoParamIsUsedChanged(const int& featureId) override {
    this->calls.push_back(std::format("trafoParamIsUsedChanged:{}", featureId));
  }
  void onTrafoParamIsDatumChanged(const int& featureId) override {
    this->calls.push_back(
        std::format("trafoParamIsDatumChanged:{}", featureId));
  }
  void onSendMessage(const std::string& msg, const oi::MessageTypes& msgType,
                     const oi::MessageDestinations& msgDest) override {
    this->calls.push_back(std::format("sendMessage:{}:{}:{}", msg,
                                      static_cast<int>(msgType),
                                      static_cast<int>(msgDest)));
  }
  void onRecalcFeatureRequested(
      const oi::Guarded<oi::Feature>& feature) override {
    this->calls.push_back(std::format("recalcFeature:{}", feature->getId()));
  }
  void onRecalcFeatureSetRequested() override {
    this->calls.push_back(std::string("recalcFeatureSet"));
  }
  void onFeaturesRecalculated() override {
    this->calls.push_back(std::string("featuresRecalculated"));
  }
  void onFeatureRecalculated(const int& featureId) override {
    this->calls.push_back(std::format("featureRecalculated:{}", featureId));
  }
  void onTrafoParamRecalculated(const int& featureId) override {
    this->calls.push_back(std::format("trafoParamRecalculated:{}", featureId));
  }

 private:
  std::vector<oi::OiJob*> attached;
};

#endif  // OI_CORE_TEST_COMMON_RECORDING_JOB_OBSERVER_H_
