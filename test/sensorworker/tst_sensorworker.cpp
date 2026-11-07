// Tests of the sensor path: SensorWorker (command queue, streams, shutdown),
// SensorControl (delivery through the Executor) and Station (command methods,
// StationObserver). They run with plain std::thread helpers and are meant to
// be run under ThreadSanitizer as well.
#include <QCoreApplication>
#include <QtTest>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "chooselalib.h"
#include "executor.h"
#include "featurewrapper.h"
#include "oijob.h"
#include "reading.h"
#include "recordingjobobserver.h"
#include "sensor.h"
#include "sensorcontrol.h"
#include "sensorworker.h"
#include "sensorworkermessage.h"
#include "station.h"
#include "stationobserver.h"

using namespace oi;
using namespace std::chrono_literals;

namespace {

// a thread-safe log of (what, on which thread)
struct Event {
  QString what;
  std::thread::id thread;
};

class Log {
 public:
  void add(const QString& what) {
    {
      const std::scoped_lock lock(this->mutex);
      this->events.push_back({what, std::this_thread::get_id()});
    }
    this->changed.notify_all();
  }

  [[nodiscard]] std::vector<Event> all() const {
    const std::scoped_lock lock(this->mutex);
    return this->events;
  }

  [[nodiscard]] QStringList names() const {
    QStringList result;
    for (const Event& e : this->all()) {
      result << e.what;
    }
    return result;
  }

  [[nodiscard]] int count(const QString& prefix) const {
    int n = 0;
    for (const Event& e : this->all()) {
      if (e.what == prefix || e.what.startsWith(prefix + ":")) {
        ++n;
      }
    }
    return n;
  }

  bool waitForCount(const QString& prefix, int n, int timeoutMs = 5000) {
    std::unique_lock lock(this->mutex);
    return this->changed.wait_for(
        lock, std::chrono::milliseconds(timeoutMs), [&] {
          int c = 0;
          for (const Event& e : this->events) {
            if (e.what == prefix || e.what.startsWith(prefix + ":")) {
              ++c;
            }
          }
          return c >= n;
        });
  }

 private:
  mutable std::mutex mutex;
  std::condition_variable changed;
  std::vector<Event> events;
};

// A sensor driver whose calls are logged with the calling thread.
class FakeSensor : public Sensor {
 public:
  Log calls;
  std::atomic<bool> connected{false};
  std::atomic<bool> async{false};
  std::atomic<int> connectDelayMs{0};
  std::atomic<int> measureDelayMs{0};
  std::atomic<int> streamDelayMs{0};
  std::atomic<int> readings{2};
  std::atomic<int> streamCounter{0};
  // a call that must never overlap another (the worker serialises them)
  std::atomic<int> inDriver{0};
  std::atomic<bool> overlapped{false};

  explicit FakeSensor(QObject* parent = nullptr) : Sensor(parent) {}

  bool isSensorAsync() const override { return this->async; }

  bool connectSensor() override {
    const Guard g(*this, "connect");
    std::this_thread::sleep_for(
        std::chrono::milliseconds(this->connectDelayMs));
    this->connected = true;
    return true;
  }
  bool disconnectSensor() override {
    const Guard g(*this, "disconnect");
    this->connected = false;
    return true;
  }
  bool getConnectionState() override { return this->connected; }
  bool getIsReadyForMeasurement() override { return true; }
  bool getIsBusy() override { return false; }
  QMap<QString, QString> getSensorStatus() override {
    this->calls.add("status");
    return {{"state", "ok"}};
  }
  QList<QPointer<Reading>> measure(const MeasurementConfig&) override {
    const Guard g(*this, "measure");
    std::this_thread::sleep_for(
        std::chrono::milliseconds(this->measureDelayMs));
    QList<QPointer<Reading>> result;
    for (int i = 0; i < this->readings; ++i) {
      ReadingCartesian r;
      r.isValid = true;
      r.xyz.setAt(0, i);
      result << new Reading(r);
    }
    return result;
  }
  QVariantMap readingStream(const ReadingTypes&) override {
    const Guard g(*this, "stream");
    std::this_thread::sleep_for(std::chrono::milliseconds(this->streamDelayMs));
    QVariantMap m;
    m["n"] = ++this->streamCounter;
    return m;
  }
  bool accept(const SensorFunctions&, const SensorAttributes&) override {
    const Guard g(*this, "accept");
    return true;
  }
  bool doSelfDefinedAction(const QString& action) override {
    const Guard g(*this, "action:" + action);
    return true;
  }
  bool search() override {
    const Guard g(*this, "search");
    return true;
  }
  QJsonObject performAsyncSensorCommand(const QJsonObject& request) override {
    this->calls.add("async:" + request.value("method").toString());
    return QJsonObject();
  }

