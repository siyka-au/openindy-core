#include "sensor.h"

#include <map>
#include <vector>

#include "containers.h"
#include "measurementconfig.h"
#include "reading.h"
#include "sensorcontrol.h"

using namespace oi;

SensorInterface::SensorInterface() {}

void SensorInterface::setSensorSink(SensorSink* sink) {
  const std::scoped_lock lock(this->sinkMutex);
  this->sink = sink;
}

SensorSink* SensorInterface::getSensorSink() const {
  const std::scoped_lock lock(this->sinkMutex);
  return this->sink;
}

void SensorInterface::sensorStatus(const SensorStatus& status,
                                   const QString& msg) {
  const std::scoped_lock lock(this->sinkMutex);
  if (this->sink != nullptr) {
    this->sink->onSensorStatus(status, msg);
  }
}

void SensorInterface::sensorMessage(const QString& msg,
                                    const MessageTypes& msgType,
                                    const MessageDestinations& msgDest) {
  const std::scoped_lock lock(this->sinkMutex);
  if (this->sink != nullptr) {
    this->sink->onSensorMessage(msg, msgType, msgDest);
  }
}

void SensorInterface::asyncSensorResponse(const QJsonObject& response,
                                          const SensorAction sensorAction) {
  const std::scoped_lock lock(this->sinkMutex);
  if (this->sink != nullptr) {
    this->sink->onAsyncSensorResponse(response, sensorAction);
  }
}

void SensorInterface::asyncMeasurementResult(
    const int& geomId, const std::vector<oi::Guarded<Reading>>& measurements,
    const SensorAction sensorAction) {
  const std::scoped_lock lock(this->sinkMutex);
  if (this->sink != nullptr) {
    this->sink->onAsyncMeasurementResult(geomId, measurements, sensorAction);
  }
}

void SensorInterface::asyncStreamResult(
    const std::map<QString, QVariant>& reading) {
  const std::scoped_lock lock(this->sinkMutex);
  if (this->sink != nullptr) {
    this->sink->onAsyncStreamResult(reading);
  }
}

void SensorInterface::asyncSensorNotification(const QJsonObject& response) {
  const std::scoped_lock lock(this->sinkMutex);
  if (this->sink != nullptr) {
    this->sink->onAsyncSensorNotification(response);
  }
}

/*!
 * \brief Sensor::Sensor
 */
Sensor::Sensor() : SensorInterface() {}

/*!
 * \brief Sensor::Sensor
 * \param copy
 */
Sensor::Sensor(const Sensor& copy)
    : SensorInterface(),
      acquisitionResultShape(copy.getAcquisitionResultShape()),
      defaultAccuracy(copy.getDefaultAccuracy()),
      doubleParameters(copy.getDoubleParameter()),
      integerParameters(copy.getIntegerParameter()),
      metaData(copy.getMetaData()),
      selfDefinedActions(copy.getSelfDefinedActions()),
      sensorConfiguration(copy.getSensorConfiguration()),
      stringParameters(copy.getStringParameter()),
      supportedConnectionTypes(copy.getSupportedConnectionTypes()),
      supportedReadingTypes(copy.getSupportedReadingTypes()),
      supportedSensorActions(copy.getSupportedSensorActions()) {
  // sensor configuration

  // meta information about the sensor

  // the needed scalar input parameters including default values
}

/*!
 * \brief Sensor::operator =
 * \param copy
 * \return
 */
Sensor& Sensor::operator=(const Sensor& copy) {
  // sensor configuration
  this->sensorConfiguration = copy.getSensorConfiguration();

  // meta information about the sensor
  this->supportedReadingTypes = copy.getSupportedReadingTypes();
  this->supportedSensorActions = copy.getSupportedSensorActions();
  this->supportedConnectionTypes = copy.getSupportedConnectionTypes();
  this->selfDefinedActions = copy.getSelfDefinedActions();
  this->defaultAccuracy = copy.getDefaultAccuracy();
  this->metaData = copy.getMetaData();
  this->acquisitionResultShape = copy.getAcquisitionResultShape();

  // the needed scalar input parameters including default values
  this->integerParameters = copy.getIntegerParameter();
  this->doubleParameters = copy.getDoubleParameter();
  this->stringParameters = copy.getStringParameter();

  return *this;
}

