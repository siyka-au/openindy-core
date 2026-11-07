#ifndef SENSORWORKER_H
#define SENSORWORKER_H

#include <QList>
#include <QMap>
#include <QPointer>
#include <QVariantMap>
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>

#include "core_export.h"
#include "sensor.h"
#include "sensorsink.h"
#include "sensorworkermessage.h"
#include "stationobserver.h"

namespace oi {

/*!
 * \brief Runs one Sensor on a dedicated thread.
 *
 * Every command is a task on a FIFO queue that a single std::jthread works
 * off, so the sensor driver is only ever driven from that thread and commands
 * run in the order they were issued. The three streams (reading, connection
 * monitoring, status monitoring) are tasks that put themselves back at the
 * end of the same queue after each iteration, exactly like the self-requeuing
 * event loop iterations they replace: commands issued while a stream runs are
 * interleaved with its iterations and never starved, and a stop request takes
 * effect before the stream's next iteration (it is a task in the same queue;
 * a stale iteration still in the queue sees the stopped state and ends).
 *
 * Results are reported to the StationObserver given at construction, from the
 * worker thread, in the order they were produced. (onSensorMessage and
 * onSensorStatus are forwarded straight from whichever thread the driver
 * reported on.) SensorControl passes an observer that hands them on through
 * an Executor.
 *
 * The worker is the SensorSink of its sensor. Reports that arrive on a thread
 * other than the worker thread (an asynchronous sensor's own thread) are
 * turned into queue tasks, so the worker's state is never touched from two
 * threads at once; reports arriving on the worker thread itself (a driver
 * reporting while it executes a command) are handled immediately.
 *
 * Callers never block on a running command. Everything that starts a sensor
 * action just enqueues it. The getters answer from a snapshot of the sensor
 * that the worker refreshes after every command (static properties) or, for
 * the live state (connected / ready / busy / status), ask the driver directly
 * if it is idle and otherwise answer with the last known state (busy: true).
 * takeSensor() is the one exception: it waits for a running command to finish,
 * because the sensor must not be used by two workers at once.
 *
 * SHUTDOWN: the destructor detaches the sensor, then tells the thread to stop
 * and joins it. A command that is running is allowed to finish (it cannot be
 * interrupted); commands still waiting in the queue are DROPPED, not drained -
 * draining would run arbitrarily long hardware commands during destruction and
 * against a sensor that is about to lose its owner. Results of the command that
 * finishes during destruction still reach the observer, which therefore has to
 * outlive the worker. Destroying a worker from one of its own callbacks (i.e.
 * on the worker thread) is not allowed.
 */
class OI_CORE_EXPORT SensorWorker final : public SensorSink {
 public:
  explicit SensorWorker(StationObserver& results);
  ~SensorWorker() override;

  SensorWorker(const SensorWorker&) = delete;
  SensorWorker& operator=(const SensorWorker&) = delete;
  SensorWorker(SensorWorker&&) = delete;
  SensorWorker& operator=(SensorWorker&&) = delete;

  // enqueue an arbitrary task on the worker thread (after everything already
  // queued); dropped once the worker is shutting down
  void runOnWorker(std::function<void()> task);

  // the id of the worker thread
  [[nodiscard]] std::thread::id workerThreadId() const;

  // ##############
  //(re)set sensor
  // ##############

  // a copy of the sensor's static properties (not the live sensor)
  [[nodiscard]] Sensor getSensor() const;
  void setSensor(const QPointer<Sensor>& sensor);
  // waits for a running command, then hands the sensor back
  QPointer<Sensor> takeSensor();
  // disconnects (if connected) and deletes the sensor on the worker thread
  void resetSensor();

  // ####################################################
  // get information about the currently connected sensor
  // ####################################################

  // live state; never blocks (see class comment)
  [[nodiscard]] bool getIsSensorConnected();
  [[nodiscard]] bool getIsReadyForMeasurement();
  [[nodiscard]] bool getIsBusy();
  [[nodiscard]] QMap<QString, QString> getSensorStatus();

  // static properties; answered from the snapshot
  [[nodiscard]] SensorType getActiveSensorType() const;
  [[nodiscard]] QList<ReadingTypes> getSupportedReadingTypes() const;
  [[nodiscard]] QList<ConnectionKind> getSupportedConnectionTypes() const;
  [[nodiscard]] QList<SensorFunctions> getSupportedSensorActions() const;
  [[nodiscard]] QStringList getSelfDefinedActions() const;
  [[nodiscard]] SensorConfiguration getSensorConfiguration() const;
  void setSensorConfiguration(const SensorConfiguration& sConfig);

  // ####################
  // start sensor actions (all of them enqueue and return at once)
  // ####################

