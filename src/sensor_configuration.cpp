#include "sensor_configuration.h"

#include <map>

#include "sensor.h"

using namespace oi;

/*!
 * \brief SensorConfiguration::SensorConfiguration
 */
SensorConfiguration::SensorConfiguration() {}

/*!
 * \brief SensorConfiguration::SensorConfiguration
 * \param copy
 */
SensorConfiguration::SensorConfiguration(const SensorConfiguration& copy)
    : name(copy.name),
      accuracy(copy.accuracy),
      availableStringParameter(copy.availableStringParameter),
      cConfig(copy.cConfig),
      doubleParameter(copy.doubleParameter),
      integerParameter(copy.integerParameter),
      isSaved(copy.isSaved),
      pluginName(copy.pluginName),
      sensorName(copy.sensorName),
      stringParameter(copy.stringParameter),
      typeOfSensor(copy.typeOfSensor) {
  // copy sensor config attributes
}

/*!
 * \brief SensorConfiguration::operator =
 * \param copy
 * \return
 */
SensorConfiguration& SensorConfiguration::operator=(
    const SensorConfiguration& copy) {
  // copy sensor config attributes
  this->name = copy.name;
  this->isSaved = copy.isSaved;
  this->typeOfSensor = copy.typeOfSensor;
  this->pluginName = copy.pluginName;
  this->sensorName = copy.sensorName;
  this->accuracy = copy.accuracy;
  this->cConfig = copy.cConfig;
  this->integerParameter = copy.integerParameter;
  this->doubleParameter = copy.doubleParameter;
  this->stringParameter = copy.stringParameter;
  this->availableStringParameter = copy.availableStringParameter;

  return *this;
}

/*!
 * \brief SensorConfiguration::getName
 * \return
 */
const std::string& SensorConfiguration::getName() const { return this->name; }

/*!
 * \brief SensorConfiguration::setName
 * \param name
 */
void SensorConfiguration::setName(const std::string& name) {
  if (this->name.compare(name) != 0) {
    this->name = name;
  }
}

/*!
 * \brief SensorConfiguration::getIsSaved
 * \return
 */
const bool& SensorConfiguration::getIsSaved() const { return this->isSaved; }

/*!
 * \brief SensorConfiguration::setIsSaved
 * \param isSaved
 */
void SensorConfiguration::setIsSaved(const bool& isSaved) {
  this->isSaved = isSaved;
}

/*!
 * \brief SensorConfiguration::getIsValid
 * \return
 */
bool SensorConfiguration::getIsValid() const {
  if (this->name.compare("") != 0) {
    return true;
  }
  return false;
}

/*!
 * \brief SensorConfiguration::getTypeOfSensor
 * \return
 */
const SensorType& SensorConfiguration::getTypeOfSensor() const {
  return this->typeOfSensor;
}

/*!
 * \brief SensorConfiguration::setTypeOfSensor
 * \param type
 */
void SensorConfiguration::setTypeOfSensor(const SensorType& type) {
  this->typeOfSensor = type;
}

/*!
 * \brief SensorConfiguration::getPluginName
 * \return
 */
const std::string& SensorConfiguration::getPluginName() const {
  return this->pluginName;
}

/*!
 * \brief SensorConfiguration::setPluginName
 * \param name
 */
void SensorConfiguration::setPluginName(const std::string& name) {
  this->pluginName = name;
}

/*!
 * \brief SensorConfiguration::getSensorName
 * \return
 */
const std::string& SensorConfiguration::getSensorName() const {
  return this->sensorName;
}

/*!
 * \brief SensorConfiguration::setSensorName
 * \param name
 */
void SensorConfiguration::setSensorName(const std::string& name) {
  this->sensorName = name;
}

/*!
 * \brief SensorConfiguration::getAccuracy
 * \return
 */
const Accuracy& SensorConfiguration::getAccuracy() const {
  return this->accuracy;
}

/*!
 * \brief SensorConfiguration::setAccuracy
 * \param accuracy
 */
void SensorConfiguration::setAccuracy(const Accuracy& accuracy) {
  this->accuracy = accuracy;
}

/*!
 * \brief SensorConfiguration::getConnectionConfig
 * \return
 */
const ConnectionConfig& SensorConfiguration::getConnectionConfig() const {
  return this->cConfig;
}

/*!
 * \brief SensorConfiguration::setConnectionConfig
 * \param cConfig
 */
void SensorConfiguration::setConnectionConfig(const ConnectionConfig& cConfig) {
  this->cConfig = cConfig;
}

/*!
 * \brief SensorConfiguration::getIntegerParameter
 * \return
 */
const std::map<std::string, int>& SensorConfiguration::getIntegerParameter()
    const {
  return this->integerParameter;
}

/*!
 * \brief SensorConfiguration::setIntegerParameter
 * \param intParams
 */
void SensorConfiguration::setIntegerParameter(
    const std::map<std::string, int>& intParams) {
  this->integerParameter = intParams;
}

/*!
 * \brief SensorConfiguration::getDoubleParameter
 * \return
 */
const std::map<std::string, double>& SensorConfiguration::getDoubleParameter()
    const {
  return this->doubleParameter;
}

/*!
 * \brief SensorConfiguration::setDoubleParameter
 * \param doubleParams
 */
void SensorConfiguration::setDoubleParameter(
    const std::map<std::string, double>& doubleParams) {
  this->doubleParameter = doubleParams;
}

/*!
 * \brief SensorConfiguration::getStringParameter
 * \return
 */
const std::map<std::string, std::string>&
SensorConfiguration::getStringParameter() const {
  return this->stringParameter;
}

/*!
 * \brief SensorConfiguration::setStringParameter
 * \param stringParams
 */
void SensorConfiguration::setStringParameter(
    const std::map<std::string, std::string>& stringParams) {
  this->stringParameter = stringParams;
}

/*!
 * \brief SensorConfiguration::getAvailableStringParameter
 * \return
 */
const std::multimap<std::string, std::string>&
SensorConfiguration::getAvailableStringParameter() const {
  return this->availableStringParameter;
}

/*!
 * \brief SensorConfiguration::setAvailableStringParameter
 * \param stringParams
 */
void SensorConfiguration::setAvailableStringParameter(
    const std::multimap<std::string, std::string>& stringParams) {
  this->availableStringParameter = stringParams;
}
