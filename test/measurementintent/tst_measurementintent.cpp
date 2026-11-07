#include <QSignalSpy>
#include <QString>
#include <QtTest>
#include <cmath>

#include "chooselalib.h"
#include "constructfunction.h"
#include "featurewrapper.h"
#include "geometry/point.h"
#include "oijob.h"
#include "reading.h"
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
  Q_OBJECT
 public:
  explicit FakeConsumer(const QString& roleName = "default",
                        QObject* parent = nullptr)
      : ConstructFunction(parent) {
    this->metaData.name = "FakeConsumer";
    NeededElement param;
    param.typeOfElement = ElementTypes::ePointElement;
    param.infinite = true;
    param.roleName = roleName;
    this->neededElements.append(param);
    this->applicableFor.append(FeatureTypes::ePointFeature);
  }
  std::expected<Point, QString> exec() override { return Point(); }
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
class MeasurementIntentTest : public QObject {
  Q_OBJECT

 private Q_SLOTS:
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
  QPointer<OiJob> makeJobWithActiveStation(QPointer<Station>& outStation);
  QPointer<Reading> makeCartesianReading(double x, double y, double z);
};

void MeasurementIntentTest::initTestCase() {
  ChooseLALib::setLinearAlgebra(ChooseLALib::LinearAlgebraLibrary::Armadillo);
}

QPointer<OiJob> MeasurementIntentTest::makeJobWithActiveStation(
    QPointer<Station>& outStation) {
  QPointer<OiJob> job = new OiJob();

  QPointer<Station> station = new Station("S1");
  QPointer<FeatureWrapper> stationFeature = new FeatureWrapper();
  stationFeature->setStation(station);
  job->addFeature(stationFeature);

  // Station::activeStationChanged is connected (OiJob::attachFeature) to
  // OiJob::setActiveStation - this is the ordinary way a station becomes
  // the job's active one, not a test-only shortcut
  station->setActiveStationState(true);

  outStation = station;
  return job;
}

QPointer<Reading> MeasurementIntentTest::makeCartesianReading(double x,
                                                              double y,
                                                              double z) {
  ReadingCartesian r;
  r.isValid = true;
  r.xyz.setAt(0, x);
  r.xyz.setAt(1, y);
  r.xyz.setAt(2, z);
  return new Reading(r);
}

void MeasurementIntentTest::pointWithObservationsAndNoFunctionAveragesThem() {
  QPointer<Point> point = new Point();

  // stand in for "TrafoController already transformed these" (core has
  // no TrafoController - see point.cpp's Point::recalc() comment)
  OiVec xyzA(4);
  xyzA.setAt(0, 0.0);
  xyzA.setAt(1, 0.0);
  xyzA.setAt(2, 0.0);
  xyzA.setAt(3, 1.0);
  QPointer<Observation> obsA = new Observation();
  obsA->setXYZ(xyzA);
  obsA->setIsSolved(true);

  OiVec xyzB(4);
  xyzB.setAt(0, 2.0);
  xyzB.setAt(1, 0.0);
  xyzB.setAt(2, 0.0);
  xyzB.setAt(3, 1.0);
  QPointer<Observation> obsB = new Observation();
  obsB->setXYZ(xyzB);
  obsB->setIsSolved(true);

  point->addObservation(obsA);
  point->addObservation(obsB);

  QVERIFY(point->getFunctions().isEmpty());
  point->recalc();

  QVERIFY2(point->getIsSolved(),
           "a point with solved observations and no producer function should "
           "solve itself");
  QVERIFY2(std::abs(point->getPosition().getVector().getAt(0) - 1.0) < 1e-9,
           "expected the midpoint of the two observations");
  QVERIFY2(std::abs(point->getPosition().getVector().getAt(1) - 0.0) < 1e-9,
           "expected the midpoint of the two observations");
}

