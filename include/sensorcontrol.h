#ifndef SENSORCONTROL_H
#define SENSORCONTROL_H

#include <QList>
#include <QMap>
#include <QPointer>
#include <memory>

#include "core_export.h"
#include "executor.h"
#include "measurementconfig.h"
#include "sensor.h"
#include "sensorworker.h"
#include "stationobserver.h"

namespace oi {

/*!
 * \brief The SensorControl class
 * A sensor control object belongs to a station and executes all sensor actions
 * on a separate thread (its SensorWorker). The sensor actions are plain method
 * calls that enqueue a command and return at once; what the sensor reports
 * back is handed to the StationObserver through the Executor, in the order the
 * worker produced it. Without an executor results are delivered inline, on the
 * worker thread.
 *
 * Destroying the control closes the delivery first - no callback reaches the
 * observer afterwards, not even one the executor still has queued - and then
 * stops the worker (see SensorWorker for what happens to queued commands).
 * The executor is only used while the control exists; it has to outlive the
 * worker thread's last post, i.e. the control.
 */
class OI_CORE_EXPORT SensorControl final {
 public:
  explicit SensorControl(StationObserver* observer = nullptr,
                         Executor* executor = nullptr);

  ~SensorControl();

  SensorControl(const SensorControl&) = delete;
  SensorControl& operator=(const SensorControl&) = delete;
  SensorControl(SensorControl&&) = delete;
  SensorControl& operator=(SensorControl&&) = delete;

  // who the results go to / on which thread (null executor: inline)
  void setObserver(StationObserver* observer);
  void setExecutor(Executor* executor);
  [[nodiscard]] Executor* getExecutor() const;

  // the worker (for tests)
  [[nodiscard]] SensorWorker& getWorker() { return *this->worker; }

  // ####################################
  // get or set sensor control attributes
  // ####################################

  // get or set sensor
  Sensor getSensor();
  void setSensor(const QPointer<Sensor>& sensor);
  QPointer<Sensor> takeSensor();
  void resetSensor();

  // previously used sensors
  const QList<Sensor>& getUsedSensors();
  void setUsedSensors(const QList<Sensor>& sensors);

  // stream format
  ReadingTypes getStreamFormat();
  void setStreamFormat(ReadingTypes streamFormat);

  // ####################################################
  // get information about the currently connected sensor
  // ####################################################

  // status information
  bool getIsSensorSet();
  bool getIsSensorConnected();
  bool getIsReadyForMeasurement();
  bool getIsBusy();
  QMap<QString, QString> getSensorStatus();

  // sensor type
  SensorType getActiveSensorType();

  // reading and connection types
  QList<ReadingTypes> getSupportedReadingTypes();
  QList<ConnectionKind> getSupportedConnectionTypes();

  // sensor actions
  QList<SensorFunctions> getSupportedSensorActions();
  QStringList getSelfDefinedActions();

  // sensor configuration
  SensorConfiguration getSensorConfiguration();
  void setSensorConfiguration(const SensorConfiguration& sConfig);

  // ####################
  // start sensor actions
  // ####################

  // connect or disconnect
  void connectSensor();
  void disconnectSensor();

  // measure
  void measure(const int& geomId, const MeasurementConfig& mConfig);

  // general sensor actions
  void move(const double& azimuth, const double& zenith, const double& distance,
            const bool& isRelative, const bool& measure, const int& geomId = -1,
            const MeasurementConfig& mConfig = MeasurementConfig());
  void move(const double& x, const double& y, const double& z,
            const bool& measure, const int& geomId = -1,
            const MeasurementConfig& mConfig = MeasurementConfig());
  void initialize();
  void motorState();
  void home();
  void toggleSight();
  void compensation();
  void selfDefinedAction(const QString& action);
  void search();

  // ################
  // sensor streaming
  // ################

  // reading stream
  void startReadingStream();
  void stopReadingStream();

  // connection monitoring stream
  void startConnectionMonitoringStream();
  void stopConnectionMonitoringStream();

  // status monitoring stream
  void startStatusMonitoringStream();
  void stopStatusMonitoringStream();

  void finishMeasurement();

 private:
  class Delivery;
  class Relay;

  // previously used sensors
  QList<Sensor> usedSensors;

  // current sensor state
  bool sensorValid{false};

  // hands worker results to the observer through the executor
  std::shared_ptr<Delivery> delivery;
  std::unique_ptr<Relay> relay;

  // the worker (declared last: stopped first)
  std::unique_ptr<SensorWorker> worker;
};

}  // namespace oi

#endif  // SENSORCONTROL_H
