#ifndef FEATURECONTAINER_H
#define FEATURECONTAINER_H

#include <QList>
#include <QMap>
#include <QMultiMap>
#include <QString>
#include <QStringList>

#include "coordinatesystem.h"
#include "core_export.h"
#include "feature.h"
#include "featurewrapper.h"
#include "guarded.h"
#include "station.h"
#include "trafoparam.h"
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
  [[nodiscard]] const QList<oi::Guarded<FeatureWrapper>>& getFeaturesList()
      const;
  [[nodiscard]] const QList<oi::Guarded<CoordinateSystem>>&
  getCoordinateSystemsList() const;
  [[nodiscard]] QList<oi::Guarded<CoordinateSystem>> getStationSystemsList()
      const;
  // QList<oi::Guarded<CoordinateSystem> > getBundleSystemList() const; // TODO
  // OI-982 remove
  [[nodiscard]] const QList<oi::Guarded<Station>>& getStationsList() const;
  [[nodiscard]] const QList<oi::Guarded<TrafoParam>>&
  getTransformationParametersList() const;
  [[nodiscard]] const QList<oi::Guarded<FeatureWrapper>>& getGeometriesList()
      const;

  // getter to get a list of all feature ids, names, groups and measurement
  // configs
  [[nodiscard]] const QList<int>& getFeatureIdList() const;
  [[nodiscard]] const QStringList& getFeatureNameList() const;
  [[nodiscard]] const QStringList& getFeatureGroupList() const;
  [[nodiscard]] const QList<MeasurementConfigKey>& getUsedMeasurementConfigs()
      const;

  // getter to access features by id, name, group, type or mConfig
  [[nodiscard]] oi::Guarded<FeatureWrapper> getFeatureById(
      const int& featureId) const;
  [[nodiscard]] QList<oi::Guarded<FeatureWrapper>> getFeaturesByName(
      const QString& name, const bool startWith = false) const;
  [[nodiscard]] QList<oi::Guarded<FeatureWrapper>> getFeaturesByGroup(
      const QString& group) const;
  [[nodiscard]] QList<oi::Guarded<FeatureWrapper>> getFeaturesByType(
      const FeatureTypes& type) const;
  [[nodiscard]] QList<oi::Guarded<Geometry>> getGeometriesByMConfig(
      const MeasurementConfigKey& key) const;

  // ######################
  // get number of features
  // ######################

  [[nodiscard]] int getFeatureCount() const;
  [[nodiscard]] int getGeometryCount() const;
  [[nodiscard]] int getFeatureCount(const FeatureTypes& type) const;
  [[nodiscard]] int getFeatureCount(const QString& group) const;

  // ######################
  // add or remove features
  // ######################

  bool addFeature(const oi::Guarded<FeatureWrapper>& feature);
  bool removeFeature(const int& featureId);

  void checkAndClean(const int& featureId, const QString& name,
                     const QString& group, const FeatureTypes& type);

  void removeAll();

  // ##########################################################
  // callbacks on feature changes to update keys of helper maps
  // ##########################################################

  bool featureNameChanged(const int& featureId, const QString& oldName);
  bool featureGroupChanged(const int& featureId, const QString& oldGroup);
  bool geometryMeasurementConfigChanged(const int& featureId,
                                        const QString& oldMConfig,
                                        const MeasurementConfigKey oldIsSaved);

 private:
  // ######################
  // feature lists and maps
  // ######################

  // feature lists (useful to provide lists of features by category (geometries,
  // stations etc.))
  QList<oi::Guarded<FeatureWrapper>>
      featuresList;  // list of all features in OpenIndy
  QList<oi::Guarded<CoordinateSystem>>
      coordSystems;  // list of all non-station coordinate systems
  QList<oi::Guarded<Station>>
      stationsList;  // list of all stations (each station has pointer to its
                     // coordinate system)
  QList<oi::Guarded<TrafoParam>> trafoParamsList;  // list of all trafo params
  QList<oi::Guarded<FeatureWrapper>>
      geometriesList;  // list of all geometry features

  // feature maps (useful to quickly find a feature with a given id, name, group
  // etc.)
  QMap<int, oi::Guarded<FeatureWrapper>>
      featuresIdMap;  // map of all features in OpenIndy with their id as key
  QMultiMap<QString, oi::Guarded<FeatureWrapper>>
      featuresNameMap;  // map of all features in OpenIndy with their name as
                        // key
  QMultiMap<QString, oi::Guarded<FeatureWrapper>>
      featuresGroupMap;  // map of all features in OpenIndy with their group as
                         // key
  QMultiMap<FeatureTypes, oi::Guarded<FeatureWrapper>>
      featuresTypeMap;  // map of all features in OpenIndy with their type as
                        // key
  QMultiMap<MeasurementConfigKey, oi::Guarded<Geometry>>
      geometriesMConfigMap;  // map of all geometries in OpenIndy with their
                             // measurement config name and saved state as key

  // lists with ids, names, groups and measurement configs
  QList<int> featureIds;
  QStringList featureNames;
  QStringList featureGroups;
  QList<MeasurementConfigKey> usedMConfigs;
};

}  // namespace oi

#endif  // FEATURECONTAINER_H
