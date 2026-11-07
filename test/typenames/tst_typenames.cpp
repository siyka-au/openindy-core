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
  const std::map<ElementTypes, std::string> golden = {
      {ElementTypes::eCircleElement, std::string("circle")},
      {ElementTypes::eConeElement, std::string("cone")},
      {ElementTypes::eCylinderElement, std::string("cylinder")},
      {ElementTypes::eEllipseElement, std::string("ellipse")},
      {ElementTypes::eEllipsoidElement, std::string("ellipsoid")},
      {ElementTypes::eHyperboloidElement, std::string("hyperboloid")},
      {ElementTypes::eLineElement, std::string("line")},
      {ElementTypes::eNurbsElement, std::string("nurbs")},
      {ElementTypes::eParaboloidElement, std::string("paraboloid")},
      {ElementTypes::ePlaneElement, std::string("plane")},
      {ElementTypes::ePointElement, std::string("point")},
      {ElementTypes::ePointCloudElement, std::string("point cloud")},
      {ElementTypes::eScalarEntityAngleElement, std::string("angle")},
      {ElementTypes::eScalarEntityDistanceElement, std::string("distance")},
      {ElementTypes::eScalarEntityMeasurementSeriesElement,
       std::string("measurement series")},
      {ElementTypes::eScalarEntityTemperatureElement,
       std::string("temperature")},
      {ElementTypes::eSlottedHoleElement, std::string("slotted hole")},
      {ElementTypes::eSphereElement, std::string("sphere")},
      {ElementTypes::eTorusElement, std::string("torus")},
      {ElementTypes::eDirectionElement, std::string("direction")},
      {ElementTypes::ePositionElement, std::string("position")},
      {ElementTypes::eRadiusElement, std::string("radius")},
      {ElementTypes::eCoordinateSystemElement,
       std::string("coordinate system")},
      {ElementTypes::eStationElement, std::string("station")},
      {ElementTypes::eTrafoParamElement,
       std::string("transformation parameters")},
      {ElementTypes::eObservationElement, std::string("observation")},
      {ElementTypes::eReadingCartesianElement,
       std::string("cartesian reading")},
      {ElementTypes::eReadingPolarElement, std::string("polar reading")},
      {ElementTypes::eReadingDistanceElement, std::string("distance reading")},
      {ElementTypes::eReadingDirectionElement,
       std::string("direction reading")},
      {ElementTypes::eReadingTemperatureElement,
       std::string("temperature reading")},
      {ElementTypes::eReadingLevelElement, std::string("level reading")},
      {ElementTypes::eLevelElement, std::string("level")},
      {ElementTypes::ePointCollectionElement, std::string("point collection")},
  };
  for (auto it = golden.cbegin(); it != golden.cend(); ++it) {
    QCOMPARE(getElementTypeName(it->first), it->second);
  }
}

