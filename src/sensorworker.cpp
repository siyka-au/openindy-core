#include "sensorworker.h"

#include <format>
#include <map>
#include <utility>
#include <vector>

#include "containers.h"
#include "value.h"

using namespace oi;

// ####################################################################
// construction, queue and thread
// ####################################################################

SensorWorker::SensorWorker(StationObserver& results)
    : out(results), thread([this] { this->run(); }) {}

SensorWorker::~SensorWorker() {
  // no more reports from the driver once we are going away; waits for a report
  // that is being delivered right now
  const oi::Guarded<Sensor> current = this->currentSensor();
  if (!current.isNull()) {
    current->setSensorSink(nullptr);
  }

  // drop the queued commands, let the running one finish
  {
    const std::scoped_lock lock(this->queueMutex);
    this->shuttingDown = true;
  }
  this->queueChanged.notify_all();
  this->thread.join();
}

std::thread::id SensorWorker::workerThreadId() const {
  return this->thread.get_id();
}

void SensorWorker::run() {
  std::unique_lock lock(this->queueMutex);
  while (true) {
    this->queueChanged.wait(
        lock, [this] { return this->shuttingDown || !this->queue.empty(); });
    if (this->shuttingDown) {
      break;  // pending commands are dropped
    }

    std::function<void()> task = std::move(this->queue.front());
    this->queue.pop_front();
    lock.unlock();
    try {
      task();
    } catch (const std::exception& e) {
      this->out.onSensorMessage(
          std::format("sensor command failed with an exception: {}",
                      std::string(e.what())),
          MessageTypes::eErrorMessage, MessageDestinations::eConsoleMessage);
    } catch (...) {
      this->out.onSensorMessage("sensor command failed with an exception",
                                MessageTypes::eErrorMessage,
                                MessageDestinations::eConsoleMessage);
    }
    task = nullptr;  // destroy the task's captures outside the lock
    lock.lock();
  }
}

void SensorWorker::enqueue(std::function<void()> task) {
  {
    const std::scoped_lock lock(this->queueMutex);
    if (this->shuttingDown) {
      return;
    }
    this->queue.push_back(std::move(task));
  }
  this->queueChanged.notify_one();
}

void SensorWorker::runOnWorker(std::function<void()> task) {
  this->enqueue(std::move(task));
}

void SensorWorker::dispatch(std::function<void()> task) {
  if (std::this_thread::get_id() == this->thread.get_id()) {
    task();
  } else {
    this->enqueue(std::move(task));
  }
}

void SensorWorker::command(std::function<void()> task) {
  this->enqueue([this, task = std::move(task)] {
    {
      const std::scoped_lock driver(this->driverMutex);
      task();
    }
    this->refreshSnapshot();
  });
}

// ####################################################################
// the sensor and its snapshot
// ####################################################################

oi::Guarded<Sensor> SensorWorker::currentSensor() const {
  const std::scoped_lock lock(this->stateMutex);
  return this->sensor;
}

// copies the static properties of the sensor; the caller guarantees nobody
// else is using the sensor (driver lock held, or the sensor not yet shared)
void SensorWorker::storeSnapshot(const oi::Guarded<Sensor>& sensor) {
  std::unique_ptr<Sensor> copy;
  if (!sensor.isNull()) {
    copy = std::make_unique<Sensor>(*sensor.data());
  }
  const std::scoped_lock lock(this->snapshotMutex);
  this->snapshot = std::move(copy);
}

void SensorWorker::refreshSnapshot() {
  const std::scoped_lock driver(this->driverMutex);
  const oi::Guarded<Sensor> current = this->currentSensor();
  this->storeSnapshot(current);
  this->lastKnownConnected = !current.isNull() && current->getConnectionState();
}

Sensor SensorWorker::getSensor() const {
  const std::scoped_lock lock(this->snapshotMutex);
  if (this->snapshot == nullptr) {
    return Sensor();
  }
  return Sensor(*this->snapshot);
}

void SensorWorker::setSensor(const oi::Guarded<Sensor>& newSensor) {
  if (newSensor.isNull()) {
    return;
  }

  oi::Guarded<Sensor> old;
  {
    const std::scoped_lock lock(this->stateMutex);
    old = this->sensor;
    this->sensor = newSensor;
  }
  if (old == newSensor) {
    return;
  }

  if (!old.isNull()) {
    old->setSensorSink(nullptr);
  }
  newSensor->setSensorSink(this);
  this->storeSnapshot(newSensor);
  this->lastKnownConnected = newSensor->getConnectionState();
}

