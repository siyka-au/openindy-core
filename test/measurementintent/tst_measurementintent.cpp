#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <string>
#include <vector>

#include "catch_compat.h"
#include "chooselalib.h"
#include "constructfunction.h"
#include "containers.h"
#include "featurewrapper.h"
#include "geometry/point.h"
#include "oijob.h"
#include "reading.h"
#include "recordingjobobserver.h"
#include "station.h"

using namespace oi;
using namespace oi::math;

namespace {

// minimal stand-in for a real plugin function - only needs to declare a
// NeededElement, exactly like tst_ownership.cpp's FakeFit/FakeConstruct.
// Used to prove tag resolution (OiJob::resolvePointIntent) feeds a
// function's InputElements without any binding step, independent of any
// particular plugin's math. Implements Producer<Point> (Stage 7d/D21) so
// Feature::addFunction() actually attaches it - a fake with no real exec()
// would otherwise be rejected at attach time.
class FakeConsumer : public ConstructFunction, public Producer<Point> {
 public:
  explicit FakeConsumer(const std::string& roleName = "default")
      : ConstructFunction() {
    this->metaData.name = "FakeConsumer";
    NeededElement param;
    param.typeOfElement = ElementTypes::ePointElement;
    param.infinite = true;
    param.roleName = roleName;
    this->neededElements.push_back(param);
    this->applicableFor.push_back(FeatureTypes::ePointFeature);
  }
  std::expected<Point, std::string> exec() override { return Point(); }
};

}  // namespace

/*!
 * Stage 7f regression coverage for D12 (intent-tagged acquisition) and
 * D15a/D20 (a Point is one physical target as captured in one
 * acquisition, owned by its Station). Exercises OiJob::addMeasurementResults
 * end to end - the mechanism the acceptance criteria describe - plus
 * Point::recalc()'s own observation-averaging in isolation, since core has
 * no TrafoController/FeatureUpdater to drive the full transform+recalc
 * cascade the real app uses (see the comments in oijob.cpp/point.cpp this
 * test exercises).
 */
class MeasurementIntentTest {
 public:
  void initTestCase();

  // Point::recalc() computing its own position from observations
  void pointWithObservationsAndNoFunctionAveragesThem();
  void pointWithObservationsIsStationOwned();
  void pointWithNoObservationsIsUnaffected();

  // OiJob::addMeasurementResults structural behaviour
  void measuringAlwaysCreatesExactlyOneNewPoint();
  void measuredPointHasObservationsDerivedPointDoesNot();
  void tagResolutionFeedsAMatchingFunctionWithNoBindingStep();
  void aPointCarryingTwoTagsIsFoundByBothConsumers();
  void renamingTheFeatureDoesNotOrphanTags();
  void unresolvableTargetIsRejectedWithAMessage();

 private:
  std::unique_ptr<OiJob> makeJobWithActiveStation(
      oi::Guarded<Station>& outStation);
  std::unique_ptr<Reading> makeCartesianReading(double x, double y, double z);
};

void MeasurementIntentTest::initTestCase() {
  ChooseLALib::setLinearAlgebra(ChooseLALib::LinearAlgebraLibrary::Armadillo);
}

std::unique_ptr<OiJob> MeasurementIntentTest::makeJobWithActiveStation(
    oi::Guarded<Station>& outStation) {
  auto job = std::make_unique<OiJob>();

  auto own_station = std::make_unique<oi::Station>("S1");
  oi::Guarded<Station> station = own_station.get();
  oi::Guarded<oi::FeatureWrapper> stationFeature = station->getFeatureWrapper();
  job->addFeature(std::move(own_station));

  // Station::activeStationChanged is connected (OiJob::attachFeature) to
  // OiJob::setActiveStation - this is the ordinary way a station becomes
  // the job's active one, not a test-only shortcut
  station->setActiveStationState(true);

  outStation = station;
  return job;
}

std::unique_ptr<Reading> MeasurementIntentTest::makeCartesianReading(double x,
                                                                     double y,
                                                                     double z) {
  ReadingCartesian r;
  r.isValid = true;
  r.xyz.setAt(0, x);
  r.xyz.setAt(1, y);
  r.xyz.setAt(2, z);
  return std::make_unique<Reading>(r);
}

