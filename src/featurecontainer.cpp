#include "featurecontainer.h"

#include <vector>

#include "containers.h"
#include "snapshot.h"
#include "stringutil.h"

using namespace oi;

/*!
 * \brief FeatureContainer::FeatureContainer
 */
FeatureContainer::FeatureContainer() {}

/*!
 * \brief FeatureContainer::~FeatureContainer
 */
FeatureContainer::~FeatureContainer() {}

/*!
 * \brief FeatureContainer::getFeaturesList
 * \return
 */
const std::vector<oi::Guarded<FeatureWrapper> >&
FeatureContainer::getFeaturesList() const {
  return this->featuresList;
}

/*!
 * \brief FeatureContainer::getCoordinateSystemsList
 * \return
 */
const std::vector<oi::Guarded<CoordinateSystem> >&
FeatureContainer::getCoordinateSystemsList() const {
  return this->coordSystems;
}

/*!
 * \brief FeatureContainer::getStationSystemsList
 * \return
 */
std::vector<oi::Guarded<CoordinateSystem> >
FeatureContainer::getStationSystemsList() const {
  std::vector<oi::Guarded<CoordinateSystem> > stationSystems;
  for (const oi::Guarded<Station>& station : oi::snapshot(this->stationsList)) {
    if (!station.isNull() && !station->getCoordinateSystem().isNull()) {
      stationSystems.push_back(station->getCoordinateSystem());
    }
  }
  return stationSystems;
}

/*!
 * \brief FeatureContainer::getBundleSystemList
 * \return
 */
/* // TODO OI-982 remove
std::vector<oi::Guarded<CoordinateSystem> >
FeatureContainer::getBundleSystemList() const{
std::vector<oi::Guarded<CoordinateSystem> > bundleSystems; for (const
oi::Guarded<CoordinateSystem> &system : oi::snapshot(this->coordSystems)){
if(!system.isNull() && system->getIsBundleSystem()){
bundleSystems.append(system);
        }
    }
    return bundleSystems;
}
*/
/*!
 * \brief FeatureContainer::getStationsList
 * \return
 */
const std::vector<oi::Guarded<Station> >& FeatureContainer::getStationsList()
    const {
  return this->stationsList;
}

/*!
 * \brief FeatureContainer::getTransformationParametersList
 * \return
 */
const std::vector<oi::Guarded<TrafoParam> >&
FeatureContainer::getTransformationParametersList() const {
  return this->trafoParamsList;
}

/*!
 * \brief FeatureContainer::getGeometriesList
 * \return
 */
const std::vector<oi::Guarded<FeatureWrapper> >&
FeatureContainer::getGeometriesList() const {
  return this->geometriesList;
}

/*!
 * \brief FeatureContainer::getFeatureIdList
 * \return
 */
const std::vector<int>& FeatureContainer::getFeatureIdList() const {
  return this->featureIds;
}

/*!
 * \brief FeatureContainer::getFeatureNameList
 * \return
 */
const std::vector<std::string>& FeatureContainer::getFeatureNameList() const {
  return this->featureNames;
}

/*!
 * \brief FeatureContainer::getFeatureGroupList
 * \return
 */
const std::vector<std::string>& FeatureContainer::getFeatureGroupList() const {
  return this->featureGroups;
}

/*!
 * \brief FeatureContainer::getUsedMeasurementConfigs
 * \return
 */
const std::vector<MeasurementConfigKey>&
FeatureContainer::getUsedMeasurementConfigs() const {
  return this->usedMConfigs;
}

/*!
 * \brief FeatureContainer::getFeatureById
 * \param featureId
 * \return
 */
oi::Guarded<FeatureWrapper> FeatureContainer::getFeatureById(
    const int& featureId) const {
  return oi::value(this->featuresIdMap, featureId,
                   oi::Guarded<FeatureWrapper>(nullptr));
}

/*!
 * \brief FeatureContainer::getFeaturesByName
 * \param name
 * \param startWith use name as "start with" pattern
 * \return
 */
