#include <QCoreApplication>
#include <QSignalSpy>
#include <QString>
#include <QtTest>

#include "chooselalib.h"
#include "constructfunction.h"
#include "featureobserver.h"
#include "fitfunction.h"
#include "group.h"
#include "oijob.h"
#include "point.h"
#include "station.h"
#include "trafoparam.h"

using namespace oi;

namespace {

// A function a Point accepts as its producer (see tst_ownership)
class FakeFit : public FitFunction, public Producer<Point> {
  Q_OBJECT
 public:
  explicit FakeFit(QObject* parent = nullptr) : FitFunction(parent) {
    this->metaData.name = "FakeFit";
  }
  std::expected<Point, QString> exec() override { return Point(); }
};

// Remembers every notification as "name:arg:arg..." in arrival order
class RecordingObserver : public FeatureObserver {
 public:
  QStringList calls;

  [[nodiscard]] int count(const QString& name) const {
    int n = 0;
    for (const QString& c : this->calls) {
      if (c == name || c.startsWith(name + ":")) {
        ++n;
      }
    }
    return n;
  }

  void onElementAboutToBeDeleted(const int& id, const QString&, const QString&,
                                 const FeatureTypes&) override {
    this->calls << QString("elementAboutToBeDeleted:%1").arg(id);
  }
  void onFeatureNameChanged(const int& id, const QString& old) override {
    this->calls << QString("featureName:%1:%2").arg(id).arg(old);
  }
  void onFeatureGroupChanged(const int& id, const QString& old) override {
    this->calls << QString("featureGroup:%1:%2").arg(id).arg(old);
  }
  void onFeatureCommentChanged(const int& id) override {
    this->calls << QString("featureComment:%1").arg(id);
  }
  void onFeatureIsSolvedChanged(const int& id) override {
    this->calls << QString("featureIsSolved:%1").arg(id);
  }
  void onFeatureFunctionListChanged(const int& id) override {
    this->calls << QString("featureFunctionList:%1").arg(id);
  }
  void onFeatureUsedForChanged(const int& id) override {
    this->calls << QString("featureUsedFor:%1").arg(id);
  }
  void onGeomIsCommonChanged(const int& id) override {
    this->calls << QString("geomIsCommon:%1").arg(id);
  }
  void onGeomMeasurementConfigChanged(const int& id, const QString& old,
                                      const MeasurementConfigKey) override {
    this->calls << QString("geomMeasurementConfig:%1:%2").arg(id).arg(old);
  }
  void onGroupMembersChanged(const int& id) override {
    this->calls << QString("groupMembers:%1").arg(id);
  }
  void onIsUsedChanged(const int& id) override {
    this->calls << QString("trafoIsUsed:%1").arg(id);
  }
  void onIsDatumTrafoChanged(const int& id) override {
    this->calls << QString("trafoIsDatum:%1").arg(id);
  }
  void onObservationsChanged(const int& id, const int& obs) override {
    this->calls << QString("systemObservations:%1:%2").arg(id).arg(obs);
  }
};

}  // namespace

/*!
 * Step 5A: the feature model reports to a plain FeatureObserver instead of
 * Qt signals; OiJob is that observer and relays into its own signals.
 */
class FeatureObserverTest : public QObject {
  Q_OBJECT

 private slots:
  void initTestCase();

  // the notifications reach an observer, once each
  void renameCommentGroupSolved();
  void functionAddedAndUsedFor();
  void geometryChanges();
  void trafoParamChanges();
  void groupMemberAdded();
  void stationSystemObservationAdded();
  void blockedSignalsStaySilent();

  // lifetime
  void featuresWithoutObserverNotifyNobody();
  void destroyingTheFeatureTellsTheObserverOnce();
  void observerIsClearedWhenFeatureIsRemovedFromJob();
  void observerIsClearedWhenJobIsDestroyed();
  void stationSystemObserverIsClearedWhenJobIsDestroyed();

  // the job as observer
  void jobRelaysIntoItsOwnSignals();
  void jobDeliversIsSolvedDeferred();
};

void FeatureObserverTest::initTestCase() {
  ChooseLALib::setLinearAlgebra(ChooseLALib::LinearAlgebraLibrary::Armadillo);
}

