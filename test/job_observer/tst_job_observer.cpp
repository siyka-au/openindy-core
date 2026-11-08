#include <catch2/catch_test_macros.hpp>
#include <string>
#include <vector>

#include "catch_compat.h"
#include "choose_la_lib.h"
#include "containers.h"
#include "job_observer.h"
#include "oi_job.h"
#include "point.h"
#include "queued_executor.h"
#include "recording_job_observer.h"
#include "station.h"
#include "trafo_param.h"

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

class JobObserverTest {
 public:
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

  auto own_point = std::make_unique<Point>();
  Point* point = own_point.get();
  point->setFeatureName("P");
  point->setFeatureName("P1");
  REQUIRE(job.addFeature(std::move(own_point)));
  OI_COMPARE(rec.count("featureSetChanged"), 1);
  OI_COMPARE(rec.count("geometrySetChanged"), 1);
  OI_COMPARE(rec.count("stationSetChanged"), 0);
  OI_COMPARE(rec.count("coordSystemSetChanged"), 0);
  OI_COMPARE(rec.count("trafoParamSetChanged"), 0);

  rec.calls.clear();
  REQUIRE(job.removeFeature(point->getId()));
  OI_COMPARE(rec.count("featureSetChanged"), 1);
  OI_COMPARE(rec.count("recalcFeatureSet"), 1);

  auto own_station = std::make_unique<Station>("S1");
  Station* station = own_station.get();
  rec.calls.clear();
  REQUIRE(job.addFeature(std::move(own_station)));
  OI_COMPARE(rec.count("featureSetChanged"), 1);
  OI_COMPARE(rec.count("stationSetChanged"), 1);
  OI_COMPARE(rec.count("coordSystemSetChanged"), 0);
  OI_COMPARE(rec.count("geometrySetChanged"), 0);
}

void JobObserverTest::renameAndGroupAreReported() {
  OiJob job;
  RecordingJobObserver rec;
  rec.attach(&job);

  auto own_point = std::make_unique<Point>();
  Point& point = *own_point;
  point.setFeatureName("point");
  point.setFeatureName("P1");
  REQUIRE(job.addFeature(std::move(own_point)));
  rec.calls.clear();

  point.setFeatureName("renamed");
  OI_COMPARE(rec.count("featureNameChanged"), 1);
  OI_COMPARE(rec.count("featureAttributesChanged"), 1);
  REQUIRE(oi::contains(rec.calls,
                       std::format("featureNameChanged:{}:P1", point.getId())));

  rec.calls.clear();
  point.setGroupName("G1");
  OI_COMPARE(rec.count("featureGroupChanged"), 1);
  OI_COMPARE(rec.count("availableGroupsChanged"), 1);
  REQUIRE(oi::contains(job.getFeatureGroupList(), "G1"));

  rec.calls.clear();
  REQUIRE(job.setActiveGroup("G1"));
  OI_COMPARE(rec.count("activeGroupChanged"), 1);
}

void JobObserverTest::activeFeatureStationAndSystemChanges() {
  OiJob job;
  RecordingJobObserver rec;
  rec.attach(&job);

  auto own_point = std::make_unique<Point>();
  Point& point = *own_point;
  point.setFeatureName("point");
  REQUIRE(job.addFeature(std::move(own_point)));
  rec.calls.clear();
  point.setActiveFeatureState(true);
  OI_COMPARE(rec.count("activeFeatureChanged"), 1);

  auto own_station = std::make_unique<Station>("S1");
  Station* station = own_station.get();
  REQUIRE(job.addFeature(std::move(own_station)));
  rec.calls.clear();
  station->setActiveStationState(true);
  OI_COMPARE(rec.count("activeStationChanged"), 1);

  rec.calls.clear();
  station->getCoordinateSystem()->setActiveCoordinateSystemState(true);
  OI_COMPARE(rec.count("activeCoordinateSystemChanged"), 1);
}