std::vector<oi::Guarded<FeatureWrapper> > FeatureContainer::getFeaturesByName(
    const std::string& name, const bool startWith) const {
  if (startWith) {
    std::vector<oi::Guarded<FeatureWrapper> > result;
    for (const oi::Guarded<FeatureWrapper>& feature :
         oi::snapshot(this->featuresList)) {
      if (!feature.isNull() && !feature->getFeature().isNull() &&
          oi::startsWithIgnoreCase(feature->getFeature()->getFeatureName(),
                                   name)) {
        result.push_back(feature);
      }
    }
    return result;
  } else {
    return oi::values(this->featuresNameMap, name);
  }
}

/*!
 * \brief FeatureContainer::getFeaturesByGroup
 * \param group
 * \return
 */
std::vector<oi::Guarded<FeatureWrapper> > FeatureContainer::getFeaturesByGroup(
    const std::string& group) const {
  return oi::values(this->featuresGroupMap, group);
}

/*!
 * \brief FeatureContainer::getFeaturesByType
 * \param type
 * \return
 */
std::vector<oi::Guarded<FeatureWrapper> > FeatureContainer::getFeaturesByType(
    const FeatureTypes& type) const {
  return oi::values(this->featuresTypeMap, type);
}

/*!
 * \brief FeatureContainer::getGeometriesByMConfig
 * \param mConfig
 * \return
 */
std::vector<oi::Guarded<Geometry> > FeatureContainer::getGeometriesByMConfig(
    const MeasurementConfigKey& key) const {
  return oi::values(this->geometriesMConfigMap, key);
}

/*!
 * \brief FeatureContainer::getFeatureCount
 * \return
 */
int FeatureContainer::getFeatureCount() const {
  return this->featuresList.size();
}

/*!
 * \brief FeatureContainer::getGeometryCount
 * \return
 */
int FeatureContainer::getGeometryCount() const {
  return this->geometriesList.size();
}

/*!
 * \brief FeatureContainer::getFeatureCount
 * \param type
 * \return
 */
int FeatureContainer::getFeatureCount(const FeatureTypes& type) const {
  return this->featuresTypeMap.count(type);
}

/*!
 * \brief FeatureContainer::getFeatureCount
 * \param group
 * \return
 */
int FeatureContainer::getFeatureCount(const std::string& group) const {
  return this->featuresGroupMap.count(group);
}

/*!
 * \brief FeatureContainer::addFeature
 * \param feature
 * \return
 */
bool FeatureContainer::addFeature(const oi::Guarded<FeatureWrapper>& feature) {
  // if the feature is not valid
  if (feature.isNull() || feature->getFeature().isNull()) {
    return false;
  }

  // if the feature already exists it is not added
  if (this->featuresIdMap.contains(feature->getFeature()->getId())) {
    return false;
  }

  // add the feature to the feature lists and maps
  this->featuresList.push_back(feature);
  this->featuresIdMap.insert_or_assign(feature->getFeature()->getId(), feature);
  this->featuresNameMap.emplace(feature->getFeature()->getFeatureName(),
                                feature);
  this->featuresTypeMap.emplace(feature->getFeatureTypeEnum(), feature);
  if (feature->getFeature()->getGroupName().compare("") != 0) {
    this->featuresGroupMap.emplace(feature->getFeature()->getGroupName(),
                                   feature);
  }
  switch (feature->getFeatureTypeEnum()) {
    case FeatureTypes::eCoordinateSystemFeature:
      this->coordSystems.push_back(feature->getCoordinateSystem());
      break;
    case FeatureTypes::eStationFeature:
      this->stationsList.push_back(feature->getStation());
      if (!feature->getStation()->getCoordinateSystem().isNull()) {
        oi::Guarded<FeatureWrapper> stationSystem = new FeatureWrapper();
        stationSystem->setCoordinateSystem(
            feature->getStation()->getCoordinateSystem());
        this->featuresIdMap.insert_or_assign(
            feature->getStation()->getCoordinateSystem()->getId(),
            stationSystem);
      }
      break;
    case FeatureTypes::eTrafoParamFeature:
      this->trafoParamsList.push_back(feature->getTrafoParam());
      break;
    default:  // geometry
      if (!feature->getGeometry().isNull()) {
        this->geometriesList.push_back(feature);
        if (feature->getGeometry()->getMeasurementConfig().isValid()) {
          ;
          this->geometriesMConfigMap.emplace(
              feature->getGeometry()->getMeasurementConfig(),
              feature->getGeometry());
        }
      }
      break;
  }

  // update lists with ids, names, groups and mConfigs
  this->featureIds.push_back(feature->getFeature()->getId());
  if (oi::values(this->featuresNameMap, feature->getFeature()->getFeatureName())
          .size() == 1) {
    this->featureNames.push_back(feature->getFeature()->getFeatureName());
  }
  if (feature->getFeature()->getGroupName().compare("") != 0 &&
      !oi::contains(this->featureGroups,
                    feature->getFeature()->getGroupName())) {
    this->featureGroups.push_back(feature->getFeature()->getGroupName());
  }
  if (!feature->getGeometry().isNull() &&
      feature->getGeometry()->getMeasurementConfig().isValid()) {
    this->usedMConfigs.push_back(
        feature->getGeometry()->getMeasurementConfig());
  }

  return true;
}

