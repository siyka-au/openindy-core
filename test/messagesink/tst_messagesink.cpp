#include <QString>
#include <QStringList>
#include <QtTest>

#include "bundleadjustment.h"
#include "chooselalib.h"
#include "constructfunction.h"
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
  QStringList messages;
  void onMessage(const QString& msg, const MessageTypes& /*type*/,
                 const MessageDestinations& /*dest*/) override {
    this->messages << msg;
  }
};

class RecordingFunctionObserver : public FunctionObserver {
 public:
  int inputElementsChanged = 0;
  int scalarInputParametersChanged = 0;
  QList<Function*> destroyed;
  void onFunctionDestroyed(Function* function) override {
    this->destroyed << function;
  }
  void onInputElementsChanged() override { ++this->inputElementsChanged; }
  void onScalarInputParametersChanged() override {
    ++this->scalarInputParametersChanged;
  }
};

class RecordingExchangeObserver : public ExchangeObserver {
 public:
  QStringList events;
  void onProgress(int percent, const QString& message) override {
    this->events << QString("progress:%1:%2").arg(percent).arg(message);
  }
  void onImportFinished(bool success) override {
    this->events << QString("import:%1").arg(success);
  }
  void onExportFinished(bool success) override {
    this->events << QString("export:%1").arg(success);
  }
};

// a real function that talks, like a plugin function does
class ChattyFunction : public ConstructFunction, public Producer<Point> {
  Q_OBJECT
 public:
  explicit ChattyFunction(QObject* parent = nullptr)
      : ConstructFunction(parent) {
    this->metaData.name = "ChattyFunction";
    NeededElement param;
    param.typeOfElement = ElementTypes::ePointElement;
    this->neededElements.append(param);
    this->applicableFor.append(FeatureTypes::ePointFeature);
  }
  std::expected<Point, QString> exec() override {
    emit this->sendMessage("chatty exec", MessageTypes::eInformationMessage);
    Point point;
    point.setPoint(Position(1.0, 2.0, 3.0));
    return point;
  }
};

// a bare exchange that reports through every channel
class ChattyExchange : public ExchangeInterface {
  Q_OBJECT
 public:
  void importOiData() override {
    emit this->sendMessage("importing", MessageTypes::eInformationMessage);
    emit this->updateProgress(50, "half");
    emit this->importFinished(true);
  }
  void exportOiData() override { emit this->exportFinished(false); }
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
  void functionObserverSeesInputElementAndParameterChanges();
  void functionObserverIsToldWhenTheFunctionIsDestroyed();

 private:
  struct Fixture {
    QPointer<FeatureWrapper> wrapper;
    QPointer<Point> point;
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
  Q_UNUSED(added);
  f.point->setActiveFeatureState(true);
  return f;
}

void MessageSinkTest::
    aFunctionOfTheJobReportsThroughTheJobsMessageNotification() {
  OiJob job;
  RecordingJobObserver rec;
  rec.attach(&job);
  const Fixture f = addActivePoint(job);

  QPointer<ChattyFunction> function = new ChattyFunction();
  job.addFunction(function);
  QCOMPARE(function->getMessageSink(), static_cast<MessageSink*>(&job));

  function->sendMessage("hello from the function",
                        MessageTypes::eWarningMessage);
  QVERIFY(rec.calls.contains(
      QString("sendMessage:hello from the function:%1:%2")
          .arg(static_cast<int>(MessageTypes::eWarningMessage))
          .arg(static_cast<int>(MessageDestinations::eConsoleMessage))));

  // a message sent from inside exec reaches the job as well
  rec.calls.clear();
  f.point->recalc();
  QVERIFY2(rec.calls.join(";").contains("sendMessage:chatty exec"),
           qPrintable(rec.calls.join(";")));
}

void MessageSinkTest::removingAFunctionFromTheJobStopsItReportingToTheJob() {
  OiJob job;
  RecordingJobObserver rec;
  rec.attach(&job);
  const Fixture f = addActivePoint(job);

  QPointer<ChattyFunction> function = new ChattyFunction();
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
  QPointer<ChattyFunction> second = new ChattyFunction();
  job2.addFunction(second);
  job2.removeFunction(0);
  QVERIFY(second.isNull());
}

void MessageSinkTest::aFunctionOutlivingItsJobDoesNotReportToADeadJob() {
  QPointer<ChattyFunction> function = new ChattyFunction();
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
  QVERIFY(after.messages.isEmpty());
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
  QCOMPARE(sink.messages, QStringList{"heard"});
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
  QVERIFY(rec.calls.last().contains("not implemented"));

  // switching the job away stops the reports
  bundle.setCurrentJob(nullptr);
  QCOMPARE(bundle.getMessageSink(), static_cast<MessageSink*>(nullptr));
  rec.calls.clear();
  QVERIFY(!bundle.runBundle());
  QCOMPARE(rec.count("sendMessage"), 0);
}

void MessageSinkTest::aBundleAdjustmentOutlivingItsJobIsDetached() {
  // destroying the job must not leave its bundle plugin pointing at it
  QPointer<BundleAdjustment> bundle = new BundleAdjustment();
  {
    OiJob job;
    QPointer<CoordinateSystem> system = new CoordinateSystem(true);
    system->setFeatureName("Bundle01");
    system->setBundlePlugin(bundle);
    QPointer<FeatureWrapper> wrapper = new FeatureWrapper();
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
  QVERIFY(sink.messages.first().contains("analyse method missing"));
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

  exchange.importOiData();
  QCOMPARE(sink.messages, QStringList{"importing"});
  QCOMPARE(observer.events, (QStringList{"progress:50:half", "import:1"}));

  observer.events.clear();
  exchange.exportOiData();
  QCOMPARE(observer.events, QStringList{"export:0"});

  // without an observer nothing happens (and nothing crashes)
  exchange.setObserver(nullptr);
  exchange.importOiData();
  QCOMPARE(observer.events, QStringList{"export:0"});
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
  QCOMPARE(observer.destroyed, QList<Function*>{raw});
}

QTEST_GUILESS_MAIN(MessageSinkTest)
#include "tst_messagesink.moc"
