#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <string>
#include <vector>

#include "catch_compat.h"
#include "choose_la_lib.h"
#include "feature_wrapper.h"
#include "fit_function.h"
#include "geometry/point.h"
#include "oi_job.h"

using namespace oi;
using namespace oi::math;

namespace {

/*!
 * \brief A minimal, real FitFunction - averages the x coordinate of its
 * ePointElement input, reports a per-element residual the same way the
 * real fit functions do (Function::addDisplayResidual) and a stdev
 * (Statistic::setStdev), and reads/reports shouldBeUsed/isUsed exactly
 * like p_best_fit_plane.cpp does. Stands in for BestFitPlane/BestFitPoint
 * so Stage 7b's generic pipeline mechanism (Function::exec()) can be
 * exercised without a cross-repo test dependency or real geometry math -
 * the mechanism under test lives entirely in Function::exec(), not in
 * any fit function's own body, so a fake average is a faithful proxy.
 */
class FakeAverageFit : public FitFunction, public Producer<Point> {
 public:
  explicit FakeAverageFit() : FitFunction() {
    this->metaData.name = "FakeAverageFit";
    this->neededElements.clear();
    NeededElement param;
    param.typeOfElement = ElementTypes::ePointElement;
    param.infinite = true;
    param.roleName = "default";
    this->neededElements.push_back(param);
    this->applicableFor.push_back(FeatureTypes::ePointFeature);
  }