void TypeNamesTest::persistedElementPluralTokensAreUnchanged() {
  // the plural map participates in getElementTypeEnum's reverse lookup, so
  // these are persisted identifiers too, not just prettier labels
  const std::map<ElementTypes, std::string> golden = {
      {ElementTypes::eCircleElement, std::string("circles")},
      {ElementTypes::eConeElement, std::string("cones")},
      {ElementTypes::eCylinderElement, std::string("cylinders")},
      {ElementTypes::eEllipseElement, std::string("ellipses")},
      {ElementTypes::eEllipsoidElement, std::string("ellipsoids")},
      {ElementTypes::eHyperboloidElement, std::string("hyperboloids")},
      {ElementTypes::eLineElement, std::string("lines")},
      {ElementTypes::eNurbsElement, std::string("nurbs")},
      {ElementTypes::eParaboloidElement, std::string("paraboloids")},
      {ElementTypes::ePlaneElement, std::string("planes")},
      {ElementTypes::ePointElement, std::string("points")},
      {ElementTypes::ePointCloudElement, std::string("point clouds")},
      {ElementTypes::eScalarEntityAngleElement, std::string("angles")},
      {ElementTypes::eScalarEntityDistanceElement, std::string("distances")},
      {ElementTypes::eScalarEntityMeasurementSeriesElement,
       std::string("measurement series")},
      {ElementTypes::eScalarEntityTemperatureElement,
       std::string("temperatures")},
      {ElementTypes::eSlottedHoleElement, std::string("slotted holes")},
      {ElementTypes::eSphereElement, std::string("spheres")},
      {ElementTypes::eTorusElement, std::string("tori")},
      {ElementTypes::eDirectionElement, std::string("directions")},
      {ElementTypes::ePositionElement, std::string("positions")},
      {ElementTypes::eRadiusElement, std::string("radii")},
      {ElementTypes::eCoordinateSystemElement,
       std::string("coordinate systems")},
      {ElementTypes::eStationElement, std::string("stations")},
      {ElementTypes::eTrafoParamElement,
       std::string("transformation parameters")},
      {ElementTypes::eObservationElement, std::string("observations")},
      {ElementTypes::eReadingCartesianElement,
       std::string("cartesian readings")},
      {ElementTypes::eReadingPolarElement, std::string("polar readings")},
      {ElementTypes::eReadingDistanceElement, std::string("distance readings")},
      {ElementTypes::eReadingDirectionElement,
       std::string("direction readings")},
      {ElementTypes::eReadingTemperatureElement,
       std::string("temperature readings")},
      {ElementTypes::eReadingLevelElement, std::string("level readings")},
      {ElementTypes::ePointCollectionElement, std::string("point collections")},
  };
  for (auto it = golden.cbegin(); it != golden.cend(); ++it) {
    QCOMPARE(getElementTypePluralName(it->first), it->second);
  }
}

void TypeNamesTest::persistedFeatureTokensAreUnchanged() {
  const std::map<FeatureTypes, std::string> golden = {
      {FeatureTypes::eCircleFeature, std::string("circle")},
      {FeatureTypes::eConeFeature, std::string("cone")},
      {FeatureTypes::eCylinderFeature, std::string("cylinder")},
      {FeatureTypes::eEllipseFeature, std::string("ellipse")},
      {FeatureTypes::eEllipsoidFeature, std::string("ellipsoid")},
      {FeatureTypes::eHyperboloidFeature, std::string("hyperboloid")},
      {FeatureTypes::eLineFeature, std::string("line")},
      {FeatureTypes::eNurbsFeature, std::string("nurbs")},
      {FeatureTypes::eParaboloidFeature, std::string("paraboloid")},
      {FeatureTypes::ePlaneFeature, std::string("plane")},
      {FeatureTypes::ePointFeature, std::string("point")},
      {FeatureTypes::ePointCloudFeature, std::string("point cloud")},
      {FeatureTypes::eScalarEntityAngleFeature, std::string("angle")},
      {FeatureTypes::eScalarEntityDistanceFeature, std::string("distance")},
      {FeatureTypes::eScalarEntityMeasurementSeriesFeature,
       std::string("measurement series")},
      {FeatureTypes::eScalarEntityTemperatureFeature,
       std::string("temperature")},
      {FeatureTypes::eSlottedHoleFeature, std::string("slotted hole")},
      {FeatureTypes::eSphereFeature, std::string("sphere")},
      {FeatureTypes::eTorusFeature, std::string("torus")},
      {FeatureTypes::eCoordinateSystemFeature,
       std::string("coordinate system")},
      {FeatureTypes::eStationFeature, std::string("station")},
      {FeatureTypes::eTrafoParamFeature,
       std::string("transformation parameters")},
      {FeatureTypes::eLevelFeature, std::string("level")},
      {FeatureTypes::eGroupFeature, std::string("group")},
      {FeatureTypes::ePointGroupFeature, std::string("point group")},
  };
  for (auto it = golden.cbegin(); it != golden.cend(); ++it) {
    QCOMPARE(getFeatureTypeName(it->first), it->second);
  }
}