oi::Guarded<Sensor> SensorWorker::takeSensor() {
  // the sensor must not be in use by a command while someone else gets it
  const std::scoped_lock driver(this->driverMutex);

  oi::Guarded<Sensor> taken;
  {
    const std::scoped_lock lock(this->stateMutex);
    taken = this->sensor;
    this->sensor = oi::Guarded<Sensor>(nullptr);
  }
  if (!taken.isNull()) {
    taken->setSensorSink(nullptr);
  }
  this->storeSnapshot(oi::Guarded<Sensor>(nullptr));
  this->lastKnownConnected = false;
  return taken;
}

void SensorWorker::resetSensor() {
  this->command([this] { this->runResetSensor(); });
}

void SensorWorker::runResetSensor() {
  const oi::Guarded<Sensor> current = this->currentSensor();
  if (current.isNull()) {
    return;
  }

  // disconnect sensor
  if (current->getConnectionState()) {
    this->runDisconnectSensor();  // handles default and async sensors
  }
  current->setSensorSink(nullptr);
  {
    const std::scoped_lock lock(this->stateMutex);
    this->sensor = oi::Guarded<Sensor>(nullptr);
  }

  // delete sensor
  delete current.data();
}

// ####################################################################
// state of the sensor
// ####################################################################

bool SensorWorker::getIsSensorConnected() {
  std::unique_lock driver(this->driverMutex, std::try_to_lock);
  if (!driver.owns_lock()) {
    return this->lastKnownConnected;  // a command is running: last known state
  }
  const oi::Guarded<Sensor> current = this->currentSensor();
  if (current.isNull()) {
    return false;
  }
  this->lastKnownConnected = current->getConnectionState();
  return this->lastKnownConnected;
}

bool SensorWorker::getIsReadyForMeasurement() {
  std::unique_lock driver(this->driverMutex, std::try_to_lock);
  if (!driver.owns_lock()) {
    return false;  // busy executing a command
  }
  const oi::Guarded<Sensor> current = this->currentSensor();
  if (current.isNull() || !current->getConnectionState()) {
    return false;
  }
  return current->getIsReadyForMeasurement();
}

bool SensorWorker::getIsBusy() {
  std::unique_lock driver(this->driverMutex, std::try_to_lock);
  if (!driver.owns_lock()) {
    return true;  // executing a command
  }
  const oi::Guarded<Sensor> current = this->currentSensor();
  if (current.isNull() || !current->getConnectionState()) {
    return false;
  }
  return current->getIsBusy();
}

std::map<std::string, std::string> SensorWorker::getSensorStatus() {
  std::unique_lock driver(this->driverMutex, std::try_to_lock);
  if (!driver.owns_lock()) {
    return std::map<std::string, std::string>();
  }
  const oi::Guarded<Sensor> current = this->currentSensor();
  if (current.isNull() || !current->getConnectionState()) {
    return std::map<std::string, std::string>();
  }
  return current->getSensorStatus();
}

SensorType SensorWorker::getActiveSensorType() const {
  const std::scoped_lock lock(this->snapshotMutex);
  if (this->snapshot == nullptr) {
    return SensorType();
  }
  return this->snapshot->getSensorConfiguration().getTypeOfSensor();
}

std::vector<ReadingTypes> SensorWorker::getSupportedReadingTypes() const {
  const std::scoped_lock lock(this->snapshotMutex);
  return this->snapshot == nullptr ? std::vector<ReadingTypes>()
                                   : this->snapshot->getSupportedReadingTypes();
}

std::vector<ConnectionKind> SensorWorker::getSupportedConnectionTypes() const {
  const std::scoped_lock lock(this->snapshotMutex);
  return this->snapshot == nullptr
             ? std::vector<ConnectionKind>()
             : this->snapshot->getSupportedConnectionTypes();
}

std::vector<SensorFunctions> SensorWorker::getSupportedSensorActions() const {
  const std::scoped_lock lock(this->snapshotMutex);
  return this->snapshot == nullptr
             ? std::vector<SensorFunctions>()
             : this->snapshot->getSupportedSensorActions();
}