 private:
  struct Guard {
    FakeSensor& s;
    Guard(FakeSensor& sensor, const QString& name) : s(sensor) {
      if (s.inDriver.fetch_add(1) != 0) {
        s.overlapped = true;
      }
      s.calls.add(name);
    }
    ~Guard() { s.inDriver.fetch_sub(1); }
  };
};

// records what the worker/station reports; also checks that callbacks of one
// kind never run concurrently
class RecordingObserver : public StationObserver {
 public:
  Log log;
  std::atomic<int> running{0};
  std::atomic<bool> overlapped{false};
  std::atomic<int> destroyedCount{0};

  struct Scope {
    RecordingObserver& o;
    explicit Scope(RecordingObserver& observer) : o(observer) {
      if (o.running.fetch_add(1) != 0) {
        o.overlapped = true;
      }
    }
    ~Scope() { o.running.fetch_sub(1); }
  };

  void onStationDestroyed(Station*) override { ++this->destroyedCount; }
  void onRealTimeReading(const QVariantMap& r) override {
    const Scope s(*this);
    this->log.add(QString("reading:%1").arg(r.value("n").toInt()));
  }
  void onRealTimeStatus(const QMap<QString, QString>&) override {
    const Scope s(*this);
    this->log.add("status");
  }
  void onConnectionLost() override { this->log.add("lost"); }
  void onConnectionReceived() override { this->log.add("received"); }
  void onCommandFinished(bool success, const QString& msg,
                         SensorAction action) override {
    const Scope s(*this);
    this->log.add(QString("finished:%1:%2:%3")
                      .arg(success)
                      .arg(msg)
                      .arg(static_cast<int>(action)));
  }
  void onMeasurementFinished(
      int geomId, const QList<QPointer<Reading>>& readings) override {
    const Scope s(*this);
    this->log.add(
        QString("measurementFinished:%1:%2").arg(geomId).arg(readings.size()));
  }
  void onMeasurementDone(bool success) override {
    const Scope s(*this);
    this->log.add(QString("done:%1").arg(success));
  }
  void onSensorMessage(const QString& msg, MessageTypes,
                       MessageDestinations) override {
    this->log.add("message:" + msg);
  }
};

// queues the tasks until the test drains them
class ManualExecutor : public Executor {
 public:
  void post(std::function<void()> task) override {
    {
      const std::scoped_lock lock(this->mutex);
      this->tasks.push_back(std::move(task));
      this->posters.push_back(std::this_thread::get_id());
    }
    this->changed.notify_all();
  }

  bool waitForPending(std::size_t n, int timeoutMs = 5000) {
    std::unique_lock lock(this->mutex);
    return this->changed.wait_for(lock, std::chrono::milliseconds(timeoutMs),
                                  [&] { return this->tasks.size() >= n; });
  }

  [[nodiscard]] std::size_t pending() const {
    const std::scoped_lock lock(this->mutex);
    return this->tasks.size();
  }

  [[nodiscard]] std::vector<std::thread::id> posterThreads() const {
    const std::scoped_lock lock(this->mutex);
    return this->posters;
  }

