#include <QString>
#include <QtTest>
#include <cmath>

#include "chooselalib.h"
#include "featurewrapper.h"
#include "fitfunction.h"
#include "geometry/point.h"
#include "oijob.h"

using namespace oi;
using namespace oi::math;

namespace {

/*!
 * \brief A minimal, real FitFunction - averages the x coordinate of its
 * ePointElement input, reports a per-element residual the same way the
 * real fit functions do (Function::addDisplayResidual) and a stdev
 * (Statistic::setStdev), and reads/reports shouldBeUsed/isUsed exactly
 * like p_bestfitplane.cpp does. Stands in for BestFitPlane/BestFitPoint
 * so Stage 7b's generic pipeline mechanism (Function::exec()) can be
 * exercised without a cross-repo test dependency or real geometry math -
 * the mechanism under test lives entirely in Function::exec(), not in
 * any fit function's own body, so a fake average is a faithful proxy.
 */
class FakeAverageFit : public FitFunction, public Producer<Point> {
  Q_OBJECT
 public:
  explicit FakeAverageFit(QObject* parent = nullptr) : FitFunction(parent) {
    this->metaData.name = "FakeAverageFit";
    this->neededElements.clear();
    NeededElement param;
    param.typeOfElement = ElementTypes::ePointElement;
    param.infinite = true;
    param.roleName = "default";
    this->neededElements.append(param);
    this->applicableFor.append(FeatureTypes::ePointFeature);
  }

  std::expected<Point, QString> exec() override {
    this->statistic.reset();

    if (!this->inputElements.contains(0) || this->inputElements[0].isEmpty()) {
      return std::unexpected(QString("FakeAverageFit: no input elements"));
    }

    QList<InputElement> usable;
    foreach (const InputElement& element, this->inputElements[0]) {
      if (element.point.isNull()) {
        this->setIsUsed(0, element.id, false);
        continue;
      }
      this->setIsUsed(0, element.id, element.shouldBeUsed);
      if (element.shouldBeUsed) {
        usable.append(element);
      }
    }

    if (usable.isEmpty()) {
      return std::unexpected(
          QString("FakeAverageFit: no usable input elements"));
    }

    double sum = 0.0;
    foreach (const InputElement& element, usable) {
      sum += element.point->getPosition().getVector().getAt(0);
    }
    const double avg = sum / static_cast<double>(usable.size());

    double sqSum = 0.0;
    foreach (const InputElement& element, usable) {
      const double v = element.point->getPosition().getVector().getAt(0) - avg;
      // exercise the same "v" key real fits use (BestFitPlane/Line/Point)
      this->addDisplayResidual(element.id, v, 0.0, 0.0, std::abs(v));
      sqSum += v * v;
    }
    this->statistic.setStdev(
        usable.size() > 1
            ? std::sqrt(sqSum / (static_cast<double>(usable.size()) - 1.0))
            : 0.0);
    this->statistic.setIsValid(true);

    Point point;
    point.setPoint(Position(avg, 0.0, 0.0));

    return point;
  }
};

}  // namespace

/*!
 * \brief Stage 7b - filter pipelines on needed elements (D16,
 * domain-model.md S5). Exercises the generic mechanism in
 * Function::exec() (applyDeterministicPipelineStages/
 * applyResidualPipelineStages) in isolation from any real fit's math,
 * using FakeAverageFit above. The literal acceptance criterion (a real
 * plane fit converging correctly with the filter declared, degrading
 * without it) is covered separately in openindy-default-plugin against
 * the real BestFitPlane/BestFitPoint, which is what actually proves the
 * "no fit function needed changes" design claim; this suite is the fast,
 * cross-cutting regression guard for the mechanism itself.
 */
class PipelineTest : public QObject {
  Q_OBJECT

 private Q_SLOTS:
  void initTestCase();

  void undeclaredPipelineLeavesShouldBeUsedUntouched();
  void explicitListStageKeepsOnlyListedIdsAndRecordsReasons();
  void excludeStageDropsSpecificIdWithReason();
  void residualOutlierRejectionExcludesTheOutlierAndConverges();
  void omittingTheFilterLeavesTheResultSkewedByTheOutlier();
  void addingANewOutlierLaterReTriggersTheFilterOnRecalc();