std::vector<std::string> SensorWorker::getSelfDefinedActions() const {
  const std::scoped_lock lock(this->snapshotMutex);
  return this->snapshot == nullptr ? std::vector<std::string>()
                                   : this->snapshot->getSelfDefinedActions();
}

SensorConfiguration SensorWorker::getSensorConfiguration() const {
  const std::scoped_lock lock(this->snapshotMutex);
  return this->snapshot == nullptr ? SensorConfiguration()
                                   : this->snapshot->getSensorConfiguration();
}

void SensorWorker::setSensorConfiguration(const SensorConfiguration& sConfig) {
  if (this->currentSensor().isNull()) {
    return;
  }

  // visible to the getters right away; the sensor gets it in order
  {
    const std::scoped_lock lock(this->snapshotMutex);
    if (this->snapshot != nullptr) {
      this->snapshot->setSensorConfiguration(sConfig);
    }
  }
  this->command([this, sConfig] {
    const oi::Guarded<Sensor> current = this->currentSensor();
    if (!current.isNull()) {
      current->setSensorConfiguration(sConfig);
    }
  });
}

// ####################################################################
// results
// ####################################################################

void SensorWorker::finished(bool success, const std::string& msg,
                            SensorAction action) {
  this->out.onCommandFinished(success, msg, action);
}

// measurementDone is reported after commandFinished and measurementFinished
void SensorWorker::finishedMeasurement(
    bool success, const std::string& msg, SensorAction action, int geomId,
    const std::vector<oi::Guarded<Reading>>& readings) {
  this->finished(success, msg, action);
  if (success) {
    this->out.onMeasurementFinished(geomId, readings);
  }
  this->out.onMeasurementDone(success);
}

// ####################################################################
// start sensor actions
// ####################################################################

void SensorWorker::connectSensor() {
  this->command([this] { this->runConnectSensor(); });
}
void SensorWorker::disconnectSensor() {
  this->command([this] { this->runDisconnectSensor(); });
}
void SensorWorker::measure(int geomId, MeasurementConfig mConfig) {
  this->command([this, geomId, mConfig = std::move(mConfig)] {
    this->runMeasure(geomId, mConfig);
  });
}
void SensorWorker::move(double azimuth, double zenith, double distance,
                        bool isRelative, bool measure, int geomId,
                        MeasurementConfig mConfig) {
  this->command([=, this, mConfig = std::move(mConfig)] {
    this->runMovePolar(azimuth, zenith, distance, isRelative, measure, geomId,
                       mConfig);
  });
}
void SensorWorker::move(double x, double y, double z, bool measure, int geomId,
                        MeasurementConfig mConfig) {
  this->command([=, this, mConfig = std::move(mConfig)] {
    this->runMoveCartesian(x, y, z, measure, geomId, mConfig);
  });
}
void SensorWorker::initialize() {
  this->command([this] { this->runInitialize(); });
}
void SensorWorker::motorState() {
  this->command([this] { this->runMotorState(); });
}
void SensorWorker::home() {
  this->command([this] { this->runHome(); });
}
void SensorWorker::toggleSight() {
  this->command([this] { this->runToggleSight(); });
}
void SensorWorker::compensation() {
  this->command([this] { this->runCompensation(); });
}
void SensorWorker::selfDefinedAction(const std::string& action) {
  this->command([this, action] { this->runSelfDefinedAction(action); });
}
void SensorWorker::search() {
  this->command([this] { this->runSearch(); });
}
void SensorWorker::finishMeasurement() {
  this->selfDefinedAction("stopMeasure");
}

// ####################################################################
// command implementations
// ####################################################################

void SensorWorker::runSearch() {
  const oi::Guarded<Sensor> sensor = this->currentSensor();
  if (sensor.isNull()) {
    return;
  }

  const bool success = sensor->search();

  this->finished(success, success ? SensorWorkerMessage::SEARCH_FINISHED
                                  : SensorWorkerMessage::FAILED_TO_SEARCH);
}

