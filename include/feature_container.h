#ifndef OI_CORE_FEATURE_CONTAINER_H_
#define OI_CORE_FEATURE_CONTAINER_H_
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "coordinate_system.h"
#include "core_export.h"
#include "feature.h"
#include "feature_wrapper.h"
#include "guarded.h"
#include "station.h"
#include "trafo_param.h"
#include "types.h"

namespace oi {

/*!
 * \brief The FeatureContainer class
 * Works as a container for all features in an OiJob.
 * The features are stored as lists and also as maps (e.g. with their id as
 * key). The lists and maps are kept in sync.
 */
class OI_CORE_EXPORT FeatureContainer {
 public:
  FeatureContainer();

  ~FeatureContainer();

  // ###############
  // access features
  // ###############

  // getter to access feature lists
  [[nodiscard]] const std::vector<oi::Guarded<FeatureWrapper>>&
  getFeaturesList() const;
  [[nodiscard]] const std::vector<oi::Guarded<CoordinateSystem>>&
  getCoordinateSystemsList() const;
  [[nodiscard]] std::vector<oi::Guarded<CoordinateSystem>>
  getStationSystemsList() const;
  // std::vector<oi::Guarded<CoordinateSystem> > getBundleSystemList() const; //
  // TODO OI-982 remove
  [[nodiscard]] const std::vector<oi::Guarded<Station>>& getStationsList()
      const;
  [[nodiscard]] const std::vector<oi::Guarded<TrafoParam>>&
  getTransformationParametersList() const;
  [[nodiscard]] const std::vector<oi::Guarded<FeatureWrapper>>&
  getGeometriesList() const;

  // getter to get a list of all feature ids, names, groups and measurement
  // configs
  [[nodiscard]] const std::vector<int>& getFeatureIdList() const;
  [[nodiscard]] const std::vector<std::string>& getFeatureNameList() const;
  [[nodiscard]] const std::vector<std::string>& getFeatureGroupList() const;
  [[nodiscard]] const std::vector<MeasurementConfigKey>&
  getUsedMeasurementConfigs() const;

  // getter to access features by id, name, group, type or mConfig
  [[nodiscard]] oi::Guarded<FeatureWrapper> getFeatureById(
      const int& featureId) const;
  [[nodiscard]] std::vector<oi::Guarded<FeatureWrapper>> getFeaturesByName(
      const std::string& name, const bool startWith = false) const;
  [[nodiscard]] std::vector<oi::Guarded<FeatureWrapper>> getFeaturesByGroup(
      const std::string& group) const;
  [[nodiscard]] std::vector<oi::Guarded<FeatureWrapper>> getFeaturesByType(
      const FeatureTypes& type) const;
  [[nodiscard]] std::vector<oi::Guarded<Geometry>> getGeometriesByMConfig(
      const MeasurementConfigKey& key) const;

  // ######################
  // get number of features
  // ######################

  [[nodiscard]] int getFeatureCount() const;
  [[nodiscard]] int getGeometryCount() const;
  [[nodiscard]] int getFeatureCount(const FeatureTypes& type) const;
  [[nodiscard]] int getFeatureCount(const std::string& group) const;

  // ######################
  // add or remove features
  // ######################

  /*!
   * Takes ownership of the feature and registers it under its own wrapper
   * (Feature::getFeatureWrapper()). Returns that wrapper, or null when the
   * feature is invalid or its id is already taken; the feature is then
   * destroyed here, so a failed add never leaks.
   */
  oi::Guarded<FeatureWrapper> addFeature(std::unique_ptr<Feature> feature);
  bool removeFeature(const int& featureId);

  void checkAndClean(const int& featureId, const std::string& name,
                     const std::string& group, const FeatureTypes& type);

  void removeAll();

  // ##########################################################
  // callbacks on feature changes to update keys of helper maps
  // ##########################################################

  bool featureNameChanged(const int& featureId, const std::string& oldName);
  bool featureGroupChanged(const int& featureId, const std::string& oldGroup);
  bool geometryMeasurementConfigChanged(const int& featureId,
                                        const std::string& oldMConfig,
                                        const MeasurementConfigKey oldIsSaved);

 private:
  // ######################
  // feature lists and maps
  // ######################

  // The one owner of every feature in the job. Everything below (and every
  // other reference in the core) is a non-owning oi::Guarded handle. A
  // feature owns its wrapper, so destroying the unique_ptr ends the whole
  // lifetime and nulls every handle.
  std::vector<std::unique_ptr<Feature>> ownedFeatures;

  // feature lists (useful to provide lists of features by category (geometries,
  // stations etc.))
  std::vector<oi::Guarded<FeatureWrapper>>
      featuresList;  // list of all features in OpenIndy
  std::vector<oi::Guarded<CoordinateSystem>>
      coordSystems;  // list of all non-station coordinate systems
  std::vector<oi::Guarded<Station>>
      stationsList;  // list of all stations (each station has pointer to its
                     // coordinate system)
  std::vector<oi::Guarded<TrafoParam>>
      trafoParamsList;  // list of all trafo params
  std::vector<oi::Guarded<FeatureWrapper>>
      geometriesList;  // list of all geometry features

  // feature maps (useful to quickly find a feature with a given id, name, group
  // etc.)
  std::map<int, oi::Guarded<FeatureWrapper>>
      featuresIdMap;  // map of all features in OpenIndy with their id as key
  std::multimap<std::string, oi::Guarded<FeatureWrapper>>
      featuresNameMap;  // map of all features in OpenIndy with their name as
                        // key
  std::multimap<std::string, oi::Guarded<FeatureWrapper>>
      featuresGroupMap;  // map of all features in OpenIndy with their group as
                         // key
  std::multimap<FeatureTypes, oi::Guarded<FeatureWrapper>>
      featuresTypeMap;  // map of all features in OpenIndy with their type as
                        // key
  std::multimap<MeasurementConfigKey, oi::Guarded<Geometry>>
      geometriesMConfigMap;  // map of all geometries in OpenIndy with their
                             // measurement config name and saved state as key

  // lists with ids, names, groups and measurement configs
  std::vector<int> featureIds;
  std::vector<std::string> featureNames;
  std::vector<std::string> featureGroups;
  std::vector<MeasurementConfigKey> usedMConfigs;
};

}  // namespace oi

#endif  // OI_CORE_FEATURE_CONTAINER_H_