 private:
  QPointer<FeatureWrapper> makeTargetPoint(const QString& name);
  QPointer<Point> makeCandidate(double x, QPointer<OiJob> job);
};

void PipelineTest::initTestCase() {
  ChooseLALib::setLinearAlgebra(ChooseLALib::LinearAlgebraLibrary::Armadillo);
}

QPointer<FeatureWrapper> PipelineTest::makeTargetPoint(const QString& name) {
  QPointer<Point> point = new Point();
  point->setFeatureName(name);
  QPointer<FeatureWrapper> wrapper = new FeatureWrapper();
  wrapper->setPoint(point);
  return wrapper;
}

QPointer<Point> PipelineTest::makeCandidate(double x, QPointer<OiJob> job) {
  QPointer<Point> point = new Point();
  QPointer<FeatureWrapper> wrapper = new FeatureWrapper();
  wrapper->setPoint(point);
  job->addFeature(wrapper);
  point->setPoint(Position(x, 0.0, 0.0));
  return point;
}

/*!
 * Opt-in requirement (function.h:NeededElement::pipeline): a NeededElement
 * with no declared pipeline must never touch InputElement::shouldBeUsed -
 * that would silently override a manual OiJob::setShouldBeUsed call (the
 * pre-existing, if UI-less, mechanism) with pipeline-derived state nobody
 * asked for.
 */
void PipelineTest::undeclaredPipelineLeavesShouldBeUsedUntouched() {
  QPointer<OiJob> job = new OiJob();
  QPointer<FeatureWrapper> target = this->makeTargetPoint("Target1");
  QVERIFY(job->addFeature(target));

  QPointer<FakeAverageFit> function = new FakeAverageFit();
  target->getPoint()->addFunction(function);

  QPointer<Point> p1 = this->makeCandidate(0.0, job);
  QPointer<Point> p2 = this->makeCandidate(10.0, job);
  InputElement e1(p1->getId());
  e1.typeOfElement = ElementTypes::ePointElement;
  e1.point = p1;
  function->addInputElement(e1, 0);
  InputElement e2(p2->getId());
  e2.typeOfElement = ElementTypes::ePointElement;
  e2.point = p2;
  function->addInputElement(e2, 0);

  // manually mark p1 unused, the pre-existing (UI-less) mechanism
  function->setShouldBeUsed(0, p1->getId(), false);

  QVERIFY(function->getPipeline(0).isEmpty());

  target->getPoint()->recalc();

  QVERIFY2(!function->getShouldBeUsed(0, p1->getId()),
           "an undeclared pipeline must not touch a manually-set shouldBeUsed");
  QVERIFY(function->getShouldBeUsed(0, p2->getId()));
  QVERIFY(function->getPipelineExclusions(0).isEmpty());
}

void PipelineTest::explicitListStageKeepsOnlyListedIdsAndRecordsReasons() {
  QPointer<OiJob> job = new OiJob();
  QPointer<FeatureWrapper> target = this->makeTargetPoint("Target2");
  QVERIFY(job->addFeature(target));

  QPointer<FakeAverageFit> function = new FakeAverageFit();
  target->getPoint()->addFunction(function);

  QPointer<Point> p1 = this->makeCandidate(0.0, job);
  QPointer<Point> p2 = this->makeCandidate(10.0, job);
  QPointer<Point> p3 = this->makeCandidate(100.0, job);
  foreach (const QPointer<Point>& p,
           (QList<QPointer<Point> >() << p1 << p2 << p3)) {
    InputElement e(p->getId());
    e.typeOfElement = ElementTypes::ePointElement;
    e.point = p;
    function->addInputElement(e, 0);
  }

  function->setPipeline(0,
                        QList<PipelineStage>() << PipelineStage::explicitList(
                            QList<int>() << p1->getId() << p2->getId()));

  target->getPoint()->recalc();

  QCOMPARE(target->getPoint()->getPosition().getVector().getAt(0),
           5.0);  // avg(0,10), p3 excluded
  QVERIFY(function->getShouldBeUsed(0, p1->getId()));
  QVERIFY(function->getShouldBeUsed(0, p2->getId()));
  QVERIFY(!function->getShouldBeUsed(0, p3->getId()));

  const QList<PipelineExclusion> exclusions =
      function->getPipelineExclusions(0);
  QCOMPARE(exclusions.size(), 1);
  QCOMPARE(exclusions.first().elementId, p3->getId());
  QVERIFY(!exclusions.first().reason.isEmpty());
}

