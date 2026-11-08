#include <catch2/catch_test_macros.hpp>
#include <string>

#include "catch_compat.h"
#include "choose_la_lib.h"
#include "construct_function.h"
#include "containers.h"
#include "feature_wrapper.h"
#include "geometry/point.h"
#include "oi_job.h"

using namespace oi;

namespace {

/*!
 * \brief A minimal, real producer function so a bare Point can become
 * "solved" without going through a real fit or acquisition - just enough
 * to stand in for an owner feature's own producer in these tests. x/y/z
 * are mutable so a test can simulate the owner moving between recalcs.
 */
class FixedPositionFunction : public ConstructFunction, public Producer<Point> {
 public:
  explicit FixedPositionFunction() : ConstructFunction() {
    this->metaData.name = "FixedPositionFunction";
    this->applicableFor.push_back(FeatureTypes::ePointFeature);
  }
  double x = 0.0, y = 0.0, z = 0.0;
  std::expected<Point, std::string> exec() override {
    Point point;
    point.setPoint(Position(x, y, z));
    return point;
  }
};

/*!
 * \brief A minimal, real ConstructFunction that copies its one
 * ePointElement input's position onto the result. Stands in for
 * LineFromPlaneNormal/PointFromPlaneOrigin (openindy-default-plugin,
 * Stage 7c-i/7h) without a cross-repo test dependency -
 * OiJob::addOwnedSubGeometry does not care which ConstructFunction it is
 * given, so this exercises the generic mechanism the real Plane accessors
 * also go through.
 */
class CopyPointFunction : public ConstructFunction, public Producer<Point> {
 public:
  explicit CopyPointFunction() : ConstructFunction() {
    this->metaData.name = "CopyPointFunction";
    NeededElement param;
    param.typeOfElement = ElementTypes::ePointElement;
    param.infinite = false;
    param.description = "the owner feature this accessor is bound to";
    this->neededElements.push_back(param);
    this->applicableFor.push_back(FeatureTypes::ePointFeature);
  }
  std::expected<Point, std::string> exec() override {
    if (!this->inputElements.contains(0) ||
        this->inputElements[0].size() != 1) {
      return std::unexpected(
          std::string("CopyPointFunction: no input element"));
    }
    const InputElement& element = this->inputElements[0].front();
    if (element.point.isNull() || !element.point->getIsSolved()) {
      return std::unexpected(
          std::string("CopyPointFunction: input point not solved"));
    }
    Point point;
    point.setPoint(element.point->getPosition());
    return point;
  }
};

}  // namespace

/*!
 * \brief Stage 7h - D19 applied to functions (domain-model.md S1/S6).
 * Covers the OiJob-level generation/regeneration/cascade-delete
 * primitives that make function-owned sub-geometry a real, reusable
 * mechanism rather than a Plane-specific special case. Plane's actual
 * accessor math (LineFromPlaneNormal/PointFromPlaneOrigin) is covered
 * separately in openindy-default-plugin's tst_derivedfeatures, which
 * drives this same OiJob::addOwnedSubGeometry entry point with the real
 * functions.
 */
class OwnedSubGeometryTest {
 public:
  void initTestCase();

  void generatingAnOwnedAccessorAttachesFunctionBindsInputAndMarksOwnership();
  void ownerRecalcOnlyDirtiesOwnedChildWhenItsValueActuallyChanges();
  void aSecondFeatureCanReferenceTheOwnedChildAsInput();
  void removingTheOwnerCascadesToItsOwnedChild();

 private:
  std::unique_ptr<OiJob> makeJob();
};

void OwnedSubGeometryTest::initTestCase() {
  ChooseLALib::setLinearAlgebra(ChooseLALib::LinearAlgebraLibrary::Armadillo);
}

std::unique_ptr<OiJob> OwnedSubGeometryTest::makeJob() {
  return std::make_unique<OiJob>();
}

