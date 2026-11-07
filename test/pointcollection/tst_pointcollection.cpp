#include <catch2/catch_test_macros.hpp>
#include <vector>

#include "catch_compat.h"
#include "chooselalib.h"
#include "featurewrapper.h"
#include "geometry/point.h"
#include "geometry/pointcloud.h"
#include "group.h"
#include "pointcollection.h"
#include "pointgroup.h"

using namespace oi;
using namespace oi::math;

namespace {

// a solved Point wrapped for group membership, which is how every real
// point reaches a group
oi::Guarded<FeatureWrapper> makePoint(double x, double y, double z, int id) {
  oi::Guarded<Point> point = new Point(id, Position(x, y, z));
  point->setIsSolved(true);
  oi::Guarded<FeatureWrapper> wrapper = new FeatureWrapper();
  wrapper->setPoint(point);
  return wrapper;
}

}  // namespace

/*!
 * Stage 7a: PointCollection is the type a fit function asks for, and both a
 * scanner's PointCloud and a PointGroup full of individually measured
 * Points satisfy it. These tests pin the semantics that make that safe -
 * particularly that a Group references and never owns, since the whole
 * point of many-to-many membership is measuring once and reusing.
 */
class PointCollectionTest {
 public:
  void initTestCase();

  void groupHoldsReferencesNotCopies();
  void pointInManyGroupsIsStoredOnce();
  void constraintRejectsWrongType();
  void duplicateAndSelfMembershipRejected();
  void compactDropsDeletedMembers();
  void deletingAGroupDoesNotDeleteItsMembers();
  void pointGroupSatisfiesPointCollection();
  void unsolvedPointsAreNotPartOfTheCollection();
  void pointCloudSatisfiesPointCollection();
  void bothCollectionsAreInterchangeableThroughTheInterface();
};

void PointCollectionTest::initTestCase() {
  ChooseLALib::setLinearAlgebra(ChooseLALib::LinearAlgebraLibrary::Armadillo);
}

void PointCollectionTest::groupHoldsReferencesNotCopies() {
  oi::Guarded<FeatureWrapper> point = makePoint(1.0, 2.0, 3.0, 1);

  PointGroup group("wall");
  REQUIRE(group.addMember(point));
  OI_COMPARE(group.memberCount(), 1);

  // mutating the point must be visible through the group - a copy would
  // silently diverge, which is exactly the failure mode this design exists
  // to prevent
  Position moved(9.0, 9.0, 9.0);
  point->getPoint()->setPoint(moved);

  const std::vector<Position> positions = group.collectionPoints();
  OI_COMPARE(positions.size(), 1);
  OI_COMPARE(positions.front().getVector().getAt(0), 9.0);
}

void PointCollectionTest::pointInManyGroupsIsStoredOnce() {
  oi::Guarded<FeatureWrapper> shared = makePoint(1.0, 1.0, 1.0, 1);

  PointGroup wall("wall");
  PointGroup control("control network");

  REQUIRE(wall.addMember(shared));
  REQUIRE(control.addMember(shared));

  // same underlying feature, reached from both groups - no copy, no second
  // measurement, no re-identification
  OI_COMPARE(wall.getPoints().front().data(),
             control.getPoints().front().data());
  OI_COMPARE(wall.getPoints().front()->getId(),
             control.getPoints().front()->getId());
}

void PointCollectionTest::constraintRejectsWrongType() {
  oi::Guarded<PointCloud> cloud = new PointCloud();
  oi::Guarded<FeatureWrapper> cloudWrapper = new FeatureWrapper();
  cloudWrapper->setPointCloud(cloud);

  PointGroup group("points only");
  OI_REQUIRE2(!group.addMember(cloudWrapper),
              "a point group must not accept a point cloud as a member");
  OI_COMPARE(group.memberCount(), 0);

  // an unconstrained Group takes anything - that generality is why there
  // is one Group class rather than one per feature type
  Group anything("mixed");
  REQUIRE(anything.addMember(cloudWrapper));
  REQUIRE(anything.addMember(makePoint(0.0, 0.0, 0.0, 51)));
  OI_COMPARE(anything.memberCount(), 2);
}

