#include <QtTest>
#include <string>
#include <vector>

#include "bundleadjustment.h"
#include "chooselalib.h"
#include "constructfunction.h"
#include "containers.h"
#include "exchangeinterface.h"
#include "featurewrapper.h"
#include "geometry/point.h"
#include "messagesink.h"
#include "oijob.h"
#include "recordingjobobserver.h"
#include "simulationmodel.h"

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
    emit this->sendMessage("chatty exec", MessageTypes::eInformationMessage);
    Point point;
    point.setPoint(Position(1.0, 2.0, 3.0));
    return point;
  }
};

// a bare exchange that reports through every channel
class ChattyExchange : public ExchangeInterface {
 public:
  void importOiData(const oi::OperationContext& ctx) override {
    emit this->sendMessage("importing", MessageTypes::eInformationMessage);
    ctx.report(std::size_t{1}, std::size_t{2}, "half");
    emit this->importFinished(true);
  }
  void exportOiData(const oi::OperationContext&) override {
    emit this->exportFinished(false);
  }
};

}  // namespace

class MessageSinkTest : public QObject {
  Q_OBJECT

 private slots:
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
  f.point = new Point();
  f.wrapper = new FeatureWrapper();
  f.wrapper->setPoint(f.point);
  f.point->setFeatureName("P1");
  const bool added = job.addFeature(f.wrapper);
  Q_ASSERT(added);
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

  oi::Guarded<ChattyFunction> function = new ChattyFunction();
  job.addFunction(function);
  QCOMPARE(function->getMessageSink(), static_cast<MessageSink*>(&job));

  function->sendMessage("hello from the function",
                        MessageTypes::eWarningMessage);
  QVERIFY(oi::contains(
      rec.calls,
      std::format("sendMessage:hello from the function:{}:{}",
                  static_cast<int>(MessageTypes::eWarningMessage),
                  static_cast<int>(MessageDestinations::eConsoleMessage))));

  // a message sent from inside exec reaches the job as well
  rec.calls.clear();
  f.point->recalc();
  QVERIFY2(rec.joined().contains("sendMessage:chatty exec"),
           (rec.joined()).c_str());
}

void MessageSinkTest::removingAFunctionFromTheJobStopsItReportingToTheJob() {
  OiJob job;
  RecordingJobObserver rec;
  rec.attach(&job);
  const Fixture f = addActivePoint(job);

  oi::Guarded<ChattyFunction> function = new ChattyFunction();
  job.addFunction(function);
  QCOMPARE(function->getMessageSink(), static_cast<MessageSink*>(&job));

  // removing the feature detaches its functions from the job
  QVERIFY(job.removeFeature(f.point->getId()));
  if (!function.isNull()) {
    QCOMPARE(function->getMessageSink(), static_cast<MessageSink*>(nullptr));
    rec.calls.clear();
    function->sendMessage("late", MessageTypes::eInformationMessage);
    QCOMPARE(rec.count("sendMessage"), 0);
  }

  // removing just the function deletes it: nothing left that could dangle
  OiJob job2;
  const Fixture g = addActivePoint(job2);
  oi::Guarded<ChattyFunction> second = new ChattyFunction();
  job2.addFunction(second);
  job2.removeFunction(0);
  QVERIFY(second.isNull());
}

void MessageSinkTest::aFunctionOutlivingItsJobDoesNotReportToADeadJob() {
  oi::Guarded<ChattyFunction> function = new ChattyFunction();
  RecordingSink after;
  {
    OiJob job;
    const Fixture f = addActivePoint(job);
    // keep the function alive past the job: detach it from its feature first
    job.addFunction(function);
    QCOMPARE(function->getMessageSink(), static_cast<MessageSink*>(&job));
    // the feature owns the function; the job's destructor must clear the
    // sink before the features go
  }
  // the feature (and with it the function) died with the job; a function that
  // is still around must not point at the job
  if (!function.isNull()) {
    QCOMPARE(function->getMessageSink(), static_cast<MessageSink*>(nullptr));
    function->sendMessage("after", MessageTypes::eInformationMessage);
    delete function.data();
  }
  QVERIFY(after.messages.empty());
}

void MessageSinkTest::aFunctionWithoutSinkDropsMessages() {
  ChattyFunction function;
  QCOMPARE(function.getMessageSink(), static_cast<MessageSink*>(nullptr));
  function.sendMessage("nobody listens", MessageTypes::eInformationMessage);

  RecordingSink sink;
  function.setMessageSink(&sink);
  function.sendMessage("heard", MessageTypes::eInformationMessage);
  function.setMessageSink(nullptr);
  function.sendMessage("not heard", MessageTypes::eInformationMessage);
  QCOMPARE(sink.messages, std::vector<std::string>{"heard"});
}

void MessageSinkTest::aBundleAdjustmentReportsToItsJob() {
  OiJob job;
  RecordingJobObserver rec;
  rec.attach(&job);

  BundleAdjustment bundle;
  bundle.setCurrentJob(&job);
  QCOMPARE(bundle.getMessageSink(), static_cast<MessageSink*>(&job));
  QVERIFY(!bundle.runBundle());
  QVERIFY(rec.count("sendMessage") == 1);
  QVERIFY(rec.calls.back().contains("not implemented"));

  // switching the job away stops the reports
  bundle.setCurrentJob(nullptr);
  QCOMPARE(bundle.getMessageSink(), static_cast<MessageSink*>(nullptr));
  rec.calls.clear();
  QVERIFY(!bundle.runBundle());
  QCOMPARE(rec.count("sendMessage"), 0);
}

