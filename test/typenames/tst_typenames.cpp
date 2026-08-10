#include <QtTest>
#include <QMap>

#include "util/util.h"
#include "util/types.h"

using namespace oi;

/*!
 * Pins the PERSISTED TYPE TOKENS against a golden list.
 *
 * These strings are not decoration. They are written into saved projects as
 * XML "type" attributes:
 *
 *     plane.setAttribute("type", getGeometryTypeName(ePlaneGeometry));
 *
 * and read back with an exact, case-sensitive QMap::key() lookup. They are
 * also interpolated into settings keys ("defaultFunction.%1",
 * "defaultMeasurementConfig.%1").
 *
 * So changing one does not fail loudly. A project still opens; its geometries
 * just quietly become eUndefined, and saved defaults quietly stop resolving.
 * That is the worst shape a bug can take, and an easy one to cause while
 * tidying up capitalisation for the UI - which is exactly why display names
 * now live in a separate table.
 *
 * If this test fails you have changed a persisted identifier. Either revert
 * it, or accept that existing projects and settings will not load and update
 * the golden list deliberately.
 */
class TypeNamesTest : public QObject
{
    Q_OBJECT

private slots:
    void persistedElementTokensAreUnchanged();
    void persistedElementPluralTokensAreUnchanged();
    void persistedFeatureTokensAreUnchanged();
    void persistedGeometryTokensAreUnchanged();

    void everyTokenRoundTripsBackToItsEnum();
    void displayNamesExistForEveryTypeThatHasAToken();
    void displayNamesAreNotAcceptedByTheReverseLookup();
};

void TypeNamesTest::persistedElementTokensAreUnchanged(){
    const QMap<ElementTypes, QString> golden = {
        { eCircleElement, QStringLiteral("circle") },
        { eConeElement, QStringLiteral("cone") },
        { eCylinderElement, QStringLiteral("cylinder") },
        { eEllipseElement, QStringLiteral("ellipse") },
        { eEllipsoidElement, QStringLiteral("ellipsoid") },
        { eHyperboloidElement, QStringLiteral("hyperboloid") },
        { eLineElement, QStringLiteral("line") },
        { eNurbsElement, QStringLiteral("nurbs") },
        { eParaboloidElement, QStringLiteral("paraboloid") },
        { ePlaneElement, QStringLiteral("plane") },
        { ePointElement, QStringLiteral("point") },
        { ePointCloudElement, QStringLiteral("point cloud") },
        { eScalarEntityAngleElement, QStringLiteral("angle") },
        { eScalarEntityDistanceElement, QStringLiteral("distance") },
        { eScalarEntityMeasurementSeriesElement, QStringLiteral("measurement series") },
        { eScalarEntityTemperatureElement, QStringLiteral("temperature") },
        { eSlottedHoleElement, QStringLiteral("slotted hole") },
        { eSphereElement, QStringLiteral("sphere") },
        { eTorusElement, QStringLiteral("torus") },
        { eDirectionElement, QStringLiteral("direction") },
        { ePositionElement, QStringLiteral("position") },
        { eRadiusElement, QStringLiteral("radius") },
        { eCoordinateSystemElement, QStringLiteral("coordinate system") },
        { eStationElement, QStringLiteral("station") },
        { eTrafoParamElement, QStringLiteral("transformation parameters") },
        { eObservationElement, QStringLiteral("observation") },
        { eReadingCartesianElement, QStringLiteral("cartesian reading") },
        { eReadingPolarElement, QStringLiteral("polar reading") },
        { eReadingDistanceElement, QStringLiteral("distance reading") },
        { eReadingDirectionElement, QStringLiteral("direction reading") },
        { eReadingTemperatureElement, QStringLiteral("temperature reading") },
        { eReadingLevelElement, QStringLiteral("level reading") },
        { eLevelElement, QStringLiteral("level") },
        { ePointCollectionElement, QStringLiteral("point collection") },
    };
    for(auto it = golden.constBegin(); it != golden.constEnd(); ++it){
        QCOMPARE(getElementTypeName(it.key()), it.value());
    }
}