  // runs everything queued so far on the calling thread
  std::size_t drain() {
    std::deque<std::function<void()>> batch;
    {
      const std::scoped_lock lock(this->mutex);
      batch.swap(this->tasks);
    }
    for (auto& task : batch) {
      task();
    }
    return batch.size();
  }

 private:
  mutable std::mutex mutex;
  std::condition_variable changed;
  std::deque<std::function<void()>> tasks;
  std::vector<std::thread::id> posters;
};

// waits until everything queued on the worker so far has been executed
void barrier(SensorWorker& worker) {
  std::promise<void> done;
  worker.runOnWorker([&done] { done.set_value(); });
  done.get_future().wait();
}

}  // namespace

class SensorWorkerTest : public QObject {
  Q_OBJECT

 private slots:
  void initTestCase();

  // worker
  void commandsRunOnTheWorkerThreadInFifoOrder();
  void aSlowCommandDoesNotBlockTheCaller();
  void measurementFlow();
  void measurementFailureReportsDoneLast();
  void commandsWithoutSensorReportAnError();
  void readingStreamYieldsReadingsThenStops();
  void aRestartedStreamKeepsRunning();
  void commandsAreInterleavedWithARunningStream();
  void statusStreamReportsStatus();
  void connectionMonitoringReportsLostAndReceived();
  void asyncSensorReportsFromForeignThreadsAreSerialised();
  void sensorMessagesFromForeignThreadsAreForwarded();
  void takeSensorWaitsForTheRunningCommandAndDetaches();
  void destructionWithPendingCommandsDropsThem();
  void destructionWhileAStreamRunsStopsCleanly();
  void destructionWhileTheDriverReportsFromAnotherThread();

  // control and station
  void controlDeliversThroughTheExecutorInOrder();
  void tasksStillQueuedAfterDestructionAreDropped();
  void anExecutorCanBeDestroyedRightAfterItWasReplaced();
  void stationCommandMethodsReachTheSensor();
  void stationObserverIsToldAndForgotten();
  void stationsOfAJobUseTheJobsExecutor();
  void stationReportsActiveStationAndSensorChangesToTheJob();
};

void SensorWorkerTest::initTestCase() {
  ChooseLALib::setLinearAlgebra(ChooseLALib::LinearAlgebraLibrary::Armadillo);
}

void SensorWorkerTest::commandsRunOnTheWorkerThreadInFifoOrder() {
  RecordingObserver results;
  auto* sensor = new FakeSensor;
  {
    SensorWorker worker(results);
    worker.setSensor(sensor);
    worker.connectSensor();
    worker.initialize();
    worker.selfDefinedAction("a");
    worker.selfDefinedAction("b");
    worker.search();
    worker.disconnectSensor();
    barrier(worker);

    const std::vector<Event> calls = sensor->calls.all();
    QStringList names;
    for (const Event& e : calls) {
      names << e.what;
      QCOMPARE(e.thread, worker.workerThreadId());
      QVERIFY(e.thread != std::this_thread::get_id());
    }
    QCOMPARE(names, (QStringList{"connect", "accept", "action:a", "action:b",
                                 "search", "disconnect"}));
    QVERIFY(!sensor->overlapped);

    // results in the same order, on the worker thread as well
    const std::vector<Event> events = results.log.all();
    QCOMPARE(events.size(), size_t{6});
    for (const Event& e : events) {
      QCOMPARE(e.thread, worker.workerThreadId());
    }
    QCOMPARE(
        events.front().what,
        QString("finished:1:%1:0").arg(SensorWorkerMessage::SENSOR_CONNECTED));
  }
  delete sensor;
}

