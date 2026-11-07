#include <QCoreApplication>
#include <QString>
#include <QtTest>

#include "chooselalib.h"
#include "jobobserver.h"
#include "oijob.h"
#include "point.h"
#include "recordingjobobserver.h"
#include "station.h"
#include "trafoparam.h"

using namespace oi;

namespace {

// removes itself (and optionally another observer) while being notified
class SelfRemovingObserver : public JobObserver {
 public:
  OiJob* job = nullptr;
  JobObserver* alsoRemove = nullptr;
  int calls = 0;
  void onFeatureSetChanged() override {
    ++this->calls;
    this->job->removeObserver(this);
    if (this->alsoRemove != nullptr) {
      this->job->removeObserver(this->alsoRemove);
    }
  }
};

}  // namespace

class JobObserverTest : public QObject {
  Q_OBJECT

 private slots:
  void initTestCase();

  void addAndRemoveFeatureNotifySetChanges();
  void renameAndGroupAreReported();
  void activeFeatureStationAndSystemChanges();
  void trafoParamChangesAreReported();
  void messageIsDelivered();
  void recalculationNotifications();
  void recalcRequestsAreReported();
  void multipleObserversAreAllNotified();
  void observersAreDeduplicated();
  void removedObserverIsNotNotified();
  void observerMayRemoveItselfInCallback();
  void observerRemovedByAnotherIsSkipped();
  void jobDestroyedIsReported();
  void isSolvedIsDeliveredDeferred();
};

void JobObserverTest::initTestCase() {
  ChooseLALib::setLinearAlgebra(ChooseLALib::LinearAlgebraLibrary::Armadillo);
}

void JobObserverTest::addAndRemoveFeatureNotifySetChanges() {
  OiJob job;
  RecordingJobObserver rec;
  rec.attach(&job);

  Point* point = new Point();
  point->setFeatureName("P");
  point->setFeatureName("P1");
  QVERIFY(job.addFeature(point->getFeatureWrapper()));
  QCOMPARE(rec.count("featureSetChanged"), 1);
  QCOMPARE(rec.count("geometrySetChanged"), 1);
  QCOMPARE(rec.count("stationSetChanged"), 0);
  QCOMPARE(rec.count("coordSystemSetChanged"), 0);
  QCOMPARE(rec.count("trafoParamSetChanged"), 0);

  rec.calls.clear();
  QVERIFY(job.removeFeature(point->getId()));
  QCOMPARE(rec.count("featureSetChanged"), 1);
  QCOMPARE(rec.count("recalcFeatureSet"), 1);

  Station* station = new Station("S1");
  rec.calls.clear();
  QVERIFY(job.addFeature(station->getFeatureWrapper()));
  QCOMPARE(rec.count("featureSetChanged"), 1);
  QCOMPARE(rec.count("stationSetChanged"), 1);
  QCOMPARE(rec.count("coordSystemSetChanged"), 0);
  QCOMPARE(rec.count("geometrySetChanged"), 0);
}

void JobObserverTest::renameAndGroupAreReported() {
  OiJob job;
  RecordingJobObserver rec;
  rec.attach(&job);

  Point point;
  point.setFeatureName("point");
  point.setFeatureName("P1");
  QVERIFY(job.addFeature(point.getFeatureWrapper()));
  rec.calls.clear();

  point.setFeatureName("renamed");
  QCOMPARE(rec.count("featureNameChanged"), 1);
  QCOMPARE(rec.count("featureAttributesChanged"), 1);
  QVERIFY(rec.calls.contains(
      QString("featureNameChanged:%1:P1").arg(point.getId())));

  rec.calls.clear();
  point.setGroupName("G1");
  QCOMPARE(rec.count("featureGroupChanged"), 1);
  QCOMPARE(rec.count("availableGroupsChanged"), 1);
  QVERIFY(job.getFeatureGroupList().contains("G1"));

  rec.calls.clear();
  QVERIFY(job.setActiveGroup("G1"));
  QCOMPARE(rec.count("activeGroupChanged"), 1);
}

void JobObserverTest::activeFeatureStationAndSystemChanges() {
  OiJob job;
  RecordingJobObserver rec;
  rec.attach(&job);

  Point point;
  point.setFeatureName("point");
  QVERIFY(job.addFeature(point.getFeatureWrapper()));
  rec.calls.clear();
  point.setActiveFeatureState(true);
  QCOMPARE(rec.count("activeFeatureChanged"), 1);

  Station* station = new Station("S1");
  QVERIFY(job.addFeature(station->getFeatureWrapper()));
  rec.calls.clear();
  station->setActiveStationState(true);
  QCOMPARE(rec.count("activeStationChanged"), 1);

  rec.calls.clear();
  station->getCoordinateSystem()->setActiveCoordinateSystemState(true);
  QCOMPARE(rec.count("activeCoordinateSystemChanged"), 1);
}

void JobObserverTest::trafoParamChangesAreReported() {
  OiJob job;
  RecordingJobObserver rec;
  rec.attach(&job);

  Station* s1 = new Station("S1");
  Station* s2 = new Station("S2");
  QVERIFY(job.addFeature(s1->getFeatureWrapper()));
  QVERIFY(job.addFeature(s2->getFeatureWrapper()));
  TrafoParam* tp = new TrafoParam();
  tp->setFeatureName("T1");
  QVERIFY(tp->setCoordinateSystems(s1->getCoordinateSystem(),
                                   s2->getCoordinateSystem()));
  rec.calls.clear();
  QVERIFY(job.addFeature(tp->getFeatureWrapper()));
  QCOMPARE(rec.count("trafoParamSetChanged"), 1);
  rec.calls.clear();

  tp->setIsUsed(true);
  QCOMPARE(rec.count("trafoParamIsUsedChanged"), 1);
  QVERIFY(rec.calls.contains(
      QString("trafoParamIsUsedChanged:%1").arg(tp->getId())));
  tp->setIsDatumTrafo(true);
  QCOMPARE(rec.count("trafoParamIsDatumChanged"), 1);
}

