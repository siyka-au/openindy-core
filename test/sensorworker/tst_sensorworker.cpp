// Tests of the sensor path: SensorWorker (command queue, streams, shutdown),
// SensorControl (delivery through the Executor) and Station (command methods,
// StationObserver). They run with plain std::thread helpers and are meant to
// be run under ThreadSanitizer as well.
#include <atomic>
#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <functional>
#include <future>
#include <iterator>
#include <map>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "catch_compat.h"
#include "chooselalib.h"
#include "containers.h"
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
  std::string what;
  std::thread::id thread;
};

class Log {
 public:
  void add(const std::string& what) {
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

  [[nodiscard]] std::vector<std::string> names() const {
    std::vector<std::string> result;
    for (const Event& e : this->all()) {
      result.push_back(e.what);
    }
    return result;
  }

  [[nodiscard]] int count(const std::string& prefix) const {
    int n = 0;
    for (const Event& e : this->all()) {
      if (e.what == prefix || e.what.starts_with(prefix + ":")) {
        ++n;
      }
    }
    return n;
  }

  bool waitForCount(const std::string& prefix, int n, int timeoutMs = 5000) {
    std::unique_lock lock(this->mutex);
    return this->changed.wait_for(
        lock, std::chrono::milliseconds(timeoutMs), [&] {
          int c = 0;
          for (const Event& e : this->events) {
            if (e.what == prefix || e.what.starts_with(prefix + ":")) {
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
  // shared so a test can keep reading the log after the worker (which owns
  // the sensor) is gone
  std::shared_ptr<Log> callsBox = std::make_shared<Log>();
  Log& calls = *callsBox;
  std::atomic<bool> stopReporter{false};
  std::thread reporter;
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

  explicit FakeSensor() : Sensor() {}
  ~FakeSensor() override {
    this->stopReporter = true;
    if (this->reporter.joinable()) {
      this->reporter.join();
    }
  }

  // a driver thread that reports on its own until the driver is destroyed
  void startReporter() {
    this->reporter = std::thread([this] {
      while (!this->stopReporter) {
        this->sensorMessage("m", MessageTypes::eInformationMessage);
        oi::ValueMap m;
        m["n"] = 1;
        this->asyncStreamResult(m);
      }
    });
  }

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
  std::map<std::string, std::string> getSensorStatus() override {
    this->calls.add("status");
    return {{"state", "ok"}};
  }
  std::vector<std::unique_ptr<Reading>> measure(
      const MeasurementConfig&) override {
    const Guard g(*this, "measure");
    std::this_thread::sleep_for(
        std::chrono::milliseconds(this->measureDelayMs));
    std::vector<std::unique_ptr<Reading>> result;
    for (int i = 0; i < this->readings; ++i) {
      ReadingCartesian r;
      r.isValid = true;
      r.xyz.setAt(0, i);
      result.push_back(std::make_unique<Reading>(r));
    }
    return result;
  }
  oi::ValueMap readingStream(const ReadingTypes&) override {
    const Guard g(*this, "stream");
    std::this_thread::sleep_for(std::chrono::milliseconds(this->streamDelayMs));
    oi::ValueMap m;
    m["n"] = ++this->streamCounter;
    return m;
  }
  bool accept(const SensorFunctions&, const SensorAttributes&) override {
    const Guard g(*this, "accept");
    return true;
  }
  bool doSelfDefinedAction(const std::string& action) override {
    const Guard g(*this, "action:" + action);
    return true;
  }
  bool search() override {
    const Guard g(*this, "search");
    return true;
  }
  oi::ValueMap performAsyncSensorCommand(const oi::ValueMap& request) override {
    this->calls.add("async:" +
                    oi::value(request, std::string("method")).toString());
    return {};
  }

 private:
  struct Guard {
    FakeSensor& s;
    Guard(FakeSensor& sensor, const std::string& name) : s(sensor) {
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
  void onRealTimeReading(const oi::ValueMap& r) override {
    const Scope s(*this);
    this->log.add(std::format("reading:{}", oi::value(r, "n").toInt()));
  }
  void onRealTimeStatus(const std::map<std::string, std::string>&) override {
    const Scope s(*this);
    this->log.add("status");
  }
  void onConnectionLost() override { this->log.add("lost"); }
  void onConnectionReceived() override { this->log.add("received"); }
  void onCommandFinished(bool success, const std::string& msg,
                         SensorAction action) override {
    const Scope s(*this);
    this->log.add(std::format("finished:{}:{}:{}", static_cast<int>(success),
                              msg, static_cast<int>(action)));
  }
  void onMeasurementFinished(
      int geomId, std::vector<std::unique_ptr<Reading>> readings) override {
    const Scope s(*this);
    this->log.add(
        std::format("measurementFinished:{}:{}", geomId, readings.size()));
  }
  void onMeasurementDone(bool success) override {
    const Scope s(*this);
    this->log.add(std::format("done:{}", static_cast<int>(success)));
  }
  void onSensorMessage(const std::string& msg, MessageTypes,
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

class SensorWorkerTest {
 public:
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
  auto own_sensor = std::make_unique<FakeSensor>();
  FakeSensor* sensor = own_sensor.get();
  {
    SensorWorker worker(results);
    worker.setSensor(std::move(own_sensor));
    worker.connectSensor();
    worker.initialize();
    worker.selfDefinedAction("a");
    worker.selfDefinedAction("b");
    worker.search();
    worker.disconnectSensor();
    barrier(worker);

    const std::vector<Event> calls = sensor->calls.all();
    std::vector<std::string> names;
    for (const Event& e : calls) {
      names.push_back(e.what);
      OI_COMPARE(e.thread, worker.workerThreadId());
      REQUIRE(e.thread != std::this_thread::get_id());
    }
    OI_COMPARE(names,
               (std::vector<std::string>{"connect", "accept", "action:a",
                                         "action:b", "search", "disconnect"}));
    REQUIRE(!sensor->overlapped);

    // results in the same order, on the worker thread as well
    const std::vector<Event> events = results.log.all();
    OI_COMPARE(events.size(), size_t{6});
    for (const Event& e : events) {
      OI_COMPARE(e.thread, worker.workerThreadId());
    }
    OI_COMPARE(
        events.front().what,
        std::format("finished:1:{}:0", SensorWorkerMessage::SENSOR_CONNECTED));
  }
}

void SensorWorkerTest::aSlowCommandDoesNotBlockTheCaller() {
  RecordingObserver results;
  auto own_sensor = std::make_unique<FakeSensor>();
  FakeSensor& sensor = *own_sensor;
  sensor.connected = true;
  sensor.measureDelayMs = 400;
  SensorWorker worker(results);
  worker.setSensor(std::move(own_sensor));

  const auto start = std::chrono::steady_clock::now();
  worker.measure(1, MeasurementConfig());
  worker.measure(2, MeasurementConfig());
  // getters answer while the first measurement is running
  while (sensor.calls.count("measure") == 0) {
    std::this_thread::yield();
  }
  REQUIRE(worker.getIsBusy());
  REQUIRE(worker.getIsSensorConnected());
  REQUIRE(worker.getSupportedReadingTypes().empty());
  const auto callerTime = std::chrono::steady_clock::now() - start;
  OI_REQUIRE2(callerTime < 300ms, "the caller was blocked by the command");

  REQUIRE(results.log.waitForCount("measurementFinished", 2));
  OI_COMPARE(results.log.count("measurementFinished"), 2);
}

void SensorWorkerTest::measurementFlow() {
  RecordingObserver results;
  auto own_sensor = std::make_unique<FakeSensor>();
  FakeSensor& sensor = *own_sensor;
  sensor.connected = true;
  SensorWorker worker(results);
  worker.setSensor(std::move(own_sensor));

  worker.measure(7, MeasurementConfig());
  REQUIRE(results.log.waitForCount("done", 1));
  OI_COMPARE(
      results.log.names(),
      (std::vector<std::string>{
          std::format("finished:1:{}:{}",
                      SensorWorkerMessage::MEASUREMENT_FINISHED,
                      static_cast<int>(SensorAction::eSensorActionMeasure)),
          "measurementFinished:7:2", "done:1"}));
  REQUIRE(!results.overlapped);
}

void SensorWorkerTest::measurementFailureReportsDoneLast() {
  RecordingObserver results;
  auto own_sensor = std::make_unique<FakeSensor>();
  FakeSensor& sensor = *own_sensor;  // not connected
  SensorWorker worker(results);
  worker.setSensor(std::move(own_sensor));

  worker.measure(7, MeasurementConfig());
  REQUIRE(results.log.waitForCount("done", 1));
  OI_COMPARE(
      results.log.names(),
      (std::vector<std::string>{
          std::format("finished:0:{}:{}",
                      SensorWorkerMessage::SENSOR_IS_NOT_CONNECTED,
                      static_cast<int>(SensorAction::eSensorActionMeasure)),
          "done:0"}));
}

void SensorWorkerTest::commandsWithoutSensorReportAnError() {
  RecordingObserver results;
  SensorWorker worker(results);
  worker.connectSensor();
  worker.startReadingStream();
  barrier(worker);
  OI_COMPARE(
      results.log.names(),
      (std::vector<std::string>{
          std::format("finished:0:{}:0",
                      SensorWorkerMessage::NO_SENSOR_INSTANCE),
          std::format("message:{}", SensorWorkerMessage::NO_SENSOR_INSTANCE)}));
}

void SensorWorkerTest::readingStreamYieldsReadingsThenStops() {
  RecordingObserver results;
  auto own_sensor = std::make_unique<FakeSensor>();
  FakeSensor& sensor = *own_sensor;
  sensor.connected = true;
  sensor.streamDelayMs = 1;
  SensorWorker worker(results);
  worker.setSensor(std::move(own_sensor));
  worker.setStreamFormat(ReadingTypes::eCartesianReading);

  worker.startReadingStream();
  worker.startReadingStream();  // already running: no second loop
  REQUIRE(results.log.waitForCount("reading", 5));

  worker.stopReadingStream();
  barrier(worker);
  const int afterStop = results.log.count("reading");
  const int driverCalls = sensor.calls.count("stream");
  std::this_thread::sleep_for(60ms);
  barrier(worker);
  OI_COMPARE(results.log.count("reading"), afterStop);
  OI_COMPARE(sensor.calls.count("stream"), driverCalls);
  REQUIRE(!sensor.overlapped);

  // the readings arrived in order, each reported once
  const std::vector<std::string> names = results.log.names();
  for (int i = 0; i < std::ssize(names); ++i) {
    OI_COMPARE(names.at(i), std::format("reading:{}", i + 1));
  }
}

void SensorWorkerTest::aRestartedStreamKeepsRunning() {
  RecordingObserver results;
  auto own_sensor = std::make_unique<FakeSensor>();
  FakeSensor& sensor = *own_sensor;
  sensor.connected = true;
  sensor.streamDelayMs = 1;
  SensorWorker worker(results);
  worker.setSensor(std::move(own_sensor));

  worker.startReadingStream();
  REQUIRE(results.log.waitForCount("reading", 3));
  worker.stopReadingStream();
  worker.startReadingStream();  // the old loop's iteration may still be queued
  const int before = results.log.count("reading");
  REQUIRE(results.log.waitForCount("reading", before + 5));
  worker.stopReadingStream();
  barrier(worker);

  // exactly one loop ran at any time: the numbers are strictly consecutive
  const std::vector<std::string> names = results.log.names();
  for (int i = 0; i < std::ssize(names); ++i) {
    OI_COMPARE(names.at(i), std::format("reading:{}", i + 1));
  }
}

void SensorWorkerTest::commandsAreInterleavedWithARunningStream() {
  RecordingObserver results;
  auto own_sensor = std::make_unique<FakeSensor>();
  FakeSensor& sensor = *own_sensor;
  sensor.connected = true;
  sensor.streamDelayMs = 1;
  SensorWorker worker(results);
  worker.setSensor(std::move(own_sensor));

  worker.startReadingStream();
  REQUIRE(results.log.waitForCount("reading", 3));
  worker.home();
  REQUIRE(results.log.waitForCount("finished", 1));  // not starved
  const int readingsAtHome = results.log.count("reading");
  REQUIRE(results.log.waitForCount("reading", readingsAtHome + 3));  // goes on
  worker.stopReadingStream();
  barrier(worker);
}

void SensorWorkerTest::statusStreamReportsStatus() {
  RecordingObserver results;
  auto own_sensor = std::make_unique<FakeSensor>();
  FakeSensor& sensor = *own_sensor;
  sensor.connected = true;
  SensorWorker worker(results);
  worker.setSensor(std::move(own_sensor));

  worker.startStatusMonitoringStream();
  REQUIRE(results.log.waitForCount("status", 3));
  worker.stopStatusMonitoringStream();
  barrier(worker);
  const int n = results.log.count("status");
  barrier(worker);
  OI_COMPARE(results.log.count("status"), n);
}

void SensorWorkerTest::connectionMonitoringReportsLostAndReceived() {
  RecordingObserver results;
  auto own_sensor = std::make_unique<FakeSensor>();
  FakeSensor& sensor = *own_sensor;
  SensorWorker worker(results);
  worker.setSensor(std::move(own_sensor));

  worker.startConnectionMonitoringStream();
  sensor.connected = true;
  REQUIRE(results.log.waitForCount("received", 1));
  sensor.connected = false;
  REQUIRE(results.log.waitForCount("lost", 1));
  sensor.connected = true;
  REQUIRE(results.log.waitForCount("received", 2));
  worker.stopConnectionMonitoringStream();
  barrier(worker);

  OI_COMPARE(results.log.names(),
             (std::vector<std::string>{"received", "lost", "received"}));
}

void SensorWorkerTest::asyncSensorReportsFromForeignThreadsAreSerialised() {
  RecordingObserver results;
  auto own_sensor = std::make_unique<FakeSensor>();
  FakeSensor& sensor = *own_sensor;
  sensor.async = true;
  SensorWorker worker(results);
  worker.setSensor(std::move(own_sensor));

  constexpr int threads = 4;
  constexpr int perThread = 100;
  std::vector<std::thread> reporters;
  for (int t = 0; t < threads; ++t) {
    reporters.emplace_back([&sensor, t] {
      for (int i = 0; i < perThread; ++i) {
        oi::ValueMap m;
        m["n"] = t * 1000 + i;
        sensor.asyncStreamResult(m);
        if (i % 25 == 0) {
          oi::ValueMap response;
          response.insert_or_assign("result", "ok");
          sensor.asyncSensorResponse(response, SensorAction::eSensorActionMove);
        }
        if (i % 50 == 0) {
          sensor.asyncMeasurementResult(t, {});
        }
      }
    });
  }
  for (std::thread& r : reporters) {
    r.join();
  }
  barrier(worker);

  // every callback ran on the worker thread and never two at once
  REQUIRE(!results.overlapped);
  OI_COMPARE(results.log.count("reading"), threads * perThread);
  for (const Event& e : results.log.all()) {
    OI_COMPARE(e.thread, worker.workerThreadId());
  }
  // a measurement without results is reported as a failed one
  OI_COMPARE(results.log.count("done:0"), threads * 2);
  // per reporting thread the order is kept
  std::map<int, int> last;
  for (const std::string& name : results.log.names()) {
    if (name.starts_with("reading:")) {
      const int n = oi::toInt(name.substr(8)).value_or(0);
      const int t = n / 1000;
      REQUIRE((!last.contains(t) || n > oi::value(last, t)));
      last[t] = n;
    }
  }
}

void SensorWorkerTest::sensorMessagesFromForeignThreadsAreForwarded() {
  RecordingObserver results;
  auto own_sensor = std::make_unique<FakeSensor>();
  FakeSensor& sensor = *own_sensor;
  SensorWorker worker(results);
  worker.setSensor(std::move(own_sensor));

  std::thread t([&sensor] {
    sensor.sensorMessage("hello", MessageTypes::eInformationMessage);
  });
  t.join();
  OI_COMPARE(results.log.names(), (std::vector<std::string>{"message:hello"}));

  // without a sink the report is dropped
  const std::unique_ptr<Sensor> taken = worker.takeSensor();
  sensor.sensorMessage("dropped", MessageTypes::eInformationMessage);
  OI_COMPARE(results.log.count("message"), 1);
}

void SensorWorkerTest::takeSensorWaitsForTheRunningCommandAndDetaches() {
  RecordingObserver results;
  auto own_sensor = std::make_unique<FakeSensor>();
  FakeSensor& sensor = *own_sensor;
  sensor.connectDelayMs = 200;
  SensorWorker worker(results);
  worker.setSensor(std::move(own_sensor));
  OI_COMPARE(sensor.getSensorSink(), &worker);

  worker.connectSensor();
  REQUIRE(sensor.calls.waitForCount("connect", 1));
  const std::unique_ptr<Sensor> taken = worker.takeSensor();
  // the command was complete when the sensor was handed back
  REQUIRE(sensor.connected);
  OI_COMPARE(taken.get(), &sensor);
  REQUIRE(sensor.getSensorSink() == nullptr);
  REQUIRE(worker.getSensor().getSensorConfiguration().getSensorName().empty());
  REQUIRE(!worker.getIsSensorConnected());
}

void SensorWorkerTest::destructionWithPendingCommandsDropsThem() {
  RecordingObserver results;
  auto own_sensor = std::make_unique<FakeSensor>();
  FakeSensor& sensor = *own_sensor;
  sensor.connectDelayMs = 150;
  const std::shared_ptr<Log> calls = sensor.callsBox;
  {
    SensorWorker worker(results);
    worker.setSensor(std::move(own_sensor));
    worker.connectSensor();
    for (int i = 0; i < 50; ++i) {
      worker.selfDefinedAction(oi::number(i));
    }
    REQUIRE(sensor.calls.waitForCount("connect", 1));
  }  // destroyed while connect runs and 50 commands wait; frees the sensor
  const int executed = calls->count("action");
  REQUIRE(executed < 50);
  std::this_thread::sleep_for(30ms);
  OI_COMPARE(calls->count("action"), executed);  // nothing runs afterwards
}

void SensorWorkerTest::destructionWhileAStreamRunsStopsCleanly() {
  RecordingObserver results;
  auto own_sensor = std::make_unique<FakeSensor>();
  FakeSensor& sensor = *own_sensor;
  sensor.connected = true;
  const std::shared_ptr<Log> calls = sensor.callsBox;
  {
    SensorWorker worker(results);
    worker.setSensor(std::move(own_sensor));
    worker.startReadingStream();
    worker.startConnectionMonitoringStream();
    worker.startStatusMonitoringStream();
    REQUIRE(results.log.waitForCount("reading", 3));
    REQUIRE(results.log.waitForCount("status", 3));
    REQUIRE(!sensor.overlapped);
  }  // frees the sensor
  const int streamCalls = calls->count("stream");
  const int statusCalls = calls->count("status");
  std::this_thread::sleep_for(30ms);
  OI_COMPARE(calls->count("stream"), streamCalls);
  OI_COMPARE(calls->count("status"), statusCalls);
}

void SensorWorkerTest::destructionWhileTheDriverReportsFromAnotherThread() {
  RecordingObserver results;
  for (int round = 0; round < 20; ++round) {
    auto own_sensor = std::make_unique<FakeSensor>();
    own_sensor->async = true;
    own_sensor->startReporter();
    SensorWorker worker(results);
    worker.setSensor(std::move(own_sensor));
    worker.startReadingStream();
    std::this_thread::sleep_for(2ms);
  }  // each worker dies while the driver is reporting; the driver (which owns
     // its reporting thread) is freed with it
}

void SensorWorkerTest::controlDeliversThroughTheExecutorInOrder() {
  RecordingObserver results;
  ManualExecutor executor;
  auto own_sensor = std::make_unique<FakeSensor>();
  FakeSensor& sensor = *own_sensor;
  sensor.connected = true;
  SensorControl control(&results, &executor);
  control.setSensor(std::move(own_sensor));

  control.measure(3, MeasurementConfig());
  control.home();
  REQUIRE(executor.waitForPending(4));  // finished, measurementFinished, done
                                        // + home's finished
  // nothing was delivered yet: the executor decides when and where
  OI_COMPARE(results.log.names().size(), 0);
  // posted from the worker thread
  for (const std::thread::id& id : executor.posterThreads()) {
    OI_COMPARE(id, control.getWorker().workerThreadId());
  }

  OI_COMPARE(executor.drain(), size_t{4});
  OI_COMPARE(results.log.names().size(), 4);
  // delivered here, in the order the worker produced them
  for (const Event& e : results.log.all()) {
    OI_COMPARE(e.thread, std::this_thread::get_id());
  }
  REQUIRE(results.log.names().at(0).starts_with("finished:1:"));
  OI_COMPARE(results.log.names().at(1), std::string("measurementFinished:3:2"));
  OI_COMPARE(results.log.names().at(2), std::string("done:1"));
  REQUIRE(results.log.names().at(3).starts_with("finished:1:"));
}

void SensorWorkerTest::tasksStillQueuedAfterDestructionAreDropped() {
  RecordingObserver results;
  ManualExecutor executor;
  auto own_sensor = std::make_unique<FakeSensor>();
  FakeSensor& sensor = *own_sensor;
  sensor.connected = true;
  {
    SensorControl control(&results, &executor);
    control.setSensor(std::move(own_sensor));
    control.measure(1, MeasurementConfig());
    REQUIRE(executor.waitForPending(3));
  }  // the control is gone, its results are still queued in the executor
  OI_COMPARE(executor.drain(), size_t{3});
  OI_COMPARE(results.log.names().size(), 0);
}

void SensorWorkerTest::anExecutorCanBeDestroyedRightAfterItWasReplaced() {
  RecordingObserver results;
  auto own_sensor = std::make_unique<FakeSensor>();
  FakeSensor& sensor = *own_sensor;
  sensor.connected = true;
  SensorControl control(&results, nullptr);
  control.setSensor(std::move(own_sensor));
  for (int round = 0; round < 50; ++round) {
    auto executor = std::make_unique<ManualExecutor>();
    control.setExecutor(executor.get());
    control.startStatusMonitoringStream();  // posts continuously
    REQUIRE(executor->waitForPending(1));
    control.stopStatusMonitoringStream();
    control.setExecutor(nullptr);
    executor.reset();  // the worker must not post to it any more
  }
  barrier(control.getWorker());
}

void SensorWorkerTest::stationCommandMethodsReachTheSensor() {
  RecordingObserver results;
  auto own_sensor = std::make_unique<FakeSensor>();
  FakeSensor& sensor = *own_sensor;
  Station station("S1");
  station.setStationObserver(&results);
  station.setActiveStationState(true);
  station.setSensor(std::move(own_sensor));
  REQUIRE(station.getIsSensorSet());

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
  REQUIRE(results.log.waitForCount("reading", 2));
  station.stopReadingStream();
  station.startStatusMonitoringStream();
  REQUIRE(results.log.waitForCount("status", 1));
  station.stopStatusMonitoringStream();
  station.startConnectionMonitoringStream();
  station.stopConnectionMonitoringStream();
  barrier(station.getSensorControl()->getWorker());

  const std::vector<std::string> calls = sensor.calls.names();
  REQUIRE(oi::contains(calls, "connect"));
  REQUIRE(oi::contains(calls, "action:stopMeasure"));
  REQUIRE(oi::contains(calls, "action:x"));
  REQUIRE(oi::contains(calls, "search"));
  REQUIRE(oi::contains(calls, "measure"));
  REQUIRE(oi::contains(calls, "disconnect"));
  // initialize, home, toggleSight, compensation, motorState, two moves
  OI_COMPARE(std::count(calls.begin(), calls.end(), std::string("accept")), 7);
  REQUIRE(oi::indexOf(calls, "connect") < oi::indexOf(calls, "measure"));
  REQUIRE(oi::indexOf(calls, "measure") < oi::indexOf(calls, "disconnect"));

  // the measurement arrived at the station's observer
  REQUIRE(oi::contains(results.log.names(), "measurementFinished:4:2"));
  REQUIRE(oi::contains(results.log.names(), "done:1"));
  REQUIRE(!results.overlapped);
}

void SensorWorkerTest::stationObserverIsToldAndForgotten() {
  RecordingObserver results;
  ManualExecutor executor;
  auto* station = new Station("S1");
  station->setExecutor(&executor);
  station->setStationObserver(&results);
  station->setActiveStationState(true);
  OI_COMPARE(station->getStationObserver(), &results);

  auto own_sensor = std::make_unique<FakeSensor>();

  FakeSensor& sensor = *own_sensor;
  sensor.connected = true;
  station->setSensor(std::move(own_sensor));
  station->measure(1, MeasurementConfig());
  REQUIRE(executor.waitForPending(3));

  delete station;  // results still queued in the executor
  OI_COMPARE(results.destroyedCount.load(), 1);
  OI_COMPARE(executor.drain(), size_t{3});
  OI_COMPARE(results.log.names().size(), 0);  // dropped: no dangling station
}

void SensorWorkerTest::stationsOfAJobUseTheJobsExecutor() {
  ManualExecutor executor;
  RecordingObserver results;
  auto own_sensor = std::make_unique<FakeSensor>();
  FakeSensor& sensor = *own_sensor;
  sensor.connected = true;
  OiJob job;

  // a station that was added before the executor was set ...
  auto own_early = std::make_unique<oi::Station>("S1");
  oi::Guarded<Station> early = own_early.get();
  oi::Guarded<oi::FeatureWrapper> earlyFeature = early->getFeatureWrapper();
  job.addFeature(std::move(own_early));
  job.setExecutor(&executor);
  OI_COMPARE(job.getExecutor(), &executor);

  // ... and one added afterwards
  auto own_late = std::make_unique<oi::Station>("S2");
  oi::Guarded<Station> late = own_late.get();
  oi::Guarded<oi::FeatureWrapper> lateFeature = late->getFeatureWrapper();
  job.addFeature(std::move(own_late));

  for (const oi::Guarded<Station>& station : {early, late}) {
    OI_COMPARE(station->getSensorControl()->getExecutor(), &executor);
  }

  late->setStationObserver(&results);
  late->setActiveStationState(true);
  late->setSensor(std::move(own_sensor));
  late->measure(1, MeasurementConfig());
  REQUIRE(executor.waitForPending(3));
  OI_COMPARE(results.log.names().size(), 0);
  executor.drain();
  REQUIRE(oi::contains(results.log.names(), "done:1"));

  // the job does not stop its stations' workers; hand the sensor back
  // before it goes out of scope
  late->takeSensor();

  // a station outside any job delivers inline
  Station alone("S3");
  REQUIRE(dynamic_cast<InlineExecutor*>(
              alone.getSensorControl()->getExecutor()) != nullptr);
}

void SensorWorkerTest::stationReportsActiveStationAndSensorChangesToTheJob() {
  OiJob job;
  RecordingJobObserver jobObserver;
  jobObserver.attach(&job);

  auto own_station = std::make_unique<oi::Station>("S1");
  oi::Guarded<Station> station = own_station.get();
  oi::Guarded<oi::FeatureWrapper> feature = station->getFeatureWrapper();
  job.addFeature(std::move(own_station));

  station->setActiveStationState(true);
  OI_COMPARE(job.getActiveStation().data(), station.data());
  REQUIRE(jobObserver.count("activeStationChanged") >= 1);

  auto own_sensor = std::make_unique<FakeSensor>();

  FakeSensor& sensor = *own_sensor;
  station->setSensor(std::move(own_sensor));
  OI_COMPARE(jobObserver.count("stationSensorChanged"), 1);
  std::unique_ptr<Sensor> taken = station->takeSensor();
  OI_COMPARE(jobObserver.count("stationSensorChanged"), 2);
  OI_COMPARE(taken.get(), &sensor);
}

TEST_CASE("SensorWorkerTest::commandsRunOnTheWorkerThreadInFifoOrder",
          "[SensorWorkerTest]") {
  SensorWorkerTest fixture;
  fixture.initTestCase();
  fixture.commandsRunOnTheWorkerThreadInFifoOrder();
}

TEST_CASE("SensorWorkerTest::aSlowCommandDoesNotBlockTheCaller",
          "[SensorWorkerTest]") {
  SensorWorkerTest fixture;
  fixture.initTestCase();
  fixture.aSlowCommandDoesNotBlockTheCaller();
}

TEST_CASE("SensorWorkerTest::measurementFlow", "[SensorWorkerTest]") {
  SensorWorkerTest fixture;
  fixture.initTestCase();
  fixture.measurementFlow();
}

TEST_CASE("SensorWorkerTest::measurementFailureReportsDoneLast",
          "[SensorWorkerTest]") {
  SensorWorkerTest fixture;
  fixture.initTestCase();
  fixture.measurementFailureReportsDoneLast();
}

TEST_CASE("SensorWorkerTest::commandsWithoutSensorReportAnError",
          "[SensorWorkerTest]") {
  SensorWorkerTest fixture;
  fixture.initTestCase();
  fixture.commandsWithoutSensorReportAnError();
}

TEST_CASE("SensorWorkerTest::readingStreamYieldsReadingsThenStops",
          "[SensorWorkerTest]") {
  SensorWorkerTest fixture;
  fixture.initTestCase();
  fixture.readingStreamYieldsReadingsThenStops();
}

TEST_CASE("SensorWorkerTest::aRestartedStreamKeepsRunning",
          "[SensorWorkerTest]") {
  SensorWorkerTest fixture;
  fixture.initTestCase();
  fixture.aRestartedStreamKeepsRunning();
}

TEST_CASE("SensorWorkerTest::commandsAreInterleavedWithARunningStream",
          "[SensorWorkerTest]") {
  SensorWorkerTest fixture;
  fixture.initTestCase();
  fixture.commandsAreInterleavedWithARunningStream();
}

TEST_CASE("SensorWorkerTest::statusStreamReportsStatus", "[SensorWorkerTest]") {
  SensorWorkerTest fixture;
  fixture.initTestCase();
  fixture.statusStreamReportsStatus();
}

TEST_CASE("SensorWorkerTest::connectionMonitoringReportsLostAndReceived",
          "[SensorWorkerTest]") {
  SensorWorkerTest fixture;
  fixture.initTestCase();
  fixture.connectionMonitoringReportsLostAndReceived();
}

TEST_CASE("SensorWorkerTest::asyncSensorReportsFromForeignThreadsAreSerialised",
          "[SensorWorkerTest]") {
  SensorWorkerTest fixture;
  fixture.initTestCase();
  fixture.asyncSensorReportsFromForeignThreadsAreSerialised();
}

TEST_CASE("SensorWorkerTest::sensorMessagesFromForeignThreadsAreForwarded",
          "[SensorWorkerTest]") {
  SensorWorkerTest fixture;
  fixture.initTestCase();
  fixture.sensorMessagesFromForeignThreadsAreForwarded();
}

TEST_CASE("SensorWorkerTest::takeSensorWaitsForTheRunningCommandAndDetaches",
          "[SensorWorkerTest]") {
  SensorWorkerTest fixture;
  fixture.initTestCase();
  fixture.takeSensorWaitsForTheRunningCommandAndDetaches();
}

TEST_CASE("SensorWorkerTest::destructionWithPendingCommandsDropsThem",
          "[SensorWorkerTest]") {
  SensorWorkerTest fixture;
  fixture.initTestCase();
  fixture.destructionWithPendingCommandsDropsThem();
}

TEST_CASE("SensorWorkerTest::destructionWhileAStreamRunsStopsCleanly",
          "[SensorWorkerTest]") {
  SensorWorkerTest fixture;
  fixture.initTestCase();
  fixture.destructionWhileAStreamRunsStopsCleanly();
}

TEST_CASE("SensorWorkerTest::destructionWhileTheDriverReportsFromAnotherThread",
          "[SensorWorkerTest]") {
  SensorWorkerTest fixture;
  fixture.initTestCase();
  fixture.destructionWhileTheDriverReportsFromAnotherThread();
}

TEST_CASE("SensorWorkerTest::controlDeliversThroughTheExecutorInOrder",
          "[SensorWorkerTest]") {
  SensorWorkerTest fixture;
  fixture.initTestCase();
  fixture.controlDeliversThroughTheExecutorInOrder();
}

TEST_CASE("SensorWorkerTest::tasksStillQueuedAfterDestructionAreDropped",
          "[SensorWorkerTest]") {
  SensorWorkerTest fixture;
  fixture.initTestCase();
  fixture.tasksStillQueuedAfterDestructionAreDropped();
}

TEST_CASE("SensorWorkerTest::anExecutorCanBeDestroyedRightAfterItWasReplaced",
          "[SensorWorkerTest]") {
  SensorWorkerTest fixture;
  fixture.initTestCase();
  fixture.anExecutorCanBeDestroyedRightAfterItWasReplaced();
}

TEST_CASE("SensorWorkerTest::stationCommandMethodsReachTheSensor",
          "[SensorWorkerTest]") {
  SensorWorkerTest fixture;
  fixture.initTestCase();
  fixture.stationCommandMethodsReachTheSensor();
}

TEST_CASE("SensorWorkerTest::stationObserverIsToldAndForgotten",
          "[SensorWorkerTest]") {
  SensorWorkerTest fixture;
  fixture.initTestCase();
  fixture.stationObserverIsToldAndForgotten();
}

TEST_CASE("SensorWorkerTest::stationsOfAJobUseTheJobsExecutor",
          "[SensorWorkerTest]") {
  SensorWorkerTest fixture;
  fixture.initTestCase();
  fixture.stationsOfAJobUseTheJobsExecutor();
}

TEST_CASE(
    "SensorWorkerTest::stationReportsActiveStationAndSensorChangesToTheJob",
    "[SensorWorkerTest]") {
  SensorWorkerTest fixture;
  fixture.initTestCase();
  fixture.stationReportsActiveStationAndSensorChangesToTheJob();
}

#include <format>
