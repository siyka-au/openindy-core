#include <catch2/catch_test_macros.hpp>
#include <string>
#include <vector>

#include "catch_compat.h"
#include "chooselalib.h"
#include "constructfunction.h"
#include "containers.h"
#include "featureobserver.h"
#include "fitfunction.h"
#include "group.h"
#include "oijob.h"
#include "point.h"
#include "queuedexecutor.h"
#include "recordingjobobserver.h"
#include "station.h"
#include "trafoparam.h"

using namespace oi;

namespace {

// A function a Point accepts as its producer (see tst_ownership)
class FakeFit : public FitFunction, public Producer<Point> {
 public:
  explicit FakeFit() : FitFunction() { this->metaData.name = "FakeFit"; }
  std::expected<Point, std::string> exec() override { return Point(); }
};

// Remembers every notification as "name:arg:arg..." in arrival order
class RecordingObserver : public FeatureObserver {
 public:
  std::vector<std::string> calls;

  [[nodiscard]] int count(const std::string& name) const {
    int n = 0;
    for (const std::string& c : this->calls) {
      if (c == name || c.starts_with(name + ":")) {
        ++n;
      }
    }
    return n;
  }

  void onElementAboutToBeDeleted(const int& id, const std::string&,
                                 const std::string&,
                                 const FeatureTypes&) override {
    this->calls.push_back(std::format("elementAboutToBeDeleted:{}", id));
  }
  void onFeatureNameChanged(const int& id, const std::string& old) override {
    this->calls.push_back(std::format("featureName:{}:{}", id, old));
  }
  void onFeatureGroupChanged(const int& id, const std::string& old) override {
    this->calls.push_back(std::format("featureGroup:{}:{}", id, old));
  }
  void onFeatureCommentChanged(const int& id) override {
    this->calls.push_back(std::format("featureComment:{}", id));
  }
  void onFeatureIsSolvedChanged(const int& id) override {
    this->calls.push_back(std::format("featureIsSolved:{}", id));
  }
  void onFeatureFunctionListChanged(const int& id) override {
    this->calls.push_back(std::format("featureFunctionList:{}", id));
  }
  void onFeatureUsedForChanged(const int& id) override {
    this->calls.push_back(std::format("featureUsedFor:{}", id));
  }
  void onGeomIsCommonChanged(const int& id) override {
    this->calls.push_back(std::format("geomIsCommon:{}", id));
  }
  void onGeomMeasurementConfigChanged(const int& id, const std::string& old,
                                      const MeasurementConfigKey) override {
    this->calls.push_back(std::format("geomMeasurementConfig:{}:{}", id, old));
  }
  void onGroupMembersChanged(const int& id) override {
    this->calls.push_back(std::format("groupMembers:{}", id));
  }
  void onIsUsedChanged(const int& id) override {
    this->calls.push_back(std::format("trafoIsUsed:{}", id));
  }
  void onIsDatumTrafoChanged(const int& id) override {
    this->calls.push_back(std::format("trafoIsDatum:{}", id));
  }
  void onObservationsChanged(const int& id, const int& obs) override {
    this->calls.push_back(std::format("systemObservations:{}:{}", id, obs));
  }
};

}  // namespace

/*!
 * Step 5A: the feature model reports to a plain FeatureObserver instead of
 * Qt signals; OiJob is that observer and relays into its own signals.
 */
class FeatureObserverTest {
 public:
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
  OI_COMPARE(point.getObserver(), &rec);

  point.setFeatureName("P1");
  OI_COMPARE(rec.count("featureName"), 1);
  OI_COMPARE(rec.calls.back(), std::format("featureName:{}:", point.getId()));

  point.setFeatureName("P1");  // unchanged: nothing
  OI_COMPARE(rec.count("featureName"), 1);

  point.setComment("hello");
  OI_COMPARE(rec.count("featureComment"), 1);

  point.setGroupName("G");
  OI_COMPARE(rec.count("featureGroup"), 1);

  point.setIsSolved(true);
  OI_COMPARE(rec.count("featureIsSolved"), 1);
}

void FeatureObserverTest::functionAddedAndUsedFor() {
  RecordingObserver rec;
  Point point;
  Point other;
  point.setObserver(&rec);

  point.addFunction(new FakeFit());
  OI_COMPARE(rec.count("featureFunctionList"), 1);

  // usedFor/previouslyNeeded bookkeeping never notified (nothing emitted the
  // old signals either); it must stay silent and harmless
  REQUIRE(point.addUsedFor(other.getFeatureWrapper()));
  OI_COMPARE(rec.count("featureUsedFor"), 0);
}

void FeatureObserverTest::geometryChanges() {
  RecordingObserver rec;
  Point point;
  point.setObserver(&rec);

  point.setCommonState(true);
  OI_COMPARE(rec.count("geomIsCommon"), 1);
  point.setCommonState(true);
  OI_COMPARE(rec.count("geomIsCommon"), 1);

  point.setMeasurementConfig(MeasurementConfigKey());
  OI_COMPARE(rec.count("geomMeasurementConfig"), 1);
}

