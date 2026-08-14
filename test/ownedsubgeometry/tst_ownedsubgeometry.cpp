#include <QString>
#include <QtTest>

#include "chooselalib.h"
#include "oijob.h"
#include "featurewrapper.h"
#include "geometry/point.h"
#include "constructfunction.h"

using namespace oi;

namespace{

/*!
 * \brief A minimal, real producer function so a bare Point can become
 * "solved" without going through a real fit or acquisition - just enough
 * to stand in for an owner feature's own producer in these tests. x/y/z
 * are mutable so a test can simulate the owner moving between recalcs.
 */
class FixedPositionFunction : public ConstructFunction, public Producer<Point> {
    Q_OBJECT
public:
    FixedPositionFunction(QObject *parent = 0) : ConstructFunction(parent){
        this->metaData.name = "FixedPositionFunction";
        this->applicableFor.append(ePointFeature);
    }
    double x = 0.0, y = 0.0, z = 0.0;
    std::expected<Point, QString> exec() override {
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
    Q_OBJECT
public:
    CopyPointFunction(QObject *parent = 0) : ConstructFunction(parent){
        this->metaData.name = "CopyPointFunction";
        NeededElement param;
        param.typeOfElement = ePointElement;
        param.infinite = false;
        param.description = "the owner feature this accessor is bound to";
        this->neededElements.append(param);
        this->applicableFor.append(ePointFeature);
    }
    std::expected<Point, QString> exec() override {
        if(!this->inputElements.contains(0) || this->inputElements[0].size() != 1){
            return std::unexpected(QString("CopyPointFunction: no input element"));
        }
        const InputElement &element = this->inputElements[0].first();
        if(element.point.isNull() || !element.point->getIsSolved()){
            return std::unexpected(QString("CopyPointFunction: input point not solved"));
        }
        Point point;
        point.setPoint(element.point->getPosition());
        return point;
    }
};

}

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
class OwnedSubGeometryTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();

    void generatingAnOwnedAccessorAttachesFunctionBindsInputAndMarksOwnership();
    void ownerRecalcOnlyDirtiesOwnedChildWhenItsValueActuallyChanges();
    void aSecondFeatureCanReferenceTheOwnedChildAsInput();
    void removingTheOwnerCascadesToItsOwnedChild();

private:
    QPointer<OiJob> makeJob();
};

void OwnedSubGeometryTest::initTestCase(){
    ChooseLALib::setLinearAlgebra(ChooseLALib::Armadillo);
}

QPointer<OiJob> OwnedSubGeometryTest::makeJob(){
    return new OiJob();
}

void OwnedSubGeometryTest::generatingAnOwnedAccessorAttachesFunctionBindsInputAndMarksOwnership(){

    QPointer<OiJob> job = this->makeJob();

    QPointer<Point> owner = new Point();
    QPointer<FeatureWrapper> ownerWrapper = new FeatureWrapper();
    ownerWrapper->setPoint(owner);
    owner->setFeatureName("Owner1");
    QVERIFY(job->addFeature(ownerWrapper));

    QPointer<Point> child = new Point();
    QPointer<FeatureWrapper> childWrapper = new FeatureWrapper();
    childWrapper->setPoint(child);
    child->setFeatureName("Child1");
    QVERIFY(job->addFeature(childWrapper));

    QPointer<CopyPointFunction> function = new CopyPointFunction();
    QVERIFY(job->addOwnedSubGeometry(ownerWrapper, childWrapper, function, 0));

    //the function got attached, and the owner got bound as its input -
    //reusing Feature::addFunction/OiJob::addInputFeature, not a parallel
    //mechanism
    QCOMPARE(child->getFunctions().size(), 1);
    QCOMPARE(function->getInputElements().value(0).size(), 1);
    QCOMPARE(function->getInputElements().value(0).first().id, owner->getId());

    //ownership recorded on both sides
    QVERIFY(child->getOwnerFeature().has_value());
    QCOMPARE(child->getOwnerFeature().value().data(), owner.data());
    QVERIFY(owner->getOwnedSubGeometries().contains(child));

    const std::optional<QString> description = child->getOwnerDescription();
    QVERIFY(description.has_value());
    QVERIFY(description->contains("Owner1"));

    //the ordinary dependency graph is wired too (previouslyNeeded/usedFor) -
    //this is what lets FeatureUpdater's existing DAG walk find and
    //recalculate the child, with no second recalculation path. Checked by
    //id rather than QPointer<FeatureWrapper> identity: a Feature's own
    //selfFeature (auto-created in its constructor) is a different
    //FeatureWrapper object than whatever external wrapper a caller
    //registers it under (see OiJob::createFeatureWrapper, which does the
    //same "new FeatureWrapper(); wrapper->setPoint(new Point())" thing
    //this test does) - the symmetric addUsedFor/addPreviouslyNeeded calls
    //use selfFeature, so list membership must be compared by the
    //underlying feature's id, not object identity, same as usedForMap/
    //previouslyNeededMap already do internally.
    QVERIFY(child->getPreviouslyNeeded().contains(ownerWrapper));
    bool ownerUsedForContainsChild = false;
    foreach(const QPointer<FeatureWrapper> &usedFor, owner->getUsedFor()){
        if(!usedFor.isNull() && !usedFor->getFeature().isNull() && usedFor->getFeature()->getId() == child->getId()){
            ownerUsedForContainsChild = true;
        }
    }
    QVERIFY(ownerUsedForContainsChild);

}

