#include <QString>
#include <QtTest>

#include "chooselalib.h"
#include "point.h"
#include "fitfunction.h"
#include "constructfunction.h"

using namespace oi;

// Minimal stand-ins for a real fit/construct plugin function. Neither
// overrides anything beyond what Function already default-implements -
// FitFunction/ConstructFunction only need to be distinguishable *types*
// for Geometry::getOwnerDescription's dynamic_cast, not do real work.
class FakeFit : public FitFunction {
    Q_OBJECT
public:
    FakeFit(QObject *parent = 0) : FitFunction(parent){
        this->metaData.name = "FakeFit";
    }
};

class FakeConstruct : public ConstructFunction {
    Q_OBJECT
public:
    FakeConstruct(QObject *parent = 0) : ConstructFunction(parent){
        this->metaData.name = "FakeConstruct";
    }
};

/*!
 * \brief Stage 7g regression coverage for D17 (nominal/actual as two
 * ordinary geometries, distinguished structurally by producer) and D19
 * (ownership determines editability, enforced at the API - not just
 * hidden in the UI).
 */
class OwnershipTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();

    void freshGeometryIsUserOwnedAndEditable();
    void constructFunctionProducerLeavesGeometryEditable();
    void fitFunctionProducerMakesGeometryReadOnly();
    void readOnlyGeometryRejectsDirectParameterEdits();
    void actualCanRealiseAnEditableNominal();
    void manyActualsMayRealiseTheSameNominal();
    void actualCannotBeRealisedByAnotherActual();
};

void OwnershipTest::initTestCase(){
    ChooseLALib::setLinearAlgebra(ChooseLALib::Armadillo);
}

void OwnershipTest::freshGeometryIsUserOwnedAndEditable(){
    Point point;
    QVERIFY(!point.getOwnerDescription().has_value());
}

void OwnershipTest::constructFunctionProducerLeavesGeometryEditable(){
    Point point;
    QPointer<FakeConstruct> construct = new FakeConstruct();
    point.addFunction(construct);
    QVERIFY(!point.getOwnerDescription().has_value());
}

void OwnershipTest::fitFunctionProducerMakesGeometryReadOnly(){
    Point point;
    QPointer<FakeFit> fit = new FakeFit();
    point.addFunction(fit);
    const std::optional<QString> owner = point.getOwnerDescription();
    QVERIFY(owner.has_value());
    QVERIFY(owner->contains("FakeFit"));
}

void OwnershipTest::readOnlyGeometryRejectsDirectParameterEdits(){

    Point point;
    point.setPoint(Position(1.0, 2.0, 3.0));

    QPointer<FakeFit> fit = new FakeFit();
    point.addFunction(fit);
    QVERIFY(point.getOwnerDescription().has_value());

    QMap<GeometryParameters, double> edit;
    edit.insert(eUnknownX, 99.0);
    point.setUnknownParameters(edit);

    //rejected at the API - the fit owns this point, so the edit must not
    //have taken effect
    QCOMPARE(point.getPosition().getVector().getAt(0), 1.0);
}

void OwnershipTest::actualCanRealiseAnEditableNominal(){

    Point nominal;
    QVERIFY(!nominal.getOwnerDescription().has_value());

    Point actual;
    actual.addFunction(new FakeFit());
    QVERIFY(actual.getOwnerDescription().has_value());

    QVERIFY(actual.setRealises(nominal.getFeatureWrapper()->getGeometry()));
    QVERIFY(actual.getRealises().has_value());
    QCOMPARE(actual.getRealises().value().data(), static_cast<Geometry*>(&nominal));

    //the nominal itself carries no back-reference - D17 deliberately
    //does not maintain a reverse list
    QVERIFY(!nominal.getRealises().has_value());
}

void OwnershipTest::manyActualsMayRealiseTheSameNominal(){

    //the case that decided the design: re-inspecting the same nominal
    //over time produces many actuals, not one
    Point nominal;

    Point actualA;
    actualA.addFunction(new FakeFit());
    Point actualB;
    actualB.addFunction(new FakeFit());

    QVERIFY(actualA.setRealises(nominal.getFeatureWrapper()->getGeometry()));
    QVERIFY(actualB.setRealises(nominal.getFeatureWrapper()->getGeometry()));

    QCOMPARE(actualA.getRealises().value().data(), static_cast<Geometry*>(&nominal));
    QCOMPARE(actualB.getRealises().value().data(), static_cast<Geometry*>(&nominal));
}

void OwnershipTest::actualCannotBeRealisedByAnotherActual(){

    Point actualA;
    actualA.addFunction(new FakeFit());

    Point actualB;
    actualB.addFunction(new FakeFit());

    //realising another actual makes no sense - only a user-owned
    //(editable) geometry can be the nominal side of the relationship
    QVERIFY(!actualB.setRealises(actualA.getFeatureWrapper()->getGeometry()));
    QVERIFY(!actualB.getRealises().has_value());
}

QTEST_MAIN(OwnershipTest)
#include "tst_ownership.moc"
