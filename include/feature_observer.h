#ifndef OI_CORE_FEATURE_OBSERVER_H_
#define OI_CORE_FEATURE_OBSERVER_H_

#include <string>

#include "core_export.h"
#include "measurement_config.h"
#include "types.h"

namespace oi {

/*!
 * \brief Receiver of the change notifications of the feature model.
 *
 * Replaces the Qt signals of Element/Feature/Geometry/Group/TrafoParam/
 * CoordinateSystem. A feature holds a plain, non-owning pointer to at most one
 * observer (normally the job it was added to); a feature without an observer
 * simply notifies nobody. Every method does nothing by default, so an observer
 * overrides only what it cares about.
 *
 * The observer must outlive the features it is attached to or detach itself
 * (Feature::setObserver(nullptr)) first - OiJob does so on removal and on
 * destruction.
 */
class OI_CORE_EXPORT FeatureObserver {
 public:
  FeatureObserver() = default;
  virtual ~FeatureObserver() = default;

  FeatureObserver(const FeatureObserver&) = delete;
  FeatureObserver& operator=(const FeatureObserver&) = delete;
  FeatureObserver(FeatureObserver&&) = delete;
  FeatureObserver& operator=(FeatureObserver&&) = delete;

  // Element: sent from the feature's destructor, the observer is detached
  // right afterwards
  virtual void onElementAboutToBeDeleted(const int& /*elementId*/,
                                         const std::string& /*name*/,
                                         const std::string& /*group*/,
                                         const FeatureTypes& /*type*/) {}

  // Feature
  virtual void onFeatureNameChanged(const int& /*featureId*/,
                                    const std::string& /*oldName*/) {}
  virtual void onFeatureGroupChanged(const int& /*featureId*/,
                                     const std::string& /*oldGroup*/) {}
  virtual void onFeatureCommentChanged(const int& /*featureId*/) {}
  virtual void onFeatureIsUpdatedChanged(const int& /*featureId*/) {}
  virtual void onFeatureIsSolvedChanged(const int& /*featureId*/) {}
  virtual void onFeatureFunctionListChanged(const int& /*featureId*/) {}
  virtual void onFeatureUsedForChanged(const int& /*featureId*/) {}
  virtual void onFeaturePreviouslyNeededChanged(const int& /*featureId*/) {}
  virtual void onFeatureIsActiveChanged(const int& /*featureId*/) {}

  // Geometry
  virtual void onGeomIsCommonChanged(const int& /*featureId*/) {}
  virtual void onGeomRealisesChanged(const int& /*featureId*/) {}
  virtual void onGeomOwnerFeatureChanged(const int& /*featureId*/) {}
  virtual void onGeomObservationsChanged(const int& /*featureId*/) {}
  virtual void onGeomNominalSystemChanged(const int& /*featureId*/) {}
  virtual void onGeomStatisticChanged(const int& /*featureId*/) {}
  virtual void onGeomSimulationDataChanged(const int& /*featureId*/) {}
  virtual void onGeomMeasurementConfigChanged(
      const int& /*featureId*/, const std::string& /*oldMConfig*/,
      const MeasurementConfigKey /*oldKey*/) {}
  virtual void onGeomParametersChanged(const int& /*featureId*/) {}

  // Group
  virtual void onGroupMembersChanged(const int& /*featureId*/) {}

  // TrafoParam
  virtual void onTransformationParameterChanged(const int& /*tpId*/) {}
  virtual void onCoordinateSystemsChanged(const int& /*tpId*/) {}
  virtual void onIsUsedChanged(const int& /*tpId*/) {}
  virtual void onIsBundleChanged(const int& /*tpId*/) {}
  virtual void onIsDatumTrafoChanged(const int& /*tpId*/) {}

  // Station (from Station::setActiveStationState and the sensor setters)
  virtual void onActiveStationChanged(const int& /*stationId*/) {}
  virtual void onSensorChanged(const int& /*stationId*/) {}

  // CoordinateSystem
  virtual void onObservationsChanged(const int& /*coordId*/,
                                     const int& /*obsId*/) {}
  virtual void onTransformationParametersChanged(const int& /*coordId*/) {}
  virtual void onNominalsChanged(const int& /*coordId*/) {}
  virtual void onActiveCoordinateSystemChanged(const int& /*coordId*/) {}
  virtual void onBundlePluginChanged(const int& /*coordId*/) {}
  // origin, axes or expansion origin of the system
  virtual void onCoordinateSystemFrameChanged(const int& /*coordId*/) {}
};

}  // namespace oi

#endif  // OI_CORE_FEATURE_OBSERVER_H_