void OwnedSubGeometryTest::
    generatingAnOwnedAccessorAttachesFunctionBindsInputAndMarksOwnership() {
  const std::unique_ptr<OiJob> job = this->makeJob();

  auto own_owner = std::make_unique<oi::Point>();
  oi::Guarded<Point> owner = own_owner.get();
  oi::Guarded<oi::FeatureWrapper> ownerWrapper = owner->getFeatureWrapper();
  owner->setFeatureName("Owner1");
  REQUIRE(job->addFeature(std::move(own_owner)));

  auto own_child = std::make_unique<oi::Point>();
  oi::Guarded<Point> child = own_child.get();
  oi::Guarded<oi::FeatureWrapper> childWrapper = child->getFeatureWrapper();
  child->setFeatureName("Child1");
  REQUIRE(job->addFeature(std::move(own_child)));

  auto owned_function = std::make_unique<CopyPointFunction>();

  const oi::Guarded<CopyPointFunction> function(owned_function.get());
  REQUIRE(job->addOwnedSubGeometry(ownerWrapper, childWrapper,
                                   std::move(owned_function), 0));

  // the function got attached, and the owner got bound as its input -
  // reusing Feature::addFunction/OiJob::addInputFeature, not a parallel
  // mechanism
  OI_COMPARE(child->getFunctions().size(), 1);
  OI_COMPARE(oi::value(function->getInputElements(), 0).size(), 1);
  OI_COMPARE(oi::value(function->getInputElements(), 0).front().id,
             owner->getId());

  // ownership recorded on both sides
  REQUIRE(child->getOwnerFeature().has_value());
  OI_COMPARE(child->getOwnerFeature().value().data(), owner.data());
  REQUIRE(oi::contains(owner->getOwnedSubGeometries(), child));

  const std::optional<std::string> description = child->getOwnerDescription();
  REQUIRE(description.has_value());
  REQUIRE(description->contains("Owner1"));

  // the ordinary dependency graph is wired too (previouslyNeeded/usedFor) -
  // this is what lets FeatureUpdater's existing DAG walk find and
  // recalculate the child, with no second recalculation path. Checked by
  // id rather than oi::Guarded<FeatureWrapper> identity: a Feature's own
  // selfFeature (auto-created in its constructor) is a different
  // FeatureWrapper object than whatever external wrapper a caller
  // registers it under (see OiJob::createFeatureWrapper, which does the
  // same "new FeatureWrapper(); wrapper->setPoint(new Point())" thing
  // this test does) - the symmetric addUsedFor/addPreviouslyNeeded calls
  // use selfFeature, so list membership must be compared by the
  // underlying feature's id, not object identity, same as usedForMap/
  // previouslyNeededMap already do internally.
  REQUIRE(oi::contains(child->getPreviouslyNeeded(), ownerWrapper));
  bool ownerUsedForContainsChild = false;
  for (const oi::Guarded<FeatureWrapper>& usedFor : owner->getUsedFor()) {
    if (!usedFor.isNull() && !usedFor->getFeature().isNull() &&
        usedFor->getFeature()->getId() == child->getId()) {
      ownerUsedForContainsChild = true;
    }
  }
  REQUIRE(ownerUsedForContainsChild);
}

void OwnedSubGeometryTest::
    ownerRecalcOnlyDirtiesOwnedChildWhenItsValueActuallyChanges() {
  const std::unique_ptr<OiJob> job = this->makeJob();

  auto own_owner = std::make_unique<oi::Point>();
  oi::Guarded<Point> owner = own_owner.get();
  oi::Guarded<oi::FeatureWrapper> ownerWrapper = owner->getFeatureWrapper();
  owner->setFeatureName("Owner2");
  REQUIRE(job->addFeature(std::move(own_owner)));

  auto owned_ownerFn = std::make_unique<FixedPositionFunction>();

  const oi::Guarded<FixedPositionFunction> ownerFn(owned_ownerFn.get());
  ownerFn->x = 1.0;
  ownerFn->y = 2.0;
  ownerFn->z = 3.0;
  owner->addFunction(std::move(owned_ownerFn));

  auto own_child = std::make_unique<oi::Point>();
  oi::Guarded<Point> child = own_child.get();
  oi::Guarded<oi::FeatureWrapper> childWrapper = child->getFeatureWrapper();
  child->setFeatureName("Child2");
  REQUIRE(job->addFeature(std::move(own_child)));

  REQUIRE(job->addOwnedSubGeometry(ownerWrapper, childWrapper,
                                   std::make_unique<CopyPointFunction>(), 0));

  // first pass: owner solves for the first time, child computes from the
  // owner's now-current position. There is no FeatureUpdater in core to
  // drive the DAG walk, so this simulates it by calling recalc() in
  // dependency order - exactly what
  // FeatureUpdater::recursiveFeatureRecalculation does in the real app.
  owner->recalc();
  child->recalc();
  child->setIsUpdated(true);

  OI_COMPARE(child->getPosition().getVector().getAt(0), 1.0);
  OI_COMPARE(child->getPosition().getVector().getAt(2), 3.0);

  // owner recalculates again to an identical value - Geometry::recalc()
  // must NOT dirty the child. This is what keeps a project reload (an
  // ordinary recalc pass over an unchanged graph) from re-executing an
  // owned accessor's construction chain (domain-model.md S1/S6).
  owner->recalc();
  OI_REQUIRE2(
      child->getIsUpdated(),
      "recalculating the owner to the same value must not dirty an owned "
      "child");

  // now the owner actually moves - the child must be dirtied so the next
  // DAG pass regenerates it, which is what keeps it from drifting
  ownerFn->x = 9.0;
  owner->recalc();
  OI_REQUIRE2(
      !child->getIsUpdated(),
      "recalculating the owner to a new value must dirty its owned child");

  child->recalc();
  OI_COMPARE(child->getPosition().getVector().getAt(0), 9.0);
}