void SensorWorker::runConnectSensor() {
  const oi::Guarded<Sensor> sensor = this->currentSensor();
  // check sensor
  if (sensor.isNull()) {
    this->finished(false, SensorWorkerMessage::NO_SENSOR_INSTANCE);
    return;
  }

  // check wether the sensor is already connected
  std::string msg = SensorWorkerMessage::SENSOR_CONNECTED;
  bool success = false;

  if (!sensor->isSensorAsync()) {
    if (sensor->getConnectionState()) {
      msg = SensorWorkerMessage::SENSOR_IS_ALREADY_CONNECTED;
    } else {
      // connect sensor
      success = sensor->connectSensor();
      if (!success) {
        msg = SensorWorkerMessage::FAILED_TO_CONNECT_SENSOR;
      } else {
        this->isSensorConnected = true;
      }
    }
    this->finished(success, msg);
  } else {
    oi::ValueMap request;
    request.insert_or_assign("method", "connect");
    oi::ValueMap status = sensor->performAsyncSensorCommand(request);
    if (oi::value(status, std::string("status")).toString() == "blocked") {
      this->finished(false, SensorWorkerMessage::CONNECTION_WAS_BLOCKED);
    }
  }
}

void SensorWorker::runDisconnectSensor() {
  const oi::Guarded<Sensor> sensor = this->currentSensor();
  // check sensor
  if (sensor.isNull()) {
    this->finished(false, SensorWorkerMessage::NO_SENSOR_INSTANCE);
    return;
  }

  // check wether the sensor is already connected
  std::string msg = SensorWorkerMessage::SENSOR_DISCONNECTED;
  bool success = false;

  if (!sensor->isSensorAsync()) {
    if (!sensor->getConnectionState()) {
      msg = SensorWorkerMessage::SENSOR_IS_NOT_CONNECTED;
    } else {
      // disconnect sensor
      success = sensor->disconnectSensor();
      if (!success) {
        msg = SensorWorkerMessage::FAILED_TO_DISCONNECT_SENSOR;
      } else {
        this->isSensorConnected = false;
      }
    }
  } else {
    oi::ValueMap request;
    request.insert_or_assign("method", "disconnect");
    oi::ValueMap status = sensor->performAsyncSensorCommand(request);
    if (oi::value(status, std::string("status")).toString() == "blocked") {
      this->finished(false, SensorWorkerMessage::CONNECTION_WAS_BLOCKED);
    }
  }

  this->finished(success, msg);
}

void SensorWorker::runMeasure(int geomId, const MeasurementConfig& mConfig) {
  const oi::Guarded<Sensor> sensor = this->currentSensor();
  // check sensor
  if (sensor.isNull()) {
    this->finished(false, SensorWorkerMessage::NO_SENSOR_INSTANCE);
    return;
  }

  // check wether the sensor is already connected
  std::string msg = SensorWorkerMessage::FAILED_TO_MEASURE;
  bool success = false;
  std::vector<oi::Guarded<Reading>> readings;

  if (!sensor->isSensorAsync()) {
    if (!sensor->getConnectionState()) {
      msg = SensorWorkerMessage::SENSOR_IS_NOT_CONNECTED;
    } else {
      // measure
      readings = sensor->measure(mConfig);
      if (readings.size() > 0) {
        msg = SensorWorkerMessage::MEASUREMENT_FINISHED;
        success = true;
      }
    }

    // same logic like SensorWorker::onAsyncMeasurementResult; measurementDone
    // is reported last (the application always saw it after the others)
    this->finishedMeasurement(success, msg, SensorAction::eSensorActionMeasure,
                              geomId, readings);
  } else {
    oi::ValueMap request;
    request.insert_or_assign("method", "measure");
    request.insert_or_assign("geomId", geomId);

    sensor->setMeasurementConfig(mConfig);
    oi::ValueMap status = sensor->performAsyncSensorCommand(request);
    if (oi::value(status, std::string("status")).toString() == "blocked") {
      this->finished(false, SensorWorkerMessage::CONNECTION_WAS_BLOCKED);
    }
  }
}