void PointCollectionTest::duplicateAndSelfMembershipRejected() {
  oi::Guarded<FeatureWrapper> point = makePoint(1.0, 1.0, 1.0, 1);

  PointGroup group("wall");
  REQUIRE(group.addMember(point));
  OI_REQUIRE2(!group.addMember(point),
              "the same feature must not be added twice");
  OI_COMPARE(group.memberCount(), 1);

  // a group containing itself would recurse forever on traversal
  Group outer("outer");
  oi::Guarded<FeatureWrapper> selfWrapper = new FeatureWrapper();
  OI_REQUIRE2(!outer.addMember(selfWrapper),
              "a null-typed wrapper must be rejected outright");
}

void PointCollectionTest::compactDropsDeletedMembers() {
  oi::Guarded<FeatureWrapper> keep = makePoint(1.0, 1.0, 1.0, 1);
  oi::Guarded<FeatureWrapper> discard = makePoint(2.0, 2.0, 2.0, 2);

  PointGroup group("wall");
  REQUIRE(group.addMember(keep));
  REQUIRE(group.addMember(discard));
  OI_COMPARE(group.memberCount(), 2);

  delete discard->getPoint().data();
  delete discard.data();

  // memberCount already ignores the dead reference; compact() removes it
  OI_COMPARE(group.memberCount(), 1);
  group.compact();
  OI_COMPARE(group.getMembers().size(), 1);
}

void PointCollectionTest::deletingAGroupDoesNotDeleteItsMembers() {
  oi::Guarded<FeatureWrapper> point = makePoint(1.0, 2.0, 3.0, 1);

  {
    PointGroup group("temporary");
    REQUIRE(group.addMember(point));
  }

  // the group is gone; the point it referenced must be untouched. A group
  // organises, it never owns.
  OI_REQUIRE2(!point.isNull(), "deleting a group must not delete its members");
  REQUIRE(!point->getPoint().isNull());
  OI_COMPARE(point->getPoint()->getPosition().getVector().getAt(0), 1.0);
}

void PointCollectionTest::pointGroupSatisfiesPointCollection() {
  PointGroup group("wall");
  group.addMember(makePoint(0.0, 0.0, 0.0, 1));
  group.addMember(makePoint(1.0, 0.0, 0.0, 2));
  group.addMember(makePoint(0.0, 1.0, 0.0, 3));

  // reached only through the interface, exactly as a fit function will
  PointCollection* collection = &group;
  OI_COMPARE(collection->collectionPointCount(), 3);
  OI_COMPARE(collection->collectionPoints().size(), 3);
}

void PointCollectionTest::unsolvedPointsAreNotPartOfTheCollection() {
  PointGroup group("wall");
  group.addMember(makePoint(0.0, 0.0, 0.0, 1));

  oi::Guarded<Point> unsolved = new Point(2, Position(5.0, 5.0, 5.0));
  unsolved->setIsSolved(false);
  oi::Guarded<FeatureWrapper> unsolvedWrapper = new FeatureWrapper();
  unsolvedWrapper->setPoint(unsolved);
  REQUIRE(group.addMember(unsolvedWrapper));

  // it is a member, but it has no trustworthy position yet - feeding it to
  // a fit would quietly corrupt the result
  OI_COMPARE(group.memberCount(), 2);
  OI_COMPARE(group.collectionPointCount(), 1);
}

void PointCollectionTest::pointCloudSatisfiesPointCollection() {
  oi::Guarded<PointCloud> cloud = new PointCloud();
  cloud->addPointCloudPoint(new Point_PC(1.0f, 2.0f, 3.0f));
  cloud->addPointCloudPoint(new Point_PC(4.0f, 5.0f, 6.0f));

  // excluded by the cloud's own inclusion flag (segmentation/filtering),
  // stored but not part of the collection a fit should see
  Point_PC* excluded = new Point_PC(9.0f, 9.0f, 9.0f);
  excluded->isUsed = false;
  cloud->addPointCloudPoint(excluded);

  PointCollection* collection = cloud.data();
  OI_COMPARE(collection->collectionPointCount(), 2);

  const std::vector<Position> positions = collection->collectionPoints();
  OI_COMPARE(positions.size(), 2);
  OI_COMPARE(positions.front().getVector().getAt(0), 1.0);
}

