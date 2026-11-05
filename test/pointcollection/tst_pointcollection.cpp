#include <QtTest>

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
QPointer<FeatureWrapper> makePoint(double x, double y, double z, int id) {
  QPointer<Point> point = new Point(id, Position(x, y, z));
  point->setIsSolved(true);
  QPointer<FeatureWrapper> wrapper = new FeatureWrapper();
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
class PointCollectionTest : public QObject {
  Q_OBJECT

 private slots:
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
  ChooseLALib::setLinearAlgebra(ChooseLALib::Armadillo);
}

void PointCollectionTest::groupHoldsReferencesNotCopies() {
  QPointer<FeatureWrapper> point = makePoint(1.0, 2.0, 3.0, 1);

  PointGroup group("wall");
  QVERIFY(group.addMember(point));
  QCOMPARE(group.memberCount(), 1);

  // mutating the point must be visible through the group - a copy would
  // silently diverge, which is exactly the failure mode this design exists
  // to prevent
  Position moved(9.0, 9.0, 9.0);
  point->getPoint()->setPoint(moved);

  const QList<Position> positions = group.collectionPoints();
  QCOMPARE(positions.size(), 1);
  QCOMPARE(positions.first().getVector().getAt(0), 9.0);
}

void PointCollectionTest::pointInManyGroupsIsStoredOnce() {
  QPointer<FeatureWrapper> shared = makePoint(1.0, 1.0, 1.0, 1);

  PointGroup wall("wall");
  PointGroup control("control network");

  QVERIFY(wall.addMember(shared));
  QVERIFY(control.addMember(shared));

  // same underlying feature, reached from both groups - no copy, no second
  // measurement, no re-identification
  QCOMPARE(wall.getPoints().first().data(), control.getPoints().first().data());
  QCOMPARE(wall.getPoints().first()->getId(),
           control.getPoints().first()->getId());
}

void PointCollectionTest::constraintRejectsWrongType() {
  QPointer<PointCloud> cloud = new PointCloud();
  QPointer<FeatureWrapper> cloudWrapper = new FeatureWrapper();
  cloudWrapper->setPointCloud(cloud);

  PointGroup group("points only");
  QVERIFY2(!group.addMember(cloudWrapper),
           "a point group must not accept a point cloud as a member");
  QCOMPARE(group.memberCount(), 0);

  // an unconstrained Group takes anything - that generality is why there
  // is one Group class rather than one per feature type
  Group anything("mixed");
  QVERIFY(anything.addMember(cloudWrapper));
  QVERIFY(anything.addMember(makePoint(0.0, 0.0, 0.0, 51)));
  QCOMPARE(anything.memberCount(), 2);
}

void PointCollectionTest::duplicateAndSelfMembershipRejected() {
  QPointer<FeatureWrapper> point = makePoint(1.0, 1.0, 1.0, 1);

  PointGroup group("wall");
  QVERIFY(group.addMember(point));
  QVERIFY2(!group.addMember(point), "the same feature must not be added twice");
  QCOMPARE(group.memberCount(), 1);

  // a group containing itself would recurse forever on traversal
  Group outer("outer");
  QPointer<FeatureWrapper> selfWrapper = new FeatureWrapper();
  QVERIFY2(!outer.addMember(selfWrapper),
           "a null-typed wrapper must be rejected outright");
}

void PointCollectionTest::compactDropsDeletedMembers() {
  QPointer<FeatureWrapper> keep = makePoint(1.0, 1.0, 1.0, 1);
  QPointer<FeatureWrapper> discard = makePoint(2.0, 2.0, 2.0, 2);

  PointGroup group("wall");
  QVERIFY(group.addMember(keep));
  QVERIFY(group.addMember(discard));
  QCOMPARE(group.memberCount(), 2);

  delete discard->getPoint().data();
  delete discard.data();

  // memberCount already ignores the dead reference; compact() removes it
  QCOMPARE(group.memberCount(), 1);
  group.compact();
  QCOMPARE(group.getMembers().size(), 1);
}

void PointCollectionTest::deletingAGroupDoesNotDeleteItsMembers() {
  QPointer<FeatureWrapper> point = makePoint(1.0, 2.0, 3.0, 1);

  {
    PointGroup group("temporary");
    QVERIFY(group.addMember(point));
  }

  // the group is gone; the point it referenced must be untouched. A group
  // organises, it never owns.
  QVERIFY2(!point.isNull(), "deleting a group must not delete its members");
  QVERIFY(!point->getPoint().isNull());
  QCOMPARE(point->getPoint()->getPosition().getVector().getAt(0), 1.0);
}

void PointCollectionTest::pointGroupSatisfiesPointCollection() {
  PointGroup group("wall");
  group.addMember(makePoint(0.0, 0.0, 0.0, 1));
  group.addMember(makePoint(1.0, 0.0, 0.0, 2));
  group.addMember(makePoint(0.0, 1.0, 0.0, 3));

  // reached only through the interface, exactly as a fit function will
  PointCollection* collection = &group;
  QCOMPARE(collection->collectionPointCount(), 3);
  QCOMPARE(collection->collectionPoints().size(), 3);
}

void PointCollectionTest::unsolvedPointsAreNotPartOfTheCollection() {
  PointGroup group("wall");
  group.addMember(makePoint(0.0, 0.0, 0.0, 1));

  QPointer<Point> unsolved = new Point(2, Position(5.0, 5.0, 5.0));
  unsolved->setIsSolved(false);
  QPointer<FeatureWrapper> unsolvedWrapper = new FeatureWrapper();
  unsolvedWrapper->setPoint(unsolved);
  QVERIFY(group.addMember(unsolvedWrapper));

  // it is a member, but it has no trustworthy position yet - feeding it to
  // a fit would quietly corrupt the result
  QCOMPARE(group.memberCount(), 2);
  QCOMPARE(group.collectionPointCount(), 1);
}

void PointCollectionTest::pointCloudSatisfiesPointCollection() {
  QPointer<PointCloud> cloud = new PointCloud();
  cloud->addPointCloudPoint(new Point_PC(1.0f, 2.0f, 3.0f));
  cloud->addPointCloudPoint(new Point_PC(4.0f, 5.0f, 6.0f));

  // excluded by the cloud's own inclusion flag (segmentation/filtering),
  // stored but not part of the collection a fit should see
  Point_PC* excluded = new Point_PC(9.0f, 9.0f, 9.0f);
  excluded->isUsed = false;
  cloud->addPointCloudPoint(excluded);

  PointCollection* collection = cloud.data();
  QCOMPARE(collection->collectionPointCount(), 2);

  const QList<Position> positions = collection->collectionPoints();
  QCOMPARE(positions.size(), 2);
  QCOMPARE(positions.first().getVector().getAt(0), 1.0);
}

void PointCollectionTest::
    bothCollectionsAreInterchangeableThroughTheInterface() {
  // the Stage 7a claim in one assertion: a consumer holding a
  // PointCollection* cannot tell, and does not need to know, whether the
  // points came from a scanner or from individually aimed shots
  PointGroup group("wall");
  group.addMember(makePoint(0.0, 0.0, 0.0, 1));
  group.addMember(makePoint(1.0, 0.0, 0.0, 2));

  QPointer<PointCloud> cloud = new PointCloud();
  cloud->addPointCloudPoint(new Point_PC(0.0f, 0.0f, 0.0f));
  cloud->addPointCloudPoint(new Point_PC(1.0f, 0.0f, 0.0f));

  QList<PointCollection*> collections;
  collections << &group << cloud.data();

  foreach (PointCollection* collection, collections) {
    QCOMPARE(collection->collectionPointCount(), 2);
    QCOMPARE(collection->collectionPoints().size(), 2);
  }
}

QTEST_APPLESS_MAIN(PointCollectionTest)
#include "tst_pointcollection.moc"