void FeatureObserverTest::renameCommentGroupSolved() {
  RecordingObserver rec;
  Point point;
  point.setObserver(&rec);
  QCOMPARE(point.getObserver(), &rec);

  point.setFeatureName("P1");
  QCOMPARE(rec.count("featureName"), 1);
  QCOMPARE(rec.calls.last(), QString("featureName:%1:").arg(point.getId()));

  point.setFeatureName("P1");  // unchanged: nothing
  QCOMPARE(rec.count("featureName"), 1);

  point.setComment("hello");
  QCOMPARE(rec.count("featureComment"), 1);

  point.setGroupName("G");
  QCOMPARE(rec.count("featureGroup"), 1);

  point.setIsSolved(true);
  QCOMPARE(rec.count("featureIsSolved"), 1);
}

void FeatureObserverTest::functionAddedAndUsedFor() {
  RecordingObserver rec;
  Point point;
  Point other;
  point.setObserver(&rec);

  point.addFunction(new FakeFit());
  QCOMPARE(rec.count("featureFunctionList"), 1);

  // usedFor/previouslyNeeded bookkeeping never notified (nothing emitted the
  // old signals either); it must stay silent and harmless
  QVERIFY(point.addUsedFor(other.getFeatureWrapper()));
  QCOMPARE(rec.count("featureUsedFor"), 0);
}

void FeatureObserverTest::geometryChanges() {
  RecordingObserver rec;
  Point point;
  point.setObserver(&rec);

  point.setCommonState(true);
  QCOMPARE(rec.count("geomIsCommon"), 1);
  point.setCommonState(true);
  QCOMPARE(rec.count("geomIsCommon"), 1);

  point.setMeasurementConfig(MeasurementConfigKey());
  QCOMPARE(rec.count("geomMeasurementConfig"), 1);
}

void FeatureObserverTest::trafoParamChanges() {
  RecordingObserver rec;
  TrafoParam tp;
  tp.setObserver(&rec);

  tp.setIsUsed(true);
  QCOMPARE(rec.count("trafoIsUsed"), 1);
  tp.setIsUsed(true);
  QCOMPARE(rec.count("trafoIsUsed"), 1);

  tp.setIsDatumTrafo(true);
  QCOMPARE(rec.count("trafoIsDatum"), 1);
}

void FeatureObserverTest::groupMemberAdded() {
  RecordingObserver rec;
  Group group("G");
  Point member;
  group.setObserver(&rec);

  QVERIFY(group.addMember(member.getFeatureWrapper()));
  QCOMPARE(rec.count("groupMembers"), 1);
  QVERIFY(!group.addMember(member.getFeatureWrapper()));  // duplicate
  QCOMPARE(rec.count("groupMembers"), 1);
  QVERIFY(group.removeMember(member.getFeatureWrapper()));
  QCOMPARE(rec.count("groupMembers"), 2);
}

void FeatureObserverTest::stationSystemObservationAdded() {
  OiJob job;
  Station* station = new Station("S1");
  QPointer<FeatureWrapper> wrapper = station->getFeatureWrapper();
  QVERIFY(job.addFeature(wrapper));

  QSignalSpy spy(&job, &OiJob::systemObservationsChanged);

  ReadingCartesian r;
  r.isValid = true;
  QPointer<Reading> reading = new Reading(r);
  QPointer<Observation> obs = new Observation();
  reading->setObservation(obs);
  QVERIFY(station->getCoordinateSystem()->addObservation(obs));
  QCOMPARE(spy.count(), 1);
}

void FeatureObserverTest::blockedSignalsStaySilent() {
  RecordingObserver rec;
  Point point;
  point.setObserver(&rec);

  point.blockSignals(true);
  point.setFeatureName("quiet");
  point.setCommonState(true);
  point.blockSignals(false);
  QVERIFY(rec.calls.isEmpty());

  point.announceObservationsChanged();
  QCOMPARE(rec.count("geomObservations"), 0);  // not recorded by this observer
}

void FeatureObserverTest::featuresWithoutObserverNotifyNobody() {
  Point point;
  QVERIFY(point.getObserver() == nullptr);
  point.setFeatureName("x");
  point.setComment("y");
  point.setGroupName("g");
  point.setIsSolved(true);
  point.setCommonState(true);
  point.announceObservationsChanged();
  point.addFunction(new FakeFit());

  TrafoParam tp;
  tp.setIsUsed(true);
  tp.setIsDatumTrafo(true);

  Group group("G");
  Point member;
  QVERIFY(group.addMember(member.getFeatureWrapper()));
  // reaching here without a crash is the assertion
}