/*!
 * \brief Sensor::~Sensor
 */
Sensor::~Sensor() {}

/*!
 * \brief Sensor::getSensorConfiguration
 * \return
 */
const SensorConfiguration& Sensor::getSensorConfiguration() const {
  return this->sensorConfiguration;
}

/*!
 * \brief Sensor::setSensorConfiguration
 * \param sConfig
 */
void Sensor::setSensorConfiguration(const SensorConfiguration& sConfig) {
  this->sensorConfiguration = sConfig;
}

/*!
 * \brief Sensor::setMeasurementConfig
 * \param mConfig
 */
void Sensor::setMeasurementConfig(const MeasurementConfig& mConfig) {
  this->actualMeasurementConfig = mConfig;
}

/*!
 * \brief Sensor::getLastReading
 * \return
 */
const std::pair<ReadingTypes, oi::Guarded<Reading>>& Sensor::getLastReading()
    const {
  return this->lastReading;
}

/*!
 * \brief Sensor::getSupportedReadingTypes
 * \return
 */
const std::vector<ReadingTypes>& Sensor::getSupportedReadingTypes() const {
  return this->supportedReadingTypes;
}

/*!
 * \brief Sensor::isSensorAsync
 * \return
 */
bool Sensor::isSensorAsync() const { return false; }

/*!
 * \brief Sensor::getSupportedSensorActions
 * \return
 */
const std::vector<SensorFunctions>& Sensor::getSupportedSensorActions() const {
  return this->supportedSensorActions;
}

/*!
 * \brief Sensor::getSupportedConnectionTypes
 * \return
 */
const std::vector<ConnectionKind>& Sensor::getSupportedConnectionTypes() const {
  return this->supportedConnectionTypes;
}

/*!
 * \brief Sensor::getSupportedMeasurementConfigs
 * \return
 */
const std::vector<MeasurementConfig>& Sensor::getSupportedMeasurementConfigs()
    const {
  return this->supportedMeasurementConfigs;
}

/*!
 * \brief Sensor::getSupportedAcquisitionModes
 * \return
 */
const std::vector<AcquisitionMode>& Sensor::getSupportedAcquisitionModes()
    const {
  return this->supportedAcquisitionModes;
}

/*!
 * \brief Sensor::getAcquisitionResultShape
 * \return
 */
AcquisitionResultShape Sensor::getAcquisitionResultShape() const {
  return this->acquisitionResultShape;
}

/*!
 * \brief Sensor::getMetaData
 * \return
 */
const PluginMetaData& Sensor::getMetaData() const { return this->metaData; }

/*!
 * \brief Sensor::getIntegerParameter
 * \return
 */
const std::map<QString, int>& Sensor::getIntegerParameter() const {
  return this->integerParameters;
}

/*!
 * \brief Sensor::getDoubleParameter
 * \return
 */
const std::map<QString, double>& Sensor::getDoubleParameter() const {
  return this->doubleParameters;
}

/*!
 * \brief Sensor::getStringParameter
 * \return
 */
const std::multimap<QString, QString>& Sensor::getStringParameter() const {
  return this->stringParameters;
}

/*!
 * \brief Sensor::getSelfDefinedActions
 * \return
 */
const std::vector<QString>& Sensor::getSelfDefinedActions() const {
  return this->selfDefinedActions;
}

/*!
 * \brief Sensor::getDefaultAccuracy
 * \return
 */
const Accuracy& Sensor::getDefaultAccuracy() const {
  return this->defaultAccuracy;
}

/*!
 * \brief Sensor::accept
 * Calls the appropriate internal sensor method
 * \param method
 * \param sAttr
 * \return
 */
bool Sensor::accept(const SensorFunctions& /*method*/,
                    const SensorAttributes& /*sAttr*/) {
  return false;
}

/*!
 * \brief Sensor::performAsyncSensorCommand
 * Calls the appropriate internal sensor method asynchronously
 * \param request
 * \return
 */
QJsonObject Sensor::performAsyncSensorCommand(const QJsonObject& request) {
  QJsonObject response;
  QJsonObject error;
  error.insert("code", -32000);
  error.insert("message",
               "this sensor does not implement the interface methode for "
               "asynchronous commands");
  response.insert("jsonrpc", "2.0");
  response.insert("error", error);
  response.insert("id", request.value("id"));
  return response;
}

