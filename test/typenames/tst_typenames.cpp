#include <QtTest>
#include <map>

#include "containers.h"
#include "util/types.h"
#include "util/util.h"

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
class TypeNamesTest : public QObject {
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

void TypeNamesTest::persistedElementTokensAreUnchanged() {
  const std::map<ElementTypes, QString> golden = {
      {ElementTypes::eCircleElement, QStringLiteral("circle")},
      {ElementTypes::eConeElement, QStringLiteral("cone")},
      {ElementTypes::eCylinderElement, QStringLiteral("cylinder")},
      {ElementTypes::eEllipseElement, QStringLiteral("ellipse")},
      {ElementTypes::eEllipsoidElement, QStringLiteral("ellipsoid")},
      {ElementTypes::eHyperboloidElement, QStringLiteral("hyperboloid")},
      {ElementTypes::eLineElement, QStringLiteral("line")},
      {ElementTypes::eNurbsElement, QStringLiteral("nurbs")},
      {ElementTypes::eParaboloidElement, QStringLiteral("paraboloid")},
      {ElementTypes::ePlaneElement, QStringLiteral("plane")},
      {ElementTypes::ePointElement, QStringLiteral("point")},
      {ElementTypes::ePointCloudElement, QStringLiteral("point cloud")},
      {ElementTypes::eScalarEntityAngleElement, QStringLiteral("angle")},
      {ElementTypes::eScalarEntityDistanceElement, QStringLiteral("distance")},
      {ElementTypes::eScalarEntityMeasurementSeriesElement,
       QStringLiteral("measurement series")},
      {ElementTypes::eScalarEntityTemperatureElement,
       QStringLiteral("temperature")},
      {ElementTypes::eSlottedHoleElement, QStringLiteral("slotted hole")},
      {ElementTypes::eSphereElement, QStringLiteral("sphere")},
      {ElementTypes::eTorusElement, QStringLiteral("torus")},
      {ElementTypes::eDirectionElement, QStringLiteral("direction")},
      {ElementTypes::ePositionElement, QStringLiteral("position")},
      {ElementTypes::eRadiusElement, QStringLiteral("radius")},
      {ElementTypes::eCoordinateSystemElement,
       QStringLiteral("coordinate system")},
      {ElementTypes::eStationElement, QStringLiteral("station")},
      {ElementTypes::eTrafoParamElement,
       QStringLiteral("transformation parameters")},
      {ElementTypes::eObservationElement, QStringLiteral("observation")},
      {ElementTypes::eReadingCartesianElement,
       QStringLiteral("cartesian reading")},
      {ElementTypes::eReadingPolarElement, QStringLiteral("polar reading")},
      {ElementTypes::eReadingDistanceElement,
       QStringLiteral("distance reading")},
      {ElementTypes::eReadingDirectionElement,
       QStringLiteral("direction reading")},
      {ElementTypes::eReadingTemperatureElement,
       QStringLiteral("temperature reading")},
      {ElementTypes::eReadingLevelElement, QStringLiteral("level reading")},
      {ElementTypes::eLevelElement, QStringLiteral("level")},
      {ElementTypes::ePointCollectionElement,
       QStringLiteral("point collection")},
  };
  for (auto it = golden.cbegin(); it != golden.cend(); ++it) {
    QCOMPARE(getElementTypeName(it->first), it->second);
  }
}

void TypeNamesTest::persistedElementPluralTokensAreUnchanged() {
  // the plural map participates in getElementTypeEnum's reverse lookup, so
  // these are persisted identifiers too, not just prettier labels
  const std::map<ElementTypes, QString> golden = {
      {ElementTypes::eCircleElement, QStringLiteral("circles")},
      {ElementTypes::eConeElement, QStringLiteral("cones")},
      {ElementTypes::eCylinderElement, QStringLiteral("cylinders")},
      {ElementTypes::eEllipseElement, QStringLiteral("ellipses")},
      {ElementTypes::eEllipsoidElement, QStringLiteral("ellipsoids")},
      {ElementTypes::eHyperboloidElement, QStringLiteral("hyperboloids")},
      {ElementTypes::eLineElement, QStringLiteral("lines")},
      {ElementTypes::eNurbsElement, QStringLiteral("nurbs")},
      {ElementTypes::eParaboloidElement, QStringLiteral("paraboloids")},
      {ElementTypes::ePlaneElement, QStringLiteral("planes")},
      {ElementTypes::ePointElement, QStringLiteral("points")},
      {ElementTypes::ePointCloudElement, QStringLiteral("point clouds")},
      {ElementTypes::eScalarEntityAngleElement, QStringLiteral("angles")},
      {ElementTypes::eScalarEntityDistanceElement, QStringLiteral("distances")},
      {ElementTypes::eScalarEntityMeasurementSeriesElement,
       QStringLiteral("measurement series")},
      {ElementTypes::eScalarEntityTemperatureElement,
       QStringLiteral("temperatures")},
      {ElementTypes::eSlottedHoleElement, QStringLiteral("slotted holes")},
      {ElementTypes::eSphereElement, QStringLiteral("spheres")},
      {ElementTypes::eTorusElement, QStringLiteral("tori")},
      {ElementTypes::eDirectionElement, QStringLiteral("directions")},
      {ElementTypes::ePositionElement, QStringLiteral("positions")},
      {ElementTypes::eRadiusElement, QStringLiteral("radii")},
      {ElementTypes::eCoordinateSystemElement,
       QStringLiteral("coordinate systems")},
      {ElementTypes::eStationElement, QStringLiteral("stations")},
      {ElementTypes::eTrafoParamElement,
       QStringLiteral("transformation parameters")},
      {ElementTypes::eObservationElement, QStringLiteral("observations")},
      {ElementTypes::eReadingCartesianElement,
       QStringLiteral("cartesian readings")},
      {ElementTypes::eReadingPolarElement, QStringLiteral("polar readings")},
      {ElementTypes::eReadingDistanceElement,
       QStringLiteral("distance readings")},
      {ElementTypes::eReadingDirectionElement,
       QStringLiteral("direction readings")},
      {ElementTypes::eReadingTemperatureElement,
       QStringLiteral("temperature readings")},
      {ElementTypes::eReadingLevelElement, QStringLiteral("level readings")},
      {ElementTypes::ePointCollectionElement,
       QStringLiteral("point collections")},
  };
  for (auto it = golden.cbegin(); it != golden.cend(); ++it) {
    QCOMPARE(getElementTypePluralName(it->first), it->second);
  }
}

void TypeNamesTest::persistedFeatureTokensAreUnchanged() {
  const std::map<FeatureTypes, QString> golden = {
      {FeatureTypes::eCircleFeature, QStringLiteral("circle")},
      {FeatureTypes::eConeFeature, QStringLiteral("cone")},
      {FeatureTypes::eCylinderFeature, QStringLiteral("cylinder")},
      {FeatureTypes::eEllipseFeature, QStringLiteral("ellipse")},
      {FeatureTypes::eEllipsoidFeature, QStringLiteral("ellipsoid")},
      {FeatureTypes::eHyperboloidFeature, QStringLiteral("hyperboloid")},
      {FeatureTypes::eLineFeature, QStringLiteral("line")},
      {FeatureTypes::eNurbsFeature, QStringLiteral("nurbs")},
      {FeatureTypes::eParaboloidFeature, QStringLiteral("paraboloid")},
      {FeatureTypes::ePlaneFeature, QStringLiteral("plane")},
      {FeatureTypes::ePointFeature, QStringLiteral("point")},
      {FeatureTypes::ePointCloudFeature, QStringLiteral("point cloud")},
      {FeatureTypes::eScalarEntityAngleFeature, QStringLiteral("angle")},
      {FeatureTypes::eScalarEntityDistanceFeature, QStringLiteral("distance")},
      {FeatureTypes::eScalarEntityMeasurementSeriesFeature,
       QStringLiteral("measurement series")},
      {FeatureTypes::eScalarEntityTemperatureFeature,
       QStringLiteral("temperature")},
      {FeatureTypes::eSlottedHoleFeature, QStringLiteral("slotted hole")},
      {FeatureTypes::eSphereFeature, QStringLiteral("sphere")},
      {FeatureTypes::eTorusFeature, QStringLiteral("torus")},
      {FeatureTypes::eCoordinateSystemFeature,
       QStringLiteral("coordinate system")},
      {FeatureTypes::eStationFeature, QStringLiteral("station")},
      {FeatureTypes::eTrafoParamFeature,
       QStringLiteral("transformation parameters")},
      {FeatureTypes::eLevelFeature, QStringLiteral("level")},
      {FeatureTypes::eGroupFeature, QStringLiteral("group")},
      {FeatureTypes::ePointGroupFeature, QStringLiteral("point group")},
  };
  for (auto it = golden.cbegin(); it != golden.cend(); ++it) {
    QCOMPARE(getFeatureTypeName(it->first), it->second);
  }
}

void TypeNamesTest::persistedGeometryTokensAreUnchanged() {
  const std::map<GeometryTypes, QString> golden = {
      {GeometryTypes::eCircleGeometry, QStringLiteral("circle")},
      {GeometryTypes::eConeGeometry, QStringLiteral("cone")},
      {GeometryTypes::eCylinderGeometry, QStringLiteral("cylinder")},
      {GeometryTypes::eEllipseGeometry, QStringLiteral("ellipse")},
      {GeometryTypes::eEllipsoidGeometry, QStringLiteral("ellipsoid")},
      {GeometryTypes::eHyperboloidGeometry, QStringLiteral("hyperboloid")},
      {GeometryTypes::eLineGeometry, QStringLiteral("line")},
      {GeometryTypes::eNurbsGeometry, QStringLiteral("nurbs")},
      {GeometryTypes::eParaboloidGeometry, QStringLiteral("paraboloid")},
      {GeometryTypes::ePlaneGeometry, QStringLiteral("plane")},
      {GeometryTypes::ePointGeometry, QStringLiteral("point")},
      {GeometryTypes::ePointCloudGeometry, QStringLiteral("point cloud")},
      {GeometryTypes::eScalarEntityAngleGeometry, QStringLiteral("angle")},
      {GeometryTypes::eScalarEntityDistanceGeometry,
       QStringLiteral("distance")},
      {GeometryTypes::eScalarEntityMeasurementSeriesGeometry,
       QStringLiteral("measurement series")},
      {GeometryTypes::eScalarEntityTemperatureGeometry,
       QStringLiteral("temperature")},
      {GeometryTypes::eSlottedHoleGeometry, QStringLiteral("slotted hole")},
      {GeometryTypes::eSphereGeometry, QStringLiteral("sphere")},
      {GeometryTypes::eTorusGeometry, QStringLiteral("torus")},
      {GeometryTypes::ePlaneLevelGeometry, QStringLiteral("level")},
  };
  for (auto it = golden.cbegin(); it != golden.cend(); ++it) {
    QCOMPARE(getGeometryTypeName(it->first), it->second);
  }
}

/*!
 * The property that actually matters on load: whatever we wrote, we read back
 * to the same enum.
 */
void TypeNamesTest::everyTokenRoundTripsBackToItsEnum() {
  for (const ElementTypes& type : getAvailableElementTypes()) {
    const QString token = getElementTypeName(type);
    if (token.isEmpty()) {
      continue;
    }
    QCOMPARE(getElementTypeEnum(token), type);
  }

  for (const FeatureTypes& type : getAvailableFeatureTypes()) {
    const QString token = getFeatureTypeName(type);
    if (token.isEmpty()) {
      continue;
    }
    QCOMPARE(getFeatureTypeEnum(token), type);
  }

  for (const GeometryTypes& type : getAvailableGeometryTypes()) {
    const QString token = getGeometryTypeName(type);
    if (token.isEmpty()) {
      continue;
    }
    QCOMPARE(getGeometryTypeEnum(token), type);
  }
}

/*!
 * A type with a token but no display name renders as an empty string in the
 * UI - the failure mode of adding an enum value and updating only one table.
 */
void TypeNamesTest::displayNamesExistForEveryTypeThatHasAToken() {
  for (const ElementTypes& type : getAvailableElementTypes()) {
    if (getElementTypeName(type).isEmpty()) {
      continue;
    }
    QVERIFY2(!getElementTypeDisplayName(type).isEmpty(),
             qPrintable(QString("element type %1 has no display name")
                            .arg(getElementTypeName(type))));
  }

  for (const FeatureTypes& type : getAvailableFeatureTypes()) {
    if (getFeatureTypeName(type).isEmpty()) {
      continue;
    }
    QVERIFY2(!getFeatureTypeDisplayName(type).isEmpty(),
             qPrintable(QString("feature type %1 has no display name")
                            .arg(getFeatureTypeName(type))));
  }

  for (const GeometryTypes& type : getAvailableGeometryTypes()) {
    if (getGeometryTypeName(type).isEmpty()) {
      continue;
    }
    QVERIFY2(!getGeometryTypeDisplayName(type).isEmpty(),
             qPrintable(QString("geometry type %1 has no display name")
                            .arg(getGeometryTypeName(type))));
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
void TypeNamesTest::displayNamesAreNotAcceptedByTheReverseLookup() {
  for (const FeatureTypes& type : getAvailableFeatureTypes()) {
    const QString display = getFeatureTypeDisplayName(type);
    if (display.isEmpty() || display == getFeatureTypeName(type)) {
      continue;
    }
    QCOMPARE(getFeatureTypeEnum(display), FeatureTypes::eUndefinedFeature);
  }

  for (const GeometryTypes& type : getAvailableGeometryTypes()) {
    const QString display = getGeometryTypeDisplayName(type);
    if (display.isEmpty() || display == getGeometryTypeName(type)) {
      continue;
    }
    QCOMPARE(getGeometryTypeEnum(display), GeometryTypes::eUndefinedGeometry);
  }
}

QTEST_APPLESS_MAIN(TypeNamesTest)
#include "tst_typenames.moc"