  void connectSensor();
  void disconnectSensor();
  void measure(int geomId, MeasurementConfig mConfig);
  void move(double azimuth, double zenith, double distance, bool isRelative,
            bool measure, int geomId = -1,
            MeasurementConfig mConfig = MeasurementConfig());
  void move(double x, double y, double z, bool measure, int geomId = -1,
            MeasurementConfig mConfig = MeasurementConfig());
  void initialize();
  void motorState();
  void home();
  void toggleSight();
  void compensation();
  void selfDefinedAction(const QString& action);
  void search();
  void finishMeasurement();

  // ###################
  // real time streaming
  // ###################

  [[nodiscard]] ReadingTypes getStreamFormat() const;
  void setStreamFormat(ReadingTypes streamFormat);

  void startReadingStream();
  void stopReadingStream();
  void startConnectionMonitoringStream();
  void stopConnectionMonitoringStream();
  void startStatusMonitoringStream();
  void stopStatusMonitoringStream();

  // ##########################################
  // SensorSink: what the sensor reports itself
  // ##########################################

  void onSensorStatus(const SensorStatus& status, const QString& msg) override;
  void onSensorMessage(const QString& msg, const MessageTypes& msgType,
                       const MessageDestinations& msgDest) override;
  void onAsyncSensorResponse(const QJsonObject& response,
                             SensorAction sensorAction) override;
  void onAsyncMeasurementResult(const int& geomId,
                                const QList<QPointer<Reading>>& measurements,
                                SensorAction sensorAction) override;
  void onAsyncStreamResult(const QVariantMap& reading) override;
  void onAsyncSensorNotification(const QJsonObject& response) override;

 private:
  // queue
  void run();
  void enqueue(std::function<void()> task);
  // runs `task` now if called on the worker thread, otherwise enqueues it
  void dispatch(std::function<void()> task);
  // enqueue a sensor command: runs holding the driver lock, then refreshes
  // the snapshot of the sensor
  void command(std::function<void()> task);

  // sensor access
  [[nodiscard]] QPointer<Sensor> currentSensor() const;
  void refreshSnapshot();
  void storeSnapshot(const QPointer<Sensor>& sensor);

  // command implementations (worker thread, driver lock held)
  void runConnectSensor();
  void runDisconnectSensor();
  void runMeasure(int geomId, const MeasurementConfig& mConfig);
  void runMovePolar(double azimuth, double zenith, double distance,
                    bool isRelative, bool measure, int geomId,
                    const MeasurementConfig& mConfig);
  void runMoveCartesian(double x, double y, double z, bool measure, int geomId,
                        const MeasurementConfig& mConfig);
  void runInitialize();
  void runMotorState();
  void runHome();
  void runToggleSight();
  void runCompensation();
  void runSelfDefinedAction(const QString& action);
  void runSearch();
  void runResetSensor();

  // result helpers
  void finished(bool success, const QString& msg,
                SensorAction action = SensorAction::eSensorActionUndefind);
  void finishedMeasurement(bool success, const QString& msg,
                           SensorAction action, int geomId,
                           const QList<QPointer<Reading>>& readings);

  // streams (worker thread)
  void runStartReadingStream();
  void runStartConnectionMonitoringStream();
  void runStartStatusMonitoringStream();
  void scheduleReadingStream();
  void scheduleConnectionMonitoring();
  void scheduleStatusStream();
  void streamReading(std::uint64_t generation);
  void monitorConnectionStatus(std::uint64_t generation);
  void streamStatus(std::uint64_t generation);

  StationObserver& out;

  // the queue
  std::mutex queueMutex;
  std::condition_variable queueChanged;
  std::deque<std::function<void()>> queue;
  bool shuttingDown{false};  // guarded by queueMutex

  // current sensor: stateMutex guards the pointer only and is never held
  // across a driver call. driverMutex is held by the worker for every driver
  // interaction (recursive: a result delivered inline may call back).
  mutable std::mutex stateMutex;
  QPointer<Sensor> sensor;
  std::recursive_mutex driverMutex;

  // snapshot of the sensor's static properties; a leaf lock
  mutable std::mutex snapshotMutex;
  std::unique_ptr<Sensor> snapshot;

  // the connection state as the worker thread last saw it (monitoring
  // compares against it) and as anybody last saw it (what getIsSensorConnected
  // answers while a command keeps the driver busy)
  std::atomic<bool> isSensorConnected{false};
  std::atomic<bool> lastKnownConnected{false};
  std::atomic<ReadingTypes> streamFormat{ReadingTypes::eUndefinedReading};

  // stream state: worker thread only. A generation number identifies the
  // running loop so that a loop that was stopped and restarted while its old
  // iteration was still queued is not run twice.
  bool isReadingStreamStarted{false};
  bool isConnectionStreamStarted{false};
  bool isStatusStreamStarted{false};
  std::uint64_t readingGeneration{0};
  std::uint64_t connectionGeneration{0};
  std::uint64_t statusGeneration{0};

  std::jthread thread;  // last: started last, joined first
};

}  // namespace oi

#endif  // SENSORWORKER_H
