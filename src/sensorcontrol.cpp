#include "sensorcontrol.h"

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <utility>

using namespace oi;

/*!
 * Carries the results from the worker thread to the observer. Shared with
 * every task posted to the executor, so a task that is still queued when the
 * control is destroyed finds the delivery closed instead of a dangling
 * pointer.
 */
class SensorControl::Delivery
    : public std::enable_shared_from_this<SensorControl::Delivery> {
 public:
  using Call = std::function<void(StationObserver&)>;

  void setObserver(StationObserver* observer) {
    const std::scoped_lock lock(this->mutex);
    this->observer = observer;
  }

  // Returns once no post() that might still use the previous executor is
  // running on another thread, so the host may destroy that executor right
  // away (a post running on this very thread - an inline delivery calling
  // back - cannot be waited for and is skipped).
  void setExecutor(Executor* executor) {
    this->executor =
        executor != nullptr ? executor : &InlineExecutor::instance();
    if (postDepth() == 0) {
      std::unique_lock lock(this->postingMutex);
      this->postingDone.wait(lock, [this] { return this->posting == 0; });
    }
  }

  [[nodiscard]] Executor* getExecutor() const { return this->executor; }

  // after this returns no call is running and none will ever start. Waits for
  // a call that is in progress on another thread.
  void close() {
    const std::scoped_lock lock(this->mutex);
    this->observer = nullptr;
    this->closed = true;
  }

  void post(Call call) {
    ++postDepth();
    ++this->posting;
    this->executor.load()->post(
        [self = this->shared_from_this(), call = std::move(call)] {
          self->deliver(call);
        });
    --postDepth();
    if (--this->posting == 0) {
      const std::scoped_lock lock(this->postingMutex);
      this->postingDone.notify_all();
    }
  }

 private:
  void deliver(const Call& call) {
    const std::scoped_lock lock(this->mutex);
    if (!this->closed && this->observer != nullptr) {
      call(*this->observer);
    }
  }

  static int& postDepth() {
    thread_local int depth = 0;
    return depth;
  }

  std::atomic<int> posting{0};
  std::mutex postingMutex;
  std::condition_variable postingDone;

  // recursive: an inline executor delivers while the worker still runs, and the
  // observer may call back into the control
  std::recursive_mutex mutex;
  StationObserver* observer{nullptr};
  bool closed{false};
  std::atomic<Executor*> executor{&InlineExecutor::instance()};
};

/*!
 * The observer of the worker: every result becomes a task for the executor.
 */
class SensorControl::Relay final : public StationObserver {
 public:
  explicit Relay(std::shared_ptr<Delivery> delivery)
      : delivery(std::move(delivery)) {}

  void onRealTimeReading(const QVariantMap& reading) override {
    this->delivery->post(
        [reading](StationObserver& o) { o.onRealTimeReading(reading); });
  }
  void onRealTimeStatus(const QMap<QString, QString>& status) override {
    this->delivery->post(
        [status](StationObserver& o) { o.onRealTimeStatus(status); });
  }
  void onConnectionLost() override {
    this->delivery->post([](StationObserver& o) { o.onConnectionLost(); });
  }
  void onConnectionReceived() override {
    this->delivery->post([](StationObserver& o) { o.onConnectionReceived(); });
  }
  void onIsReadyForMeasurement(bool isReady) override {
    this->delivery->post(
        [isReady](StationObserver& o) { o.onIsReadyForMeasurement(isReady); });
  }
  void onCommandFinished(bool success, const QString& msg,
                         SensorAction action) override {
    this->delivery->post([success, msg, action](StationObserver& o) {
      o.onCommandFinished(success, msg, action);
    });
  }
  void onMeasurementFinished(
      int geomId, const QList<oi::Guarded<Reading>>& readings) override {
    this->delivery->post([geomId, readings](StationObserver& o) {
      o.onMeasurementFinished(geomId, readings);
    });
  }
  void onMeasurementDone(bool success) override {
    this->delivery->post(
        [success](StationObserver& o) { o.onMeasurementDone(success); });
  }
  void onSensorMessage(const QString& msg, MessageTypes msgType,
                       MessageDestinations msgDest) override {
    this->delivery->post([msg, msgType, msgDest](StationObserver& o) {
      o.onSensorMessage(msg, msgType, msgDest);
    });
  }
  void onSensorStatus(SensorStatus status, const QString& msg) override {
    this->delivery->post(
        [status, msg](StationObserver& o) { o.onSensorStatus(status, msg); });
  }

 private:
  std::shared_ptr<Delivery> delivery;
};

/*!
 * \brief SensorControl::SensorControl
 * \param observer
 * \param executor
 */
SensorControl::SensorControl(StationObserver* observer, Executor* executor)
    : delivery(std::make_shared<Delivery>()),
      relay(std::make_unique<Relay>(this->delivery)) {
  this->delivery->setObserver(observer);
  this->delivery->setExecutor(executor);
  this->worker = std::make_unique<SensorWorker>(*this->relay);
}

/*!
 * \brief SensorControl::~SensorControl
 */
SensorControl::~SensorControl() {
  // nothing reaches the observer any more, then stop the worker (members are
  // destroyed in reverse order, but the worker would still be able to post
  // before it is stopped)
  this->delivery->close();
  this->worker.reset();
}

void SensorControl::setObserver(StationObserver* observer) {
  this->delivery->setObserver(observer);
}

void SensorControl::setExecutor(Executor* executor) {
  this->delivery->setExecutor(executor);
}

