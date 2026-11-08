#ifndef JOBOBSERVER_H
#define JOBOBSERVER_H

#include <string>

#include "core_export.h"
#include "guarded.h"
#include "measurement_config.h"
#include "types.h"

namespace oi {

class Feature;
class OiJob;

/*!
 * Receives the notifications an OiJob used to publish as Qt signals. Every
 * method defaults to doing nothing, so an observer overrides only what it
 * needs. Parameters are feature ids and std types only.
 *
 * Observers are registered with OiJob::addObserver (non-owning) and are
 * called synchronously on the thread that triggered the change.
 *
 * onRecalcFeatureRequested keeps the oi::Guarded<Feature> of the old
 * recalcFeature signal: the request is usually forwarded across threads
 * through queued delivery, and the guarded pointer is what keeps a feature
 * deleted in the meantime from being dereferenced.
 */
class JobObserver {
 public:
  JobObserver() = default;
  virtual ~JobObserver() = default;
  JobObserver(const JobObserver&) = delete;
  JobObserver& operator=(const JobObserver&) = delete;

  // called from ~OiJob, before the job's state is torn down; the observer must
  // not use the job afterwards and should drop its reference to it
  virtual void onJobDestroyed(OiJob* /*job*/) {}

  virtual void onActiveFeatureChanged() {}

  virtual void onActiveStationChanged() {}

  virtual void onActiveCoordinateSystemChanged() {}

  virtual void onFeatureSetChanged() {}

  virtual void onCoordSystemSetChanged() {}

  virtual void onStationSetChanged() {}

  virtual void onTrafoParamSetChanged() {}

  virtual void onGeometrySetChanged() {}

  virtual void onAvailableGroupsChanged() {}

  virtual void onActiveGroupChanged() {}

  virtual void onFeatureAttributesChanged() {}

  virtual void onFeatureNameChanged(const int& /*featureId*/,
                                    const std::string& /*oldName*/) {}

  virtual void onFeatureGroupChanged(const int& /*featureId*/,
                                     const std::string& /*oldGroup*/) {}

  virtual void onFeatureCommentChanged(const int& /*featureId*/) {}

  virtual void onFeatureIsUpdatedChanged(const int& /*featureId*/) {}

  virtual void onFeatureIsSolvedChanged(const int& /*featureId*/) {}

  virtual void onFeatureFunctionsChanged(const int& /*featureId*/) {}

  virtual void onFeatureUsedForChanged(const int& /*featureId*/) {}

  virtual void onFeaturePreviouslyNeededChanged(const int& /*featureId*/) {}

  virtual void onGeometryIsCommonChanged(const int& /*featureId*/) {}

  virtual void onGeometryRealisesChanged(const int& /*featureId*/) {}

  virtual void onGeometryObservationsChanged(const int& /*featureId*/) {}

  virtual void onGroupMembersChanged(const int& /*featureId*/) {}

  virtual void onGeometryNominalSystemChanged(const int& /*featureId*/) {}

  virtual void onGeometryStatisticChanged(const int& /*featureId*/) {}

  virtual void onGeometrySimulationDataChanged(const int& /*featureId*/) {}

  virtual void onGeometryMeasurementConfigChanged(
      const int& /*featureId*/, const std::string& /*oldMConfig*/,
      const MeasurementConfigKey /*oldKey*/) {}

  virtual void onSystemObservationsChanged(const int& /*featureId*/,
                                           const int& /*obsId*/) {}

  virtual void onSystemTrafoParamsChanged(const int& /*featureId*/) {}

  virtual void onSystemsNominalsChanged(const int& /*featureId*/) {}

  virtual void onStationSensorChanged(const int& /*featureId*/) {}

  virtual void onTrafoParamParametersChanged(const int& /*featureId*/) {}

  virtual void onTrafoParamSystemsChanged(const int& /*featureId*/) {}

  virtual void onTrafoParamIsUsedChanged(const int& /*featureId*/) {}

  virtual void onTrafoParamIsDatumChanged(const int& /*featureId*/) {}

  virtual void onSendMessage(const std::string& /*msg*/,
                             const MessageTypes& /*msgType*/,
                             const MessageDestinations& /*msgDest*/) {}

  virtual void onRecalcFeatureRequested(
      const oi::Guarded<Feature>& /*feature*/) {}

  virtual void onRecalcFeatureSetRequested() {}

  virtual void onFeaturesRecalculated() {}

  virtual void onFeatureRecalculated(const int& /*featureId*/) {}

  virtual void onTrafoParamRecalculated(const int& /*featureId*/) {}
};

}  // namespace oi

#endif  // JOBOBSERVER_H