/*!
 * \brief FeatureContainer::removeFeature
 * \param featureId
 * \return
 */
bool FeatureContainer::removeFeature(const int& featureId) {
  // check if the feature exists
  if (!this->featuresIdMap.contains(featureId)) {
    return false;
  }

  // get the feature pointer
  oi::Guarded<FeatureWrapper> feature =
      oi::value(this->featuresIdMap, featureId, oi::Guarded<FeatureWrapper>());

  // check if the feature is valid
  if (feature.isNull() || feature->getFeature().isNull()) {
    return false;
  }

  // remove the feature from lists and map
  oi::removeOne(this->featuresList, feature);
  this->featuresIdMap.erase(featureId);
  oi::eraseValue(this->featuresNameMap, feature->getFeature()->getFeatureName(),
                 feature);
  oi::eraseValue(this->featuresTypeMap, feature->getFeatureTypeEnum(), feature);
  if (feature->getFeature()->getGroupName().compare("") != 0) {
    oi::eraseValue(this->featuresGroupMap,
                   feature->getFeature()->getGroupName(), feature);
  }
  switch (feature->getFeatureTypeEnum()) {
    case FeatureTypes::eCoordinateSystemFeature:
      oi::removeOne(this->coordSystems, feature->getCoordinateSystem());
      break;
    case FeatureTypes::eStationFeature:
      oi::removeOne(this->stationsList, feature->getStation());
      break;
    case FeatureTypes::eTrafoParamFeature:
      oi::removeOne(this->trafoParamsList, feature->getTrafoParam());
      break;
    default:  // geometry
      if (!feature->getGeometry().isNull()) {
        oi::removeOne(this->geometriesList, feature);
        if (feature->getGeometry()->getMeasurementConfig().isValid()) {
          oi::eraseValue(this->geometriesMConfigMap,
                         feature->getGeometry()->getMeasurementConfig(),
                         feature->getGeometry());
        }
      }
      break;
  }

  // update lists with ids, names and groups
  oi::removeOne(this->featureIds, feature->getFeature()->getId());
  if (!this->featuresNameMap.contains(
          feature->getFeature()->getFeatureName())) {
    oi::removeOne(this->featureNames, feature->getFeature()->getFeatureName());
  }
  if (feature->getFeature()->getGroupName().compare("") != 0 &&
      !this->featuresGroupMap.contains(feature->getFeature()->getGroupName())) {
    oi::removeOne(this->featureGroups, feature->getFeature()->getGroupName());
  }
  if (!feature->getGeometry().isNull() &&
      feature->getGeometry()->getMeasurementConfig().isValid()) {
    oi::removeOne(this->usedMConfigs,
                  feature->getGeometry()->getMeasurementConfig());
  }

  // delete the feature
  delete feature->getFeature().data();
  delete feature.data();

  return true;
}

/*!
 * \brief FeatureContainer::checkAndClean
 * Checks wether there is a feature with the given id and removes it
 * \param featureId
 * \param name
 * \param group
 * \param type
 */