void MeasurementIntentTest::pointWithObservationsIsStationOwned() {
  QPointer<Station> station = new Station("S1");
  QPointer<Point> point = new Point();

  QPointer<Observation> obs = new Observation();
  obs->setStation(station);
  point->addObservation(obs);

  const std::optional<QString> owner = point->getOwnerDescription();
  QVERIFY2(owner.has_value(),
           "a point carrying observations must be read-only");
  QVERIFY2(owner->contains("S1"),
           "the owner description should name the station");
}

void MeasurementIntentTest::pointWithNoObservationsIsUnaffected() {
  // D19: a fresh, ordinary Point (no observations, no producer function)
  // is still user-owned and editable - this stage must not have widened
  // ownership beyond what 7g already established
  Point point;
  QVERIFY(!point.getOwnerDescription().has_value());
}

void MeasurementIntentTest::measuringAlwaysCreatesExactlyOneNewPoint() {
  QPointer<Station> station;
  QPointer<OiJob> job = makeJobWithActiveStation(station);

  QPointer<Point> targetPoint = new Point();
  QPointer<FeatureWrapper> targetFeature = new FeatureWrapper();
  targetFeature->setPoint(targetPoint);
  targetFeature->getFeature()->setFeatureName("target");
  job->addFeature(targetFeature);

  const int featureCountBefore = job->getFeaturesList().size();

  // D15a: a MeasurementConfig count of 10 yields one Point with ten
  // readings, not ten Points
  QList<QPointer<Reading> > readings;
  readings << makeCartesianReading(1.0, 2.0, 3.0)
           << makeCartesianReading(1.01, 2.0, 3.0)
           << makeCartesianReading(0.99, 2.0, 3.0);

  job->addMeasurementResults(targetFeature->getFeature()->getId(), readings);

  QCOMPARE(job->getFeaturesList().size(), featureCountBefore + 1);
}

void MeasurementIntentTest::measuredPointHasObservationsDerivedPointDoesNot() {
  QPointer<Station> station;
  QPointer<OiJob> job = makeJobWithActiveStation(station);

  QPointer<Point> targetPoint = new Point();
  QPointer<FeatureWrapper> targetFeature = new FeatureWrapper();
  targetFeature->setPoint(targetPoint);
  targetFeature->getFeature()->setFeatureName("target");
  job->addFeature(targetFeature);

  QList<QPointer<Reading> > readings;
  readings << makeCartesianReading(1.0, 2.0, 3.0)
           << makeCartesianReading(1.0, 2.0, 3.0);
  job->addMeasurementResults(targetFeature->getFeature()->getId(), readings);

  // find the newly created point - the only ePointFeature besides the
  // target itself
  QPointer<Point> measured;
  foreach (const QPointer<FeatureWrapper>& fw, job->getFeaturesList()) {
    if (!fw.isNull() && !fw->getPoint().isNull() &&
        fw->getPoint() != targetPoint) {
      measured = fw->getPoint();
    }
  }
  QVERIFY2(!measured.isNull(),
           "addMeasurementResults must have created a new point");

  //"a measured point has observations and a derived one has none" -
  // the stage's own acceptance wording
  QCOMPARE(measured->getObservations().size(), 2);

  // a plain, freshly-constructed point (standing in for AverageFromPoints'
  // output, which is built from ePointElement inputs, never from
  // Geometry::addObservation) has none
  Point derived;
  QVERIFY(derived.getObservations().isEmpty());
}

void MeasurementIntentTest::
    tagResolutionFeedsAMatchingFunctionWithNoBindingStep() {
  QPointer<Station> station;
  QPointer<OiJob> job = makeJobWithActiveStation(station);

  QPointer<Point> targetPoint = new Point();
  QPointer<FeatureWrapper> targetFeature = new FeatureWrapper();
  targetFeature->setPoint(targetPoint);
  targetFeature->getFeature()->setFeatureName("target");
  job->addFeature(targetFeature);

  QPointer<FakeConsumer> consumer = new FakeConsumer("default");
  targetPoint->addFunction(consumer);

  QVERIFY2(consumer->getInputElements().value(0).isEmpty(),
           "nothing tagged yet");

  QList<QPointer<Reading> > readings;
  readings << makeCartesianReading(5.0, 5.0, 5.0);
  job->addMeasurementResults(targetFeature->getFeature()->getId(), readings);

  // no binding step: the function already declared role "default" for
  // ePointFeature, and measuring for that same feature fed it directly
  QCOMPARE(consumer->getInputElements().value(0).size(), 1);
}

