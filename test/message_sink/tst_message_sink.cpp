#include <cassert>
#include <catch2/catch_test_macros.hpp>
#include <string>
#include <vector>

#include "bundle_adjustment.h"
#include "catch_compat.h"
#include "choose_la_lib.h"
#include "construct_function.h"
#include "containers.h"
#include "exchange_interface.h"
#include "feature_wrapper.h"
#include "geometry/point.h"
#include "message_sink.h"
#include "oi_job.h"
#include "recording_job_observer.h"
#include "simulation_model.h"

using namespace oi;

namespace {

class RecordingSink : public MessageSink {
 public:
  std::vector<std::string> messages;
  void onMessage(const std::string& msg, const MessageTypes& /*type*/,
                 const MessageDestinations& /*dest*/) override {
    this->messages.push_back(msg);
  }
};

class RecordingFunctionObserver : public FunctionObserver {
 public:
  int inputElementsChanged = 0;
  int scalarInputParametersChanged = 0;
  std::vector<Function*> destroyed;
  void onFunctionDestroyed(Function* function) override {
    this->destroyed.push_back(function);
  }
  void onInputElementsChanged() override { ++this->inputElementsChanged; }
  void onScalarInputParametersChanged() override {
    ++this->scalarInputParametersChanged;
  }
};

class RecordingProgress : public oi::ProgressSink {
 public:
  std::vector<std::string> events;
  void onProgress(const oi::Progress& progress) override {
    this->events.push_back(std::format(
        "progress:{}:{}", static_cast<int>(progress.fraction * 100 + 0.5),
        progress.message));
  }
};

class RecordingExchangeObserver : public ExchangeObserver {
 public:
  std::vector<std::string> events;
  void onImportFinished(bool success) override {
    this->events.push_back(std::format("import:{}", static_cast<int>(success)));
  }
  void onExportFinished(bool success) override {
    this->events.push_back(std::format("export:{}", static_cast<int>(success)));
  }
};

// a real function that talks, like a plugin function does
class ChattyFunction : public ConstructFunction, public Producer<Point> {
 public:
  explicit ChattyFunction() : ConstructFunction() {
    this->metaData.name = "ChattyFunction";
    NeededElement param;
    param.typeOfElement = ElementTypes::ePointElement;
    this->neededElements.push_back(param);
    this->applicableFor.push_back(FeatureTypes::ePointFeature);
  }
  std::expected<Point, std::string> exec() override {
    this->sendMessage("chatty exec", MessageTypes::eInformationMessage);
    Point point;
    point.setPoint(Position(1.0, 2.0, 3.0));
    return point;
  }
};

// a bare exchange that reports through every channel
class ChattyExchange : public ExchangeInterface {
 public:
  void importOiData(const oi::OperationContext& ctx) override {
    this->sendMessage("importing", MessageTypes::eInformationMessage);
    ctx.report(std::size_t{1}, std::size_t{2}, "half");
    this->importFinished(true);
  }
  void exportOiData(const oi::OperationContext&) override {
    this->exportFinished(false);
  }
};

}  // namespace

class MessageSinkTest {
 public:
  void initTestCase();

  void aFunctionOfTheJobReportsThroughTheJobsMessageNotification();
  void removingAFunctionFromTheJobStopsItReportingToTheJob();
  void aFunctionOutlivingItsJobDoesNotReportToADeadJob();
  void aFunctionWithoutSinkDropsMessages();
  void aBundleAdjustmentReportsToItsJob();
  void aBundleAdjustmentOutlivingItsJobIsDetached();
  void simulationModelReportsToItsSink();
  void exchangeReportsMessagesProgressAndCompletion();
  void aFunctionSeesTheOperationContextOfItsExec();
  void functionObserverSeesInputElementAndParameterChanges();
  void functionObserverIsToldWhenTheFunctionIsDestroyed();

 private:
  struct Fixture {
    oi::Guarded<FeatureWrapper> wrapper;
    oi::Guarded<Point> point;
  };
  static Fixture addActivePoint(OiJob& job);
};

void MessageSinkTest::initTestCase() {
  ChooseLALib::setLinearAlgebra(ChooseLALib::LinearAlgebraLibrary::Armadillo);
}

MessageSinkTest::Fixture MessageSinkTest::addActivePoint(OiJob& job) {
  Fixture f;
  auto point = std::make_unique<Point>();
  f.point = point.get();
  f.wrapper = f.point->getFeatureWrapper();
  f.point->setFeatureName("P1");
  const bool added = !job.addFeature(std::move(point)).isNull();
  assert(added);
  static_cast<void>(added);
  f.point->setActiveFeatureState(true);
  return f;
}