void FeatureContainer::checkAndClean(const int& featureId,
                                     const std::string& name,
                                     const std::string& group,
                                     const FeatureTypes& type) {
  if (!this->featuresIdMap.contains(featureId)) {
    return;
  }

  // get and check feature by id
  oi::Guarded<FeatureWrapper> feature =
      oi::value(this->featuresIdMap, featureId);
  if (feature.isNull() || feature->getFeature().isNull()) {
    return;
  }

  // clean feature list
  oi::removeOne(this->featuresList, feature);

  // clean coordinate system list
  if (!feature->getCoordinateSystem().isNull()) {
    oi::removeOne(this->coordSystems, feature->getCoordinateSystem());
  }

  // clean station list
  if (!feature->getStation().isNull()) {
    oi::removeOne(this->stationsList, feature->getStation());
  }

  // clean trafo param list
  if (!feature->getTrafoParam().isNull()) {
    oi::removeOne(this->trafoParamsList, feature->getTrafoParam());
  }

  // clean geometry list
  if (!feature->getGeometry().isNull()) {
    oi::removeOne(this->geometriesList, feature);
  }

  // clean feature maps
  this->featuresIdMap.erase(featureId);
  oi::eraseValue(this->featuresNameMap, name, feature);
  oi::eraseValue(this->featuresGroupMap, group, feature);
  oi::eraseValue(this->featuresTypeMap, type, feature);
  if (!feature->getGeometry().isNull()) {
    oi::eraseValue(this->geometriesMConfigMap,
                   feature->getGeometry()->getMeasurementConfig(),
                   feature->getGeometry());
  }

  // clean list with ids, names, groups and mConfigs
  oi::removeOne(this->featureIds, featureId);
  if (!this->featuresNameMap.contains(name)) {
    oi::removeOne(this->featureNames, name);
  }
  if (!this->featuresGroupMap.contains(group)) {
    oi::removeOne(this->featureGroups, group);
  }
  if (!feature->getGeometry().isNull()) {
    if (!this->geometriesMConfigMap.contains(
            feature->getGeometry()->getMeasurementConfig())) {
      oi::removeOne(this->usedMConfigs,
                    feature->getGeometry()->getMeasurementConfig());
    }
  }
}

/*!
 * \brief FeatureContainer::removeAll
 * Remove and delete all features
 */
void FeatureContainer::removeAll() {
  // delete all features
  for (const oi::Guarded<FeatureWrapper>& feature :
       oi::snapshot(this->featuresList)) {
    if (!feature.isNull()) {
      if (feature->getFeature().isNull()) {
        delete feature->getFeature().data();
      }
      delete feature.data();
    }
  }

  // clear all lists and maps
  this->featuresList.clear();
  this->coordSystems.clear();
  this->stationsList.clear();
  this->trafoParamsList.clear();
  this->geometriesList.clear();
  this->featuresIdMap.clear();
  this->featuresNameMap.clear();
  this->featuresGroupMap.clear();
  this->featuresTypeMap.clear();
  this->geometriesMConfigMap.clear();
  this->featureIds.clear();
  this->featureNames.clear();
  this->featureGroups.clear();
  this->usedMConfigs.clear();
}

/*!
 * \brief FeatureContainer::featureNameChanged
 * Is called whenever the name of a feature has changed
 * \param featureId
 * \param oldName
 * \return
 */
bool FeatureContainer::featureNameChanged(const int& featureId,
                                          const std::string& oldName) {
  // check if the feature exists
  if (!this->featuresIdMap.contains(featureId)) {
    return false;
  }

  // get the feature pointer
  oi::Guarded<FeatureWrapper> feature =
      oi::value(this->featuresIdMap, featureId, oi::Guarded<FeatureWrapper>());

  // check if the feature is valid
  if (feature.isNull() || feature->getFeature().isNull()) {
    return false;
  }

  // update lists and maps
  if (this->featuresNameMap.contains(oldName)) {
    // QMultiMap::take removed the most recently inserted value of the key
    const auto range = this->featuresNameMap.equal_range(oldName);
    auto last = std::prev(range.second);
    auto moved = last->second;
    this->featuresNameMap.erase(last);
    this->featuresNameMap.emplace(feature->getFeature()->getFeatureName(),
                                  std::move(moved));
  }
  if (!this->featuresNameMap.contains(oldName)) {
    oi::removeOne(this->featureNames, oldName);
  }
  if (!oi::contains(this->featureNames,
                    feature->getFeature()->getFeatureName())) {
    this->featureNames.push_back(feature->getFeature()->getFeatureName());
  }

  return true;
}