void PipelineTest::excludeStageDropsSpecificIdWithReason() {
  QPointer<OiJob> job = new OiJob();
  QPointer<FeatureWrapper> target = this->makeTargetPoint("Target3");
  QVERIFY(job->addFeature(target));

  QPointer<FakeAverageFit> function = new FakeAverageFit();
  target->getPoint()->addFunction(function);

  QPointer<Point> p1 = this->makeCandidate(0.0, job);
  QPointer<Point> p2 = this->makeCandidate(10.0, job);
  foreach (const QPointer<Point>& p, (QList<QPointer<Point> >() << p1 << p2)) {
    InputElement e(p->getId());
    e.typeOfElement = ElementTypes::ePointElement;
    e.point = p;
    function->addInputElement(e, 0);
  }

  // D16: hand-picking is an Exclude stage carrying a reason, the same
  // mechanism as an automated rejection - not an untag, not a boolean.
  function->addPipelineStage(
      0, PipelineStage::exclude(p2->getId(),
                                "known bad target - operator flagged on site"));

  target->getPoint()->recalc();

  QCOMPARE(target->getPoint()->getPosition().getVector().getAt(0),
           0.0);  // only p1 survives
  QVERIFY(!function->getShouldBeUsed(0, p2->getId()));

  const QList<PipelineExclusion> exclusions =
      function->getPipelineExclusions(0);
  QCOMPARE(exclusions.size(), 1);
  QCOMPARE(exclusions.first().elementId, p2->getId());
  QCOMPARE(exclusions.first().reason,
           QString("known bad target - operator flagged on site"));
}

/*!
 * The driving acceptance case, at the generic-mechanism level: a set
 * containing one gross outlier converges to the correct value once the
 * residual/sigma filter is declared, and the outlier is individually
 * reportable with a reason (traceability requirement).
 */
void PipelineTest::residualOutlierRejectionExcludesTheOutlierAndConverges() {
  QPointer<OiJob> job = new OiJob();
  QPointer<FeatureWrapper> target = this->makeTargetPoint("Target4");
  QVERIFY(job->addFeature(target));

  QPointer<FakeAverageFit> function = new FakeAverageFit();
  target->getPoint()->addFunction(function);

  // 20 good points plus one deliberate gross outlier. A small sample
  //(e.g. 5-6 points) suffers the classic sigma-clipping masking effect:
  // a single huge outlier inflates the *contaminated* stdev enough to
  // hide itself under a 3-sigma threshold. With N=21 the arithmetic
  // flips (a single outlier's contribution to the pass-1 stdev scales
  // as ~outlierResidual/sqrt(N-1), so 3-sigma catches it once
  // sqrt(N-1) > 3, i.e. N > 10) - a large-enough sample is a real
  // precondition for naive population-stdev outlier rejection to work,
  // not a test artefact, and worth recording for whoever tunes this
  // stage's default sigmaMultiplier later.
  QList<double> xs;
  for (int i = 0; i < 20; i++) {
    xs.append(0.0);
  }
  xs.append(1000.0);  // the deliberate outlier
  QList<QPointer<Point> > candidates;
  foreach (double x, xs) {
    candidates.append(this->makeCandidate(x, job));
  }
  foreach (const QPointer<Point>& p, candidates) {
    InputElement e(p->getId());
    e.typeOfElement = ElementTypes::ePointElement;
    e.point = p;
    function->addInputElement(e, 0);
  }

  function->setPipeline(0, QList<PipelineStage>()
                               << PipelineStage::residualOutlierRejection(3.0));

  target->getPoint()->recalc();

  const double converged =
      target->getPoint()->getPosition().getVector().getAt(0);
  QVERIFY2(std::abs(converged) < 1.0,
           qPrintable(
               QString("expected convergence near 0, got %1").arg(converged)));

  const int outlierId = candidates.last()->getId();
  QVERIFY2(!function->getShouldBeUsed(0, outlierId),
           "the outlier must be excluded");

  const QList<PipelineExclusion> exclusions =
      function->getPipelineExclusions(0);
  bool found = false;
  foreach (const PipelineExclusion& excl, exclusions) {
    if (excl.elementId == outlierId) {
      found = true;
      QVERIFY2(excl.reason.contains("residual"), qPrintable(excl.reason));
    }
  }
  QVERIFY2(
      found,
      "the outlier's exclusion must be individually reportable with a reason");
}