void OwnedSubGeometryTest::aSecondFeatureCanReferenceTheOwnedChildAsInput() {
  const std::unique_ptr<OiJob> job = this->makeJob();

  auto own_owner = std::make_unique<oi::Point>();
  oi::Guarded<Point> owner = own_owner.get();
  oi::Guarded<oi::FeatureWrapper> ownerWrapper = owner->getFeatureWrapper();
  owner->setFeatureName("Owner3");
  REQUIRE(job->addFeature(std::move(own_owner)));

  auto own_child = std::make_unique<oi::Point>();
  oi::Guarded<Point> child = own_child.get();
  oi::Guarded<oi::FeatureWrapper> childWrapper = child->getFeatureWrapper();
  child->setFeatureName("Child3");
  REQUIRE(job->addFeature(std::move(own_child)));

  REQUIRE(job->addOwnedSubGeometry(ownerWrapper, childWrapper,
                                   std::make_unique<CopyPointFunction>(), 0));

  // a third, ordinary feature takes the owned child as an input - proves
  // an owned accessor is just an ordinary feature to the input
  // machinery, no special-casing needed (domain-model.md S6: "no new
  // addressing syntax")
  auto own_consumer = std::make_unique<oi::Point>();
  oi::Guarded<Point> consumer = own_consumer.get();
  oi::Guarded<oi::FeatureWrapper> consumerWrapper =
      consumer->getFeatureWrapper();
  consumer->setFeatureName("Consumer3");
  REQUIRE(job->addFeature(std::move(own_consumer)));

  auto owned_consumerFn = std::make_unique<CopyPointFunction>();

  const oi::Guarded<CopyPointFunction> consumerFn(owned_consumerFn.get());
  consumer->addFunction(std::move(owned_consumerFn));
  job->addInputFeature(consumerWrapper, 0, 0, childWrapper);

  OI_COMPARE(oi::value(consumerFn->getInputElements(), 0).size(), 1);
  OI_COMPARE(oi::value(consumerFn->getInputElements(), 0).front().id,
             child->getId());
  REQUIRE(oi::contains(consumer->getPreviouslyNeeded(), childWrapper));
}

void OwnedSubGeometryTest::removingTheOwnerCascadesToItsOwnedChild() {
  const std::unique_ptr<OiJob> job = this->makeJob();

  auto own_owner = std::make_unique<oi::Point>();
  oi::Guarded<Point> owner = own_owner.get();
  oi::Guarded<oi::FeatureWrapper> ownerWrapper = owner->getFeatureWrapper();
  owner->setFeatureName("Owner4");
  REQUIRE(job->addFeature(std::move(own_owner)));

  auto own_child = std::make_unique<oi::Point>();
  oi::Guarded<Point> child = own_child.get();
  oi::Guarded<oi::FeatureWrapper> childWrapper = child->getFeatureWrapper();
  child->setFeatureName("Child4");
  REQUIRE(job->addFeature(std::move(own_child)));

  REQUIRE(job->addOwnedSubGeometry(ownerWrapper, childWrapper,
                                   std::make_unique<CopyPointFunction>(), 0));

  const int childId = child->getId();
  REQUIRE(!job->getFeatureById(childId).isNull());

  REQUIRE(job->removeFeature(ownerWrapper->getFeature()->getId()));

  // the owner is gone, and so is what it owned - D19 "deleting the owner
  // deletes its owned sub-geometry" - not left dangling for some other
  // feature to reference a now-orphaned id
  REQUIRE(job->getFeatureById(childId).isNull());
}

TEST_CASE(
    "OwnedSubGeometryTest::"
    "generatingAnOwnedAccessorAttachesFunctionBindsInputAndMarksOwnership",
    "[OwnedSubGeometryTest]") {
  OwnedSubGeometryTest fixture;
  fixture.initTestCase();
  fixture
      .generatingAnOwnedAccessorAttachesFunctionBindsInputAndMarksOwnership();
}

TEST_CASE(
    "OwnedSubGeometryTest::"
    "ownerRecalcOnlyDirtiesOwnedChildWhenItsValueActuallyChanges",
    "[OwnedSubGeometryTest]") {
  OwnedSubGeometryTest fixture;
  fixture.initTestCase();
  fixture.ownerRecalcOnlyDirtiesOwnedChildWhenItsValueActuallyChanges();
}

TEST_CASE(
    "OwnedSubGeometryTest::aSecondFeatureCanReferenceTheOwnedChildAsInput",
    "[OwnedSubGeometryTest]") {
  OwnedSubGeometryTest fixture;
  fixture.initTestCase();
  fixture.aSecondFeatureCanReferenceTheOwnedChildAsInput();
}

TEST_CASE("OwnedSubGeometryTest::removingTheOwnerCascadesToItsOwnedChild",
          "[OwnedSubGeometryTest]") {
  OwnedSubGeometryTest fixture;
  fixture.initTestCase();
  fixture.removingTheOwnerCascadesToItsOwnedChild();
}