void MessageSinkTest::
    aFunctionOfTheJobReportsThroughTheJobsMessageNotification() {
  OiJob job;
  RecordingJobObserver rec;
  rec.attach(&job);
  const Fixture f = addActivePoint(job);

  auto owned_function = std::make_unique<ChattyFunction>();

  const oi::Guarded<ChattyFunction> function(owned_function.get());
  job.addFunction(std::move(owned_function));
  OI_COMPARE(function->getMessageSink(), static_cast<MessageSink*>(&job));

  function->sendMessage("hello from the function",
                        MessageTypes::eWarningMessage);
  REQUIRE(oi::contains(
      rec.calls,
      std::format("sendMessage:hello from the function:{}:{}",
                  static_cast<int>(MessageTypes::eWarningMessage),
                  static_cast<int>(MessageDestinations::eConsoleMessage))));

  // a message sent from inside exec reaches the job as well
  rec.calls.clear();
  f.point->recalc();
  OI_REQUIRE2(rec.joined().contains("sendMessage:chatty exec"),
              (rec.joined()).c_str());
}

void MessageSinkTest::removingAFunctionFromTheJobStopsItReportingToTheJob() {
  OiJob job;
  RecordingJobObserver rec;
  rec.attach(&job);
  const Fixture f = addActivePoint(job);

  auto owned_function = std::make_unique<ChattyFunction>();

  const oi::Guarded<ChattyFunction> function(owned_function.get());
  job.addFunction(std::move(owned_function));
  OI_COMPARE(function->getMessageSink(), static_cast<MessageSink*>(&job));

  // removing the feature detaches its functions from the job
  REQUIRE(job.removeFeature(f.point->getId()));
  if (!function.isNull()) {
    OI_COMPARE(function->getMessageSink(), static_cast<MessageSink*>(nullptr));
    rec.calls.clear();
    function->sendMessage("late", MessageTypes::eInformationMessage);
    OI_COMPARE(rec.count("sendMessage"), 0);
  }

  // removing just the function deletes it: nothing left that could dangle
  OiJob job2;
  const Fixture g = addActivePoint(job2);
  auto owned_second = std::make_unique<ChattyFunction>();
  const oi::Guarded<ChattyFunction> second(owned_second.get());
  job2.addFunction(std::move(owned_second));
  job2.removeFunction(0);
  REQUIRE(second.isNull());
}

void MessageSinkTest::aFunctionOutlivingItsJobDoesNotReportToADeadJob() {
  auto owned_function = std::make_unique<ChattyFunction>();
  const oi::Guarded<ChattyFunction> function(owned_function.get());
  RecordingSink after;
  {
    OiJob job;
    const Fixture f = addActivePoint(job);
    // keep the function alive past the job: detach it from its feature first
    job.addFunction(std::move(owned_function));
    OI_COMPARE(function->getMessageSink(), static_cast<MessageSink*>(&job));
    // the feature owns the function; the job's destructor must clear the
    // sink before the features go
  }
  // the feature (and with it the function) died with the job; a function that
  // is still around must not point at the job
  if (!function.isNull()) {
    OI_COMPARE(function->getMessageSink(), static_cast<MessageSink*>(nullptr));
    function->sendMessage("after", MessageTypes::eInformationMessage);
    delete function.data();
  }
  REQUIRE(after.messages.empty());
}

void MessageSinkTest::aFunctionWithoutSinkDropsMessages() {
  ChattyFunction function;
  OI_COMPARE(function.getMessageSink(), static_cast<MessageSink*>(nullptr));
  function.sendMessage("nobody listens", MessageTypes::eInformationMessage);

  RecordingSink sink;
  function.setMessageSink(&sink);
  function.sendMessage("heard", MessageTypes::eInformationMessage);
  function.setMessageSink(nullptr);
  function.sendMessage("not heard", MessageTypes::eInformationMessage);
  OI_COMPARE(sink.messages, std::vector<std::string>{"heard"});
}

