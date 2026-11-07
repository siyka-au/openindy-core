#include <catch2/catch_test_macros.hpp>
#include <map>
#include <string>

#include "catch_compat.h"
#include "chooselalib.h"
#include "constructfunction.h"
#include "containers.h"
#include "fitfunction.h"
#include "point.h"

using namespace oi;

// Minimal stand-ins for a real fit/construct plugin function. Beyond being
// distinguishable *types* for Geometry::getOwnerDescription's dynamic_cast,
// each now also has to implement Producer<Point> (Stage 7d/D21) - since
// Feature::addFunction() only attaches a function that implements the
// slot's required mixin, a fake with no real exec() would simply be
// rejected at attach time and never become this Point's producer at all.
class FakeFit : public FitFunction, public Producer<Point> {
 public:
  explicit FakeFit() : FitFunction() { this->metaData.name = "FakeFit"; }
  std::expected<Point, std::string> exec() override { return Point(); }
};

class FakeConstruct : public ConstructFunction, public Producer<Point> {
 public:
  explicit FakeConstruct() : ConstructFunction() {
    this->metaData.name = "FakeConstruct";
  }
  std::expected<Point, std::string> exec() override { return Point(); }
};

/*!
 * \brief Stage 7g regression coverage for D17 (nominal/actual as two
 * ordinary geometries, distinguished structurally by producer) and D19
 * (ownership determines editability, enforced at the API - not just
 * hidden in the UI).
 */
class OwnershipTest {
 public:
  void initTestCase();

  void freshGeometryIsUserOwnedAndEditable();
  void constructFunctionProducerLeavesGeometryEditable();
  void fitFunctionProducerMakesGeometryReadOnly();
  void readOnlyGeometryRejectsDirectParameterEdits();
  void actualCanRealiseAnEditableNominal();
  void manyActualsMayRealiseTheSameNominal();
  void actualCannotBeRealisedByAnotherActual();

  // Stage 7h: D19 applied to functions - function-owned sub-geometry
  void ownedSubGeometryIsReadOnlyAndNamesItsOwner();
  void ownedSubGeometryRejectsDirectParameterEdits();
  void settingOwnerFeatureRegistersOnBothSides();
  void aGeometryCannotOwnItself();
  void destroyingAnOwnedGeometryUnregistersFromItsOwner();
};

void OwnershipTest::initTestCase() {
  ChooseLALib::setLinearAlgebra(ChooseLALib::LinearAlgebraLibrary::Armadillo);
}

void OwnershipTest::freshGeometryIsUserOwnedAndEditable() {
  Point point;
  REQUIRE(!point.getOwnerDescription().has_value());
}

void OwnershipTest::constructFunctionProducerLeavesGeometryEditable() {
  Point point;
  oi::Guarded<FakeConstruct> construct = new FakeConstruct();
  point.addFunction(construct);
  REQUIRE(!point.getOwnerDescription().has_value());
}

void OwnershipTest::fitFunctionProducerMakesGeometryReadOnly() {
  Point point;
  oi::Guarded<FakeFit> fit = new FakeFit();
  point.addFunction(fit);
  const std::optional<std::string> owner = point.getOwnerDescription();
  REQUIRE(owner.has_value());
  REQUIRE(owner->contains("FakeFit"));
}

void OwnershipTest::readOnlyGeometryRejectsDirectParameterEdits() {
  Point point;
  point.setPoint(Position(1.0, 2.0, 3.0));

  oi::Guarded<FakeFit> fit = new FakeFit();
  point.addFunction(fit);
  REQUIRE(point.getOwnerDescription().has_value());

  std::map<GeometryParameters, double> edit;
  edit.insert_or_assign(GeometryParameters::eUnknownX, 99.0);
  point.setUnknownParameters(edit);

  // rejected at the API - the fit owns this point, so the edit must not
  // have taken effect
  OI_COMPARE(point.getPosition().getVector().getAt(0), 1.0);
}