void SensorWorkerTest::aSlowCommandDoesNotBlockTheCaller() {
  RecordingObserver results;
  FakeSensor sensor;
  sensor.connected = true;
  sensor.measureDelayMs = 400;
  SensorWorker worker(results);
  worker.setSensor(&sensor);

  const auto start = std::chrono::steady_clock::now();
  worker.measure(1, MeasurementConfig());
  worker.measure(2, MeasurementConfig());
  // getters answer while the first measurement is running
  while (sensor.calls.count("measure") == 0) {
    std::this_thread::yield();
  }
  QVERIFY(worker.getIsBusy());
  QVERIFY(worker.getIsSensorConnected());
  QVERIFY(worker.getSupportedReadingTypes().isEmpty());
  const auto callerTime = std::chrono::steady_clock::now() - start;
  QVERIFY2(callerTime < 300ms, "the caller was blocked by the command");

  QVERIFY(results.log.waitForCount("measurementFinished", 2));
  QCOMPARE(results.log.count("measurementFinished"), 2);
}

void SensorWorkerTest::measurementFlow() {
  RecordingObserver results;
  FakeSensor sensor;
  sensor.connected = true;
  SensorWorker worker(results);
  worker.setSensor(&sensor);

  worker.measure(7, MeasurementConfig());
  QVERIFY(results.log.waitForCount("done", 1));
  QCOMPARE(results.log.names(),
           (QStringList{
               QString("finished:1:%1:%2")
                   .arg(SensorWorkerMessage::MEASUREMENT_FINISHED)
                   .arg(static_cast<int>(SensorAction::eSensorActionMeasure)),
               "measurementFinished:7:2", "done:1"}));
  QVERIFY(!results.overlapped);
}

void SensorWorkerTest::measurementFailureReportsDoneLast() {
  RecordingObserver results;
  FakeSensor sensor;  // not connected
  SensorWorker worker(results);
  worker.setSensor(&sensor);

  worker.measure(7, MeasurementConfig());
  QVERIFY(results.log.waitForCount("done", 1));
  QCOMPARE(results.log.names(),
           (QStringList{
               QString("finished:0:%1:%2")
                   .arg(SensorWorkerMessage::SENSOR_IS_NOT_CONNECTED)
                   .arg(static_cast<int>(SensorAction::eSensorActionMeasure)),
               "done:0"}));
}

void SensorWorkerTest::commandsWithoutSensorReportAnError() {
  RecordingObserver results;
  SensorWorker worker(results);
  worker.connectSensor();
  worker.startReadingStream();
  barrier(worker);
  QCOMPARE(
      results.log.names(),
      (QStringList{
          QString("finished:0:%1:0")
              .arg(SensorWorkerMessage::NO_SENSOR_INSTANCE),
          QString("message:%1").arg(SensorWorkerMessage::NO_SENSOR_INSTANCE)}));
}

void SensorWorkerTest::readingStreamYieldsReadingsThenStops() {
  RecordingObserver results;
  FakeSensor sensor;
  sensor.connected = true;
  sensor.streamDelayMs = 1;
  SensorWorker worker(results);
  worker.setSensor(&sensor);
  worker.setStreamFormat(ReadingTypes::eCartesianReading);

  worker.startReadingStream();
  worker.startReadingStream();  // already running: no second loop
  QVERIFY(results.log.waitForCount("reading", 5));

  worker.stopReadingStream();
  barrier(worker);
  const int afterStop = results.log.count("reading");
  const int driverCalls = sensor.calls.count("stream");
  std::this_thread::sleep_for(60ms);
  barrier(worker);
  QCOMPARE(results.log.count("reading"), afterStop);
  QCOMPARE(sensor.calls.count("stream"), driverCalls);
  QVERIFY(!sensor.overlapped);

  // the readings arrived in order, each reported once
  const QStringList names = results.log.names();
  for (int i = 0; i < names.size(); ++i) {
    QCOMPARE(names.at(i), QString("reading:%1").arg(i + 1));
  }
}