void JobObserverTest::trafoParamChangesAreReported() {
  OiJob job;
  RecordingJobObserver rec;
  rec.attach(&job);

  auto own_s1 = std::make_unique<Station>("S1");
  Station* s1 = own_s1.get();
  auto own_s2 = std::make_unique<Station>("S2");
  Station* s2 = own_s2.get();
  REQUIRE(job.addFeature(std::move(own_s1)));
  REQUIRE(job.addFeature(std::move(own_s2)));
  auto own_tp = std::make_unique<TrafoParam>();
  TrafoParam* tp = own_tp.get();
  tp->setFeatureName("T1");
  REQUIRE(tp->setCoordinateSystems(s1->getCoordinateSystem(),
                                   s2->getCoordinateSystem()));
  rec.calls.clear();
  REQUIRE(job.addFeature(std::move(own_tp)));
  OI_COMPARE(rec.count("trafoParamSetChanged"), 1);
  rec.calls.clear();

  tp->setIsUsed(true);
  OI_COMPARE(rec.count("trafoParamIsUsedChanged"), 1);
  REQUIRE(oi::contains(rec.calls,
                       std::format("trafoParamIsUsedChanged:{}", tp->getId())));
  tp->setIsDatumTrafo(true);
  OI_COMPARE(rec.count("trafoParamIsDatumChanged"), 1);
}

void JobObserverTest::messageIsDelivered() {
  OiJob job;
  RecordingJobObserver rec;
  rec.attach(&job);

  job.sendMessage("hello", MessageTypes::eWarningMessage);
  OI_COMPARE(rec.count("sendMessage"), 1);
  OI_COMPARE(
      rec.calls.back(),
      std::format("sendMessage:hello:{}:{}",
                  static_cast<int>(MessageTypes::eWarningMessage),
                  static_cast<int>(MessageDestinations::eConsoleMessage)));

  // a failing operation reports through the same path
  rec.calls.clear();
  REQUIRE(!job.removeFeature(424242));
  REQUIRE(rec.count("sendMessage") <= 1);
}

void JobObserverTest::recalculationNotifications() {
  OiJob job;
  RecordingJobObserver rec;
  rec.attach(&job);

  job.featureRecalculated(7);
  job.featuresRecalculated();
  job.trafoParamRecalculated(9);
  OI_COMPARE(rec.calls, (std::vector<std::string>{"featureRecalculated:7",
                                                  "featuresRecalculated",
                                                  "trafoParamRecalculated:9"}));
}

void JobObserverTest::recalcRequestsAreReported() {
  OiJob job;
  RecordingJobObserver rec;
  rec.attach(&job);

  auto own_point = std::make_unique<Point>();
  Point* point = own_point.get();
  point->setFeatureName("P");
  REQUIRE(job.addFeature(std::move(own_point)));
  rec.calls.clear();
  REQUIRE(job.removeFeature(point->getId()));
  OI_COMPARE(rec.count("recalcFeatureSet"), 1);
}

void JobObserverTest::multipleObserversAreAllNotified() {
  OiJob job;
  RecordingJobObserver a, b;
  a.attach(&job);
  b.attach(&job);

  auto own_point = std::make_unique<Point>();
  Point& point = *own_point;
  point.setFeatureName("point");
  REQUIRE(job.addFeature(std::move(own_point)));
  OI_COMPARE(a.count("featureSetChanged"), 1);
  OI_COMPARE(b.count("featureSetChanged"), 1);
  OI_COMPARE(a.calls, b.calls);
}

void JobObserverTest::observersAreDeduplicated() {
  OiJob job;
  RecordingJobObserver rec;
  job.addObserver(&rec);
  job.addObserver(&rec);
  rec.attach(&job);

  job.featureRecalculated(1);
  OI_COMPARE(rec.count("featureRecalculated"), 1);
  job.removeObserver(&rec);
  job.featureRecalculated(1);
  OI_COMPARE(rec.count("featureRecalculated"), 1);
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
  OI_COMPARE(a.count("featuresRecalculated"), 0);
  OI_COMPARE(b.count("featuresRecalculated"), 1);
}

void JobObserverTest::observerMayRemoveItselfInCallback() {
  OiJob job;
  SelfRemovingObserver self;
  RecordingJobObserver after;
  self.job = &job;
  job.addObserver(&self);
  after.attach(&job);

  auto p1 = std::make_unique<Point>();
  auto p2 = std::make_unique<Point>();
  p1->setFeatureName("p1");
  p2->setFeatureName("p2");
  REQUIRE(job.addFeature(std::move(p1)));
  OI_COMPARE(self.calls, 1);
  OI_COMPARE(after.count("featureSetChanged"), 1);  // still reached
  REQUIRE(job.addFeature(std::move(p2)));
  OI_COMPARE(self.calls, 1);  // gone
  OI_COMPARE(after.count("featureSetChanged"), 2);
}

void JobObserverTest::observerRemovedByAnotherIsSkipped() {
  OiJob job;
  SelfRemovingObserver first;
  RecordingJobObserver victim;
  first.job = &job;
  first.alsoRemove = &victim;
  job.addObserver(&first);
  victim.attach(&job);

  auto own_p = std::make_unique<Point>();
  Point& p = *own_p;
  p.setFeatureName("p");
  REQUIRE(job.addFeature(std::move(own_p)));
  OI_COMPARE(victim.count("featureSetChanged"), 0);
}