void MeasurementIntentTest::pointWithObservationsAndNoFunctionAveragesThem() {
  auto own_point = std::make_unique<Point>();
  const oi::Guarded<Point> point = own_point.get();

  // stand in for "TrafoController already transformed these" (core has
  // no TrafoController - see point.cpp's Point::recalc() comment)
  OiVec xyzA(4);
  xyzA.setAt(0, 0.0);
  xyzA.setAt(1, 0.0);
  xyzA.setAt(2, 0.0);
  xyzA.setAt(3, 1.0);
  auto own_obsA = std::make_unique<Observation>();
  const oi::Guarded<Observation> obsA = own_obsA.get();
  obsA->setXYZ(xyzA);
  obsA->setIsSolved(true);

  OiVec xyzB(4);
  xyzB.setAt(0, 2.0);
  xyzB.setAt(1, 0.0);
  xyzB.setAt(2, 0.0);
  xyzB.setAt(3, 1.0);
  auto own_obsB = std::make_unique<Observation>();
  const oi::Guarded<Observation> obsB = own_obsB.get();
  obsB->setXYZ(xyzB);
  obsB->setIsSolved(true);

  point->addObservation(obsA);
  point->addObservation(obsB);

  REQUIRE(point->getFunctions().empty());
  point->recalc();

  OI_REQUIRE2(
      point->getIsSolved(),
      "a point with solved observations and no producer function should "
      "solve itself");
  OI_REQUIRE2(std::abs(point->getPosition().getVector().getAt(0) - 1.0) < 1e-9,
              "expected the midpoint of the two observations");
  OI_REQUIRE2(std::abs(point->getPosition().getVector().getAt(1) - 0.0) < 1e-9,
              "expected the midpoint of the two observations");
}

void MeasurementIntentTest::pointWithObservationsIsStationOwned() {
  auto own_station = std::make_unique<Station>("S1");
  const oi::Guarded<Station> station = own_station.get();
  auto own_point = std::make_unique<Point>();
  const oi::Guarded<Point> point = own_point.get();

  auto own_obs = std::make_unique<Observation>();
  const oi::Guarded<Observation> obs = own_obs.get();
  obs->setStation(station);
  point->addObservation(obs);

  const std::optional<std::string> owner = point->getOwnerDescription();
  OI_REQUIRE2(owner.has_value(),
              "a point carrying observations must be read-only");
  OI_REQUIRE2(owner->contains("S1"),
              "the owner description should name the station");
}

void MeasurementIntentTest::pointWithNoObservationsIsUnaffected() {
  // D19: a fresh, ordinary Point (no observations, no producer function)
  // is still user-owned and editable - this stage must not have widened
  // ownership beyond what 7g already established
  Point point;
  REQUIRE(!point.getOwnerDescription().has_value());
}

void MeasurementIntentTest::measuringAlwaysCreatesExactlyOneNewPoint() {
  oi::Guarded<Station> station;
  auto job = makeJobWithActiveStation(station);

  auto own_targetPoint = std::make_unique<oi::Point>();
  oi::Guarded<Point> targetPoint = own_targetPoint.get();
  oi::Guarded<oi::FeatureWrapper> targetFeature =
      targetPoint->getFeatureWrapper();
  targetFeature->getFeature()->setFeatureName("target");
  job->addFeature(std::move(own_targetPoint));

  const int featureCountBefore = job->getFeaturesList().size();

  // D15a: a MeasurementConfig count of 10 yields one Point with ten
  // readings, not ten Points
  std::vector<std::unique_ptr<Reading>> readings;
  readings.push_back(makeCartesianReading(1.0, 2.0, 3.0));
  readings.push_back(makeCartesianReading(1.01, 2.0, 3.0));
  readings.push_back(makeCartesianReading(0.99, 2.0, 3.0));

  job->addMeasurementResults(targetFeature->getFeature()->getId(),
                             std::move(readings));

  OI_COMPARE(job->getFeaturesList().size(), featureCountBefore + 1);
}

void MeasurementIntentTest::measuredPointHasObservationsDerivedPointDoesNot() {
  oi::Guarded<Station> station;
  auto job = makeJobWithActiveStation(station);

  auto own_targetPoint = std::make_unique<oi::Point>();
  oi::Guarded<Point> targetPoint = own_targetPoint.get();
  oi::Guarded<oi::FeatureWrapper> targetFeature =
      targetPoint->getFeatureWrapper();
  targetFeature->getFeature()->setFeatureName("target");
  job->addFeature(std::move(own_targetPoint));

  std::vector<std::unique_ptr<Reading>> readings;
  readings.push_back(makeCartesianReading(1.0, 2.0, 3.0));
  readings.push_back(makeCartesianReading(1.0, 2.0, 3.0));
  job->addMeasurementResults(targetFeature->getFeature()->getId(),
                             std::move(readings));

  // find the newly created point - the only ePointFeature besides the
  // target itself
  oi::Guarded<Point> measured;
  for (const oi::Guarded<FeatureWrapper>& fw : job->getFeaturesList()) {
    if (!fw.isNull() && !fw->getPoint().isNull() &&
        fw->getPoint() != targetPoint) {
      measured = fw->getPoint();
    }
  }
  OI_REQUIRE2(!measured.isNull(),
              "addMeasurementResults must have created a new point");

  //"a measured point has observations and a derived one has none" -
  // the stage's own acceptance wording
  OI_COMPARE(measured->getObservations().size(), 2);

  // a plain, freshly-constructed point (standing in for AverageFromPoints'
  // output, which is built from ePointElement inputs, never from
  // Geometry::addObservation) has none
  Point derived;
  REQUIRE(derived.getObservations().empty());
}