void MeasurementIntentTest::aPointCarryingTwoTagsIsFoundByBothConsumers() {
  QPointer<Station> station;
  QPointer<OiJob> job = makeJobWithActiveStation(station);

  QPointer<Point> featureA = new Point();
  QPointer<FeatureWrapper> wrapperA = new FeatureWrapper();
  wrapperA->setPoint(featureA);
  wrapperA->getFeature()->setFeatureName("A");
  job->addFeature(wrapperA);

  QPointer<Point> featureB = new Point();
  QPointer<FeatureWrapper> wrapperB = new FeatureWrapper();
  wrapperB->setPoint(featureB);
  wrapperB->getFeature()->setFeatureName("B");
  job->addFeature(wrapperB);

  QPointer<FakeConsumer> consumerA = new FakeConsumer("default");
  featureA->addFunction(consumerA);
  QPointer<FakeConsumer> consumerB = new FakeConsumer("default");
  featureB->addFunction(consumerB);

  QList<QPointer<Reading> > readings;
  readings << makeCartesianReading(1.0, 1.0, 1.0);
  job->addMeasurementResults(wrapperA->getFeature()->getId(), readings);

  // find the point that was just created and tag it for B as well - the
  // retroactive hand-allocation path (domain-model.md S3: "hand-allocation
  // and field acquisition are the same operation")
  QPointer<Point> measured;
  foreach (const QPointer<FeatureWrapper>& fw, job->getFeaturesList()) {
    if (!fw.isNull() && !fw->getPoint().isNull() &&
        fw->getPoint() != featureA && fw->getPoint() != featureB) {
      measured = fw->getPoint();
    }
  }
  QVERIFY(!measured.isNull());

  job->tagPoint(measured, wrapperB->getFeature()->getId(), "default");

  QCOMPARE(consumerA->getInputElements().value(0).size(), 1);
  QCOMPARE(consumerB->getInputElements().value(0).size(), 1);
  QCOMPARE(measured->getIntents().size(), 2);
}

void MeasurementIntentTest::renamingTheFeatureDoesNotOrphanTags() {
  QPointer<Station> station;
  QPointer<OiJob> job = makeJobWithActiveStation(station);

  QPointer<Point> targetPoint = new Point();
  QPointer<FeatureWrapper> targetFeature = new FeatureWrapper();
  targetFeature->setPoint(targetPoint);
  targetFeature->getFeature()->setFeatureName("target");
  job->addFeature(targetFeature);

  QPointer<FakeConsumer> consumer = new FakeConsumer("default");
  targetPoint->addFunction(consumer);

  // rename *before* measuring - tags anchor to id (MeasurementIntent,
  // measurementintent.h), never to the name in effect at tag time
  targetFeature->getFeature()->setFeatureName("renamed-wall-01");

  QList<QPointer<Reading> > readings;
  readings << makeCartesianReading(3.0, 3.0, 3.0);
  job->addMeasurementResults(targetFeature->getFeature()->getId(), readings);

  QCOMPARE(consumer->getInputElements().value(0).size(), 1);
}

void MeasurementIntentTest::unresolvableTargetIsRejectedWithAMessage() {
  QPointer<Station> station;
  QPointer<OiJob> job = makeJobWithActiveStation(station);

  QSignalSpy spy(job.data(), &OiJob::sendMessage);

  const int featureCountBefore = job->getFeaturesList().size();

  QList<QPointer<Reading> > readings;
  readings << makeCartesianReading(1.0, 1.0, 1.0);
  job->addMeasurementResults(999999, readings);

  QVERIFY2(spy.count() > 0,
           "core must report why the measurement was rejected");
  QCOMPARE(job->getFeaturesList().size(), featureCountBefore);
}

QTEST_APPLESS_MAIN(MeasurementIntentTest)
#include "tst_measurementintent.moc"