Executor* SensorControl::getExecutor() const {
  return this->delivery->getExecutor();
}

/*!
 * \brief SensorControl::getSensor
 * Returns a copy of the current sensor
 * \return
 */
Sensor SensorControl::getSensor() { return this->worker->getSensor(); }

/*!
 * \brief SensorControl::setSensor
 * Sets the current sensor to the given one
 * \param sensor
 */
void SensorControl::setSensor(const oi::Guarded<Sensor>& sensor) {
  // check sensor
  if (sensor.isNull()) {
    return;
  }

  // check old sensor and add it to the list of used sensors
  if (this->sensorValid) {
    this->usedSensors.append(this->getSensor());
  }

  this->worker->setSensor(sensor);

  // set sensor valid
  this->sensorValid = true;
}

/*!
 * \brief SensorControl::takeSensor
 * Returns the current sensor instance. That sensor will no longer be used by
 * the sensor worker
 * \return
 */
oi::Guarded<Sensor> SensorControl::takeSensor() {
  // check old sensor and add it to the list of used sensors
  if (this->sensorValid) {
    this->usedSensors.append(this->getSensor());
  }

  oi::Guarded<Sensor> sensor = this->worker->takeSensor();

  // set sensor invalid
  this->sensorValid = false;

  return sensor;
}

/*!
 * \brief SensorControl::resetSensor
 * Disconnects and deletes the current sensor
 */
void SensorControl::resetSensor() {
  // check old sensor and add it to the list of used sensors
  if (this->sensorValid) {
    this->usedSensors.append(this->getSensor());
  }

  this->worker->resetSensor();

  // set sensor invalid
  this->sensorValid = false;
}

const QList<Sensor>& SensorControl::getUsedSensors() {
  return this->usedSensors;
}

void SensorControl::setUsedSensors(const QList<Sensor>& sensors) {
  this->usedSensors = sensors;
}

ReadingTypes SensorControl::getStreamFormat() {
  return this->worker->getStreamFormat();
}

void SensorControl::setStreamFormat(ReadingTypes streamFormat) {
  this->worker->setStreamFormat(streamFormat);
}

bool SensorControl::getIsSensorSet() { return this->sensorValid; }

bool SensorControl::getIsSensorConnected() {
  return this->worker->getIsSensorConnected();
}

bool SensorControl::getIsReadyForMeasurement() {
  return this->worker->getIsReadyForMeasurement();
}

bool SensorControl::getIsBusy() { return this->worker->getIsBusy(); }

QMap<QString, QString> SensorControl::getSensorStatus() {
  return this->worker->getSensorStatus();
}

SensorType SensorControl::getActiveSensorType() {
  return this->worker->getActiveSensorType();
}

QList<ReadingTypes> SensorControl::getSupportedReadingTypes() {
  return this->worker->getSupportedReadingTypes();
}

QList<ConnectionKind> SensorControl::getSupportedConnectionTypes() {
  return this->worker->getSupportedConnectionTypes();
}

QList<SensorFunctions> SensorControl::getSupportedSensorActions() {
  return this->worker->getSupportedSensorActions();
}

QStringList SensorControl::getSelfDefinedActions() {
  return this->worker->getSelfDefinedActions();
}

SensorConfiguration SensorControl::getSensorConfiguration() {
  return this->worker->getSensorConfiguration();
}

void SensorControl::setSensorConfiguration(const SensorConfiguration& sConfig) {
  this->worker->setSensorConfiguration(sConfig);
}

void SensorControl::connectSensor() { this->worker->connectSensor(); }

void SensorControl::disconnectSensor() { this->worker->disconnectSensor(); }

void SensorControl::measure(const int& geomId,
                            const MeasurementConfig& mConfig) {
  this->worker->measure(geomId, mConfig);
}

void SensorControl::move(const double& azimuth, const double& zenith,
                         const double& distance, const bool& isRelative,
                         const bool& measure, const int& geomId,
                         const MeasurementConfig& mConfig) {
  this->worker->move(azimuth, zenith, distance, isRelative, measure, geomId,
                     mConfig);
}

void SensorControl::move(const double& x, const double& y, const double& z,
                         const bool& measure, const int& geomId,
                         const MeasurementConfig& mConfig) {
  this->worker->move(x, y, z, measure, geomId, mConfig);
}

void SensorControl::initialize() { this->worker->initialize(); }
void SensorControl::motorState() { this->worker->motorState(); }
void SensorControl::home() { this->worker->home(); }
void SensorControl::toggleSight() { this->worker->toggleSight(); }
void SensorControl::compensation() { this->worker->compensation(); }

void SensorControl::selfDefinedAction(const QString& action) {
  this->worker->selfDefinedAction(action);
}

void SensorControl::search() { this->worker->search(); }

void SensorControl::startReadingStream() { this->worker->startReadingStream(); }
void SensorControl::stopReadingStream() { this->worker->stopReadingStream(); }
void SensorControl::startConnectionMonitoringStream() {
  this->worker->startConnectionMonitoringStream();
}
void SensorControl::stopConnectionMonitoringStream() {
  this->worker->stopConnectionMonitoringStream();
}
void SensorControl::startStatusMonitoringStream() {
  this->worker->startStatusMonitoringStream();
}
void SensorControl::stopStatusMonitoringStream() {
  this->worker->stopStatusMonitoringStream();
}

void SensorControl::finishMeasurement() { this->worker->finishMeasurement(); }