void MeasurementIntentTest::
    tagResolutionFeedsAMatchingFunctionWithNoBindingStep() {
  oi::Guarded<Station> station;
  auto job = makeJobWithActiveStation(station);

  auto own_targetPoint = std::make_unique<oi::Point>();
  oi::Guarded<Point> targetPoint = own_targetPoint.get();
  oi::Guarded<oi::FeatureWrapper> targetFeature =
      targetPoint->getFeatureWrapper();
  targetFeature->getFeature()->setFeatureName("target");
  job->addFeature(std::move(own_targetPoint));

  auto own_consumer = std::make_unique<FakeConsumer>("default");
  const oi::Guarded<FakeConsumer> consumer = own_consumer.get();
  targetPoint->addFunction(std::move(own_consumer));

  OI_REQUIRE2(oi::value(consumer->getInputElements(), 0).empty(),
              "nothing tagged yet");

  std::vector<std::unique_ptr<Reading>> readings;
  readings.push_back(makeCartesianReading(5.0, 5.0, 5.0));
  job->addMeasurementResults(targetFeature->getFeature()->getId(),
                             std::move(readings));

  // no binding step: the function already declared role "default" for
  // ePointFeature, and measuring for that same feature fed it directly
  OI_COMPARE(oi::value(consumer->getInputElements(), 0).size(), 1);
}

void MeasurementIntentTest::aPointCarryingTwoTagsIsFoundByBothConsumers() {
  oi::Guarded<Station> station;
  auto job = makeJobWithActiveStation(station);

  auto own_featureA = std::make_unique<oi::Point>();
  oi::Guarded<Point> featureA = own_featureA.get();
  oi::Guarded<oi::FeatureWrapper> wrapperA = featureA->getFeatureWrapper();
  wrapperA->getFeature()->setFeatureName("A");
  job->addFeature(std::move(own_featureA));

  auto own_featureB = std::make_unique<oi::Point>();
  oi::Guarded<Point> featureB = own_featureB.get();
  oi::Guarded<oi::FeatureWrapper> wrapperB = featureB->getFeatureWrapper();
  wrapperB->getFeature()->setFeatureName("B");
  job->addFeature(std::move(own_featureB));

  auto own_consumerA = std::make_unique<FakeConsumer>("default");
  const oi::Guarded<FakeConsumer> consumerA = own_consumerA.get();
  featureA->addFunction(std::move(own_consumerA));
  auto own_consumerB = std::make_unique<FakeConsumer>("default");
  const oi::Guarded<FakeConsumer> consumerB = own_consumerB.get();
  featureB->addFunction(std::move(own_consumerB));

  std::vector<std::unique_ptr<Reading>> readings;
  readings.push_back(makeCartesianReading(1.0, 1.0, 1.0));
  job->addMeasurementResults(wrapperA->getFeature()->getId(),
                             std::move(readings));

  // find the point that was just created and tag it for B as well - the
  // retroactive hand-allocation path (domain-model.md S3: "hand-allocation
  // and field acquisition are the same operation")
  oi::Guarded<Point> measured;
  for (const oi::Guarded<FeatureWrapper>& fw : job->getFeaturesList()) {
    if (!fw.isNull() && !fw->getPoint().isNull() &&
        fw->getPoint() != featureA && fw->getPoint() != featureB) {
      measured = fw->getPoint();
    }
  }
  REQUIRE(!measured.isNull());

  job->tagPoint(measured, wrapperB->getFeature()->getId(), "default");

  OI_COMPARE(oi::value(consumerA->getInputElements(), 0).size(), 1);
  OI_COMPARE(oi::value(consumerB->getInputElements(), 0).size(), 1);
  OI_COMPARE(measured->getIntents().size(), 2);
}

