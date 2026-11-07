#include "measurementconfig.h"

#include <vector>

#include "containers.h"
#include "value.h"

using namespace oi;

/*!
 * \brief MeasurementConfig::MeasurementConfig
 */
MeasurementConfig::MeasurementConfig()
    : distanceInterval(0.0),
      maxObservations(500),
      measureTwoSides(false),
      measurementMode(MeasurementModes::eFast_MeasurementMode),
      measurementType(MeasurementTypes::eSinglePoint_MeasurementType),
      timeInterval(0.0) {
  // set defaults
}

/*!
 * \brief MeasurementConfig::MeasurementConfig
 * \param copy
 */
MeasurementConfig::MeasurementConfig(const MeasurementConfig& copy)
    : name(copy.name),
      configType(copy.configType),
      distanceInterval(copy.distanceInterval),
      editable(copy.editable),
      maxObservations(copy.maxObservations),
      measureTwoSides(copy.measureTwoSides),
      measurementMode(copy.measurementMode),
      measurementType(copy.measurementType),
      sensorType(copy.sensorType),
      timeInterval(copy.timeInterval),
      transientData(copy.transientData) {
  // copy measurement config attributes
}

/*!
 * \brief MeasurementConfig::operator =
 * \param copy
 * \return
 */
MeasurementConfig& MeasurementConfig::operator=(const MeasurementConfig& copy) {
  // copy measurement config attributes
  this->name = copy.name;
  this->measurementType = copy.measurementType;
  this->measurementMode = copy.measurementMode;
  this->measureTwoSides = copy.measureTwoSides;
  this->maxObservations = copy.maxObservations;
  this->timeInterval = copy.timeInterval;
  this->distanceInterval = copy.distanceInterval;

  this->configType = copy.configType;
  this->editable = copy.editable;
  this->sensorType = copy.sensorType;

  this->transientData = copy.transientData;

  return *this;
}

/*!
 * \brief MeasurementConfig::getName
 * \return
 */
const std::string& MeasurementConfig::getName() const { return this->name; }

/*!
 * \brief MeasurementConfig::setName
 * \param name
 */
void MeasurementConfig::setName(const std::string& name) { this->name = name; }

/* key for maps */
MeasurementConfigKey MeasurementConfig::getKey() const {
  return MeasurementConfigKey(this->name, this->configType, this->sensorType);
}

bool MeasurementConfig::isUserConfig() const {
  return ConfigTypes::eUserConfig == this->configType;
}
bool MeasurementConfig::isProjectConfig() const {
  return ConfigTypes::eProjectConfig == this->configType ||
         ConfigTypes::eStandardConfig == this->configType;
}
bool MeasurementConfig::isStandardConfig() const {
  return ConfigTypes::eStandardConfig == this->configType;
}
bool MeasurementConfig::isEditable() const { return this->editable; }
bool MeasurementConfig::isValid() const {
  return !this->name.empty() && this->configType != ConfigTypes::eUndefinded;
}

/*!
 * \brief MeasurementConfig::getMeasureTwoSides
 * \return
 */
const bool& MeasurementConfig::getMeasureTwoSides() const {
  return this->measureTwoSides;
}

/*!
 * \brief MeasurementConfig::setMeasureTwoSides
 * \param twoSides
 */
void MeasurementConfig::setMeasureTwoSides(const bool& measureTwoSides) {
  this->measureTwoSides = measureTwoSides;
}

/*!
 * \brief MeasurementConfig::getTimeInterval
 * \return
 */
const double& MeasurementConfig::getTimeInterval() const {
  return this->timeInterval;
}

/*!
 * \brief MeasurementConfig::setTimeInterval
 * \param interval
 */
void MeasurementConfig::setTimeInterval(const double& interval) {
  this->timeInterval = interval;
}

/*!
 * \brief MeasurementConfig::getDistanceInterval
 * \return interval [mm]
 */
const double& MeasurementConfig::getDistanceInterval() const {
  return this->distanceInterval;
}

/*!
 * \brief MeasurementConfig::setDistanceInterval
 * \param interval [mm]
 */
void MeasurementConfig::setDistanceInterval(const double& interval) {
  this->distanceInterval = interval;
}

/*!
 * \brief MeasurementConfig::getSensorType
 * \return
 */
const SensorType& MeasurementConfig::getSensorType() const {
  return this->sensorType;
}

/*!
 * \brief MeasurementConfig::setSensorType
 * \param type
 */
void MeasurementConfig::setSensorType(const SensorType& type) {
  this->sensorType = type;
}

const oi::Value MeasurementConfig::getTransientData(
    const std::string key) const {
  return oi::value(this->transientData, key);
}
void MeasurementConfig::setTransientData(const std::string key,
                                         const oi::Value value) {
  this->transientData.insert_or_assign(key, value);
}

/**
 * @brief MeasurementConfig::applicableFor
 * @param elementType
 * @param typeOfFeature
 * @return true if elementType and typeOfFeature can handled by this
 * MeasurementConfig
 */
bool MeasurementConfig::applicableFor(
    const ElementTypes elementType,
    std::vector<FeatureTypes> functionIsApplicableFor) {
  if (ElementTypes::eUndefinedElement ==
      elementType) {  // used to filter all configs:
                      // CreateFeatureDialog::initMeasurementConfigUI
    return false;
  }

  // handle level ->
  if (oi::contains(functionIsApplicableFor, FeatureTypes::eLevelFeature)) {
    return this->measurementType == MeasurementTypes::eLevel_MeasurementType;
  }
  if (this->measurementType == MeasurementTypes::eLevel_MeasurementType) {
    return false;
  }
  // <- handle level

  return true;
}

void MeasurementConfig::setMeasurementMode(const MeasurementModes mode) {
  this->measurementMode = mode;
}

MeasurementModes MeasurementConfig::getMeasurementMode() const {
  return this->measurementMode;
}

void MeasurementConfig::setMeasurementType(const MeasurementTypes type) {
  this->measurementType = type;
}

MeasurementTypes MeasurementConfig::getMeasurementType() const {
  return this->measurementType;
}

const int& MeasurementConfig::getMaxObservations() const {
  return this->maxObservations;
}
void MeasurementConfig::setMaxObservations(const int& maxObservations) {
  this->maxObservations = maxObservations;
}

void MeasurementConfig::makeUserConfig() {
  this->editable = true;
  this->configType = ConfigTypes::eUserConfig;
}
void MeasurementConfig::makeProjectConfig() {
  this->editable = true;
  this->configType = ConfigTypes::eProjectConfig;
}
void MeasurementConfig::makeStandardConfig() {
  this->editable = false;
  this->configType = ConfigTypes::eStandardConfig;
}