void SensorWorker::runMovePolar(double azimuth, double zenith, double distance,
                                bool isRelative, bool measure, int geomId,
                                const MeasurementConfig& mConfig) {
  const oi::Guarded<Sensor> sensor = this->currentSensor();
  // check sensor
  if (sensor.isNull()) {
    this->finished(false, SensorWorkerMessage::NO_SENSOR_INSTANCE,
                   SensorAction::eSensorActionMove);
    return;
  }

  // check wether the sensor is already connected
  std::string msg = SensorWorkerMessage::FAILED_TO_MOVE_SENSOR;
  bool success = false;
  std::vector<oi::Guarded<Reading>> readings;
  if (!sensor->getConnectionState()) {
    msg = SensorWorkerMessage::SENSOR_IS_NOT_CONNECTED;
  } else {
    // set up sensor attributes
    SensorAttributes attr;
    attr.moveAzimuth = azimuth;
    attr.moveZenith = zenith;
    attr.moveDistance = distance;
    attr.moveIsRelative = isRelative;

    // move
    success = sensor->accept(SensorFunctions::eMoveAngle, attr);
    if (success) {
      msg = SensorWorkerMessage::MOVING_SENSOR_FINISHED_PREFIX;

      // optionally perform measurement after move
      if (measure) {
        // start measure
        readings = sensor->measure(mConfig);
        if (readings.size() > 0) {
          msg.append(", measurement finished");
        } else {
          msg.append(", failed to measure");
          success = false;
        }
      }
    }
  }

  this->finished(success, msg, SensorAction::eSensorActionMove);
  if (success && measure) {
    this->out.onMeasurementFinished(geomId, readings);
  }
}

void SensorWorker::runMoveCartesian(double x, double y, double z, bool measure,
                                    int geomId,
                                    const MeasurementConfig& mConfig) {
  const oi::Guarded<Sensor> sensor = this->currentSensor();
  // check sensor
  if (sensor.isNull()) {
    this->finished(false, SensorWorkerMessage::NO_SENSOR_INSTANCE,
                   SensorAction::eSensorActionMove);
    return;
  }

  if (!sensor->isSensorAsync()) {
    // check wether the sensor is already connected
    std::string msg = SensorWorkerMessage::FAILED_TO_MOVE_SENSOR;
    bool success = false;
    std::vector<oi::Guarded<Reading>> readings;
    if (!sensor->getConnectionState()) {
      msg = SensorWorkerMessage::SENSOR_IS_NOT_CONNECTED;
    } else {
      // set up sensor attributes
      SensorAttributes attr;
      attr.moveX = x;
      attr.moveY = y;
      attr.moveZ = z;
      attr.moveIsRelative = false;

      // move
      success = sensor->accept(SensorFunctions::eMoveXYZ, attr);
      if (success) {
        msg = SensorWorkerMessage::MOVING_SENSOR_FINISHED_PREFIX;

        // optionally perform measurement after move
        if (measure) {
          // start measure
          readings = sensor->measure(mConfig);
          if (readings.size() > 0) {
            msg = SensorWorkerMessage::
                MOVING_SENSOR_FINISHED_MEASUREMENT_FINISHED;
          } else {
            msg = SensorWorkerMessage::MOVING_SENSOR_FINISHED_FAILED_TO_MEASURE;
            success = false;
          }
        }
      }
    }

    this->finished(success, msg, SensorAction::eSensorActionMove);
    if (success && measure) {
      this->out.onMeasurementFinished(geomId, readings);
    }
  } else {
    oi::ValueMap request;
    request.insert_or_assign("method", "move (cartesian)");
    request.insert_or_assign("x", x);
    request.insert_or_assign("y", y);
    request.insert_or_assign("z", z);
    oi::ValueMap status = sensor->performAsyncSensorCommand(request);
    if (oi::value(status, std::string("status")).toString() == "blocked") {
      this->finished(false, SensorWorkerMessage::CONNECTION_WAS_BLOCKED,
                     SensorAction::eSensorActionMove);
    }
  }
}

void SensorWorker::runInitialize() {
  const oi::Guarded<Sensor> sensor = this->currentSensor();
  // check sensor
  if (sensor.isNull()) {
    this->finished(false, SensorWorkerMessage::NO_SENSOR_INSTANCE);
    return;
  }

  // check wether the sensor is already connected
  if (!sensor->isSensorAsync()) {
    std::string msg = SensorWorkerMessage::FAILED_TO_INITIALIZE_SENSOR;
    bool success = false;
    if (!sensor->getConnectionState()) {
      msg = SensorWorkerMessage::SENSOR_IS_NOT_CONNECTED;
    } else {
      // initialize sensor
      success =
          sensor->accept(SensorFunctions::eInitialize, SensorAttributes());
      if (success) {
        msg = SensorWorkerMessage::INITIALIZING_FINISHED;
      }
    }

    this->finished(success, msg);
  } else {
    oi::ValueMap request;
    request.insert_or_assign("method", "initialize");
    oi::ValueMap status = sensor->performAsyncSensorCommand(request);
    if (oi::value(status, std::string("status")).toString() == "blocked") {
      this->finished(false, SensorWorkerMessage::CONNECTION_WAS_BLOCKED);
    }
  }
}