void JobObserverTest::messageIsDelivered() {
  OiJob job;
  RecordingJobObserver rec;
  rec.attach(&job);

  job.sendMessage("hello", MessageTypes::eWarningMessage);
  QCOMPARE(rec.count("sendMessage"), 1);
  QCOMPARE(rec.calls.last(),
           QString("sendMessage:hello:%1:%2")
               .arg(static_cast<int>(MessageTypes::eWarningMessage))
               .arg(static_cast<int>(MessageDestinations::eConsoleMessage)));

  // a failing operation reports through the same path
  rec.calls.clear();
  QVERIFY(!job.removeFeature(424242));
  QVERIFY(rec.count("sendMessage") <= 1);
}

void JobObserverTest::recalculationNotifications() {
  OiJob job;
  RecordingJobObserver rec;
  rec.attach(&job);

  job.featureRecalculated(7);
  job.featuresRecalculated();
  job.trafoParamRecalculated(9);
  QCOMPARE(rec.calls,
           (QStringList{"featureRecalculated:7", "featuresRecalculated",
                        "trafoParamRecalculated:9"}));
}

void JobObserverTest::recalcRequestsAreReported() {
  OiJob job;
  RecordingJobObserver rec;
  rec.attach(&job);

  Point* point = new Point();
  point->setFeatureName("P");
  QVERIFY(job.addFeature(point->getFeatureWrapper()));
  rec.calls.clear();
  QVERIFY(job.removeFeature(point->getId()));
  QCOMPARE(rec.count("recalcFeatureSet"), 1);
}

void JobObserverTest::multipleObserversAreAllNotified() {
  OiJob job;
  RecordingJobObserver a, b;
  a.attach(&job);
  b.attach(&job);

  Point point;
  point.setFeatureName("point");
  QVERIFY(job.addFeature(point.getFeatureWrapper()));
  QCOMPARE(a.count("featureSetChanged"), 1);
  QCOMPARE(b.count("featureSetChanged"), 1);
  QCOMPARE(a.calls, b.calls);
}

void JobObserverTest::observersAreDeduplicated() {
  OiJob job;
  RecordingJobObserver rec;
  job.addObserver(&rec);
  job.addObserver(&rec);
  rec.attach(&job);

  job.featureRecalculated(1);
  QCOMPARE(rec.count("featureRecalculated"), 1);
  job.removeObserver(&rec);
  job.featureRecalculated(1);
  QCOMPARE(rec.count("featureRecalculated"), 1);
}

void JobObserverTest::removedObserverIsNotNotified() {
  OiJob job;
  RecordingJobObserver a, b;
  a.attach(&job);
  b.attach(&job);
  job.removeObserver(&a);
  job.removeObserver(&a);  // harmless
  job.removeObserver(nullptr);

  job.featuresRecalculated();
  QCOMPARE(a.count("featuresRecalculated"), 0);
  QCOMPARE(b.count("featuresRecalculated"), 1);
}

void JobObserverTest::observerMayRemoveItselfInCallback() {
  OiJob job;
  SelfRemovingObserver self;
  RecordingJobObserver after;
  self.job = &job;
  job.addObserver(&self);
  after.attach(&job);

  Point p1, p2;
  p1.setFeatureName("p1");
  p2.setFeatureName("p2");
  QVERIFY(job.addFeature(p1.getFeatureWrapper()));
  QCOMPARE(self.calls, 1);
  QCOMPARE(after.count("featureSetChanged"), 1);  // still reached
  QVERIFY(job.addFeature(p2.getFeatureWrapper()));
  QCOMPARE(self.calls, 1);  // gone
  QCOMPARE(after.count("featureSetChanged"), 2);
}

void JobObserverTest::observerRemovedByAnotherIsSkipped() {
  OiJob job;
  SelfRemovingObserver first;
  RecordingJobObserver victim;
  first.job = &job;
  first.alsoRemove = &victim;
  job.addObserver(&first);
  victim.attach(&job);

  Point p;
  p.setFeatureName("p");
  QVERIFY(job.addFeature(p.getFeatureWrapper()));
  QCOMPARE(victim.count("featureSetChanged"), 0);
}

void JobObserverTest::jobDestroyedIsReported() {
  RecordingJobObserver rec;
  RecordingJobObserver other;
  OiJob* job = new OiJob();
  rec.attach(job);
  other.attach(job);
  delete job;
  QCOMPARE(rec.destroyed.size(), 1);
  QCOMPARE(rec.destroyed.first(), job);
  QCOMPARE(other.destroyed.size(), 1);
}

void JobObserverTest::isSolvedIsDeliveredDeferred() {
  OiJob job;
  RecordingJobObserver rec;
  rec.attach(&job);
  Point point;
  point.setFeatureName("point");
  QVERIFY(job.addFeature(point.getFeatureWrapper()));
  rec.calls.clear();

  point.setIsSolved(true);
  QCOMPARE(rec.count("featureIsSolvedChanged"), 0);  // queued
  QCoreApplication::processEvents();
  QCOMPARE(rec.count("featureIsSolvedChanged"), 1);
}

QTEST_MAIN(JobObserverTest)
#include "tst_jobobserver.moc"