void MessageSinkTest::aBundleAdjustmentReportsToItsJob() {
  OiJob job;
  RecordingJobObserver rec;
  rec.attach(&job);

  BundleAdjustment bundle;
  bundle.setCurrentJob(&job);
  OI_COMPARE(bundle.getMessageSink(), static_cast<MessageSink*>(&job));
  REQUIRE(!bundle.runBundle());
  REQUIRE(rec.count("sendMessage") == 1);
  REQUIRE(rec.calls.back().contains("not implemented"));

  // switching the job away stops the reports
  bundle.setCurrentJob(nullptr);
  OI_COMPARE(bundle.getMessageSink(), static_cast<MessageSink*>(nullptr));
  rec.calls.clear();
  REQUIRE(!bundle.runBundle());
  OI_COMPARE(rec.count("sendMessage"), 0);
}

void MessageSinkTest::aBundleAdjustmentOutlivingItsJobIsDetached() {
  // destroying the job must not leave its bundle plugin pointing at it
  auto owned_bundle = std::make_unique<BundleAdjustment>();
  const oi::Guarded<BundleAdjustment> bundle(owned_bundle.get());
  {
    OiJob job;
    auto own_system = std::make_unique<oi::CoordinateSystem>(true);
    oi::Guarded<CoordinateSystem> system = own_system.get();
    oi::Guarded<oi::FeatureWrapper> wrapper = system->getFeatureWrapper();
    system->setFeatureName("Bundle01");
    system->setBundlePlugin(std::move(owned_bundle));
    REQUIRE(job.addFeature(std::move(own_system)));
    bundle->setCurrentJob(&job);
    OI_COMPARE(bundle->getMessageSink(), static_cast<MessageSink*>(&job));
    REQUIRE(!bundle->runBundle());
  }
  // whether the system took the bundle with it or not, it must not be left
  // pointing at the dead job
  if (!bundle.isNull()) {
    OI_COMPARE(bundle->getMessageSink(), static_cast<MessageSink*>(nullptr));
    REQUIRE(!bundle->runBundle());
    delete bundle.data();
  }
}

void MessageSinkTest::simulationModelReportsToItsSink() {
  SimulationModel model;
  RecordingSink sink;
  model.setMessageSink(&sink);
  UncertaintyData data;
  REQUIRE(!model.analyseSimulationData(data));
  OI_COMPARE(sink.messages.size(), 1);
  REQUIRE(sink.messages.front().contains("analyse method missing"));
  model.setMessageSink(nullptr);
  REQUIRE(!model.analyseSimulationData(data));
  OI_COMPARE(sink.messages.size(), 1);
}

void MessageSinkTest::exchangeReportsMessagesProgressAndCompletion() {
  ChattyExchange exchange;
  RecordingSink sink;
  RecordingExchangeObserver observer;
  exchange.setMessageSink(&sink);
  exchange.setObserver(&observer);

  RecordingProgress progress;
  exchange.importOiData(oi::OperationContext(&progress));
  OI_COMPARE(sink.messages, std::vector<std::string>{"importing"});
  OI_COMPARE(observer.events, std::vector<std::string>{"import:1"});
  OI_COMPARE(progress.events, std::vector<std::string>{"progress:50:half"});

  observer.events.clear();
  exchange.exportOiData({});
  OI_COMPARE(observer.events, std::vector<std::string>{"export:0"});

  // without an observer nothing happens (and nothing crashes)
  exchange.setObserver(nullptr);
  exchange.importOiData({});
  OI_COMPARE(observer.events, std::vector<std::string>{"export:0"});
}

void MessageSinkTest::aFunctionSeesTheOperationContextOfItsExec() {
  OiJob job;
  const Fixture f = addActivePoint(job);
  auto owned_function = std::make_unique<ChattyFunction>();
  const oi::Guarded<ChattyFunction> function(owned_function.get());
  job.addFunction(std::move(owned_function));

  // progress and the context visible to the function
  RecordingProgress progress;
  REQUIRE(function->execChecked(f.wrapper, oi::OperationContext(&progress))
              .has_value());
  OI_COMPARE(progress.events,
             (std::vector<std::string>{"progress:0:ChattyFunction",
                                       "progress:100:ChattyFunction"}));

  // a cancelled context stops before running and is an error, not a throw
  std::stop_source stop;
  stop.request_stop();
  progress.events.clear();
  const auto cancelled = function->execChecked(
      f.wrapper, oi::OperationContext(&progress, stop.get_token()));
  REQUIRE(!cancelled.has_value());
  OI_COMPARE(cancelled.error(), std::string("operation cancelled"));
  REQUIRE(progress.events.empty());

  // outside an exec the function has no context to report to
  function->operation().report(0.5, "ignored");
  REQUIRE(!function->operation().cancelRequested());
}