void JobObserverTest::jobDestroyedIsReported() {
  RecordingJobObserver rec;
  RecordingJobObserver other;
  OiJob* job = new OiJob();
  rec.attach(job);
  other.attach(job);
  delete job;
  OI_COMPARE(rec.destroyed.size(), 1);
  OI_COMPARE(rec.destroyed.front(), job);
  OI_COMPARE(other.destroyed.size(), 1);
}

void JobObserverTest::isSolvedIsDeliveredDeferred() {
  QueuedExecutor executor;
  OiJob job;
  job.setExecutor(&executor);
  RecordingJobObserver rec;
  rec.attach(&job);
  auto own_point = std::make_unique<Point>();
  Point& point = *own_point;
  point.setFeatureName("point");
  REQUIRE(job.addFeature(std::move(own_point)));
  rec.calls.clear();

  point.setIsSolved(true);
  OI_COMPARE(rec.count("featureIsSolvedChanged"), 0);  // queued
  executor.flush();
  OI_COMPARE(rec.count("featureIsSolvedChanged"), 1);
}

TEST_CASE("JobObserverTest::addAndRemoveFeatureNotifySetChanges",
          "[JobObserverTest]") {
  JobObserverTest fixture;
  fixture.initTestCase();
  fixture.addAndRemoveFeatureNotifySetChanges();
}

TEST_CASE("JobObserverTest::renameAndGroupAreReported", "[JobObserverTest]") {
  JobObserverTest fixture;
  fixture.initTestCase();
  fixture.renameAndGroupAreReported();
}

TEST_CASE("JobObserverTest::activeFeatureStationAndSystemChanges",
          "[JobObserverTest]") {
  JobObserverTest fixture;
  fixture.initTestCase();
  fixture.activeFeatureStationAndSystemChanges();
}

TEST_CASE("JobObserverTest::trafoParamChangesAreReported",
          "[JobObserverTest]") {
  JobObserverTest fixture;
  fixture.initTestCase();
  fixture.trafoParamChangesAreReported();
}

TEST_CASE("JobObserverTest::messageIsDelivered", "[JobObserverTest]") {
  JobObserverTest fixture;
  fixture.initTestCase();
  fixture.messageIsDelivered();
}

TEST_CASE("JobObserverTest::recalculationNotifications", "[JobObserverTest]") {
  JobObserverTest fixture;
  fixture.initTestCase();
  fixture.recalculationNotifications();
}

TEST_CASE("JobObserverTest::recalcRequestsAreReported", "[JobObserverTest]") {
  JobObserverTest fixture;
  fixture.initTestCase();
  fixture.recalcRequestsAreReported();
}

TEST_CASE("JobObserverTest::multipleObserversAreAllNotified",
          "[JobObserverTest]") {
  JobObserverTest fixture;
  fixture.initTestCase();
  fixture.multipleObserversAreAllNotified();
}

TEST_CASE("JobObserverTest::observersAreDeduplicated", "[JobObserverTest]") {
  JobObserverTest fixture;
  fixture.initTestCase();
  fixture.observersAreDeduplicated();
}

TEST_CASE("JobObserverTest::removedObserverIsNotNotified",
          "[JobObserverTest]") {
  JobObserverTest fixture;
  fixture.initTestCase();
  fixture.removedObserverIsNotNotified();
}

TEST_CASE("JobObserverTest::observerMayRemoveItselfInCallback",
          "[JobObserverTest]") {
  JobObserverTest fixture;
  fixture.initTestCase();
  fixture.observerMayRemoveItselfInCallback();
}

TEST_CASE("JobObserverTest::observerRemovedByAnotherIsSkipped",
          "[JobObserverTest]") {
  JobObserverTest fixture;
  fixture.initTestCase();
  fixture.observerRemovedByAnotherIsSkipped();
}

TEST_CASE("JobObserverTest::jobDestroyedIsReported", "[JobObserverTest]") {
  JobObserverTest fixture;
  fixture.initTestCase();
  fixture.jobDestroyedIsReported();
}

TEST_CASE("JobObserverTest::isSolvedIsDeliveredDeferred", "[JobObserverTest]") {
  JobObserverTest fixture;
  fixture.initTestCase();
  fixture.isSolvedIsDeliveredDeferred();
}

#include <format>