void TypeNamesTest::persistedElementPluralTokensAreUnchanged(){
    //the plural map participates in getElementTypeEnum's reverse lookup, so
    //these are persisted identifiers too, not just prettier labels
    const QMap<ElementTypes, QString> golden = {
        { eCircleElement, QStringLiteral("circles") },
        { eConeElement, QStringLiteral("cones") },
        { eCylinderElement, QStringLiteral("cylinders") },
        { eEllipseElement, QStringLiteral("ellipses") },
        { eEllipsoidElement, QStringLiteral("ellipsoids") },
        { eHyperboloidElement, QStringLiteral("hyperboloids") },
        { eLineElement, QStringLiteral("lines") },
        { eNurbsElement, QStringLiteral("nurbs") },
        { eParaboloidElement, QStringLiteral("paraboloids") },
        { ePlaneElement, QStringLiteral("planes") },
        { ePointElement, QStringLiteral("points") },
        { ePointCloudElement, QStringLiteral("point clouds") },
        { eScalarEntityAngleElement, QStringLiteral("angles") },
        { eScalarEntityDistanceElement, QStringLiteral("distances") },
        { eScalarEntityMeasurementSeriesElement, QStringLiteral("measurement series") },
        { eScalarEntityTemperatureElement, QStringLiteral("temperatures") },
        { eSlottedHoleElement, QStringLiteral("slotted holes") },
        { eSphereElement, QStringLiteral("spheres") },
        { eTorusElement, QStringLiteral("tori") },
        { eDirectionElement, QStringLiteral("directions") },
        { ePositionElement, QStringLiteral("positions") },
        { eRadiusElement, QStringLiteral("radii") },
        { eCoordinateSystemElement, QStringLiteral("coordinate systems") },
        { eStationElement, QStringLiteral("stations") },
        { eTrafoParamElement, QStringLiteral("transformation parameters") },
        { eObservationElement, QStringLiteral("observations") },
        { eReadingCartesianElement, QStringLiteral("cartesian readings") },
        { eReadingPolarElement, QStringLiteral("polar readings") },
        { eReadingDistanceElement, QStringLiteral("distance readings") },
        { eReadingDirectionElement, QStringLiteral("direction readings") },
        { eReadingTemperatureElement, QStringLiteral("temperature readings") },
        { eReadingLevelElement, QStringLiteral("level readings") },
        { ePointCollectionElement, QStringLiteral("point collections") },
    };
    for(auto it = golden.constBegin(); it != golden.constEnd(); ++it){
        QCOMPARE(getElementTypePluralName(it.key()), it.value());
    }
}

void TypeNamesTest::persistedFeatureTokensAreUnchanged(){
    const QMap<FeatureTypes, QString> golden = {
        { eCircleFeature, QStringLiteral("circle") },
        { eConeFeature, QStringLiteral("cone") },
        { eCylinderFeature, QStringLiteral("cylinder") },
        { eEllipseFeature, QStringLiteral("ellipse") },
        { eEllipsoidFeature, QStringLiteral("ellipsoid") },
        { eHyperboloidFeature, QStringLiteral("hyperboloid") },
        { eLineFeature, QStringLiteral("line") },
        { eNurbsFeature, QStringLiteral("nurbs") },
        { eParaboloidFeature, QStringLiteral("paraboloid") },
        { ePlaneFeature, QStringLiteral("plane") },
        { ePointFeature, QStringLiteral("point") },
        { ePointCloudFeature, QStringLiteral("point cloud") },
        { eScalarEntityAngleFeature, QStringLiteral("angle") },
        { eScalarEntityDistanceFeature, QStringLiteral("distance") },
        { eScalarEntityMeasurementSeriesFeature, QStringLiteral("measurement series") },
        { eScalarEntityTemperatureFeature, QStringLiteral("temperature") },
        { eSlottedHoleFeature, QStringLiteral("slotted hole") },
        { eSphereFeature, QStringLiteral("sphere") },
        { eTorusFeature, QStringLiteral("torus") },
        { eCoordinateSystemFeature, QStringLiteral("coordinate system") },
        { eStationFeature, QStringLiteral("station") },
        { eTrafoParamFeature, QStringLiteral("transformation parameters") },
        { eLevelFeature, QStringLiteral("level") },
        { eGroupFeature, QStringLiteral("group") },
        { ePointGroupFeature, QStringLiteral("point group") },
    };
    for(auto it = golden.constBegin(); it != golden.constEnd(); ++it){
        QCOMPARE(getFeatureTypeName(it.key()), it.value());
    }
}

