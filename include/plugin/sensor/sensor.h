#ifndef SENSOR_H
#define SENSOR_H
#include <QJsonObject>
#include <QString>
#include <QVariant>
#include <map>
#include <mutex>
#include <utility>
#include <vector>

#include "core_export.h"
#include "guarded.h"
#include "measurementconfig.h"
#include "pluginmetadata.h"
#include "reading.h"
#include "sensorconfiguration.h"
#include "sensorsink.h"
#include "types.h"
#include "util.h"

namespace oi {

/*!
 * \brief The SensorAttributes class
 */
class OI_CORE_EXPORT SensorAttributes {
 public:
  double moveAzimuth{};
  double moveZenith{};
  double moveDistance{};
  bool moveIsRelative{};
  double moveX{};
  double moveY{};
  double moveZ{};
  QString action;
};

class OI_CORE_EXPORT SensorInterface : public oi::Trackable {
 public:
  explicit SensorInterface();
  // ############################
  // sensor initialization method
  // ############################

  virtual void init() = 0;

  // ####################################
  // get or set general sensor attributes
  // ####################################

  [[nodiscard]] virtual const SensorConfiguration& getSensorConfiguration()
      const = 0;
  virtual void setSensorConfiguration(const SensorConfiguration& sConfig) = 0;

  virtual void setMeasurementConfig(const MeasurementConfig& mConfig) = 0;

  [[nodiscard]] virtual const std::pair<ReadingTypes, oi::Guarded<Reading>>&
  getLastReading() const = 0;

  [[nodiscard]] virtual bool isSensorAsync() const = 0;

  // #########################################################
  // methods to get or set further information to use a sensor
  // #########################################################

  // reading types, sensor actions and connection types
  [[nodiscard]] virtual const std::vector<ReadingTypes>&
  getSupportedReadingTypes() const = 0;
  [[nodiscard]] virtual const std::vector<SensorFunctions>&
  getSupportedSensorActions() const = 0;
  [[nodiscard]] virtual const std::vector<ConnectionKind>&
  getSupportedConnectionTypes() const = 0;

  // the measurement configurations this sensor publishes, and how it can
  // acquire - both are declared by the driver rather than inferred from its
  // type
  [[nodiscard]] virtual const std::vector<MeasurementConfig>&
  getSupportedMeasurementConfigs() const = 0;
  [[nodiscard]] virtual const std::vector<AcquisitionMode>&
  getSupportedAcquisitionModes() const = 0;

  // how many Points one acquisition action yields (D2/D15a) - a capability,
  // never inferred from sensor type. Every current driver is eOnePoint;
  // declared here so eManyPoints/eCloud are expressible without an ABI
  // change when a V-STARS/scanner driver needs them.
  [[nodiscard]] virtual AcquisitionResultShape getAcquisitionResultShape()
      const = 0;

  // meta data
  [[nodiscard]] virtual const PluginMetaData& getMetaData() const = 0;

  // integer, double and string parameter
  [[nodiscard]] virtual const std::map<QString, int>& getIntegerParameter()
      const = 0;
  [[nodiscard]] virtual const std::map<QString, double>& getDoubleParameter()
      const = 0;
  [[nodiscard]] virtual const std::multimap<QString, QString>&
  getStringParameter() const = 0;

  // self deined actions
  [[nodiscard]] virtual const std::vector<QString>& getSelfDefinedActions()
      const = 0;

  // accuracy
  [[nodiscard]] virtual const Accuracy& getDefaultAccuracy() const = 0;

  // ########################
  // sensor state and actions
  // ########################

  // sensor actions
  virtual bool accept(const SensorFunctions& method,
                      const SensorAttributes& sAttr) = 0;

  virtual QJsonObject performAsyncSensorCommand(const QJsonObject& request) = 0;

  // abort actions
  virtual bool abortAction() = 0;

  // connect or disconnect
  virtual bool connectSensor() = 0;
  virtual bool disconnectSensor() = 0;

  // measurements
  virtual std::vector<oi::Guarded<Reading>> measure(
      const MeasurementConfig& mConfig) = 0;
  virtual std::map<QString, QVariant> readingStream(
      const ReadingTypes& streamFormat) = 0;

  // status information
  virtual bool getConnectionState() = 0;
  virtual bool getIsReadyForMeasurement() = 0;
  virtual bool getIsBusy() = 0;
  virtual std::map<QString, QString> getSensorStatus() = 0;

  // self defined actions
  virtual bool doSelfDefinedAction(const QString& action) = 0;

  virtual bool search() = 0;

  [[nodiscard]] virtual ReadingTypes getReadingType(
      const MeasurementConfig& mConfig) const = 0;

  // ##############################################################
  // reporting to OpenIndy: replaces the former Qt signals of this
  // class. The names, parameters and default arguments are unchanged,
  // so a driver keeps writing `this->sensorMessage(...)`; each
  // call is forwarded to the sink, or dropped if there is none. They
  // may be called from any thread the driver owns.
  // ##############################################################

  // the sink the reports go to (nullptr: drop them). Setting it waits for a
  // report that is being delivered on another thread to finish, so the old
  // sink may be destroyed right after setSensorSink returns.
  void setSensorSink(SensorSink* sink);
  [[nodiscard]] SensorSink* getSensorSink() const;

  void sensorStatus(const SensorStatus& status, const QString& msg);
  void sensorMessage(const QString& msg, const MessageTypes& msgType,
                     const MessageDestinations& msgDest =
                         MessageDestinations::eConsoleMessage);
  void asyncSensorResponse(
      const QJsonObject& response,
      const SensorAction sensorAction = SensorAction::eSensorActionUndefind);
  void asyncMeasurementResult(
      const int& geomId, const std::vector<oi::Guarded<Reading>>& measurements,
      const SensorAction sensorAction = SensorAction::eSensorActionUndefind);
  void asyncStreamResult(const std::map<QString, QVariant>& reading);
  void asyncSensorNotification(const QJsonObject& response);