void SensorWorker::runMotorState() {
  const oi::Guarded<Sensor> sensor = this->currentSensor();
  // check sensor
  if (sensor.isNull()) {
    this->finished(false, SensorWorkerMessage::NO_SENSOR_INSTANCE);
    return;
  }

  // check wether the sensor is already connected
  std::string msg = SensorWorkerMessage::FAILED_TO_CHANGE_MOTOR_STATE;
  bool success = false;
  if (!sensor->getConnectionState()) {
    msg = SensorWorkerMessage::SENSOR_IS_NOT_CONNECTED;
  } else {
    // changing motor state
    success = sensor->accept(SensorFunctions::eMotorState, SensorAttributes());
    if (success) {
      msg = SensorWorkerMessage::CHANGING_MOTOR_STATE_FINISHED;
    }
  }

  this->finished(success, msg);
}

void SensorWorker::runHome() {
  const oi::Guarded<Sensor> sensor = this->currentSensor();
  // check sensor
  if (sensor.isNull()) {
    this->finished(false, SensorWorkerMessage::NO_SENSOR_INSTANCE);
    return;
  }

  // check wether the sensor is already connected
  std::string msg = SensorWorkerMessage::FAILED_TO_SET_SENSOR_TO_HOME_POSITION;
  bool success = false;
  if (!sensor->getConnectionState()) {
    msg = SensorWorkerMessage::SENSOR_IS_NOT_CONNECTED;
  } else {
    // setting sensor to home position
    success = sensor->accept(SensorFunctions::eHome, SensorAttributes());
    if (success) {
      msg = SensorWorkerMessage::SETTING_HOME_FINISHED;
    }
  }

  this->finished(success, msg);
}

void SensorWorker::runToggleSight() {
  const oi::Guarded<Sensor> sensor = this->currentSensor();
  // check sensor
  if (sensor.isNull()) {
    this->finished(false, SensorWorkerMessage::NO_SENSOR_INSTANCE);
    return;
  }

  // check wether the sensor is already connected
  std::string msg = SensorWorkerMessage::FAILED_TO_TOGGLE_SIGHT_ORIENTATION;
  bool success = false;

  if (!sensor->isSensorAsync()) {
    if (!sensor->getConnectionState()) {
      msg = SensorWorkerMessage::SENSOR_IS_NOT_CONNECTED;
    } else {
      // toggle sight
      success =
          sensor->accept(SensorFunctions::eToggleSight, SensorAttributes());
      if (success) {
        msg = SensorWorkerMessage::TOGGLE_SIGHT_ORIENTATION_FINISHED;
      }
    }

    this->finished(success, msg);
  } else {
    oi::ValueMap request;
    request.insert_or_assign("method", "toggle sight orientation");
    oi::ValueMap status = sensor->performAsyncSensorCommand(request);
    if (oi::value(status, std::string("status")).toString() == "blocked") {
      this->finished(false, SensorWorkerMessage::CONNECTION_WAS_BLOCKED);
    }
  }
}

void SensorWorker::runCompensation() {
  const oi::Guarded<Sensor> sensor = this->currentSensor();
  // check sensor
  if (sensor.isNull()) {
    this->finished(false, SensorWorkerMessage::NO_SENSOR_INSTANCE);
    return;
  }

  // check wether the sensor is already connected
  std::string msg = SensorWorkerMessage::FAILED_TO_START_COMPENSATION;
  bool success = false;
  if (!sensor->getConnectionState()) {
    msg = SensorWorkerMessage::SENSOR_IS_NOT_CONNECTED;
  } else {
    // starting compensation
    success =
        sensor->accept(SensorFunctions::eCompensation, SensorAttributes());
    if (success) {
      msg = SensorWorkerMessage::STARTING_COMPENSATION_FINISHED;
    }
  }

  this->finished(success, msg);
}