void FeatureObserverTest::destroyingTheFeatureTellsTheObserverOnce() {
  RecordingObserver rec;
  int id = -1;
  {
    Point point;
    id = point.getId();
    point.setObserver(&rec);
  }
  QCOMPARE(rec.count("elementAboutToBeDeleted"), 1);
  QCOMPARE(rec.calls.last(), QString("elementAboutToBeDeleted:%1").arg(id));
}

void FeatureObserverTest::observerIsClearedWhenFeatureIsRemovedFromJob() {
  OiJob job;
  Point* point = new Point();
  point->setFeatureName("P1");
  QPointer<Point> guard(point);
  QVERIFY(job.addFeature(point->getFeatureWrapper()));
  QCOMPARE(point->getObserver(), static_cast<FeatureObserver*>(&job));
  QCOMPARE(job.getFeatureCount(), 1);

  // removing deletes the feature; the job must have let go of it first so
  // that its destruction does not call back into the job
  QVERIFY(job.removeFeature(point->getFeatureWrapper()));
  QVERIFY(guard.isNull());
  QCOMPARE(job.getFeatureCount(), 0);
  QCoreApplication::processEvents();
}

void FeatureObserverTest::observerIsClearedWhenJobIsDestroyed() {
  Point* point = new Point();
  point->setFeatureName("P1");
  QPointer<Point> guard(point);
  RecordingObserver rec;

  {
    OiJob job;
    QVERIFY(job.addFeature(point->getFeatureWrapper()));
    QCOMPARE(point->getObserver(), static_cast<FeatureObserver*>(&job));
  }

  // the job's destruction deletes only the feature's wrapper, so the feature
  // outlives it (and is deliberately leaked here: its wrapper is gone) -
  // nothing may still point at the dead job
  QVERIFY(!guard.isNull());
  {
    QVERIFY(point->getObserver() == nullptr);
    point->setFeatureName("after");  // would call into a dead job otherwise
    point->setComment("after");
    point->setIsSolved(true);
    point->setObserver(&rec);
    point->setFeatureName("after2");
    QCOMPARE(rec.count("featureName"), 1);
    point->setObserver(nullptr);
  }
  // let any queued delivery run: it must not reach the destroyed job
  QCoreApplication::processEvents();
}

void FeatureObserverTest::stationSystemObserverIsClearedWhenJobIsDestroyed() {
  Station* station = new Station("S1");
  QPointer<Station> guard(station);
  QPointer<CoordinateSystem> system = station->getCoordinateSystem();
  QVERIFY(!system.isNull());
  {
    OiJob job;
    QVERIFY(job.addFeature(station->getFeatureWrapper()));
    QVERIFY(system->getObserver() != nullptr);
  }
  if (!guard.isNull() && !system.isNull()) {
    QVERIFY(system->getObserver() == nullptr);
    system->setActiveCoordinateSystemState(true);  // must not crash
  }
}

void FeatureObserverTest::jobRelaysIntoItsOwnSignals() {
  OiJob job;
  Point point;
  point.setFeatureName("P1");
  QVERIFY(job.addFeature(point.getFeatureWrapper()));

  QSignalSpy nameSpy(&job, &OiJob::featureNameChanged);
  QSignalSpy commentSpy(&job, &OiJob::featureCommentChanged);
  QSignalSpy commonSpy(&job, &OiJob::geometryIsCommonChanged);
  QSignalSpy functionSpy(&job, &OiJob::featureFunctionsChanged);

  point.setFeatureName("renamed");
  QCOMPARE(nameSpy.count(), 1);  // synchronous
  QCOMPARE(nameSpy.first().at(0).toInt(), point.getId());

  point.setComment("c");
  QCOMPARE(commentSpy.count(), 1);

  point.setCommonState(true);
  QCOMPARE(commonSpy.count(), 1);

  point.addFunction(new FakeFit());
  QCOMPARE(functionSpy.count(), 1);
}

void FeatureObserverTest::jobDeliversIsSolvedDeferred() {
  OiJob job;
  Point point;
  point.setFeatureName("P1");
  QVERIFY(job.addFeature(point.getFeatureWrapper()));

  QSignalSpy spy(&job, &OiJob::featureIsSolvedChanged);
  point.setIsSolved(true);
  QCOMPARE(spy.count(), 0);  // queued, not delivered inside the call
  QCoreApplication::processEvents();
  QCOMPARE(spy.count(), 1);
  QCOMPARE(spy.first().at(0).toInt(), point.getId());
}

QTEST_MAIN(FeatureObserverTest)
#include "tst_featureobserver.moc"