/*!
 * \brief Sensor::abortAction
 * \return
 */
bool Sensor::abortAction() { return false; }

/*!
 * \brief Sensor::connectSensor
 * \return
 */
bool Sensor::connectSensor() {
  this->sensorMessage(
      QString("Sensor not implemented correctly: connect method missing"),
      MessageTypes::eCriticalMessage, MessageDestinations::eMessageBoxMessage);
  return false;
}

/*!
 * \brief Sensor::disconnectSensor
 * \return
 */
bool Sensor::disconnectSensor() {
  this->sensorMessage(
      QString("Sensor not implemented correctly: disconnect method missing"),
      MessageTypes::eCriticalMessage, MessageDestinations::eMessageBoxMessage);
  return false;
}

/*!
 * \brief Sensor::measure
 * \param mConfig
 * \return
 */
std::vector<oi::Guarded<Reading>> Sensor::measure(
    const MeasurementConfig& /*mConfig*/) {
  this->sensorMessage(
      QString("Sensor not implemented correctly: measure method missing"),
      MessageTypes::eCriticalMessage, MessageDestinations::eMessageBoxMessage);
  return std::vector<oi::Guarded<Reading>>();
}

/*!
 * \brief Sensor::readingStream
 * \param streamFormat
 * \return
 */
std::map<QString, QVariant> Sensor::readingStream(
    const ReadingTypes& /*streamFormat*/) {
  this->sensorMessage(
      QString(
          "Sensor not implemented correctly: reading stream method missing"),
      MessageTypes::eCriticalMessage, MessageDestinations::eMessageBoxMessage);
  return std::map<QString, QVariant>();
}

/*!
 * \brief Sensor::getConnectionState
 * \return
 */
bool Sensor::getConnectionState() {
  this->sensorMessage(
      QString(
          "Sensor not implemented correctly: connection state method missing"),
      MessageTypes::eCriticalMessage, MessageDestinations::eMessageBoxMessage);
  return false;
}

/*!
 * \brief Sensor::getIsReadyForMeasurement
 * \return
 */
bool Sensor::getIsReadyForMeasurement() {
  this->sensorMessage(
      QString("Sensor not implemented correctly: is ready method missing"),
      MessageTypes::eCriticalMessage, MessageDestinations::eMessageBoxMessage);
  return false;
}

/*!
 * \brief Sensor::getIsBusy
 * \return
 */
bool Sensor::getIsBusy() {
  this->sensorMessage(
      QString("Sensor not implemented correctly: is busy method missing"),
      MessageTypes::eCriticalMessage, MessageDestinations::eMessageBoxMessage);
  return false;
}

/*!
 * \brief Sensor::getSensorStatus
 * \return
 */
std::map<QString, QString> Sensor::getSensorStatus() {
  this->sensorMessage(
      QString("Sensor not implemented correctly: sensor status method missing"),
      MessageTypes::eCriticalMessage, MessageDestinations::eMessageBoxMessage);
  return std::map<QString, QString>();
}

/*!
 * \brief Sensor::doSelfDefinedAction
 * \param action
 * \return
 */
bool Sensor::doSelfDefinedAction(const QString& /*action*/) { return false; }

bool Sensor::search() { return false; }

/*!
 * \brief Sensor::init
 */
void Sensor::init() {}

ReadingTypes Sensor::getReadingType(const MeasurementConfig& mConfig) const {
  switch (mConfig.getMeasurementType()) {
    case MeasurementTypes::eSinglePoint_MeasurementType:
    case MeasurementTypes::eScanDistanceDependent_MeasurementType:
    case MeasurementTypes::eScanTimeDependent_MeasurementType:
      return getReadingTypeEnum(
          oi::value(this->sensorConfiguration.getStringParameter(),
                    "reading type", "cartesian"));
    case MeasurementTypes::eDistance_MeasurementType:
      return ReadingTypes::eDistanceReading;
    case MeasurementTypes::eDirection_MeasurementType:
      return ReadingTypes::eDirectionReading;
    case MeasurementTypes::eLevel_MeasurementType:
      return ReadingTypes::eLevelReading;
    case MeasurementTypes::eTemperature_MeasurementType:
      return ReadingTypes::eTemperatureReading;
    default:
      return ReadingTypes::eUndefinedReading;
  }
}