void PointCollectionTest::
    bothCollectionsAreInterchangeableThroughTheInterface() {
  // the Stage 7a claim in one assertion: a consumer holding a
  // PointCollection* cannot tell, and does not need to know, whether the
  // points came from a scanner or from individually aimed shots
  PointGroup group("wall");
  group.addMember(makePoint(0.0, 0.0, 0.0, 1));
  group.addMember(makePoint(1.0, 0.0, 0.0, 2));

  oi::Guarded<PointCloud> cloud = new PointCloud();
  cloud->addPointCloudPoint(new Point_PC(0.0f, 0.0f, 0.0f));
  cloud->addPointCloudPoint(new Point_PC(1.0f, 0.0f, 0.0f));

  std::vector<PointCollection*> collections;
  collections.push_back(&group);
  collections.push_back(cloud.data());

  for (PointCollection* collection : collections) {
    OI_COMPARE(collection->collectionPointCount(), 2);
    OI_COMPARE(collection->collectionPoints().size(), 2);
  }
}

TEST_CASE("PointCollectionTest::groupHoldsReferencesNotCopies",
          "[PointCollectionTest]") {
  PointCollectionTest fixture;
  fixture.initTestCase();
  fixture.groupHoldsReferencesNotCopies();
}

TEST_CASE("PointCollectionTest::pointInManyGroupsIsStoredOnce",
          "[PointCollectionTest]") {
  PointCollectionTest fixture;
  fixture.initTestCase();
  fixture.pointInManyGroupsIsStoredOnce();
}

TEST_CASE("PointCollectionTest::constraintRejectsWrongType",
          "[PointCollectionTest]") {
  PointCollectionTest fixture;
  fixture.initTestCase();
  fixture.constraintRejectsWrongType();
}

TEST_CASE("PointCollectionTest::duplicateAndSelfMembershipRejected",
          "[PointCollectionTest]") {
  PointCollectionTest fixture;
  fixture.initTestCase();
  fixture.duplicateAndSelfMembershipRejected();
}

TEST_CASE("PointCollectionTest::compactDropsDeletedMembers",
          "[PointCollectionTest]") {
  PointCollectionTest fixture;
  fixture.initTestCase();
  fixture.compactDropsDeletedMembers();
}

TEST_CASE("PointCollectionTest::deletingAGroupDoesNotDeleteItsMembers",
          "[PointCollectionTest]") {
  PointCollectionTest fixture;
  fixture.initTestCase();
  fixture.deletingAGroupDoesNotDeleteItsMembers();
}

TEST_CASE("PointCollectionTest::pointGroupSatisfiesPointCollection",
          "[PointCollectionTest]") {
  PointCollectionTest fixture;
  fixture.initTestCase();
  fixture.pointGroupSatisfiesPointCollection();
}

TEST_CASE("PointCollectionTest::unsolvedPointsAreNotPartOfTheCollection",
          "[PointCollectionTest]") {
  PointCollectionTest fixture;
  fixture.initTestCase();
  fixture.unsolvedPointsAreNotPartOfTheCollection();
}

TEST_CASE("PointCollectionTest::pointCloudSatisfiesPointCollection",
          "[PointCollectionTest]") {
  PointCollectionTest fixture;
  fixture.initTestCase();
  fixture.pointCloudSatisfiesPointCollection();
}

TEST_CASE(
    "PointCollectionTest::bothCollectionsAreInterchangeableThroughTheInterface",
    "[PointCollectionTest]") {
  PointCollectionTest fixture;
  fixture.initTestCase();
  fixture.bothCollectionsAreInterchangeableThroughTheInterface();
}