void MessageSinkTest::functionObserverSeesInputElementAndParameterChanges() {
  ChattyFunction function;
  RecordingFunctionObserver observer;
  function.setObserver(&observer);

  InputElement element(7);
  element.typeOfElement = ElementTypes::ePointElement;
  function.addInputElement(element, 0);
  OI_COMPARE(observer.inputElementsChanged, 1);

  function.setShouldBeUsed(0, 7, false);
  OI_COMPARE(observer.inputElementsChanged, 2);

  function.removeInputElement(7, 0);
  OI_COMPARE(observer.inputElementsChanged, 3);

  function.setScalarInputParams(ScalarInputParams());
  OI_COMPARE(observer.scalarInputParametersChanged, 1);

  function.clear();
  OI_COMPARE(observer.inputElementsChanged, 4);

  function.setObserver(nullptr);
  function.clear();
  OI_COMPARE(observer.inputElementsChanged, 4);
}

void MessageSinkTest::functionObserverIsToldWhenTheFunctionIsDestroyed() {
  RecordingFunctionObserver observer;
  auto* function = new ChattyFunction();
  Function* raw = function;
  function->setObserver(&observer);
  delete function;
  OI_COMPARE(observer.destroyed, std::vector<Function*>{raw});
}

TEST_CASE(
    "MessageSinkTest::"
    "aFunctionOfTheJobReportsThroughTheJobsMessageNotification",
    "[MessageSinkTest]") {
  MessageSinkTest fixture;
  fixture.initTestCase();
  fixture.aFunctionOfTheJobReportsThroughTheJobsMessageNotification();
}

TEST_CASE(
    "MessageSinkTest::removingAFunctionFromTheJobStopsItReportingToTheJob",
    "[MessageSinkTest]") {
  MessageSinkTest fixture;
  fixture.initTestCase();
  fixture.removingAFunctionFromTheJobStopsItReportingToTheJob();
}

TEST_CASE("MessageSinkTest::aFunctionOutlivingItsJobDoesNotReportToADeadJob",
          "[MessageSinkTest]") {
  MessageSinkTest fixture;
  fixture.initTestCase();
  fixture.aFunctionOutlivingItsJobDoesNotReportToADeadJob();
}

TEST_CASE("MessageSinkTest::aFunctionWithoutSinkDropsMessages",
          "[MessageSinkTest]") {
  MessageSinkTest fixture;
  fixture.initTestCase();
  fixture.aFunctionWithoutSinkDropsMessages();
}

TEST_CASE("MessageSinkTest::aBundleAdjustmentReportsToItsJob",
          "[MessageSinkTest]") {
  MessageSinkTest fixture;
  fixture.initTestCase();
  fixture.aBundleAdjustmentReportsToItsJob();
}

TEST_CASE("MessageSinkTest::aBundleAdjustmentOutlivingItsJobIsDetached",
          "[MessageSinkTest]") {
  MessageSinkTest fixture;
  fixture.initTestCase();
  fixture.aBundleAdjustmentOutlivingItsJobIsDetached();
}

TEST_CASE("MessageSinkTest::simulationModelReportsToItsSink",
          "[MessageSinkTest]") {
  MessageSinkTest fixture;
  fixture.initTestCase();
  fixture.simulationModelReportsToItsSink();
}

TEST_CASE("MessageSinkTest::exchangeReportsMessagesProgressAndCompletion",
          "[MessageSinkTest]") {
  MessageSinkTest fixture;
  fixture.initTestCase();
  fixture.exchangeReportsMessagesProgressAndCompletion();
}

TEST_CASE("MessageSinkTest::aFunctionSeesTheOperationContextOfItsExec",
          "[MessageSinkTest]") {
  MessageSinkTest fixture;
  fixture.initTestCase();
  fixture.aFunctionSeesTheOperationContextOfItsExec();
}

TEST_CASE(
    "MessageSinkTest::functionObserverSeesInputElementAndParameterChanges",
    "[MessageSinkTest]") {
  MessageSinkTest fixture;
  fixture.initTestCase();
  fixture.functionObserverSeesInputElementAndParameterChanges();
}

TEST_CASE("MessageSinkTest::functionObserverIsToldWhenTheFunctionIsDestroyed",
          "[MessageSinkTest]") {
  MessageSinkTest fixture;
  fixture.initTestCase();
  fixture.functionObserverIsToldWhenTheFunctionIsDestroyed();
}

#include <format>