void SensorWorkerTest::aRestartedStreamKeepsRunning() {
  RecordingObserver results;
  FakeSensor sensor;
  sensor.connected = true;
  sensor.streamDelayMs = 1;
  SensorWorker worker(results);
  worker.setSensor(&sensor);

  worker.startReadingStream();
  QVERIFY(results.log.waitForCount("reading", 3));
  worker.stopReadingStream();
  worker.startReadingStream();  // the old loop's iteration may still be queued
  const int before = results.log.count("reading");
  QVERIFY(results.log.waitForCount("reading", before + 5));
  worker.stopReadingStream();
  barrier(worker);

  // exactly one loop ran at any time: the numbers are strictly consecutive
  const QStringList names = results.log.names();
  for (int i = 0; i < names.size(); ++i) {
    QCOMPARE(names.at(i), QString("reading:%1").arg(i + 1));
  }
}

void SensorWorkerTest::commandsAreInterleavedWithARunningStream() {
  RecordingObserver results;
  FakeSensor sensor;
  sensor.connected = true;
  sensor.streamDelayMs = 1;
  SensorWorker worker(results);
  worker.setSensor(&sensor);

  worker.startReadingStream();
  QVERIFY(results.log.waitForCount("reading", 3));
  worker.home();
  QVERIFY(results.log.waitForCount("finished", 1));  // not starved
  const int readingsAtHome = results.log.count("reading");
  QVERIFY(results.log.waitForCount("reading", readingsAtHome + 3));  // goes on
  worker.stopReadingStream();
  barrier(worker);
}

void SensorWorkerTest::statusStreamReportsStatus() {
  RecordingObserver results;
  FakeSensor sensor;
  sensor.connected = true;
  SensorWorker worker(results);
  worker.setSensor(&sensor);

  worker.startStatusMonitoringStream();
  QVERIFY(results.log.waitForCount("status", 3));
  worker.stopStatusMonitoringStream();
  barrier(worker);
  const int n = results.log.count("status");
  barrier(worker);
  QCOMPARE(results.log.count("status"), n);
}

void SensorWorkerTest::connectionMonitoringReportsLostAndReceived() {
  RecordingObserver results;
  FakeSensor sensor;
  SensorWorker worker(results);
  worker.setSensor(&sensor);

  worker.startConnectionMonitoringStream();
  sensor.connected = true;
  QVERIFY(results.log.waitForCount("received", 1));
  sensor.connected = false;
  QVERIFY(results.log.waitForCount("lost", 1));
  sensor.connected = true;
  QVERIFY(results.log.waitForCount("received", 2));
  worker.stopConnectionMonitoringStream();
  barrier(worker);

  QCOMPARE(results.log.names(), (QStringList{"received", "lost", "received"}));
}

void SensorWorkerTest::asyncSensorReportsFromForeignThreadsAreSerialised() {
  RecordingObserver results;
  FakeSensor sensor;
  sensor.async = true;
  SensorWorker worker(results);
  worker.setSensor(&sensor);

  constexpr int threads = 4;
  constexpr int perThread = 100;
  std::vector<std::thread> reporters;
  for (int t = 0; t < threads; ++t) {
    reporters.emplace_back([&sensor, t] {
      for (int i = 0; i < perThread; ++i) {
        QVariantMap m;
        m["n"] = t * 1000 + i;
        sensor.asyncStreamResult(m);
        if (i % 25 == 0) {
          QJsonObject response;
          response.insert("result", "ok");
          sensor.asyncSensorResponse(response, SensorAction::eSensorActionMove);
        }
        if (i % 50 == 0) {
          QList<QPointer<Reading>> none;
          sensor.asyncMeasurementResult(t, none);
        }
      }
    });
  }
  for (std::thread& r : reporters) {
    r.join();
  }
  barrier(worker);

  // every callback ran on the worker thread and never two at once
  QVERIFY(!results.overlapped);
  QCOMPARE(results.log.count("reading"), threads * perThread);
  for (const Event& e : results.log.all()) {
    QCOMPARE(e.thread, worker.workerThreadId());
  }
  // a measurement without results is reported as a failed one
  QCOMPARE(results.log.count("done:0"), threads * 2);
  // per reporting thread the order is kept
  QMap<int, int> last;
  for (const QString& name : results.log.names()) {
    if (name.startsWith("reading:")) {
      const int n = name.mid(8).toInt();
      const int t = n / 1000;
      QVERIFY(!last.contains(t) || n > last.value(t));
      last[t] = n;
    }
  }
}