void OwnedSubGeometryTest::ownerRecalcOnlyDirtiesOwnedChildWhenItsValueActuallyChanges(){

    QPointer<OiJob> job = this->makeJob();

    QPointer<Point> owner = new Point();
    QPointer<FeatureWrapper> ownerWrapper = new FeatureWrapper();
    ownerWrapper->setPoint(owner);
    owner->setFeatureName("Owner2");
    QVERIFY(job->addFeature(ownerWrapper));

    QPointer<FixedPositionFunction> ownerFn = new FixedPositionFunction();
    ownerFn->x = 1.0; ownerFn->y = 2.0; ownerFn->z = 3.0;
    owner->addFunction(ownerFn);

    QPointer<Point> child = new Point();
    QPointer<FeatureWrapper> childWrapper = new FeatureWrapper();
    childWrapper->setPoint(child);
    child->setFeatureName("Child2");
    QVERIFY(job->addFeature(childWrapper));

    QVERIFY(job->addOwnedSubGeometry(ownerWrapper, childWrapper, new CopyPointFunction(), 0));

    //first pass: owner solves for the first time, child computes from the
    //owner's now-current position. There is no FeatureUpdater in core to
    //drive the DAG walk, so this simulates it by calling recalc() in
    //dependency order - exactly what
    //FeatureUpdater::recursiveFeatureRecalculation does in the real app.
    owner->recalc();
    child->recalc();
    child->setIsUpdated(true);

    QCOMPARE(child->getPosition().getVector().getAt(0), 1.0);
    QCOMPARE(child->getPosition().getVector().getAt(2), 3.0);

    //owner recalculates again to an identical value - Geometry::recalc()
    //must NOT dirty the child. This is what keeps a project reload (an
    //ordinary recalc pass over an unchanged graph) from re-executing an
    //owned accessor's construction chain (domain-model.md S1/S6).
    owner->recalc();
    QVERIFY2(child->getIsUpdated(), "recalculating the owner to the same value must not dirty an owned child");

    //now the owner actually moves - the child must be dirtied so the next
    //DAG pass regenerates it, which is what keeps it from drifting
    ownerFn->x = 9.0;
    owner->recalc();
    QVERIFY2(!child->getIsUpdated(), "recalculating the owner to a new value must dirty its owned child");

    child->recalc();
    QCOMPARE(child->getPosition().getVector().getAt(0), 9.0);

}

void OwnedSubGeometryTest::aSecondFeatureCanReferenceTheOwnedChildAsInput(){

    QPointer<OiJob> job = this->makeJob();

    QPointer<Point> owner = new Point();
    QPointer<FeatureWrapper> ownerWrapper = new FeatureWrapper();
    ownerWrapper->setPoint(owner);
    owner->setFeatureName("Owner3");
    QVERIFY(job->addFeature(ownerWrapper));

    QPointer<Point> child = new Point();
    QPointer<FeatureWrapper> childWrapper = new FeatureWrapper();
    childWrapper->setPoint(child);
    child->setFeatureName("Child3");
    QVERIFY(job->addFeature(childWrapper));

    QVERIFY(job->addOwnedSubGeometry(ownerWrapper, childWrapper, new CopyPointFunction(), 0));

    //a third, ordinary feature takes the owned child as an input - proves
    //an owned accessor is just an ordinary feature to the input
    //machinery, no special-casing needed (domain-model.md S6: "no new
    //addressing syntax")
    QPointer<Point> consumer = new Point();
    QPointer<FeatureWrapper> consumerWrapper = new FeatureWrapper();
    consumerWrapper->setPoint(consumer);
    consumer->setFeatureName("Consumer3");
    QVERIFY(job->addFeature(consumerWrapper));

    QPointer<CopyPointFunction> consumerFn = new CopyPointFunction();
    consumer->addFunction(consumerFn);
    job->addInputFeature(consumerWrapper, 0, 0, childWrapper);

    QCOMPARE(consumerFn->getInputElements().value(0).size(), 1);
    QCOMPARE(consumerFn->getInputElements().value(0).first().id, child->getId());
    QVERIFY(consumer->getPreviouslyNeeded().contains(childWrapper));

}

void OwnedSubGeometryTest::removingTheOwnerCascadesToItsOwnedChild(){

    QPointer<OiJob> job = this->makeJob();

    QPointer<Point> owner = new Point();
    QPointer<FeatureWrapper> ownerWrapper = new FeatureWrapper();
    ownerWrapper->setPoint(owner);
    owner->setFeatureName("Owner4");
    QVERIFY(job->addFeature(ownerWrapper));

    QPointer<Point> child = new Point();
    QPointer<FeatureWrapper> childWrapper = new FeatureWrapper();
    childWrapper->setPoint(child);
    child->setFeatureName("Child4");
    QVERIFY(job->addFeature(childWrapper));

    QVERIFY(job->addOwnedSubGeometry(ownerWrapper, childWrapper, new CopyPointFunction(), 0));

    const int childId = child->getId();
    QVERIFY(!job->getFeatureById(childId).isNull());

    QVERIFY(job->removeFeature(ownerWrapper->getFeature()->getId()));

    //the owner is gone, and so is what it owned - D19 "deleting the owner
    //deletes its owned sub-geometry" - not left dangling for some other
    //feature to reference a now-orphaned id
    QVERIFY(job->getFeatureById(childId).isNull());

}

QTEST_MAIN(OwnedSubGeometryTest)
#include "tst_ownedsubgeometry.moc"
