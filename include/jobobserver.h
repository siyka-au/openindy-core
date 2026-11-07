#ifndef JOBOBSERVER_H
#define JOBOBSERVER_H

#include <QString>

#include "core_export.h"
#include "guarded.h"
#include "measurementconfig.h"
#include "types.h"

namespace oi {

class Feature;
class OiJob;

/*!
 * Receives the notifications an OiJob used to publish as Qt signals. Every
 * method defaults to doing nothing, so an observer overrides only what it
 * needs. Parameters deliberately keep the Qt types for now.
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
  virtual void onJobDestroyed(OiJob* job) { Q_UNUSED(job); }

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

  virtual void onFeatureNameChanged(const int& featureId,
                                    const QString& oldName) {
    Q_UNUSED(featureId);
    Q_UNUSED(oldName);
  }

  virtual void onFeatureGroupChanged(const int& featureId,
                                     const QString& oldGroup) {
    Q_UNUSED(featureId);
    Q_UNUSED(oldGroup);
  }

  virtual void onFeatureCommentChanged(const int& featureId) {
    Q_UNUSED(featureId);
  }

  virtual void onFeatureIsUpdatedChanged(const int& featureId) {
    Q_UNUSED(featureId);
  }

  virtual void onFeatureIsSolvedChanged(const int& featureId) {
    Q_UNUSED(featureId);
  }

  virtual void onFeatureFunctionsChanged(const int& featureId) {
    Q_UNUSED(featureId);
  }

  virtual void onFeatureUsedForChanged(const int& featureId) {
    Q_UNUSED(featureId);
  }

  virtual void onFeaturePreviouslyNeededChanged(const int& featureId) {
    Q_UNUSED(featureId);
  }

  virtual void onGeometryIsCommonChanged(const int& featureId) {
    Q_UNUSED(featureId);
  }

  virtual void onGeometryRealisesChanged(const int& featureId) {
    Q_UNUSED(featureId);
  }

  virtual void onGeometryObservationsChanged(const int& featureId) {
    Q_UNUSED(featureId);
  }

  virtual void onGroupMembersChanged(const int& featureId) {
    Q_UNUSED(featureId);
  }

  virtual void onGeometryNominalSystemChanged(const int& featureId) {
    Q_UNUSED(featureId);
  }

  virtual void onGeometryStatisticChanged(const int& featureId) {
    Q_UNUSED(featureId);
  }

  virtual void onGeometrySimulationDataChanged(const int& featureId) {
    Q_UNUSED(featureId);
  }

  virtual void onGeometryMeasurementConfigChanged(
      const int& featureId, const QString& oldMConfig,
      const MeasurementConfigKey oldKey) {
    Q_UNUSED(featureId);
    Q_UNUSED(oldMConfig);
    Q_UNUSED(oldKey);
  }

  virtual void onSystemObservationsChanged(const int& featureId,
                                           const int& obsId) {
    Q_UNUSED(featureId);
    Q_UNUSED(obsId);
  }

  virtual void onSystemTrafoParamsChanged(const int& featureId) {
    Q_UNUSED(featureId);
  }

  virtual void onSystemsNominalsChanged(const int& featureId) {
    Q_UNUSED(featureId);
  }

  virtual void onStationSensorChanged(const int& featureId) {
    Q_UNUSED(featureId);
  }

  virtual void onTrafoParamParametersChanged(const int& featureId) {
    Q_UNUSED(featureId);
  }

  virtual void onTrafoParamSystemsChanged(const int& featureId) {
    Q_UNUSED(featureId);
  }

  virtual void onTrafoParamIsUsedChanged(const int& featureId) {
    Q_UNUSED(featureId);
  }

  virtual void onTrafoParamIsDatumChanged(const int& featureId) {
    Q_UNUSED(featureId);
  }

  virtual void onSendMessage(const QString& msg, const MessageTypes& msgType,
                             const MessageDestinations& msgDest) {
    Q_UNUSED(msg);
    Q_UNUSED(msgType);
    Q_UNUSED(msgDest);
  }

  virtual void onRecalcFeatureRequested(const oi::Guarded<Feature>& feature) {
    Q_UNUSED(feature);
  }

  virtual void onRecalcFeatureSetRequested() {}

  virtual void onFeaturesRecalculated() {}

  virtual void onFeatureRecalculated(const int& featureId) {
    Q_UNUSED(featureId);
  }

  virtual void onTrafoParamRecalculated(const int& featureId) {
    Q_UNUSED(featureId);
  }
};

}  // namespace oi

#endif  // JOBOBSERVER_H