void MeasurementIntentTest::renamingTheFeatureDoesNotOrphanTags() {
  oi::Guarded<Station> station;
  auto job = makeJobWithActiveStation(station);

  auto own_targetPoint = std::make_unique<oi::Point>();
  oi::Guarded<Point> targetPoint = own_targetPoint.get();
  oi::Guarded<oi::FeatureWrapper> targetFeature =
      targetPoint->getFeatureWrapper();
  targetFeature->getFeature()->setFeatureName("target");
  job->addFeature(std::move(own_targetPoint));

  auto own_consumer = std::make_unique<FakeConsumer>("default");
  const oi::Guarded<FakeConsumer> consumer = own_consumer.get();
  targetPoint->addFunction(std::move(own_consumer));

  // rename *before* measuring - tags anchor to id (MeasurementIntent,
  // measurementintent.h), never to the name in effect at tag time
  targetFeature->getFeature()->setFeatureName("renamed-wall-01");

  std::vector<std::unique_ptr<Reading>> readings;
  readings.push_back(makeCartesianReading(3.0, 3.0, 3.0));
  job->addMeasurementResults(targetFeature->getFeature()->getId(),
                             std::move(readings));

  OI_COMPARE(oi::value(consumer->getInputElements(), 0).size(), 1);
}

void MeasurementIntentTest::unresolvableTargetIsRejectedWithAMessage() {
  oi::Guarded<Station> station;
  auto job = makeJobWithActiveStation(station);

  RecordingJobObserver spy;
  spy.attach(job.get());

  const int featureCountBefore = job->getFeaturesList().size();

  std::vector<std::unique_ptr<Reading>> readings;
  readings.push_back(makeCartesianReading(1.0, 1.0, 1.0));
  job->addMeasurementResults(999999, std::move(readings));

  OI_REQUIRE2(spy.count("sendMessage") > 0,
              "core must report why the measurement was rejected");
  OI_COMPARE(job->getFeaturesList().size(), featureCountBefore);
}

TEST_CASE(
    "MeasurementIntentTest::pointWithObservationsAndNoFunctionAveragesThem",
    "[MeasurementIntentTest]") {
  MeasurementIntentTest fixture;
  fixture.initTestCase();
  fixture.pointWithObservationsAndNoFunctionAveragesThem();
}

TEST_CASE("MeasurementIntentTest::pointWithObservationsIsStationOwned",
          "[MeasurementIntentTest]") {
  MeasurementIntentTest fixture;
  fixture.initTestCase();
  fixture.pointWithObservationsIsStationOwned();
}

TEST_CASE("MeasurementIntentTest::pointWithNoObservationsIsUnaffected",
          "[MeasurementIntentTest]") {
  MeasurementIntentTest fixture;
  fixture.initTestCase();
  fixture.pointWithNoObservationsIsUnaffected();
}

TEST_CASE("MeasurementIntentTest::measuringAlwaysCreatesExactlyOneNewPoint",
          "[MeasurementIntentTest]") {
  MeasurementIntentTest fixture;
  fixture.initTestCase();
  fixture.measuringAlwaysCreatesExactlyOneNewPoint();
}

TEST_CASE(
    "MeasurementIntentTest::measuredPointHasObservationsDerivedPointDoesNot",
    "[MeasurementIntentTest]") {
  MeasurementIntentTest fixture;
  fixture.initTestCase();
  fixture.measuredPointHasObservationsDerivedPointDoesNot();
}

TEST_CASE(
    "MeasurementIntentTest::"
    "tagResolutionFeedsAMatchingFunctionWithNoBindingStep",
    "[MeasurementIntentTest]") {
  MeasurementIntentTest fixture;
  fixture.initTestCase();
  fixture.tagResolutionFeedsAMatchingFunctionWithNoBindingStep();
}

TEST_CASE("MeasurementIntentTest::aPointCarryingTwoTagsIsFoundByBothConsumers",
          "[MeasurementIntentTest]") {
  MeasurementIntentTest fixture;
  fixture.initTestCase();
  fixture.aPointCarryingTwoTagsIsFoundByBothConsumers();
}

TEST_CASE("MeasurementIntentTest::renamingTheFeatureDoesNotOrphanTags",
          "[MeasurementIntentTest]") {
  MeasurementIntentTest fixture;
  fixture.initTestCase();
  fixture.renamingTheFeatureDoesNotOrphanTags();
}

TEST_CASE("MeasurementIntentTest::unresolvableTargetIsRejectedWithAMessage",
          "[MeasurementIntentTest]") {
  MeasurementIntentTest fixture;
  fixture.initTestCase();
  fixture.unresolvableTargetIsRejectedWithAMessage();
}