/*!
 * The negative half of the acceptance criterion: the same data, without
 * the filter declared, must NOT converge correctly - the mechanism has
 * to actually change the result, not just be inert scaffolding.
 */
void PipelineTest::omittingTheFilterLeavesTheResultSkewedByTheOutlier() {
  QPointer<OiJob> job = new OiJob();
  QPointer<FeatureWrapper> target = this->makeTargetPoint("Target5");
  QVERIFY(job->addFeature(target));

  QPointer<FakeAverageFit> function = new FakeAverageFit();
  target->getPoint()->addFunction(function);

  QList<double> xs;
  for (int i = 0; i < 20; i++) {
    xs.append(0.0);
  }
  xs.append(1000.0);  // the deliberate outlier
  foreach (double x, xs) {
    QPointer<Point> p = this->makeCandidate(x, job);
    InputElement e(p->getId());
    e.typeOfElement = ElementTypes::ePointElement;
    e.point = p;
    function->addInputElement(e, 0);
  }

  // no pipeline declared

  target->getPoint()->recalc();

  const double skewed = target->getPoint()->getPosition().getVector().getAt(0);
  QVERIFY2(
      skewed > 10.0,
      qPrintable(QString("expected the outlier to skew the average, got %1")
                     .arg(skewed)));
}

/*!
 * "Adding a new outlier re-triggers the filter and the fit" (acceptance
 * criteria): the pipeline is declared once and re-evaluates on every
 * ordinary recalc, not just the recalc immediately after it was declared.
 */
void PipelineTest::addingANewOutlierLaterReTriggersTheFilterOnRecalc() {
  QPointer<OiJob> job = new OiJob();
  QPointer<FeatureWrapper> target = this->makeTargetPoint("Target6");
  QVERIFY(job->addFeature(target));

  QPointer<FakeAverageFit> function = new FakeAverageFit();
  target->getPoint()->addFunction(function);

  // 20 good points, same reasoning as
  // residualOutlierRejectionExcludesTheOutlierAndConverges() re: sample
  // size and the sigma-clipping masking effect
  QList<QPointer<Point> > candidates;
  for (int i = 0; i < 20; i++) {
    QPointer<Point> p = this->makeCandidate(0.0, job);
    candidates.append(p);
    InputElement e(p->getId());
    e.typeOfElement = ElementTypes::ePointElement;
    e.point = p;
    function->addInputElement(e, 0);
  }

  function->setPipeline(0, QList<PipelineStage>()
                               << PipelineStage::residualOutlierRejection(3.0));

  target->getPoint()->recalc();
  QVERIFY(function->getPipelineExclusions(0).isEmpty());
  QCOMPARE(target->getPoint()->getPosition().getVector().getAt(0), 0.0);

  // a new outlier arrives later - no pipeline redeclaration, just new data
  QPointer<Point> outlier = this->makeCandidate(1000.0, job);
  InputElement e(outlier->getId());
  e.typeOfElement = ElementTypes::ePointElement;
  e.point = outlier;
  function->addInputElement(e, 0);

  target->getPoint()->recalc();

  QVERIFY2(!function->getShouldBeUsed(0, outlier->getId()),
           "a newly-added outlier must be caught on the next recalc without "
           "redeclaring anything");
  QVERIFY2(
      std::abs(target->getPoint()->getPosition().getVector().getAt(0)) < 1.0,
      "the fit must still converge correctly after the new outlier is "
      "filtered");
}

QTEST_APPLESS_MAIN(PipelineTest)
#include "tst_pipeline.moc"