void TypeNamesTest::persistedGeometryTokensAreUnchanged() {
  const std::map<GeometryTypes, std::string> golden = {
      {GeometryTypes::eCircleGeometry, std::string("circle")},
      {GeometryTypes::eConeGeometry, std::string("cone")},
      {GeometryTypes::eCylinderGeometry, std::string("cylinder")},
      {GeometryTypes::eEllipseGeometry, std::string("ellipse")},
      {GeometryTypes::eEllipsoidGeometry, std::string("ellipsoid")},
      {GeometryTypes::eHyperboloidGeometry, std::string("hyperboloid")},
      {GeometryTypes::eLineGeometry, std::string("line")},
      {GeometryTypes::eNurbsGeometry, std::string("nurbs")},
      {GeometryTypes::eParaboloidGeometry, std::string("paraboloid")},
      {GeometryTypes::ePlaneGeometry, std::string("plane")},
      {GeometryTypes::ePointGeometry, std::string("point")},
      {GeometryTypes::ePointCloudGeometry, std::string("point cloud")},
      {GeometryTypes::eScalarEntityAngleGeometry, std::string("angle")},
      {GeometryTypes::eScalarEntityDistanceGeometry, std::string("distance")},
      {GeometryTypes::eScalarEntityMeasurementSeriesGeometry,
       std::string("measurement series")},
      {GeometryTypes::eScalarEntityTemperatureGeometry,
       std::string("temperature")},
      {GeometryTypes::eSlottedHoleGeometry, std::string("slotted hole")},
      {GeometryTypes::eSphereGeometry, std::string("sphere")},
      {GeometryTypes::eTorusGeometry, std::string("torus")},
      {GeometryTypes::ePlaneLevelGeometry, std::string("level")},
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
    const std::string token = getElementTypeName(type);
    if (token.empty()) {
      continue;
    }
    QCOMPARE(getElementTypeEnum(token), type);
  }

  for (const FeatureTypes& type : getAvailableFeatureTypes()) {
    const std::string token = getFeatureTypeName(type);
    if (token.empty()) {
      continue;
    }
    QCOMPARE(getFeatureTypeEnum(token), type);
  }

  for (const GeometryTypes& type : getAvailableGeometryTypes()) {
    const std::string token = getGeometryTypeName(type);
    if (token.empty()) {
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
    if (getElementTypeName(type).empty()) {
      continue;
    }
    QVERIFY2(!getElementTypeDisplayName(type).empty(),
             (std::format("element type {} has no display name",
                          getElementTypeName(type)))
                 .c_str());
  }

  for (const FeatureTypes& type : getAvailableFeatureTypes()) {
    if (getFeatureTypeName(type).empty()) {
      continue;
    }
    QVERIFY2(!getFeatureTypeDisplayName(type).empty(),
             (std::format("feature type {} has no display name",
                          getFeatureTypeName(type)))
                 .c_str());
  }

  for (const GeometryTypes& type : getAvailableGeometryTypes()) {
    if (getGeometryTypeName(type).empty()) {
      continue;
    }
    QVERIFY2(!getGeometryTypeDisplayName(type).empty(),
             (std::format("geometry type {} has no display name",
                          getGeometryTypeName(type)))
                 .c_str());
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
    const std::string display = getFeatureTypeDisplayName(type);
    if (display.empty() || display == getFeatureTypeName(type)) {
      continue;
    }
    QCOMPARE(getFeatureTypeEnum(display), FeatureTypes::eUndefinedFeature);
  }

  for (const GeometryTypes& type : getAvailableGeometryTypes()) {
    const std::string display = getGeometryTypeDisplayName(type);
    if (display.empty() || display == getGeometryTypeName(type)) {
      continue;
    }
    QCOMPARE(getGeometryTypeEnum(display), GeometryTypes::eUndefinedGeometry);
  }
}

QTEST_APPLESS_MAIN(TypeNamesTest)
#include <format>

#include "tst_typenames.moc"