void OwnershipTest::actualCanRealiseAnEditableNominal() {
  Point nominal;
  REQUIRE(!nominal.getOwnerDescription().has_value());

  Point actual;
  actual.addFunction(new FakeFit());
  REQUIRE(actual.getOwnerDescription().has_value());

  REQUIRE(actual.setRealises(nominal.getFeatureWrapper()->getGeometry()));
  REQUIRE(actual.getRealises().has_value());
  OI_COMPARE(actual.getRealises().value().data(),
             static_cast<Geometry*>(&nominal));

  // the nominal itself carries no back-reference - D17 deliberately
  // does not maintain a reverse list
  REQUIRE(!nominal.getRealises().has_value());
}

void OwnershipTest::manyActualsMayRealiseTheSameNominal() {
  // the case that decided the design: re-inspecting the same nominal
  // over time produces many actuals, not one
  Point nominal;

  Point actualA;
  actualA.addFunction(new FakeFit());
  Point actualB;
  actualB.addFunction(new FakeFit());

  REQUIRE(actualA.setRealises(nominal.getFeatureWrapper()->getGeometry()));
  REQUIRE(actualB.setRealises(nominal.getFeatureWrapper()->getGeometry()));

  OI_COMPARE(actualA.getRealises().value().data(),
             static_cast<Geometry*>(&nominal));
  OI_COMPARE(actualB.getRealises().value().data(),
             static_cast<Geometry*>(&nominal));
}

void OwnershipTest::actualCannotBeRealisedByAnotherActual() {
  Point actualA;
  actualA.addFunction(new FakeFit());

  Point actualB;
  actualB.addFunction(new FakeFit());

  // realising another actual makes no sense - only a user-owned
  //(editable) geometry can be the nominal side of the relationship
  REQUIRE(!actualB.setRealises(actualA.getFeatureWrapper()->getGeometry()));
  REQUIRE(!actualB.getRealises().has_value());
}

/*!
 * Stage 7h coverage for D19 applied to functions (domain-model.md S6):
 * function-owned sub-geometry is a real feature in the tree, owned by an
 * owner *feature* (not identified by its producer's function type, unlike
 * the FitFunction/nominal-actual case above) and read-only. The exact
 * same ConstructFunction (e.g. LineFromPlaneNormal, openindy-default-plugin)
 * produces an independent, editable feature when attached directly by a
 * user (7c-i, still covered by constructFunctionProducerLeavesGeometryEditable
 * above) and a bound, read-only one here - what differs is only whether
 * setOwnerFeature was called, which is OiJob::addOwnedSubGeometry's job,
 * not the function's.
 */
void OwnershipTest::ownedSubGeometryIsReadOnlyAndNamesItsOwner() {
  Point owner;
  owner.setFeatureName("Plane1");

  Point child;
  oi::Guarded<FakeConstruct> construct = new FakeConstruct();
  child.addFunction(construct);

  // still editable before ownership is recorded - the same construct
  // function, unowned, is 7c-i's independent path
  REQUIRE(!child.getOwnerDescription().has_value());

  REQUIRE(child.setOwnerFeature(owner.getFeatureWrapper()->getGeometry()));

  const std::optional<std::string> description = child.getOwnerDescription();
  REQUIRE(description.has_value());
  REQUIRE(description->contains("FakeConstruct"));
  REQUIRE(description->contains("Plane1"));
}

void OwnershipTest::ownedSubGeometryRejectsDirectParameterEdits() {
  Point owner;
  Point child;
  child.setPoint(Position(1.0, 2.0, 3.0));
  child.addFunction(new FakeConstruct());
  REQUIRE(child.setOwnerFeature(owner.getFeatureWrapper()->getGeometry()));

  std::map<GeometryParameters, double> edit;
  edit.insert_or_assign(GeometryParameters::eUnknownX, 99.0);
  child.setUnknownParameters(edit);

  // rejected at the API, exactly like the fit-owned case above
  OI_COMPARE(child.getPosition().getVector().getAt(0), 1.0);
}

void OwnershipTest::settingOwnerFeatureRegistersOnBothSides() {
  Point owner;
  Point childA;
  Point childB;

  REQUIRE(childA.setOwnerFeature(owner.getFeatureWrapper()->getGeometry()));
  REQUIRE(childB.setOwnerFeature(owner.getFeatureWrapper()->getGeometry()));

  OI_COMPARE(owner.getOwnedSubGeometries().size(), 2);
  REQUIRE(oi::contains(owner.getOwnedSubGeometries(),
                       childA.getFeatureWrapper()->getGeometry()));
  REQUIRE(oi::contains(owner.getOwnedSubGeometries(),
                       childB.getFeatureWrapper()->getGeometry()));

  REQUIRE(childA.getOwnerFeature().has_value());
  OI_COMPARE(childA.getOwnerFeature().value().data(),
             static_cast<Geometry*>(&owner));
}