void SensorWorker::runSelfDefinedAction(const std::string& action) {
  const oi::Guarded<Sensor> sensor = this->currentSensor();
  // check sensor
  if (sensor.isNull()) {
    this->finished(false, SensorWorkerMessage::NO_SENSOR_INSTANCE);
    return;
  }

  // check wether the sensor is already connected
  std::string msg = SensorWorkerMessage::FAILED_TO_DO_SELF_DEFINED_ACTION;
  bool success = false;
  if (!sensor->getConnectionState()) {
    msg = SensorWorkerMessage::SENSOR_IS_NOT_CONNECTED;
  } else {
    // starting compensation
    success = sensor->doSelfDefinedAction(action);
    if (success) {
      msg = SensorWorkerMessage::SELF_DEFINED_ACTION_FINISHED;
    }
  }

  this->finished(success, msg);
}

// ####################################################################
// real time streaming
// ####################################################################

ReadingTypes SensorWorker::getStreamFormat() const {
  return this->streamFormat;
}

void SensorWorker::setStreamFormat(ReadingTypes format) {
  this->streamFormat = format;
}

void SensorWorker::startReadingStream() {
  this->enqueue([this] { this->runStartReadingStream(); });
}
void SensorWorker::stopReadingStream() {
  this->enqueue([this] { this->isReadingStreamStarted = false; });
}
void SensorWorker::startConnectionMonitoringStream() {
  this->enqueue([this] { this->runStartConnectionMonitoringStream(); });
}
void SensorWorker::stopConnectionMonitoringStream() {
  this->enqueue([this] { this->isConnectionStreamStarted = false; });
}
void SensorWorker::startStatusMonitoringStream() {
  this->enqueue([this] { this->runStartStatusMonitoringStream(); });
}
void SensorWorker::stopStatusMonitoringStream() {
  this->enqueue([this] { this->isStatusStreamStarted = false; });
}

void SensorWorker::runStartReadingStream() {
  if (this->currentSensor().isNull()) {
    this->isReadingStreamStarted = false;
    this->out.onSensorMessage(SensorWorkerMessage::NO_SENSOR_INSTANCE,
                              MessageTypes::eErrorMessage,
                              MessageDestinations::eConsoleMessage);
    return;
  }
  if (this->isReadingStreamStarted) {
    return;  // already running
  }
  this->isReadingStreamStarted = true;
  ++this->readingGeneration;
  this->scheduleReadingStream();
}

void SensorWorker::runStartConnectionMonitoringStream() {
  if (this->currentSensor().isNull()) {
    this->isConnectionStreamStarted = false;
    this->out.onSensorMessage(SensorWorkerMessage::NO_SENSOR_INSTANCE,
                              MessageTypes::eErrorMessage,
                              MessageDestinations::eConsoleMessage);
    return;
  }
  if (this->isConnectionStreamStarted) {
    return;
  }
  this->isConnectionStreamStarted = true;
  ++this->connectionGeneration;
  this->scheduleConnectionMonitoring();
}

void SensorWorker::runStartStatusMonitoringStream() {
  if (this->currentSensor().isNull()) {
    this->isStatusStreamStarted = false;
    this->out.onSensorMessage(SensorWorkerMessage::NO_SENSOR_INSTANCE,
                              MessageTypes::eErrorMessage,
                              MessageDestinations::eConsoleMessage);
    return;
  }
  if (this->isStatusStreamStarted) {
    return;
  }
  this->isStatusStreamStarted = true;
  ++this->statusGeneration;
  this->scheduleStatusStream();
}

// an iteration goes to the end of the queue, behind whatever was queued since
void SensorWorker::scheduleReadingStream() {
  this->enqueue([this, generation = this->readingGeneration] {
    this->streamReading(generation);
  });
}
void SensorWorker::scheduleConnectionMonitoring() {
  this->enqueue([this, generation = this->connectionGeneration] {
    this->monitorConnectionStatus(generation);
  });
}
void SensorWorker::scheduleStatusStream() {
  this->enqueue([this, generation = this->statusGeneration] {
    this->streamStatus(generation);
  });
}