void SensorWorkerTest::sensorMessagesFromForeignThreadsAreForwarded() {
  RecordingObserver results;
  FakeSensor sensor;
  SensorWorker worker(results);
  worker.setSensor(&sensor);

  std::thread t([&sensor] {
    sensor.sensorMessage("hello", MessageTypes::eInformationMessage);
  });
  t.join();
  QCOMPARE(results.log.names(), (QStringList{"message:hello"}));

  // without a sink the report is dropped
  worker.takeSensor();
  sensor.sensorMessage("dropped", MessageTypes::eInformationMessage);
  QCOMPARE(results.log.count("message"), 1);
}

void SensorWorkerTest::takeSensorWaitsForTheRunningCommandAndDetaches() {
  RecordingObserver results;
  FakeSensor sensor;
  sensor.connectDelayMs = 200;
  SensorWorker worker(results);
  worker.setSensor(&sensor);
  QCOMPARE(sensor.getSensorSink(), &worker);

  worker.connectSensor();
  QVERIFY(sensor.calls.waitForCount("connect", 1));
  const QPointer<Sensor> taken = worker.takeSensor();
  // the command was complete when the sensor was handed back
  QVERIFY(sensor.connected);
  QCOMPARE(taken.data(), &sensor);
  QVERIFY(sensor.getSensorSink() == nullptr);
  QVERIFY(
      worker.getSensor().getSensorConfiguration().getSensorName().isEmpty());
  QVERIFY(!worker.getIsSensorConnected());
}

void SensorWorkerTest::destructionWithPendingCommandsDropsThem() {
  RecordingObserver results;
  FakeSensor sensor;
  sensor.connectDelayMs = 150;
  {
    SensorWorker worker(results);
    worker.setSensor(&sensor);
    worker.connectSensor();
    for (int i = 0; i < 50; ++i) {
      worker.selfDefinedAction(QString::number(i));
    }
    QVERIFY(sensor.calls.waitForCount("connect", 1));
  }  // destroyed while connect runs and 50 commands wait
  const int executed = sensor.calls.count("action");
  QVERIFY(executed < 50);
  QVERIFY(sensor.getSensorSink() == nullptr);
  std::this_thread::sleep_for(30ms);
  QCOMPARE(sensor.calls.count("action"), executed);  // nothing runs afterwards
}

void SensorWorkerTest::destructionWhileAStreamRunsStopsCleanly() {
  RecordingObserver results;
  FakeSensor sensor;
  sensor.connected = true;
  {
    SensorWorker worker(results);
    worker.setSensor(&sensor);
    worker.startReadingStream();
    worker.startConnectionMonitoringStream();
    worker.startStatusMonitoringStream();
    QVERIFY(results.log.waitForCount("reading", 3));
    QVERIFY(results.log.waitForCount("status", 3));
  }
  const int streamCalls = sensor.calls.count("stream");
  const int statusCalls = sensor.calls.count("status");
  std::this_thread::sleep_for(30ms);
  QCOMPARE(sensor.calls.count("stream"), streamCalls);
  QCOMPARE(sensor.calls.count("status"), statusCalls);
  QVERIFY(!sensor.overlapped);
}

void SensorWorkerTest::destructionWhileTheDriverReportsFromAnotherThread() {
  RecordingObserver results;
  FakeSensor sensor;
  sensor.async = true;
  std::atomic<bool> stop{false};
  std::thread reporter([&] {
    while (!stop) {
      sensor.sensorMessage("m", MessageTypes::eInformationMessage);
      QVariantMap m;
      m["n"] = 1;
      sensor.asyncStreamResult(m);
    }
  });
  for (int round = 0; round < 20; ++round) {
    SensorWorker worker(results);
    worker.setSensor(&sensor);
    worker.startReadingStream();
    std::this_thread::sleep_for(2ms);
  }  // each worker dies while the driver is reporting
  stop = true;
  reporter.join();
  QVERIFY(sensor.getSensorSink() == nullptr);
}