  std::expected<Point, std::string> exec() override {
    this->statistic.reset();

    if (!this->inputElements.contains(0) || this->inputElements[0].empty()) {
      return std::unexpected(std::string("FakeAverageFit: no input elements"));
    }

    std::vector<InputElement> usable;
    for (const InputElement& element : this->inputElements[0]) {
      if (element.point.isNull()) {
        this->setIsUsed(0, element.id, false);
        continue;
      }
      this->setIsUsed(0, element.id, element.shouldBeUsed);
      if (element.shouldBeUsed) {
        usable.push_back(element);
      }
    }

    if (usable.empty()) {
      return std::unexpected(
          std::string("FakeAverageFit: no usable input elements"));
    }

    double sum = 0.0;
    for (const InputElement& element : usable) {
      sum += element.point->getPosition().getVector().getAt(0);
    }
    const double avg = sum / static_cast<double>(usable.size());

    double sqSum = 0.0;
    for (const InputElement& element : usable) {
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
class PipelineTest {
 public:
  void initTestCase();

  void undeclaredPipelineLeavesShouldBeUsedUntouched();
  void explicitListStageKeepsOnlyListedIdsAndRecordsReasons();
  void excludeStageDropsSpecificIdWithReason();
  void residualOutlierRejectionExcludesTheOutlierAndConverges();
  void omittingTheFilterLeavesTheResultSkewedByTheOutlier();
  void addingANewOutlierLaterReTriggersTheFilterOnRecalc();

 private:
  std::unique_ptr<Point> makeTargetPoint(const std::string& name);
  oi::Guarded<Point> makeCandidate(double x, const std::unique_ptr<OiJob>& job);
};

void PipelineTest::initTestCase() {
  ChooseLALib::setLinearAlgebra(ChooseLALib::LinearAlgebraLibrary::Armadillo);
}

std::unique_ptr<Point> PipelineTest::makeTargetPoint(const std::string& name) {
  auto point = std::make_unique<Point>();
  point->setFeatureName(name);
  return point;
}

oi::Guarded<Point> PipelineTest::makeCandidate(
    double x, const std::unique_ptr<OiJob>& job) {
  auto own_point = std::make_unique<oi::Point>();
  oi::Guarded<Point> point = own_point.get();
  // a job rejects an unnamed feature, and a duplicate name
  static int candidateCount = 0;
  point->setFeatureName(std::format("Candidate{}", ++candidateCount));
  REQUIRE(job->addFeature(std::move(own_point)));
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
  auto job = std::make_unique<OiJob>();
  oi::Guarded<FeatureWrapper> target =
      job->addFeature(this->makeTargetPoint("Target1"));
  REQUIRE(target);

  auto own_function = std::make_unique<FakeAverageFit>();
  const oi::Guarded<FakeAverageFit> function = own_function.get();
  target->getPoint()->addFunction(std::move(own_function));

  oi::Guarded<Point> p1 = this->makeCandidate(0.0, job);
  oi::Guarded<Point> p2 = this->makeCandidate(10.0, job);
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

  REQUIRE(function->getPipeline(0).empty());

  target->getPoint()->recalc();

  OI_REQUIRE2(
      !function->getShouldBeUsed(0, p1->getId()),
      "an undeclared pipeline must not touch a manually-set shouldBeUsed");
  REQUIRE(function->getShouldBeUsed(0, p2->getId()));
  REQUIRE(function->getPipelineExclusions(0).empty());
}

void PipelineTest::explicitListStageKeepsOnlyListedIdsAndRecordsReasons() {
  auto job = std::make_unique<OiJob>();
  oi::Guarded<FeatureWrapper> target =
      job->addFeature(this->makeTargetPoint("Target2"));
  REQUIRE(target);

  auto own_function = std::make_unique<FakeAverageFit>();
  const oi::Guarded<FakeAverageFit> function = own_function.get();
  target->getPoint()->addFunction(std::move(own_function));

  oi::Guarded<Point> p1 = this->makeCandidate(0.0, job);
  oi::Guarded<Point> p2 = this->makeCandidate(10.0, job);
  oi::Guarded<Point> p3 = this->makeCandidate(100.0, job);
  for (const oi::Guarded<Point>& p :
       std::vector<oi::Guarded<Point>>{p1, p2, p3}) {
    InputElement e(p->getId());
    e.typeOfElement = ElementTypes::ePointElement;
    e.point = p;
    function->addInputElement(e, 0);
  }

  function->setPipeline(0,
                        std::vector<PipelineStage>{PipelineStage::explicitList(
                            std::vector<int>{p1->getId(), p2->getId()})});

  target->getPoint()->recalc();

  OI_COMPARE(target->getPoint()->getPosition().getVector().getAt(0),
             5.0);  // avg(0,10), p3 excluded
  REQUIRE(function->getShouldBeUsed(0, p1->getId()));
  REQUIRE(function->getShouldBeUsed(0, p2->getId()));
  REQUIRE(!function->getShouldBeUsed(0, p3->getId()));

  const std::vector<PipelineExclusion> exclusions =
      function->getPipelineExclusions(0);
  OI_COMPARE(exclusions.size(), 1);
  OI_COMPARE(exclusions.front().elementId, p3->getId());
  REQUIRE(!exclusions.front().reason.empty());
}

void PipelineTest::excludeStageDropsSpecificIdWithReason() {
  auto job = std::make_unique<OiJob>();
  oi::Guarded<FeatureWrapper> target =
      job->addFeature(this->makeTargetPoint("Target3"));
  REQUIRE(target);

  auto own_function = std::make_unique<FakeAverageFit>();
  const oi::Guarded<FakeAverageFit> function = own_function.get();
  target->getPoint()->addFunction(std::move(own_function));

  oi::Guarded<Point> p1 = this->makeCandidate(0.0, job);
  oi::Guarded<Point> p2 = this->makeCandidate(10.0, job);
  for (const oi::Guarded<Point>& p : std::vector<oi::Guarded<Point>>{p1, p2}) {
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

  OI_COMPARE(target->getPoint()->getPosition().getVector().getAt(0),
             0.0);  // only p1 survives
  REQUIRE(!function->getShouldBeUsed(0, p2->getId()));

  const std::vector<PipelineExclusion> exclusions =
      function->getPipelineExclusions(0);
  OI_COMPARE(exclusions.size(), 1);
  OI_COMPARE(exclusions.front().elementId, p2->getId());
  OI_COMPARE(exclusions.front().reason,
             std::string("known bad target - operator flagged on site"));
}

/*!
 * The driving acceptance case, at the generic-mechanism level: a set
 * containing one gross outlier converges to the correct value once the
 * residual/sigma filter is declared, and the outlier is individually
 * reportable with a reason (traceability requirement).
 */
void PipelineTest::residualOutlierRejectionExcludesTheOutlierAndConverges() {
  auto job = std::make_unique<OiJob>();
  oi::Guarded<FeatureWrapper> target =
      job->addFeature(this->makeTargetPoint("Target4"));
  REQUIRE(target);

  auto own_function = std::make_unique<FakeAverageFit>();
  const oi::Guarded<FakeAverageFit> function = own_function.get();
  target->getPoint()->addFunction(std::move(own_function));

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
  std::vector<double> xs;
  for (int i = 0; i < 20; i++) {
    xs.push_back(0.0);
  }
  xs.push_back(1000.0);  // the deliberate outlier
  std::vector<oi::Guarded<Point>> candidates;
  for (double x : xs) {
    candidates.push_back(this->makeCandidate(x, job));
  }
  for (const oi::Guarded<Point>& p : candidates) {
    InputElement e(p->getId());
    e.typeOfElement = ElementTypes::ePointElement;
    e.point = p;
    function->addInputElement(e, 0);
  }

  function->setPipeline(0, std::vector<PipelineStage>{
                               PipelineStage::residualOutlierRejection(3.0)});

  target->getPoint()->recalc();

  const double converged =
      target->getPoint()->getPosition().getVector().getAt(0);
  OI_REQUIRE2(
      std::abs(converged) < 1.0,
      (std::format("expected convergence near 0, got {}", converged)).c_str());

  const int outlierId = candidates.back()->getId();
  OI_REQUIRE2(!function->getShouldBeUsed(0, outlierId),
              "the outlier must be excluded");

  const std::vector<PipelineExclusion> exclusions =
      function->getPipelineExclusions(0);
  bool found = false;
  for (const PipelineExclusion& excl : exclusions) {
    if (excl.elementId == outlierId) {
      found = true;
      OI_REQUIRE2(excl.reason.contains("residual"), (excl.reason).c_str());
    }
  }
  OI_REQUIRE2(
      found,
      "the outlier's exclusion must be individually reportable with a reason");
}

/*!
 * The negative half of the acceptance criterion: the same data, without
 * the filter declared, must NOT converge correctly - the mechanism has
 * to actually change the result, not just be inert scaffolding.
 */
void PipelineTest::omittingTheFilterLeavesTheResultSkewedByTheOutlier() {
  auto job = std::make_unique<OiJob>();
  oi::Guarded<FeatureWrapper> target =
      job->addFeature(this->makeTargetPoint("Target5"));
  REQUIRE(target);

  auto own_function = std::make_unique<FakeAverageFit>();
  const oi::Guarded<FakeAverageFit> function = own_function.get();
  target->getPoint()->addFunction(std::move(own_function));

  std::vector<double> xs;
  for (int i = 0; i < 20; i++) {
    xs.push_back(0.0);
  }
  xs.push_back(1000.0);  // the deliberate outlier
  for (double x : xs) {
    oi::Guarded<Point> p = this->makeCandidate(x, job);
    InputElement e(p->getId());
    e.typeOfElement = ElementTypes::ePointElement;
    e.point = p;
    function->addInputElement(e, 0);
  }

  // no pipeline declared

  target->getPoint()->recalc();

  const double skewed = target->getPoint()->getPosition().getVector().getAt(0);
  OI_REQUIRE2(
      skewed > 10.0,
      (std::format("expected the outlier to skew the average, got {}", skewed))
          .c_str());
}

/*!
 * "Adding a new outlier re-triggers the filter and the fit" (acceptance
 * criteria): the pipeline is declared once and re-evaluates on every
 * ordinary recalc, not just the recalc immediately after it was declared.
 */
void PipelineTest::addingANewOutlierLaterReTriggersTheFilterOnRecalc() {
  auto job = std::make_unique<OiJob>();
  oi::Guarded<FeatureWrapper> target =
      job->addFeature(this->makeTargetPoint("Target6"));
  REQUIRE(target);

  auto own_function = std::make_unique<FakeAverageFit>();
  const oi::Guarded<FakeAverageFit> function = own_function.get();
  target->getPoint()->addFunction(std::move(own_function));

  // 20 good points, same reasoning as
  // residualOutlierRejectionExcludesTheOutlierAndConverges() re: sample
  // size and the sigma-clipping masking effect
  std::vector<oi::Guarded<Point>> candidates;
  for (int i = 0; i < 20; i++) {
    oi::Guarded<Point> p = this->makeCandidate(0.0, job);
    candidates.push_back(p);
    InputElement e(p->getId());
    e.typeOfElement = ElementTypes::ePointElement;
    e.point = p;
    function->addInputElement(e, 0);
  }

  function->setPipeline(0, std::vector<PipelineStage>{
                               PipelineStage::residualOutlierRejection(3.0)});

  target->getPoint()->recalc();
  REQUIRE(function->getPipelineExclusions(0).empty());
  OI_COMPARE(target->getPoint()->getPosition().getVector().getAt(0), 0.0);

  // a new outlier arrives later - no pipeline redeclaration, just new data
  oi::Guarded<Point> outlier = this->makeCandidate(1000.0, job);
  InputElement e(outlier->getId());
  e.typeOfElement = ElementTypes::ePointElement;
  e.point = outlier;
  function->addInputElement(e, 0);

  target->getPoint()->recalc();

  OI_REQUIRE2(!function->getShouldBeUsed(0, outlier->getId()),
              "a newly-added outlier must be caught on the next recalc without "
              "redeclaring anything");
  OI_REQUIRE2(
      std::abs(target->getPoint()->getPosition().getVector().getAt(0)) < 1.0,
      "the fit must still converge correctly after the new outlier is "
      "filtered");
}

TEST_CASE("PipelineTest::undeclaredPipelineLeavesShouldBeUsedUntouched",
          "[PipelineTest]") {
  PipelineTest fixture;
  fixture.initTestCase();
  fixture.undeclaredPipelineLeavesShouldBeUsedUntouched();
}

TEST_CASE("PipelineTest::explicitListStageKeepsOnlyListedIdsAndRecordsReasons",
          "[PipelineTest]") {
  PipelineTest fixture;
  fixture.initTestCase();
  fixture.explicitListStageKeepsOnlyListedIdsAndRecordsReasons();
}

TEST_CASE("PipelineTest::excludeStageDropsSpecificIdWithReason",
          "[PipelineTest]") {
  PipelineTest fixture;
  fixture.initTestCase();
  fixture.excludeStageDropsSpecificIdWithReason();
}

TEST_CASE(
    "PipelineTest::residualOutlierRejectionExcludesTheOutlierAndConverges",
    "[PipelineTest]") {
  PipelineTest fixture;
  fixture.initTestCase();
  fixture.residualOutlierRejectionExcludesTheOutlierAndConverges();
}

TEST_CASE("PipelineTest::omittingTheFilterLeavesTheResultSkewedByTheOutlier",
          "[PipelineTest]") {
  PipelineTest fixture;
  fixture.initTestCase();
  fixture.omittingTheFilterLeavesTheResultSkewedByTheOutlier();
}

TEST_CASE("PipelineTest::addingANewOutlierLaterReTriggersTheFilterOnRecalc",
          "[PipelineTest]") {
  PipelineTest fixture;
  fixture.initTestCase();
  fixture.addingANewOutlierLaterReTriggersTheFilterOnRecalc();
}

#include <format>