 private:
  // recursive: a sink may call back into the sensor (or swap itself) from
  // within a report that is being delivered on the same thread
  mutable std::recursive_mutex sinkMutex;
  SensorSink* sink{nullptr};
};

/*!
 * \brief The Sensor class
 */
class OI_CORE_EXPORT Sensor : public SensorInterface {
 public:
  explicit Sensor();

  Sensor(const Sensor& copy);

  Sensor& operator=(const Sensor& copy);

  ~Sensor() override;

  enum class SensorMessage { eSensorOk, eSensorWarning, eSensorError };

  // ############################
  // sensor initialization method
  // ############################

  void init() override;

  // ####################################
  // get or set general sensor attributes
  // ####################################

  [[nodiscard]] const SensorConfiguration& getSensorConfiguration()
      const override;
  void setSensorConfiguration(const SensorConfiguration& sConfig) override;

  void setMeasurementConfig(const MeasurementConfig& mConfig) override;

  [[nodiscard]] const std::pair<ReadingTypes, oi::Guarded<Reading>>&
  getLastReading() const override;

  [[nodiscard]] bool isSensorAsync() const override;

  // #########################################################
  // methods to get or set further information to use a sensor
  // #########################################################

  // reading types, sensor actions and connection types
  [[nodiscard]] const std::vector<ReadingTypes>& getSupportedReadingTypes()
      const override;
  [[nodiscard]] const std::vector<SensorFunctions>& getSupportedSensorActions()
      const override;
  [[nodiscard]] const std::vector<ConnectionKind>& getSupportedConnectionTypes()
      const override;

  [[nodiscard]] const std::vector<MeasurementConfig>&
  getSupportedMeasurementConfigs() const override;
  [[nodiscard]] const std::vector<AcquisitionMode>&
  getSupportedAcquisitionModes() const override;
  [[nodiscard]] AcquisitionResultShape getAcquisitionResultShape()
      const override;

  // meta data
  [[nodiscard]] const PluginMetaData& getMetaData() const override;

  // integer, double and string parameter
  [[nodiscard]] const std::map<QString, int>& getIntegerParameter()
      const override;
  [[nodiscard]] const std::map<QString, double>& getDoubleParameter()
      const override;
  [[nodiscard]] const std::multimap<QString, QString>& getStringParameter()
      const override;

  // self deined actions
  [[nodiscard]] const std::vector<QString>& getSelfDefinedActions()
      const override;

  // accuracy
  [[nodiscard]] const Accuracy& getDefaultAccuracy() const override;

  // ########################
  // sensor state and actions
  // ########################

  // sensor actions
  bool accept(const SensorFunctions& method,
              const SensorAttributes& sAttr) override;

  QJsonObject performAsyncSensorCommand(const QJsonObject& request) override;

  // abort actions
  bool abortAction() override;

  // connect or disconnect
  bool connectSensor() override;
  bool disconnectSensor() override;

  // measurements
  std::vector<oi::Guarded<Reading>> measure(
      const MeasurementConfig& mConfig) override;
  std::map<QString, QVariant> readingStream(
      const ReadingTypes& streamFormat) override;

  // status information
  bool getConnectionState() override;
  bool getIsReadyForMeasurement() override;
  bool getIsBusy() override;
  std::map<QString, QString> getSensorStatus() override;

  // self defined actions
  bool doSelfDefinedAction(const QString& action) override;

  bool search() override;

  [[nodiscard]] ReadingTypes getReadingType(
      const MeasurementConfig& mConfig) const override;

 protected:
  // #########################
  // general sensor attributes
  // #########################

  SensorConfiguration sensorConfiguration;
  MeasurementConfig actualMeasurementConfig;

  std::pair<ReadingTypes, oi::Guarded<Reading>>
      lastReading;  // the last reading produced by this sensor

  // ###########################
  // input and output parameters
  // ###########################

  // meta information about the sensor
  std::vector<ReadingTypes> supportedReadingTypes;
  std::vector<SensorFunctions> supportedSensorActions;
  std::vector<ConnectionKind> supportedConnectionTypes;
  std::vector<MeasurementConfig> supportedMeasurementConfigs;
  std::vector<AcquisitionMode> supportedAcquisitionModes;

  // D2/D15a: every driver in this repo produces one Point per acquisition
  // today, so this is the default; a future V-STARS/scanner driver
  // overrides it in its own constructor rather than this needing a
  // per-subclass virtual override.
  AcquisitionResultShape acquisitionResultShape =
      AcquisitionResultShape::eOnePoint;
  std::vector<QString> selfDefinedActions;
  Accuracy defaultAccuracy;
  PluginMetaData metaData;

  // the needed scalar input parameters including default values
  std::map<QString, int> integerParameters;
  std::map<QString, double> doubleParameters;
  std::multimap<QString, QString> stringParameters;
};

}  // namespace oi

#ifndef STR
#define STR(x) #x
#endif
#ifndef STR2
#define STR2(x) STR(x)
#endif

#ifndef CONCAT
#define CONCAT(a, b) a##b
#endif
#ifndef CONCAT2
#define CONCAT2(a, b) CONCAT(a, b)
#endif

#include "pluginabi.h"

#define SENSOR_PREFIX de.openIndy.plugin.sensor.v
#define Sensor_iidd STR2(CONCAT2(SENSOR_PREFIX, OI_PLUGIN_ABI_VERSION))

#endif  // SENSOR_H