void OwnershipTest::aGeometryCannotOwnItself() {
  Point point;
  REQUIRE(!point.setOwnerFeature(point.getFeatureWrapper()->getGeometry()));
  REQUIRE(!point.getOwnerFeature().has_value());
}

void OwnershipTest::destroyingAnOwnedGeometryUnregistersFromItsOwner() {
  Point owner;

  {
    Point child;
    REQUIRE(child.setOwnerFeature(owner.getFeatureWrapper()->getGeometry()));
    OI_COMPARE(owner.getOwnedSubGeometries().size(), 1);
  }

  // child just went out of scope (~Geometry ran) - the owner's list must
  // not be left pointing at a destroyed object
  OI_COMPARE(owner.getOwnedSubGeometries().size(), 0);
}

TEST_CASE("OwnershipTest::freshGeometryIsUserOwnedAndEditable",
          "[OwnershipTest]") {
  OwnershipTest fixture;
  fixture.initTestCase();
  fixture.freshGeometryIsUserOwnedAndEditable();
}

TEST_CASE("OwnershipTest::constructFunctionProducerLeavesGeometryEditable",
          "[OwnershipTest]") {
  OwnershipTest fixture;
  fixture.initTestCase();
  fixture.constructFunctionProducerLeavesGeometryEditable();
}

TEST_CASE("OwnershipTest::fitFunctionProducerMakesGeometryReadOnly",
          "[OwnershipTest]") {
  OwnershipTest fixture;
  fixture.initTestCase();
  fixture.fitFunctionProducerMakesGeometryReadOnly();
}

TEST_CASE("OwnershipTest::readOnlyGeometryRejectsDirectParameterEdits",
          "[OwnershipTest]") {
  OwnershipTest fixture;
  fixture.initTestCase();
  fixture.readOnlyGeometryRejectsDirectParameterEdits();
}

TEST_CASE("OwnershipTest::actualCanRealiseAnEditableNominal",
          "[OwnershipTest]") {
  OwnershipTest fixture;
  fixture.initTestCase();
  fixture.actualCanRealiseAnEditableNominal();
}

TEST_CASE("OwnershipTest::manyActualsMayRealiseTheSameNominal",
          "[OwnershipTest]") {
  OwnershipTest fixture;
  fixture.initTestCase();
  fixture.manyActualsMayRealiseTheSameNominal();
}

TEST_CASE("OwnershipTest::actualCannotBeRealisedByAnotherActual",
          "[OwnershipTest]") {
  OwnershipTest fixture;
  fixture.initTestCase();
  fixture.actualCannotBeRealisedByAnotherActual();
}

TEST_CASE("OwnershipTest::ownedSubGeometryIsReadOnlyAndNamesItsOwner",
          "[OwnershipTest]") {
  OwnershipTest fixture;
  fixture.initTestCase();
  fixture.ownedSubGeometryIsReadOnlyAndNamesItsOwner();
}

TEST_CASE("OwnershipTest::ownedSubGeometryRejectsDirectParameterEdits",
          "[OwnershipTest]") {
  OwnershipTest fixture;
  fixture.initTestCase();
  fixture.ownedSubGeometryRejectsDirectParameterEdits();
}

TEST_CASE("OwnershipTest::settingOwnerFeatureRegistersOnBothSides",
          "[OwnershipTest]") {
  OwnershipTest fixture;
  fixture.initTestCase();
  fixture.settingOwnerFeatureRegistersOnBothSides();
}

TEST_CASE("OwnershipTest::aGeometryCannotOwnItself", "[OwnershipTest]") {
  OwnershipTest fixture;
  fixture.initTestCase();
  fixture.aGeometryCannotOwnItself();
}

TEST_CASE("OwnershipTest::destroyingAnOwnedGeometryUnregistersFromItsOwner",
          "[OwnershipTest]") {
  OwnershipTest fixture;
  fixture.initTestCase();
  fixture.destroyingAnOwnedGeometryUnregistersFromItsOwner();
}