/*!
 * \brief FeatureContainer::featureGroupChanged
 * Is called whenever the group of a feature has changed
 * \param featureId
 * \param oldGroup
 * \return
 */
bool FeatureContainer::featureGroupChanged(const int& featureId,
                                           const std::string& oldGroup) {
  // check if the feature exists
  if (!this->featuresIdMap.contains(featureId)) {
    return false;
  }

  // get the feature pointer
  oi::Guarded<FeatureWrapper> feature =
      oi::value(this->featuresIdMap, featureId, oi::Guarded<FeatureWrapper>());

  // check if the feature is valid
  if (feature.isNull() || feature->getFeature().isNull()) {
    return false;
  }

  // check if the old group was empty
  if (oldGroup.compare("") == 0) {
    // check if the new group is empty
    if (feature->getFeature()->getGroupName().compare("") != 0) {
      this->featuresGroupMap.emplace(feature->getFeature()->getGroupName(),
                                     feature);
      if (!oi::contains(this->featureGroups,
                        feature->getFeature()->getGroupName())) {
        this->featureGroups.push_back(feature->getFeature()->getGroupName());
      }
    }

  } else {
    // check if the new group is empty
    if (feature->getFeature()->getGroupName().compare("") != 0) {
      oi::eraseValue(this->featuresGroupMap, oldGroup, feature);
      this->featuresGroupMap.emplace(feature->getFeature()->getGroupName(),
                                     feature);

      if (!this->featuresGroupMap.contains(oldGroup)) {
        oi::removeOne(this->featureGroups, oldGroup);
      }
      if (!oi::contains(this->featureGroups,
                        feature->getFeature()->getGroupName())) {
        this->featureGroups.push_back(feature->getFeature()->getGroupName());
      }

    } else {
      oi::eraseValue(this->featuresGroupMap, oldGroup, feature);
      if (!this->featuresGroupMap.contains(oldGroup)) {
        oi::removeOne(this->featureGroups, oldGroup);
      }
    }
  }

  return true;
}

/*!
 * \brief FeatureContainer::geometryMeasurementConfigChanged
 * \param featureId
 * \param oldMConfig
 * \param oldIsSaved
 * \return
 */
bool FeatureContainer::geometryMeasurementConfigChanged(
    const int& featureId, const std::string& oldMConfig,
    const MeasurementConfigKey oldKey) {
  // check if the feature exists
  if (!this->featuresIdMap.contains(featureId)) {
    return false;
  }

  // get the feature pointer
  oi::Guarded<FeatureWrapper> feature =
      oi::value(this->featuresIdMap, featureId, oi::Guarded<FeatureWrapper>());

  // check if the feature is valid and is a geometry
  if (feature.isNull() || feature->getGeometry().isNull()) {
    return false;
  }
  oi::Guarded<Geometry> geometry = feature->getGeometry();

  // if both, old and new mConfig, are empty nothing should happen
  if (oldMConfig.compare("") == 0 &&
      !geometry->getMeasurementConfig().isValid()) {
    return true;
  }

  // get map-keys for old and new mConfig
  MeasurementConfigKey newKey = feature->getGeometry()->getMeasurementConfig();

  // if the old mConfig was empty
  if (oldMConfig.compare("") == 0) {
    this->geometriesMConfigMap.emplace(newKey, geometry);
    if (!oi::contains(this->usedMConfigs, newKey)) {
      this->usedMConfigs.push_back(newKey);
    }
    return true;
  }

  // if the new mConfig is empty
  if (!geometry->getMeasurementConfig().isValid()) {
    oi::eraseValue(this->geometriesMConfigMap, oldKey, geometry);
    if (!this->geometriesMConfigMap.contains(oldKey)) {
      oi::removeOne(this->usedMConfigs, oldKey);
    }
    return true;
  }

  // if both mConfigs are non-empty
  oi::eraseValue(this->geometriesMConfigMap, oldKey, geometry);
  this->geometriesMConfigMap.emplace(newKey, geometry);
  if (!this->geometriesMConfigMap.contains(oldKey)) {
    oi::removeOne(this->usedMConfigs, oldKey);
  }
  if (!oi::contains(this->usedMConfigs, newKey)) {
    this->usedMConfigs.push_back(newKey);
  }

  return true;
}