void FeatureObserverTest::trafoParamChanges() {
  RecordingObserver rec;
  TrafoParam tp;
  tp.setObserver(&rec);

  tp.setIsUsed(true);
  OI_COMPARE(rec.count("trafoIsUsed"), 1);
  tp.setIsUsed(true);
  OI_COMPARE(rec.count("trafoIsUsed"), 1);

  tp.setIsDatumTrafo(true);
  OI_COMPARE(rec.count("trafoIsDatum"), 1);
}

void FeatureObserverTest::groupMemberAdded() {
  RecordingObserver rec;
  Group group("G");
  Point member;
  group.setObserver(&rec);

  REQUIRE(group.addMember(member.getFeatureWrapper()));
  OI_COMPARE(rec.count("groupMembers"), 1);
  REQUIRE(!group.addMember(member.getFeatureWrapper()));  // duplicate
  OI_COMPARE(rec.count("groupMembers"), 1);
  REQUIRE(group.removeMember(member.getFeatureWrapper()));
  OI_COMPARE(rec.count("groupMembers"), 2);
}

void FeatureObserverTest::stationSystemObservationAdded() {
  OiJob job;
  auto ownedStation = std::make_unique<Station>("S1");
  Station* station = ownedStation.get();
  REQUIRE(job.addFeature(std::move(ownedStation)));

  RecordingJobObserver spy;
  spy.attach(&job);

  ReadingCartesian r;
  r.isValid = true;
  oi::Guarded<Reading> reading = new Reading(r);
  oi::Guarded<Observation> obs = new Observation();
  reading->setObservation(obs);
  REQUIRE(station->getCoordinateSystem()->addObservation(obs));
  OI_COMPARE(spy.count("systemObservationsChanged"), 1);
}

void FeatureObserverTest::blockedSignalsStaySilent() {
  RecordingObserver rec;
  Point point;
  point.setObserver(&rec);

  point.blockSignals(true);
  point.setFeatureName("quiet");
  point.setCommonState(true);
  point.blockSignals(false);
  REQUIRE(rec.calls.empty());

  point.announceObservationsChanged();
  OI_COMPARE(rec.count("geomObservations"),
             0);  // not recorded by this observer
}

void FeatureObserverTest::featuresWithoutObserverNotifyNobody() {
  Point point;
  REQUIRE(point.getObserver() == nullptr);
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
  REQUIRE(group.addMember(member.getFeatureWrapper()));
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
  OI_COMPARE(rec.count("elementAboutToBeDeleted"), 1);
  OI_COMPARE(rec.calls.back(), std::format("elementAboutToBeDeleted:{}", id));
}

void FeatureObserverTest::observerIsClearedWhenFeatureIsRemovedFromJob() {
  OiJob job;
  auto own_point = std::make_unique<Point>();
  Point* point = own_point.get();
  point->setFeatureName("P1");
  oi::Guarded<Point> guard(point);
  REQUIRE(job.addFeature(std::move(own_point)));
  OI_COMPARE(point->getObserver(), static_cast<FeatureObserver*>(&job));
  OI_COMPARE(job.getFeatureCount(), 1);

  // removing deletes the feature; the job must have let go of it first so
  // that its destruction does not call back into the job
  REQUIRE(job.removeFeature(point->getFeatureWrapper()));
  REQUIRE(guard.isNull());
  OI_COMPARE(job.getFeatureCount(), 0);
}

void FeatureObserverTest::observerIsClearedWhenJobIsDestroyed() {
  auto own_point = std::make_unique<Point>();
  Point* point = own_point.get();
  point->setFeatureName("P1");
  oi::Guarded<Point> guard(point);
  RecordingObserver rec;

  {
    OiJob job;
    REQUIRE(job.addFeature(std::move(own_point)));
    OI_COMPARE(point->getObserver(), static_cast<FeatureObserver*>(&job));
  }

  // the job owns its features: destroying it destroys them, after letting
  // go of them, so nothing ever calls into the dead job and the feature is
  // not leaked
  REQUIRE(guard.isNull());
  OI_COMPARE(rec.count("elementAboutToBeDeleted"), 0);
}

void FeatureObserverTest::stationSystemObserverIsClearedWhenJobIsDestroyed() {
  auto own_station = std::make_unique<Station>("S1");
  Station* station = own_station.get();
  oi::Guarded<Station> guard(station);
  oi::Guarded<CoordinateSystem> system = station->getCoordinateSystem();
  REQUIRE(!system.isNull());
  {
    OiJob job;
    REQUIRE(job.addFeature(std::move(own_station)));
    REQUIRE(system->getObserver() != nullptr);
  }
  if (!guard.isNull() && !system.isNull()) {
    REQUIRE(system->getObserver() == nullptr);
    system->setActiveCoordinateSystemState(true);  // must not crash
  }
}