void SensorWorkerTest::controlDeliversThroughTheExecutorInOrder() {
  RecordingObserver results;
  ManualExecutor executor;
  FakeSensor sensor;
  sensor.connected = true;
  SensorControl control(&results, &executor);
  control.setSensor(&sensor);

  control.measure(3, MeasurementConfig());
  control.home();
  QVERIFY(executor.waitForPending(4));  // finished, measurementFinished, done
                                        // + home's finished
  // nothing was delivered yet: the executor decides when and where
  QCOMPARE(results.log.names().size(), 0);
  // posted from the worker thread
  for (const std::thread::id& id : executor.posterThreads()) {
    QCOMPARE(id, control.getWorker().workerThreadId());
  }

  QCOMPARE(executor.drain(), size_t{4});
  QCOMPARE(results.log.names().size(), 4);
  // delivered here, in the order the worker produced them
  for (const Event& e : results.log.all()) {
    QCOMPARE(e.thread, std::this_thread::get_id());
  }
  QVERIFY(results.log.names().at(0).startsWith("finished:1:"));
  QCOMPARE(results.log.names().at(1), QString("measurementFinished:3:2"));
  QCOMPARE(results.log.names().at(2), QString("done:1"));
  QVERIFY(results.log.names().at(3).startsWith("finished:1:"));
}

void SensorWorkerTest::tasksStillQueuedAfterDestructionAreDropped() {
  RecordingObserver results;
  ManualExecutor executor;
  FakeSensor sensor;
  sensor.connected = true;
  {
    SensorControl control(&results, &executor);
    control.setSensor(&sensor);
    control.measure(1, MeasurementConfig());
    QVERIFY(executor.waitForPending(3));
  }  // the control is gone, its results are still queued in the executor
  QCOMPARE(executor.drain(), size_t{3});
  QCOMPARE(results.log.names().size(), 0);
}

void SensorWorkerTest::anExecutorCanBeDestroyedRightAfterItWasReplaced() {
  RecordingObserver results;
  FakeSensor sensor;
  sensor.connected = true;
  SensorControl control(&results, nullptr);
  control.setSensor(&sensor);
  for (int round = 0; round < 50; ++round) {
    auto executor = std::make_unique<ManualExecutor>();
    control.setExecutor(executor.get());
    control.startStatusMonitoringStream();  // posts continuously
    QVERIFY(executor->waitForPending(1));
    control.stopStatusMonitoringStream();
    control.setExecutor(nullptr);
    executor.reset();  // the worker must not post to it any more
  }
  barrier(control.getWorker());
}

void SensorWorkerTest::stationCommandMethodsReachTheSensor() {
  RecordingObserver results;
  FakeSensor sensor;
  Station station("S1");
  station.setStationObserver(&results);
  station.setActiveStationState(true);
  station.setSensor(&sensor);
  QVERIFY(station.getIsSensorSet());

  station.connectSensor();
  station.initialize();
  station.home();
  station.toggleSight();
  station.compensation();
  station.motorState();
  station.search();
  station.selfDefinedAction("x");
  station.finishMeasurement();
  station.move(1.0, 2.0, 3.0, false);
  station.move(0.1, 0.2, 1.0, true, false);
  station.measure(4, MeasurementConfig());
  station.disconnectSensor();
  station.startReadingStream();
  QVERIFY(results.log.waitForCount("reading", 2));
  station.stopReadingStream();
  station.startStatusMonitoringStream();
  QVERIFY(results.log.waitForCount("status", 1));
  station.stopStatusMonitoringStream();
  station.startConnectionMonitoringStream();
  station.stopConnectionMonitoringStream();
  barrier(station.getSensorControl()->getWorker());

  const QStringList calls = sensor.calls.names();
  QVERIFY(calls.contains("connect"));
  QVERIFY(calls.contains("action:stopMeasure"));
  QVERIFY(calls.contains("action:x"));
  QVERIFY(calls.contains("search"));
  QVERIFY(calls.contains("measure"));
  QVERIFY(calls.contains("disconnect"));
  // initialize, home, toggleSight, compensation, motorState, two moves
  QCOMPARE(calls.count("accept"), 7);
  QVERIFY(calls.indexOf("connect") < calls.indexOf("measure"));
  QVERIFY(calls.indexOf("measure") < calls.indexOf("disconnect"));

  // the measurement arrived at the station's observer
  QVERIFY(results.log.names().contains("measurementFinished:4:2"));
  QVERIFY(results.log.names().contains("done:1"));
  QVERIFY(!results.overlapped);
}