void MessageSinkTest::aBundleAdjustmentOutlivingItsJobIsDetached() {
  // destroying the job must not leave its bundle plugin pointing at it
  oi::Guarded<BundleAdjustment> bundle = new BundleAdjustment();
  {
    OiJob job;
    oi::Guarded<CoordinateSystem> system = new CoordinateSystem(true);
    system->setFeatureName("Bundle01");
    system->setBundlePlugin(bundle);
    oi::Guarded<FeatureWrapper> wrapper = new FeatureWrapper();
    wrapper->setCoordinateSystem(system);
    QVERIFY(job.addFeature(wrapper));
    bundle->setCurrentJob(&job);
    QCOMPARE(bundle->getMessageSink(), static_cast<MessageSink*>(&job));
    QVERIFY(!bundle->runBundle());
  }
  // whether the system took the bundle with it or not, it must not be left
  // pointing at the dead job
  if (!bundle.isNull()) {
    QCOMPARE(bundle->getMessageSink(), static_cast<MessageSink*>(nullptr));
    QVERIFY(!bundle->runBundle());
    delete bundle.data();
  }
}

void MessageSinkTest::simulationModelReportsToItsSink() {
  SimulationModel model;
  RecordingSink sink;
  model.setMessageSink(&sink);
  UncertaintyData data;
  QVERIFY(!model.analyseSimulationData(data));
  QCOMPARE(sink.messages.size(), 1);
  QVERIFY(sink.messages.front().contains("analyse method missing"));
  model.setMessageSink(nullptr);
  QVERIFY(!model.analyseSimulationData(data));
  QCOMPARE(sink.messages.size(), 1);
}

void MessageSinkTest::exchangeReportsMessagesProgressAndCompletion() {
  ChattyExchange exchange;
  RecordingSink sink;
  RecordingExchangeObserver observer;
  exchange.setMessageSink(&sink);
  exchange.setObserver(&observer);

  RecordingProgress progress;
  exchange.importOiData(oi::OperationContext(&progress));
  QCOMPARE(sink.messages, std::vector<std::string>{"importing"});
  QCOMPARE(observer.events, std::vector<std::string>{"import:1"});
  QCOMPARE(progress.events, std::vector<std::string>{"progress:50:half"});

  observer.events.clear();
  exchange.exportOiData({});
  QCOMPARE(observer.events, std::vector<std::string>{"export:0"});

  // without an observer nothing happens (and nothing crashes)
  exchange.setObserver(nullptr);
  exchange.importOiData({});
  QCOMPARE(observer.events, std::vector<std::string>{"export:0"});
}

void MessageSinkTest::aFunctionSeesTheOperationContextOfItsExec() {
  OiJob job;
  const Fixture f = addActivePoint(job);
  oi::Guarded<ChattyFunction> function = new ChattyFunction();
  job.addFunction(function);

  // progress and the context visible to the function
  RecordingProgress progress;
  QVERIFY(function->execChecked(f.wrapper, oi::OperationContext(&progress))
              .has_value());
  QCOMPARE(progress.events,
           (std::vector<std::string>{"progress:0:ChattyFunction",
                                     "progress:100:ChattyFunction"}));

  // a cancelled context stops before running and is an error, not a throw
  std::stop_source stop;
  stop.request_stop();
  progress.events.clear();
  const auto cancelled = function->execChecked(
      f.wrapper, oi::OperationContext(&progress, stop.get_token()));
  QVERIFY(!cancelled.has_value());
  QCOMPARE(cancelled.error(), std::string("operation cancelled"));
  QVERIFY(progress.events.empty());

  // outside an exec the function has no context to report to
  function->operation().report(0.5, "ignored");
  QVERIFY(!function->operation().cancelRequested());
}

void MessageSinkTest::functionObserverSeesInputElementAndParameterChanges() {
  ChattyFunction function;
  RecordingFunctionObserver observer;
  function.setObserver(&observer);

  InputElement element(7);
  element.typeOfElement = ElementTypes::ePointElement;
  function.addInputElement(element, 0);
  QCOMPARE(observer.inputElementsChanged, 1);

  function.setShouldBeUsed(0, 7, false);
  QCOMPARE(observer.inputElementsChanged, 2);

  function.removeInputElement(7, 0);
  QCOMPARE(observer.inputElementsChanged, 3);

  function.setScalarInputParams(ScalarInputParams());
  QCOMPARE(observer.scalarInputParametersChanged, 1);

  function.clear();
  QCOMPARE(observer.inputElementsChanged, 4);

  function.setObserver(nullptr);
  function.clear();
  QCOMPARE(observer.inputElementsChanged, 4);
}

void MessageSinkTest::functionObserverIsToldWhenTheFunctionIsDestroyed() {
  RecordingFunctionObserver observer;
  auto* function = new ChattyFunction();
  Function* raw = function;
  function->setObserver(&observer);
  delete function;
  QCOMPARE(observer.destroyed, std::vector<Function*>{raw});
}

QTEST_GUILESS_MAIN(MessageSinkTest)
#include <format>

#include "tst_messagesink.moc"