void FeatureObserverTest::jobRelaysIntoItsOwnSignals() {
  OiJob job;
  auto own_point = std::make_unique<Point>();
  Point& point = *own_point;
  point.setFeatureName("P1");
  REQUIRE(job.addFeature(std::move(own_point)));

  RecordingJobObserver rec;
  rec.attach(&job);

  point.setFeatureName("renamed");
  OI_COMPARE(rec.count("featureNameChanged"), 1);  // synchronous
  REQUIRE(oi::contains(rec.calls,
                       std::format("featureNameChanged:{}:P1", point.getId())));

  point.setComment("c");
  OI_COMPARE(rec.count("featureCommentChanged"), 1);

  point.setCommonState(true);
  OI_COMPARE(rec.count("geometryIsCommonChanged"), 1);

  point.addFunction(new FakeFit());
  OI_COMPARE(rec.count("featureFunctionsChanged"), 1);
}

void FeatureObserverTest::jobDeliversIsSolvedDeferred() {
  QueuedExecutor executor;
  OiJob job;
  job.setExecutor(&executor);
  auto own_point = std::make_unique<Point>();
  Point& point = *own_point;
  point.setFeatureName("P1");
  REQUIRE(job.addFeature(std::move(own_point)));

  RecordingJobObserver spy;
  spy.attach(&job);
  point.setIsSolved(true);
  OI_COMPARE(spy.count("featureIsSolvedChanged"), 0);  // queued, not delivered
  executor.flush();
  OI_COMPARE(spy.count("featureIsSolvedChanged"), 1);
  REQUIRE(oi::contains(
      spy.calls, std::format("featureIsSolvedChanged:{}", point.getId())));
}

TEST_CASE("FeatureObserverTest::renameCommentGroupSolved",
          "[FeatureObserverTest]") {
  FeatureObserverTest fixture;
  fixture.initTestCase();
  fixture.renameCommentGroupSolved();
}

TEST_CASE("FeatureObserverTest::functionAddedAndUsedFor",
          "[FeatureObserverTest]") {
  FeatureObserverTest fixture;
  fixture.initTestCase();
  fixture.functionAddedAndUsedFor();
}

TEST_CASE("FeatureObserverTest::geometryChanges", "[FeatureObserverTest]") {
  FeatureObserverTest fixture;
  fixture.initTestCase();
  fixture.geometryChanges();
}

TEST_CASE("FeatureObserverTest::trafoParamChanges", "[FeatureObserverTest]") {
  FeatureObserverTest fixture;
  fixture.initTestCase();
  fixture.trafoParamChanges();
}

TEST_CASE("FeatureObserverTest::groupMemberAdded", "[FeatureObserverTest]") {
  FeatureObserverTest fixture;
  fixture.initTestCase();
  fixture.groupMemberAdded();
}

TEST_CASE("FeatureObserverTest::stationSystemObservationAdded",
          "[FeatureObserverTest]") {
  FeatureObserverTest fixture;
  fixture.initTestCase();
  fixture.stationSystemObservationAdded();
}

TEST_CASE("FeatureObserverTest::blockedSignalsStaySilent",
          "[FeatureObserverTest]") {
  FeatureObserverTest fixture;
  fixture.initTestCase();
  fixture.blockedSignalsStaySilent();
}

TEST_CASE("FeatureObserverTest::featuresWithoutObserverNotifyNobody",
          "[FeatureObserverTest]") {
  FeatureObserverTest fixture;
  fixture.initTestCase();
  fixture.featuresWithoutObserverNotifyNobody();
}

TEST_CASE("FeatureObserverTest::destroyingTheFeatureTellsTheObserverOnce",
          "[FeatureObserverTest]") {
  FeatureObserverTest fixture;
  fixture.initTestCase();
  fixture.destroyingTheFeatureTellsTheObserverOnce();
}

TEST_CASE("FeatureObserverTest::observerIsClearedWhenFeatureIsRemovedFromJob",
          "[FeatureObserverTest]") {
  FeatureObserverTest fixture;
  fixture.initTestCase();
  fixture.observerIsClearedWhenFeatureIsRemovedFromJob();
}

TEST_CASE("FeatureObserverTest::observerIsClearedWhenJobIsDestroyed",
          "[FeatureObserverTest]") {
  FeatureObserverTest fixture;
  fixture.initTestCase();
  fixture.observerIsClearedWhenJobIsDestroyed();
}

TEST_CASE(
    "FeatureObserverTest::stationSystemObserverIsClearedWhenJobIsDestroyed",
    "[FeatureObserverTest]") {
  FeatureObserverTest fixture;
  fixture.initTestCase();
  fixture.stationSystemObserverIsClearedWhenJobIsDestroyed();
}

TEST_CASE("FeatureObserverTest::jobRelaysIntoItsOwnSignals",
          "[FeatureObserverTest]") {
  FeatureObserverTest fixture;
  fixture.initTestCase();
  fixture.jobRelaysIntoItsOwnSignals();
}

TEST_CASE("FeatureObserverTest::jobDeliversIsSolvedDeferred",
          "[FeatureObserverTest]") {
  FeatureObserverTest fixture;
  fixture.initTestCase();
  fixture.jobDeliversIsSolvedDeferred();
}

#include <format>