void TypeNamesTest::persistedGeometryTokensAreUnchanged(){
    const QMap<GeometryTypes, QString> golden = {
        { eCircleGeometry, QStringLiteral("circle") },
        { eConeGeometry, QStringLiteral("cone") },
        { eCylinderGeometry, QStringLiteral("cylinder") },
        { eEllipseGeometry, QStringLiteral("ellipse") },
        { eEllipsoidGeometry, QStringLiteral("ellipsoid") },
        { eHyperboloidGeometry, QStringLiteral("hyperboloid") },
        { eLineGeometry, QStringLiteral("line") },
        { eNurbsGeometry, QStringLiteral("nurbs") },
        { eParaboloidGeometry, QStringLiteral("paraboloid") },
        { ePlaneGeometry, QStringLiteral("plane") },
        { ePointGeometry, QStringLiteral("point") },
        { ePointCloudGeometry, QStringLiteral("point cloud") },
        { eScalarEntityAngleGeometry, QStringLiteral("angle") },
        { eScalarEntityDistanceGeometry, QStringLiteral("distance") },
        { eScalarEntityMeasurementSeriesGeometry, QStringLiteral("measurement series") },
        { eScalarEntityTemperatureGeometry, QStringLiteral("temperature") },
        { eSlottedHoleGeometry, QStringLiteral("slotted hole") },
        { eSphereGeometry, QStringLiteral("sphere") },
        { eTorusGeometry, QStringLiteral("torus") },
        { ePlaneLevelGeometry, QStringLiteral("level") },
    };
    for(auto it = golden.constBegin(); it != golden.constEnd(); ++it){
        QCOMPARE(getGeometryTypeName(it.key()), it.value());
    }
}

/*!
 * The property that actually matters on load: whatever we wrote, we read back
 * to the same enum.
 */
void TypeNamesTest::everyTokenRoundTripsBackToItsEnum(){

    foreach(const ElementTypes &type, getAvailableElementTypes()){
        const QString token = getElementTypeName(type);
        if(token.isEmpty()){ continue; }
        QCOMPARE(getElementTypeEnum(token), type);
    }

    foreach(const FeatureTypes &type, getAvailableFeatureTypes()){
        const QString token = getFeatureTypeName(type);
        if(token.isEmpty()){ continue; }
        QCOMPARE(getFeatureTypeEnum(token), type);
    }

    foreach(const GeometryTypes &type, getAvailableGeometryTypes()){
        const QString token = getGeometryTypeName(type);
        if(token.isEmpty()){ continue; }
        QCOMPARE(getGeometryTypeEnum(token), type);
    }

}

/*!
 * A type with a token but no display name renders as an empty string in the
 * UI - the failure mode of adding an enum value and updating only one table.
 */
void TypeNamesTest::displayNamesExistForEveryTypeThatHasAToken(){

    foreach(const ElementTypes &type, getAvailableElementTypes()){
        if(getElementTypeName(type).isEmpty()){ continue; }
        QVERIFY2(!getElementTypeDisplayName(type).isEmpty(),
                 qPrintable(QString("element type %1 has no display name").arg(getElementTypeName(type))));
    }

    foreach(const FeatureTypes &type, getAvailableFeatureTypes()){
        if(getFeatureTypeName(type).isEmpty()){ continue; }
        QVERIFY2(!getFeatureTypeDisplayName(type).isEmpty(),
                 qPrintable(QString("feature type %1 has no display name").arg(getFeatureTypeName(type))));
    }

    foreach(const GeometryTypes &type, getAvailableGeometryTypes()){
        if(getGeometryTypeName(type).isEmpty()){ continue; }
        QVERIFY2(!getGeometryTypeDisplayName(type).isEmpty(),
                 qPrintable(QString("geometry type %1 has no display name").arg(getGeometryTypeName(type))));
    }

}

/*!
 * The separation only holds if display names stay out of the reverse lookup.
 * If a display name ever resolved to an enum the two tables would effectively
 * be one again, and re-casing a label could start changing behaviour.
 *
 * A display name identical to its token is not a violation - it is the same
 * string, and single lowercase words often survive title-casing unchanged.
 */
void TypeNamesTest::displayNamesAreNotAcceptedByTheReverseLookup(){

    foreach(const FeatureTypes &type, getAvailableFeatureTypes()){
        const QString display = getFeatureTypeDisplayName(type);
        if(display.isEmpty() || display == getFeatureTypeName(type)){ continue; }
        QCOMPARE(getFeatureTypeEnum(display), eUndefinedFeature);
    }

    foreach(const GeometryTypes &type, getAvailableGeometryTypes()){
        const QString display = getGeometryTypeDisplayName(type);
        if(display.isEmpty() || display == getGeometryTypeName(type)){ continue; }
        QCOMPARE(getGeometryTypeEnum(display), eUndefinedGeometry);
    }

}

QTEST_APPLESS_MAIN(TypeNamesTest)
#include "tst_typenames.moc"