void SensorWorker::streamReading(std::uint64_t generation) {
  if (!this->isReadingStreamStarted || generation != this->readingGeneration) {
    return;
  }

  const std::scoped_lock driver(this->driverMutex);
  const oi::Guarded<Sensor> sensor = this->currentSensor();
  if (sensor.isNull()) {
    this->isReadingStreamStarted = false;
    return;
  }

  if (!sensor->isSensorAsync()) {
    // get real time reading
    const oi::ValueMap reading = sensor->readingStream(this->streamFormat);
    this->out.onRealTimeReading(reading);

    // next iteration
    this->scheduleReadingStream();
  } else {
    // the next iteration is scheduled when the sensor delivers a reading
    oi::ValueMap request;
    request.insert_or_assign("method", "stream");
    const oi::ValueMap status = sensor->performAsyncSensorCommand(request);
    if (oi::value(status, std::string("status")).toString() == "blocked") {
      this->finished(false, SensorWorkerMessage::CONNECTION_WAS_BLOCKED);
    }
  }
}

void SensorWorker::monitorConnectionStatus(std::uint64_t generation) {
  if (!this->isConnectionStreamStarted ||
      generation != this->connectionGeneration) {
    return;
  }

  const std::scoped_lock driver(this->driverMutex);
  const oi::Guarded<Sensor> sensor = this->currentSensor();
  if (sensor.isNull()) {
    this->isConnectionStreamStarted = false;
    return;
  }

  // get and compare connection status
  const bool isConnected = sensor->getConnectionState();
  if (!isConnected && this->isSensorConnected) {
    this->isSensorConnected = false;
    this->out.onConnectionLost();
  } else if (isConnected && !this->isSensorConnected) {
    this->isSensorConnected = true;
    this->out.onConnectionReceived();
  }

  this->scheduleConnectionMonitoring();
}

void SensorWorker::streamStatus(std::uint64_t generation) {
  if (!this->isStatusStreamStarted || generation != this->statusGeneration) {
    return;
  }

  const std::scoped_lock driver(this->driverMutex);
  const oi::Guarded<Sensor> sensor = this->currentSensor();
  if (sensor.isNull()) {
    this->isStatusStreamStarted = false;
    return;
  }

  this->out.onRealTimeStatus(sensor->getSensorStatus());
  this->scheduleStatusStream();
}

// ####################################################################
// SensorSink
// ####################################################################

// these two touch no worker state: forwarded from the reporting thread
void SensorWorker::onSensorStatus(const SensorStatus& status,
                                  const std::string& msg) {
  this->out.onSensorStatus(status, msg);
}

void SensorWorker::onSensorMessage(const std::string& msg,
                                   const MessageTypes& msgType,
                                   const MessageDestinations& msgDest) {
  this->out.onSensorMessage(msg, msgType, msgDest);
}

void SensorWorker::onAsyncSensorResponse(const oi::ValueMap& response,
                                         SensorAction sensorAction) {
  this->dispatch([this, response, sensorAction] {
    bool success = false;
    std::string msg;
    if (response.contains("error")) {
      msg = oi::value(response, std::string("error")).toString();
    } else {
      msg = oi::value(response, std::string("result")).toString();
      success = true;
    }
    this->finished(success, msg, sensorAction);
  });
}

void SensorWorker::onAsyncMeasurementResult(
    const int& geomId, const std::vector<oi::Guarded<Reading>>& measurements,
    SensorAction sensorAction) {
  this->dispatch([this, geomId, measurements, sensorAction] {
    // same logic like SensorWorker::runMeasure
    const bool success = measurements.size() > 0;
    this->finishedMeasurement(
        success,
        success ? SensorWorkerMessage::MEASUREMENT_DATA_RECEIVED
                : SensorWorkerMessage::MEASUREMENT_DIT_NOT_DELIVER_ANY_RESULTS,
        sensorAction, geomId, measurements);
  });
}

void SensorWorker::onAsyncStreamResult(const oi::ValueMap& reading) {
  this->dispatch([this, reading] {
    this->out.onRealTimeReading(reading);

    // the sensor delivered, ask for the next one
    this->scheduleReadingStream();
  });
}

// nobody consumed these so far; the worker drops them as before
void SensorWorker::onAsyncSensorNotification(const oi::ValueMap& /*response*/) {
}