void SensorWorkerTest::stationObserverIsToldAndForgotten() {
  RecordingObserver results;
  ManualExecutor executor;
  auto* station = new Station("S1");
  station->setExecutor(&executor);
  station->setStationObserver(&results);
  station->setActiveStationState(true);
  QCOMPARE(station->getStationObserver(), &results);

  FakeSensor sensor;
  sensor.connected = true;
  station->setSensor(&sensor);
  station->measure(1, MeasurementConfig());
  QVERIFY(executor.waitForPending(3));

  delete station;  // results still queued in the executor
  QCOMPARE(results.destroyedCount.load(), 1);
  QCOMPARE(executor.drain(), size_t{3});
  QCOMPARE(results.log.names().size(), 0);  // dropped: no dangling station
}

void SensorWorkerTest::stationsOfAJobUseTheJobsExecutor() {
  ManualExecutor executor;
  RecordingObserver results;
  FakeSensor sensor;
  sensor.connected = true;
  OiJob job;

  // a station that was added before the executor was set ...
  QPointer<Station> early = new Station("S1");
  QPointer<FeatureWrapper> earlyFeature = new FeatureWrapper();
  earlyFeature->setStation(early);
  job.addFeature(earlyFeature);
  job.setExecutor(&executor);
  QCOMPARE(job.getExecutor(), &executor);

  // ... and one added afterwards
  QPointer<Station> late = new Station("S2");
  QPointer<FeatureWrapper> lateFeature = new FeatureWrapper();
  lateFeature->setStation(late);
  job.addFeature(lateFeature);

  for (const QPointer<Station>& station : {early, late}) {
    QCOMPARE(station->getSensorControl()->getExecutor(), &executor);
  }

  late->setStationObserver(&results);
  late->setActiveStationState(true);
  late->setSensor(&sensor);
  late->measure(1, MeasurementConfig());
  QVERIFY(executor.waitForPending(3));
  QCOMPARE(results.log.names().size(), 0);
  executor.drain();
  QVERIFY(results.log.names().contains("done:1"));

  // the job does not stop its stations' workers; hand the sensor back
  // before it goes out of scope
  late->takeSensor();

  // a station outside any job delivers inline
  Station alone("S3");
  QVERIFY(dynamic_cast<InlineExecutor*>(
              alone.getSensorControl()->getExecutor()) != nullptr);
}

void SensorWorkerTest::stationReportsActiveStationAndSensorChangesToTheJob() {
  OiJob job;
  RecordingJobObserver jobObserver;
  jobObserver.attach(&job);

  QPointer<Station> station = new Station("S1");
  QPointer<FeatureWrapper> feature = new FeatureWrapper();
  feature->setStation(station);
  job.addFeature(feature);

  station->setActiveStationState(true);
  QCOMPARE(job.getActiveStation().data(), station.data());
  QVERIFY(jobObserver.count("activeStationChanged") >= 1);

  FakeSensor sensor;
  station->setSensor(&sensor);
  QCOMPARE(jobObserver.count("stationSensorChanged"), 1);
  QPointer<Sensor> taken = station->takeSensor();
  QCOMPARE(jobObserver.count("stationSensorChanged"), 2);
  QCOMPARE(taken.data(), &sensor);
}

QTEST_MAIN(SensorWorkerTest)
#include "tst_sensorworker.moc"
