#include "util.h"

#include <cmath>
#include <map>
#include <vector>

#include "containers.h"
#include "feature_wrapper.h"
#include "string_util.h"

namespace oi {

// #################################################
// maps with element enums and corresponding strings
// #################################################

namespace {

// ############################################################################
// UNIT CONVERSION FACTORS - each factor is defined once; convertToDefault()
// multiplies by 1 / factor where convertFromDefault() multiplies by factor, so
// the two directions cannot drift apart. The default units are metre, radian
// and degree Celsius.
// ############################################################################

//! millimetres per metre
constexpr double kMillimetersPerMeter = 1000.0;
//! milliradians per radian
constexpr double kMilliradiansPerRadian = 1000.0;
//! international inches per metre (1 / 0.0254, rounded to 8 decimals)
constexpr double kInchesPerMeter = 39.37007874;
//! arc seconds per radian (180 * 3600 / pi)
constexpr double kArcSecondsPerRadian = 206264.8062471;
//! degrees Fahrenheit per degree Celsius (temperature difference)
constexpr double kFahrenheitPerCelsius = 1.8;
//! the Fahrenheit reading at 0 degrees Celsius
constexpr double kFahrenheitAtZeroCelsius = 32.0;
//! the Kelvin reading at 0 degrees Celsius
constexpr double kKelvinAtZeroCelsius = 273.15;

// ############################################################################
// LINEAR THERMAL EXPANSION COEFFICIENTS used by the temperature compensation,
// in 1/K (the same as 1/degC): a length changes by length * alpha * dT. The
// values are the ones of the original OpenIndy material table and are
// deliberately unchanged.
// ############################################################################

constexpr double kThermalExpansionSteel = 0.00001139;
constexpr double kThermalExpansionAluminum = 0.000024;
constexpr double kThermalExpansionPlumb = 0.000030;
constexpr double kThermalExpansionIron = 0.000012;
constexpr double kThermalExpansionGrayCastIron = 0.000009;
constexpr double kThermalExpansionCopper = 0.000017;
constexpr double kThermalExpansionBrass = 0.000018;
constexpr double kThermalExpansionZinc = 0.000027;
constexpr double kThermalExpansionPlatinum = 0.000009;
constexpr double kThermalExpansionConcrete = 0.000012;
constexpr double kThermalExpansionReinforcedConcrete = 0.000013;

}  // namespace

namespace internal {

// ############################################################################
// PERSISTED TOKENS - these strings are written into saved projects and into
// settings keys. Changing one silently breaks loading. See util.h.
// ############################################################################

struct Tables {
  std::map<ElementTypes, std::string> elementTypesMap;
  std::map<ElementTypes, std::string> elementTypesPluralMap;
  std::map<FeatureTypes, std::string> featureTypesMap;
  std::map<GeometryTypes, std::string> geometryTypesMap;

  // ############################################################################
  // DISPLAY NAMES - shown to the user, never persisted, safe to edit freely.
  // Deliberately separate maps rather than a transform of the tokens above, so
  // that changing what a user reads can never change what a file contains.
  // ############################################################################

  std::map<ElementTypes, std::string> elementTypesDisplayMap;
  std::map<ElementTypes, std::string> elementTypesDisplayPluralMap;
  std::map<FeatureTypes, std::string> featureTypesDisplayMap;
  std::map<GeometryTypes, std::string> geometryTypesDisplayMap;

  std::map<ElementTypes, std::string> elementTypeIcons;
  std::map<FeatureTypes, std::string> featureTypeIcons;

  std::map<ReadingTypes, std::string> readingTypesMap;

  std::map<FeatureDisplayAttributes, std::string> featureDisplayAttributesMap;
  std::map<TrafoParamDisplayAttributes, std::string>
      trafoParamDisplayAttributesMap;
  std::map<ObservationDisplayAttributes, std::string>
      observationDisplayAttributesMap;
  std::map<ReadingDisplayAttributes, std::string> readingDisplayAttributesMap;

  std::vector<int> featureDisplayAttributes;
  std::vector<ObservationDisplayAttributes> observationDisplayAttributes;
  std::vector<ReadingDisplayAttributes> readingDisplayAttributes;

  std::map<UnitType, std::string> unitTypesMap;

  std::map<SensorFunctions, std::string> sensorFunctionsMap;

  std::map<ToolTypes, std::string> toolTypesMap;

  std::map<MessageTypes, std::string> messageTypesMap;

  std::map<GeometryParameters, std::string> geometryParametersMap;
  std::map<TrafoParamParameters, std::string> trafoParamParametersMap;

  std::map<ActualNominalFilter, std::string> actualNominalFilterMap;

  std::map<MaterialsTempComp, std::string> materialStringsMap;
  std::map<MaterialsTempComp, double> materialValuesMap;

  Tables();
};

//! The lookup tables, built once on first use (thread-safe).
const Tables& tables();

const std::string undefined;

//! The name stored for `key`, or the empty string when there is none.
template <typename Key>
const std::string& nameOrUndefined(const std::map<Key, std::string>& names,
                                   const Key& key) {
  const auto it = names.find(key);
  return it == names.cend() ? undefined : it->second;
}

Tables::Tables() {
  // fill element map
  elementTypesMap.insert_or_assign(ElementTypes::eCircleElement, "circle");
  elementTypesMap.insert_or_assign(ElementTypes::eConeElement, "cone");
  elementTypesMap.insert_or_assign(ElementTypes::eCylinderElement, "cylinder");
  elementTypesMap.insert_or_assign(ElementTypes::eEllipseElement, "ellipse");
  elementTypesMap.insert_or_assign(ElementTypes::eEllipsoidElement,
                                   "ellipsoid");
  elementTypesMap.insert_or_assign(ElementTypes::eHyperboloidElement,
                                   "hyperboloid");
  elementTypesMap.insert_or_assign(ElementTypes::eLineElement, "line");
  elementTypesMap.insert_or_assign(ElementTypes::eNurbsElement, "nurbs");
  elementTypesMap.insert_or_assign(ElementTypes::eParaboloidElement,
                                   "paraboloid");
  elementTypesMap.insert_or_assign(ElementTypes::ePlaneElement, "plane");
  elementTypesMap.insert_or_assign(ElementTypes::ePointElement, "point");
  elementTypesMap.insert_or_assign(ElementTypes::ePointCloudElement,
                                   "point cloud");
  elementTypesMap.insert_or_assign(ElementTypes::eScalarEntityAngleElement,
                                   "angle");
  elementTypesMap.insert_or_assign(ElementTypes::eScalarEntityDistanceElement,
                                   "distance");
  elementTypesMap.insert_or_assign(
      ElementTypes::eScalarEntityMeasurementSeriesElement,
      "measurement series");
  elementTypesMap.insert_or_assign(
      ElementTypes::eScalarEntityTemperatureElement, "temperature");
  elementTypesMap.insert_or_assign(ElementTypes::eSlottedHoleElement,
                                   "slotted hole");
  elementTypesMap.insert_or_assign(ElementTypes::eSphereElement, "sphere");
  elementTypesMap.insert_or_assign(ElementTypes::eTorusElement, "torus");
  elementTypesMap.insert_or_assign(ElementTypes::eDirectionElement,
                                   "direction");
  elementTypesMap.insert_or_assign(ElementTypes::ePositionElement, "position");
  elementTypesMap.insert_or_assign(ElementTypes::eRadiusElement, "radius");
  elementTypesMap.insert_or_assign(ElementTypes::eCoordinateSystemElement,
                                   "coordinate system");
  elementTypesMap.insert_or_assign(ElementTypes::eStationElement, "station");
  elementTypesMap.insert_or_assign(ElementTypes::eTrafoParamElement,
                                   "transformation parameters");
  elementTypesMap.insert_or_assign(ElementTypes::eObservationElement,
                                   "observation");
  elementTypesMap.insert_or_assign(ElementTypes::eReadingCartesianElement,
                                   "cartesian reading");
  elementTypesMap.insert_or_assign(ElementTypes::eReadingPolarElement,
                                   "polar reading");
  elementTypesMap.insert_or_assign(ElementTypes::eReadingDistanceElement,
                                   "distance reading");
  elementTypesMap.insert_or_assign(ElementTypes::eReadingDirectionElement,
                                   "direction reading");
  elementTypesMap.insert_or_assign(ElementTypes::eReadingTemperatureElement,
                                   "temperature reading");
  elementTypesMap.insert_or_assign(ElementTypes::eReadingLevelElement,
                                   "level reading");
  elementTypesMap.insert_or_assign(ElementTypes::eLevelElement, "level");
  elementTypesMap.insert_or_assign(ElementTypes::ePointCollectionElement,
                                   "point collection");

  // fill element plural map
  elementTypesPluralMap.insert_or_assign(ElementTypes::eCircleElement,
                                         "circles");
  elementTypesPluralMap.insert_or_assign(ElementTypes::eConeElement, "cones");
  elementTypesPluralMap.insert_or_assign(ElementTypes::eCylinderElement,
                                         "cylinders");
  elementTypesPluralMap.insert_or_assign(ElementTypes::eEllipseElement,
                                         "ellipses");
  elementTypesPluralMap.insert_or_assign(ElementTypes::eEllipsoidElement,
                                         "ellipsoids");
  elementTypesPluralMap.insert_or_assign(ElementTypes::eHyperboloidElement,
                                         "hyperboloids");
  elementTypesPluralMap.insert_or_assign(ElementTypes::eLineElement, "lines");
  elementTypesPluralMap.insert_or_assign(ElementTypes::eNurbsElement, "nurbs");
  elementTypesPluralMap.insert_or_assign(ElementTypes::eParaboloidElement,
                                         "paraboloids");
  elementTypesPluralMap.insert_or_assign(ElementTypes::ePlaneElement, "planes");
  elementTypesPluralMap.insert_or_assign(ElementTypes::ePointElement, "points");
  elementTypesPluralMap.insert_or_assign(ElementTypes::ePointCloudElement,
                                         "point clouds");
  elementTypesPluralMap.insert_or_assign(
      ElementTypes::eScalarEntityAngleElement, "angles");
  elementTypesPluralMap.insert_or_assign(
      ElementTypes::eScalarEntityDistanceElement, "distances");
  elementTypesPluralMap.insert_or_assign(
      ElementTypes::eScalarEntityMeasurementSeriesElement,
      "measurement series");
  elementTypesPluralMap.insert_or_assign(
      ElementTypes::eScalarEntityTemperatureElement, "temperatures");
  elementTypesPluralMap.insert_or_assign(ElementTypes::eSlottedHoleElement,
                                         "slotted holes");
  elementTypesPluralMap.insert_or_assign(ElementTypes::eSphereElement,
                                         "spheres");
  elementTypesPluralMap.insert_or_assign(ElementTypes::eTorusElement, "tori");
  elementTypesPluralMap.insert_or_assign(ElementTypes::eDirectionElement,
                                         "directions");
  elementTypesPluralMap.insert_or_assign(ElementTypes::ePositionElement,
                                         "positions");
  elementTypesPluralMap.insert_or_assign(ElementTypes::eRadiusElement, "radii");
  elementTypesPluralMap.insert_or_assign(ElementTypes::eCoordinateSystemElement,
                                         "coordinate systems");
  elementTypesPluralMap.insert_or_assign(ElementTypes::eStationElement,
                                         "stations");
  elementTypesPluralMap.insert_or_assign(ElementTypes::eTrafoParamElement,
                                         "transformation parameters");
  elementTypesPluralMap.insert_or_assign(ElementTypes::eObservationElement,
                                         "observations");
  elementTypesPluralMap.insert_or_assign(ElementTypes::eReadingCartesianElement,
                                         "cartesian readings");
  elementTypesPluralMap.insert_or_assign(ElementTypes::eReadingPolarElement,
                                         "polar readings");
  elementTypesPluralMap.insert_or_assign(ElementTypes::eReadingDistanceElement,
                                         "distance readings");
  elementTypesPluralMap.insert_or_assign(ElementTypes::eReadingDirectionElement,
                                         "direction readings");
  elementTypesPluralMap.insert_or_assign(
      ElementTypes::eReadingTemperatureElement, "temperature readings");
  elementTypesPluralMap.insert_or_assign(ElementTypes::eReadingLevelElement,
                                         "level readings");
  elementTypesPluralMap.insert_or_assign(ElementTypes::ePointCollectionElement,
                                         "point collections");

  // fill feature map
  featureTypesMap.insert_or_assign(FeatureTypes::eCircleFeature, "circle");
  featureTypesMap.insert_or_assign(FeatureTypes::eConeFeature, "cone");
  featureTypesMap.insert_or_assign(FeatureTypes::eCylinderFeature, "cylinder");
  featureTypesMap.insert_or_assign(FeatureTypes::eEllipseFeature, "ellipse");
  featureTypesMap.insert_or_assign(FeatureTypes::eEllipsoidFeature,
                                   "ellipsoid");
  featureTypesMap.insert_or_assign(FeatureTypes::eHyperboloidFeature,
                                   "hyperboloid");
  featureTypesMap.insert_or_assign(FeatureTypes::eLineFeature, "line");
  featureTypesMap.insert_or_assign(FeatureTypes::eNurbsFeature, "nurbs");
  featureTypesMap.insert_or_assign(FeatureTypes::eParaboloidFeature,
                                   "paraboloid");
  featureTypesMap.insert_or_assign(FeatureTypes::ePlaneFeature, "plane");
  featureTypesMap.insert_or_assign(FeatureTypes::ePointFeature, "point");
  featureTypesMap.insert_or_assign(FeatureTypes::ePointCloudFeature,
                                   "point cloud");
  featureTypesMap.insert_or_assign(FeatureTypes::eScalarEntityAngleFeature,
                                   "angle");
  featureTypesMap.insert_or_assign(FeatureTypes::eScalarEntityDistanceFeature,
                                   "distance");
  featureTypesMap.insert_or_assign(
      FeatureTypes::eScalarEntityMeasurementSeriesFeature,
      "measurement series");
  featureTypesMap.insert_or_assign(
      FeatureTypes::eScalarEntityTemperatureFeature, "temperature");
  featureTypesMap.insert_or_assign(FeatureTypes::eSlottedHoleFeature,
                                   "slotted hole");
  featureTypesMap.insert_or_assign(FeatureTypes::eSphereFeature, "sphere");
  featureTypesMap.insert_or_assign(FeatureTypes::eTorusFeature, "torus");
  featureTypesMap.insert_or_assign(FeatureTypes::eCoordinateSystemFeature,
                                   "coordinate system");
  featureTypesMap.insert_or_assign(FeatureTypes::eStationFeature, "station");
  featureTypesMap.insert_or_assign(FeatureTypes::eTrafoParamFeature,
                                   "transformation parameters");
  featureTypesMap.insert_or_assign(FeatureTypes::eLevelFeature, "level");
  featureTypesMap.insert_or_assign(FeatureTypes::eGroupFeature, "group");
  featureTypesMap.insert_or_assign(FeatureTypes::ePointGroupFeature,
                                   "point group");

  // fill geometry map
  geometryTypesMap.insert_or_assign(GeometryTypes::eCircleGeometry, "circle");
  geometryTypesMap.insert_or_assign(GeometryTypes::eConeGeometry, "cone");
  geometryTypesMap.insert_or_assign(GeometryTypes::eCylinderGeometry,
                                    "cylinder");
  geometryTypesMap.insert_or_assign(GeometryTypes::eEllipseGeometry, "ellipse");
  geometryTypesMap.insert_or_assign(GeometryTypes::eEllipsoidGeometry,
                                    "ellipsoid");
  geometryTypesMap.insert_or_assign(GeometryTypes::eHyperboloidGeometry,
                                    "hyperboloid");
  geometryTypesMap.insert_or_assign(GeometryTypes::eLineGeometry, "line");
  geometryTypesMap.insert_or_assign(GeometryTypes::eNurbsGeometry, "nurbs");
  geometryTypesMap.insert_or_assign(GeometryTypes::eParaboloidGeometry,
                                    "paraboloid");
  geometryTypesMap.insert_or_assign(GeometryTypes::ePlaneGeometry, "plane");
  geometryTypesMap.insert_or_assign(GeometryTypes::ePointGeometry, "point");
  geometryTypesMap.insert_or_assign(GeometryTypes::ePointCloudGeometry,
                                    "point cloud");
  geometryTypesMap.insert_or_assign(GeometryTypes::eScalarEntityAngleGeometry,
                                    "angle");
  geometryTypesMap.insert_or_assign(
      GeometryTypes::eScalarEntityDistanceGeometry, "distance");
  geometryTypesMap.insert_or_assign(
      GeometryTypes::eScalarEntityMeasurementSeriesGeometry,
      "measurement series");
  geometryTypesMap.insert_or_assign(
      GeometryTypes::eScalarEntityTemperatureGeometry, "temperature");
  geometryTypesMap.insert_or_assign(GeometryTypes::eSlottedHoleGeometry,
                                    "slotted hole");
  geometryTypesMap.insert_or_assign(GeometryTypes::eSphereGeometry, "sphere");
  geometryTypesMap.insert_or_assign(GeometryTypes::eTorusGeometry, "torus");
  geometryTypesMap.insert_or_assign(GeometryTypes::ePlaneLevelGeometry,
                                    "level");

  // fill element type icons
  elementTypeIcons.insert_or_assign(
      ElementTypes::eCircleElement,
      ":/icons/icons/toolbars/feature/Circle.png");
  elementTypeIcons.insert_or_assign(ElementTypes::eConeElement,
                                    ":/icons/icons/toolbars/feature/Cone.png");
  elementTypeIcons.insert_or_assign(
      ElementTypes::eCylinderElement,
      ":/icons/icons/toolbars/feature/Cylinder.png");
  elementTypeIcons.insert_or_assign(
      ElementTypes::eEllipseElement,
      ":/icons/icons/toolbars/feature/Ellipse.png");
  elementTypeIcons.insert_or_assign(
      ElementTypes::eEllipsoidElement,
      ":/icons/icons/toolbars/feature/Ellipsoid.png");
  elementTypeIcons.insert_or_assign(
      ElementTypes::eHyperboloidElement,
      ":/icons/icons/toolbars/feature/Hyperboloid.png");
  elementTypeIcons.insert_or_assign(ElementTypes::eLineElement,
                                    ":/icons/icons/toolbars/feature/Line.png");
  elementTypeIcons.insert_or_assign(
      ElementTypes::eNurbsElement,
      ":/icons/icons/toolbars/feature/Nurbs Surface.png");
  elementTypeIcons.insert_or_assign(
      ElementTypes::eParaboloidElement,
      ":/icons/icons/toolbars/feature/Paraboloid.png");
  elementTypeIcons.insert_or_assign(ElementTypes::ePlaneElement,
                                    ":/icons/icons/toolbars/feature/Plane.png");
  elementTypeIcons.insert_or_assign(ElementTypes::ePointElement,
                                    ":/icons/icons/toolbars/feature/Point.png");
  elementTypeIcons.insert_or_assign(
      ElementTypes::ePointCloudElement,
      ":/icons/icons/toolbars/feature/Point Cloud.png");
  elementTypeIcons.insert_or_assign(
      ElementTypes::eScalarEntityAngleElement,
      ":/icons/icons/toolbars/feature/Scalar Entities.png");
  elementTypeIcons.insert_or_assign(
      ElementTypes::eScalarEntityDistanceElement,
      ":/icons/icons/toolbars/feature/Scalar Entities.png");
  elementTypeIcons.insert_or_assign(
      ElementTypes::eScalarEntityMeasurementSeriesElement,
      ":/icons/icons/toolbars/feature/Scalar Entities.png");
  elementTypeIcons.insert_or_assign(
      ElementTypes::eScalarEntityTemperatureElement,
      ":/icons/icons/toolbars/feature/Scalar Entities.png");
  elementTypeIcons.insert_or_assign(
      ElementTypes::eSlottedHoleElement,
      ":/icons/icons/toolbars/feature/Slotted Hole.png");
  elementTypeIcons.insert_or_assign(
      ElementTypes::eSphereElement,
      ":/icons/icons/toolbars/feature/Sphere.png");
  elementTypeIcons.insert_or_assign(ElementTypes::eTorusElement,
                                    ":/icons/icons/toolbars/feature/Torus.png");
  elementTypeIcons.insert_or_assign(ElementTypes::eDirectionElement, "");
  elementTypeIcons.insert_or_assign(ElementTypes::ePositionElement, "");
  elementTypeIcons.insert_or_assign(ElementTypes::eRadiusElement, "");
  elementTypeIcons.insert_or_assign(
      ElementTypes::eCoordinateSystemElement,
      ":/icons/icons/toolbars/feature/Coordinatesystem.png");
  elementTypeIcons.insert_or_assign(
      ElementTypes::eStationElement,
      ":/icons/icons/toolbars/feature/Station.png");
  elementTypeIcons.insert_or_assign(
      ElementTypes::eTrafoParamElement,
      ":/icons/icons/toolbars/feature/TrafoParam.png");
  elementTypeIcons.insert_or_assign(ElementTypes::eObservationElement, "");
  elementTypeIcons.insert_or_assign(ElementTypes::eReadingCartesianElement, "");
  elementTypeIcons.insert_or_assign(ElementTypes::eReadingPolarElement, "");
  elementTypeIcons.insert_or_assign(ElementTypes::eReadingDistanceElement, "");
  elementTypeIcons.insert_or_assign(ElementTypes::eReadingDirectionElement, "");
  elementTypeIcons.insert_or_assign(ElementTypes::eReadingTemperatureElement,
                                    "");
  elementTypeIcons.insert_or_assign(ElementTypes::eReadingLevelElement, "");

  // fill feature type icons
  featureTypeIcons.insert_or_assign(
      FeatureTypes::eCircleFeature,
      ":/icons/icons/toolbars/feature/Circle.png");
  featureTypeIcons.insert_or_assign(FeatureTypes::eConeFeature,
                                    ":/icons/icons/toolbars/feature/Cone.png");
  featureTypeIcons.insert_or_assign(
      FeatureTypes::eCylinderFeature,
      ":/icons/icons/toolbars/feature/Cylinder.png");
  featureTypeIcons.insert_or_assign(
      FeatureTypes::eEllipseFeature,
      ":/icons/icons/toolbars/feature/Ellipse.png");
  featureTypeIcons.insert_or_assign(
      FeatureTypes::eEllipsoidFeature,
      ":/icons/icons/toolbars/feature/Ellipsoid.png");
  featureTypeIcons.insert_or_assign(
      FeatureTypes::eHyperboloidFeature,
      ":/icons/icons/toolbars/feature/Hyperboloid.png");
  featureTypeIcons.insert_or_assign(FeatureTypes::eLineFeature,
                                    ":/icons/icons/toolbars/feature/Line.png");
  featureTypeIcons.insert_or_assign(
      FeatureTypes::eNurbsFeature,
      ":/icons/icons/toolbars/feature/Nurbs Surface.png");
  featureTypeIcons.insert_or_assign(
      FeatureTypes::eParaboloidFeature,
      ":/icons/icons/toolbars/feature/Paraboloid.png");
  featureTypeIcons.insert_or_assign(FeatureTypes::ePlaneFeature,
                                    ":/icons/icons/toolbars/feature/Plane.png");
  featureTypeIcons.insert_or_assign(FeatureTypes::ePointFeature,
                                    ":/icons/icons/toolbars/feature/Point.png");
  featureTypeIcons.insert_or_assign(
      FeatureTypes::ePointCloudFeature,
      ":/icons/icons/toolbars/feature/Point Cloud.png");
  featureTypeIcons.insert_or_assign(
      FeatureTypes::eScalarEntityAngleFeature,
      ":/icons/icons/toolbars/feature/Scalar Entities.png");
  featureTypeIcons.insert_or_assign(
      FeatureTypes::eScalarEntityDistanceFeature,
      ":/icons/icons/toolbars/feature/Scalar Entities.png");
  featureTypeIcons.insert_or_assign(
      FeatureTypes::eScalarEntityMeasurementSeriesFeature,
      ":/icons/icons/toolbars/feature/Scalar Entities.png");
  featureTypeIcons.insert_or_assign(
      FeatureTypes::eScalarEntityTemperatureFeature,
      ":/icons/icons/toolbars/feature/Scalar Entities.png");
  featureTypeIcons.insert_or_assign(
      FeatureTypes::eSlottedHoleFeature,
      ":/icons/icons/toolbars/feature/Slotted Hole.png");
  featureTypeIcons.insert_or_assign(
      FeatureTypes::eSphereFeature,
      ":/icons/icons/toolbars/feature/Sphere.png");
  featureTypeIcons.insert_or_assign(FeatureTypes::eTorusFeature,
                                    ":/icons/icons/toolbars/feature/Torus.png");
  featureTypeIcons.insert_or_assign(
      FeatureTypes::eCoordinateSystemFeature,
      ":/icons/icons/toolbars/feature/Coordinatesystem.png");
  featureTypeIcons.insert_or_assign(
      FeatureTypes::eStationFeature,
      ":/icons/icons/toolbars/feature/Station.png");
  featureTypeIcons.insert_or_assign(
      FeatureTypes::eTrafoParamFeature,
      ":/icons/icons/toolbars/feature/TrafoParam.png");

  // fill reading map
  readingTypesMap.insert_or_assign(ReadingTypes::eDistanceReading, "distance");
  readingTypesMap.insert_or_assign(ReadingTypes::eCartesianReading,
                                   "cartesian");
  readingTypesMap.insert_or_assign(ReadingTypes::eCartesianReading6D,
                                   "cartesian6D");
  readingTypesMap.insert_or_assign(ReadingTypes::ePolarReading, "polar");
  readingTypesMap.insert_or_assign(ReadingTypes::eDirectionReading,
                                   "direction");
  readingTypesMap.insert_or_assign(ReadingTypes::eTemperatureReading,
                                   "temperature");
  readingTypesMap.insert_or_assign(ReadingTypes::eLevelReading, "level");

  // fill feature display attributes map
  featureDisplayAttributesMap.insert_or_assign(eFeatureDisplayType, "type");
  featureDisplayAttributesMap.insert_or_assign(eFeatureDisplayName, "name");
  featureDisplayAttributesMap.insert_or_assign(eFeatureDisplayComment,
                                               "comment");
  featureDisplayAttributesMap.insert_or_assign(eFeatureDisplayGroup, "group");
  featureDisplayAttributesMap.insert_or_assign(eFeatureDisplayIsSolved,
                                               "solved");
  featureDisplayAttributesMap.insert_or_assign(eFeatureDisplayIsUpdated,
                                               "updated");
  featureDisplayAttributesMap.insert_or_assign(eFeatureDisplayFunctions,
                                               "functions");
  featureDisplayAttributesMap.insert_or_assign(eFeatureDisplayIsCommon,
                                               "common");
  featureDisplayAttributesMap.insert_or_assign(eFeatureDisplayUsedFor,
                                               "used for");
  featureDisplayAttributesMap.insert_or_assign(eFeatureDisplayPreviouslyNeeded,
                                               "previously needed");
  featureDisplayAttributesMap.insert_or_assign(eFeatureDisplayStDev, "stdev");
  featureDisplayAttributesMap.insert_or_assign(eFeatureDisplayMeasurementConfig,
                                               "measurement config");
  featureDisplayAttributesMap.insert_or_assign(eFeatureDisplayObservations,
                                               "observations");
  featureDisplayAttributesMap.insert_or_assign(eFeatureDisplayX, "x");
  featureDisplayAttributesMap.insert_or_assign(eFeatureDisplayY, "y");
  featureDisplayAttributesMap.insert_or_assign(eFeatureDisplayZ, "z");
  featureDisplayAttributesMap.insert_or_assign(eFeatureDisplayPrimaryI, "i");
  featureDisplayAttributesMap.insert_or_assign(eFeatureDisplayPrimaryJ, "j");
  featureDisplayAttributesMap.insert_or_assign(eFeatureDisplayPrimaryK, "k");
  featureDisplayAttributesMap.insert_or_assign(eFeatureDisplayRadiusA,
                                               "radius");
  featureDisplayAttributesMap.insert_or_assign(eFeatureDisplayRadiusB,
                                               "radius 2");
  featureDisplayAttributesMap.insert_or_assign(eFeatureDisplaySecondaryI,
                                               "i 2");
  featureDisplayAttributesMap.insert_or_assign(eFeatureDisplaySecondaryJ,
                                               "j 2");
  featureDisplayAttributesMap.insert_or_assign(eFeatureDisplaySecondaryK,
                                               "k 2");
  featureDisplayAttributesMap.insert_or_assign(eFeatureDisplayAperture,
                                               "aperture");
  featureDisplayAttributesMap.insert_or_assign(eFeatureDisplayA, "a");
  featureDisplayAttributesMap.insert_or_assign(eFeatureDisplayB, "b");
  featureDisplayAttributesMap.insert_or_assign(eFeatureDisplayC, "c");
  featureDisplayAttributesMap.insert_or_assign(eFeatureDisplayAngle, "angle");
  featureDisplayAttributesMap.insert_or_assign(eFeatureDisplayDistance,
                                               "distance");
  featureDisplayAttributesMap.insert_or_assign(eFeatureDisplayMeasurementSeries,
                                               "measurement series");
  featureDisplayAttributesMap.insert_or_assign(eFeatureDisplayTemperature,
                                               "temperature");
  featureDisplayAttributesMap.insert_or_assign(eFeatureDisplayLength, "length");
  featureDisplayAttributesMap.insert_or_assign(eFeatureDisplayExpansionOriginX,
                                               "expansion x");
  featureDisplayAttributesMap.insert_or_assign(eFeatureDisplayExpansionOriginY,
                                               "expansion y");
  featureDisplayAttributesMap.insert_or_assign(eFeatureDisplayExpansionOriginZ,
                                               "expansion z");
  featureDisplayAttributesMap.insert_or_assign(eFeatureDisplayIsActual,
                                               "actual feature");
  featureDisplayAttributesMap.insert_or_assign(eFeatureDisplayFormError,
                                               "form error");

  // fill observation display attributes map
  observationDisplayAttributesMap.insert_or_assign(eObservationDisplayId, "id");
  observationDisplayAttributesMap.insert_or_assign(eObservationDisplayStation,
                                                   "station");
  observationDisplayAttributesMap.insert_or_assign(
      eObservationDisplayTargetGeometries, "target geometries");
  observationDisplayAttributesMap.insert_or_assign(eObservationDisplayX, "x");
  observationDisplayAttributesMap.insert_or_assign(eObservationDisplayY, "y");
  observationDisplayAttributesMap.insert_or_assign(eObservationDisplayZ, "z");
  observationDisplayAttributesMap.insert_or_assign(eObservationDisplayI, "i");
  observationDisplayAttributesMap.insert_or_assign(eObservationDisplayJ, "j");
  observationDisplayAttributesMap.insert_or_assign(eObservationDisplayK, "k");
  observationDisplayAttributesMap.insert_or_assign(eObservationDisplaySigmaX,
                                                   "sigma x");
  observationDisplayAttributesMap.insert_or_assign(eObservationDisplaySigmaY,
                                                   "sigma y");
  observationDisplayAttributesMap.insert_or_assign(eObservationDisplaySigmaZ,
                                                   "sigma z");
  observationDisplayAttributesMap.insert_or_assign(eObservationDisplaySigmaI,
                                                   "sigma i");
  observationDisplayAttributesMap.insert_or_assign(eObservationDisplaySigmaJ,
                                                   "sigma j");
  observationDisplayAttributesMap.insert_or_assign(eObservationDisplaySigmaK,
                                                   "sigma k");
  observationDisplayAttributesMap.insert_or_assign(eObservationDisplayIsValid,
                                                   "valid");
  observationDisplayAttributesMap.insert_or_assign(eObservationDisplayIsSolved,
                                                   "solved");
  observationDisplayAttributesMap.insert_or_assign(eObservationDisplayVX, "vx");
  observationDisplayAttributesMap.insert_or_assign(eObservationDisplayVY, "vy");
  observationDisplayAttributesMap.insert_or_assign(eObservationDisplayVZ, "vz");
  observationDisplayAttributesMap.insert_or_assign(eObservationDisplayV, "v");
  observationDisplayAttributesMap.insert_or_assign(eObservationDisplayVR, "vr");
  observationDisplayAttributesMap.insert_or_assign(eObservationDisplayVI, "vi");
  observationDisplayAttributesMap.insert_or_assign(eObservationDisplayVJ, "vj");
  observationDisplayAttributesMap.insert_or_assign(eObservationDisplayVK, "vk");
  observationDisplayAttributesMap.insert_or_assign(eObservationDisplayIsUsed,
                                                   "used");
  observationDisplayAttributesMap.insert_or_assign(
      eObservationDisplayIsDummyPoint, "dummy point");
  observationDisplayAttributesMap.insert_or_assign(
      eObservationDisplayReadingTime, "time");

  // fill reading display attributes map
  readingDisplayAttributesMap.insert_or_assign(eReadingDisplayId, "id");
  readingDisplayAttributesMap.insert_or_assign(eReadingDisplayType, "type");
  readingDisplayAttributesMap.insert_or_assign(eReadingDisplayTime, "time");
  readingDisplayAttributesMap.insert_or_assign(eReadingDisplaySensorConfigName,
                                               "sensor config");
  readingDisplayAttributesMap.insert_or_assign(
      eReadingDisplayMeasurementConfigName, "measurement config");
  readingDisplayAttributesMap.insert_or_assign(eReadingDisplaySide, "side");
  readingDisplayAttributesMap.insert_or_assign(eReadingDisplayAzimuth,
                                               "azimuth");
  readingDisplayAttributesMap.insert_or_assign(eReadingDisplayZenith, "zenith");
  readingDisplayAttributesMap.insert_or_assign(eReadingDisplayDistance,
                                               "distance");
  readingDisplayAttributesMap.insert_or_assign(eReadingDisplayX, "x");
  readingDisplayAttributesMap.insert_or_assign(eReadingDisplayY, "y");
  readingDisplayAttributesMap.insert_or_assign(eReadingDisplayZ, "z");
  readingDisplayAttributesMap.insert_or_assign(eReadingDisplayI, "i");
  readingDisplayAttributesMap.insert_or_assign(eReadingDisplayJ, "j");
  readingDisplayAttributesMap.insert_or_assign(eReadingDisplayK, "k");
  readingDisplayAttributesMap.insert_or_assign(eReadingDisplayTemperature,
                                               "temperature");
  readingDisplayAttributesMap.insert_or_assign(eReadingDisplaySigmaAzimuth,
                                               "sigma azimuth");
  readingDisplayAttributesMap.insert_or_assign(eReadingDisplaySigmaZenith,
                                               "sigma zenith");
  readingDisplayAttributesMap.insert_or_assign(eReadingDisplaySigmaDistance,
                                               "sigma distance");
  readingDisplayAttributesMap.insert_or_assign(eReadingDisplaySigmaX,
                                               "sigma x");
  readingDisplayAttributesMap.insert_or_assign(eReadingDisplaySigmaY,
                                               "sigma y");
  readingDisplayAttributesMap.insert_or_assign(eReadingDisplaySigmaZ,
                                               "sigma z");
  readingDisplayAttributesMap.insert_or_assign(eReadingDisplaySigmaI,
                                               "sigma i");
  readingDisplayAttributesMap.insert_or_assign(eReadingDisplaySigmaJ,
                                               "sigma j");
  readingDisplayAttributesMap.insert_or_assign(eReadingDisplaySigmaK,
                                               "sigma k");
  readingDisplayAttributesMap.insert_or_assign(eReadingDisplaySigmaTemperature,
                                               "sigma temperature");
  readingDisplayAttributesMap.insert_or_assign(eReadingDisplayImported,
                                               "imported");

  // fill trafo param display attributes map
  trafoParamDisplayAttributesMap.insert_or_assign(eTrafoParamDisplayType,
                                                  "type");
  trafoParamDisplayAttributesMap.insert_or_assign(eTrafoParamDisplayName,
                                                  "name");
  trafoParamDisplayAttributesMap.insert_or_assign(eTrafoParamDisplayComment,
                                                  "comment");
  trafoParamDisplayAttributesMap.insert_or_assign(eTrafoParamDisplayGroup,
                                                  "group");
  trafoParamDisplayAttributesMap.insert_or_assign(eTrafoParamDisplayIsSolved,
                                                  "solved");
  trafoParamDisplayAttributesMap.insert_or_assign(eTrafoParamDisplayIsUpdated,
                                                  "updated");
  trafoParamDisplayAttributesMap.insert_or_assign(eTrafoParamDisplayFunctions,
                                                  "functions");
  trafoParamDisplayAttributesMap.insert_or_assign(eTrafoParamDisplayUsedFor,
                                                  "used for");
  trafoParamDisplayAttributesMap.insert_or_assign(
      eTrafoParamDisplayPreviouslyNeeded, "previously needed");
  trafoParamDisplayAttributesMap.insert_or_assign(eTrafoParamDisplayStDev,
                                                  "stdev");
  trafoParamDisplayAttributesMap.insert_or_assign(eTrafoParamDisplayStartSystem,
                                                  "start");
  trafoParamDisplayAttributesMap.insert_or_assign(
      eTrafoParamDisplayDestinationSystem, "destination");
  trafoParamDisplayAttributesMap.insert_or_assign(
      eTrafoParamDisplayTranslationX, "tx");
  trafoParamDisplayAttributesMap.insert_or_assign(
      eTrafoParamDisplayTranslationY, "ty");
  trafoParamDisplayAttributesMap.insert_or_assign(
      eTrafoParamDisplayTranslationZ, "tz");
  trafoParamDisplayAttributesMap.insert_or_assign(eTrafoParamDisplayRotationX,
                                                  "rx");
  trafoParamDisplayAttributesMap.insert_or_assign(eTrafoParamDisplayRotationY,
                                                  "ry");
  trafoParamDisplayAttributesMap.insert_or_assign(eTrafoParamDisplayRotationZ,
                                                  "rz");
  trafoParamDisplayAttributesMap.insert_or_assign(eTrafoParamDisplayScaleX,
                                                  "sx");
  trafoParamDisplayAttributesMap.insert_or_assign(eTrafoParamDisplayScaleY,
                                                  "sy");
  trafoParamDisplayAttributesMap.insert_or_assign(eTrafoParamDisplayScaleZ,
                                                  "sz");
  trafoParamDisplayAttributesMap.insert_or_assign(eTrafoParamDisplayIsUsed,
                                                  "used");
  trafoParamDisplayAttributesMap.insert_or_assign(eTrafoParamDisplayValidTime,
                                                  "time");
  trafoParamDisplayAttributesMap.insert_or_assign(eTrafoParamDisplayIsMovement,
                                                  "movement");
  trafoParamDisplayAttributesMap.insert_or_assign(
      eTrafoParamDisplayIsDatumTransformation, "datum transformation");

  // fill available display attributes
  for (int i = eFeatureDisplayType; i <= eFeatureDisplayStDev;
       i++) {  // general feature attributes
    featureDisplayAttributes.push_back(i);
  }
  for (int i = eFeatureDisplayMeasurementConfig; i <= eFeatureDisplayIsActual;
       i++) {  // geometry specific attributes
    featureDisplayAttributes.push_back(i);
  }
  for (int i = eFeatureDisplayX; i <= eFeatureDisplayFormError;
       i++) {  // unknown geometry parameters
    featureDisplayAttributes.push_back(i);
  }
  for (int i = eFeatureDisplayExpansionOriginX;
       i <= eFeatureDisplayExpansionOriginZ;
       i++) {  // coordinate system specific
    featureDisplayAttributes.push_back(i);
  }
  for (int i = eTrafoParamDisplayStartSystem;
       i <= eTrafoParamDisplayIsDatumTransformation;
       i++) {  // trafo param specific
    featureDisplayAttributes.push_back(i);
  }

  // fill available observation display attributes
  observationDisplayAttributes = oi::keys(observationDisplayAttributesMap);

  // fill available reading display attributes
  readingDisplayAttributes = oi::keys(readingDisplayAttributesMap);

  // fill unit types map
  unitTypesMap.insert_or_assign(UnitType::eUnitArcSeconds, "[arcsec]");
  unitTypesMap.insert_or_assign(UnitType::eUnitDecimalDegree, "[°]");
  unitTypesMap.insert_or_assign(UnitType::eUnitFahrenheit, "[°F]");
  unitTypesMap.insert_or_assign(UnitType::eUnitGon, "[gon]");
  unitTypesMap.insert_or_assign(UnitType::eUnitGrad, "[°C]");
  unitTypesMap.insert_or_assign(UnitType::eUnitInch, "[inch]");
  unitTypesMap.insert_or_assign(UnitType::eUnitKelvin, "[°K]");
  unitTypesMap.insert_or_assign(UnitType::eUnitMeter, "[m]");
  unitTypesMap.insert_or_assign(UnitType::eUnitMilliMeter, "[mm]");
  unitTypesMap.insert_or_assign(UnitType::eUnitMilliRadians, "[mrad]");
  unitTypesMap.insert_or_assign(UnitType::eUnitRadiant, "[rad]");

  // fill sensor functions map
  sensorFunctionsMap.insert_or_assign(SensorFunctions::eMoveAngle,
                                      "move (polar)");
  sensorFunctionsMap.insert_or_assign(SensorFunctions::eMoveXYZ,
                                      "move (cartesian)");
  sensorFunctionsMap.insert_or_assign(SensorFunctions::eMotorState,
                                      "change motor state");
  sensorFunctionsMap.insert_or_assign(SensorFunctions::eToggleSight,
                                      "toggle sight orientation");
  sensorFunctionsMap.insert_or_assign(SensorFunctions::eInitialize,
                                      "initialize");
  sensorFunctionsMap.insert_or_assign(SensorFunctions::eHome, "home");
  sensorFunctionsMap.insert_or_assign(SensorFunctions::eCompensation,
                                      "compensation");
  sensorFunctionsMap.insert_or_assign(SensorFunctions::eConnect, "connect");
  sensorFunctionsMap.insert_or_assign(SensorFunctions::eDisconnect,
                                      "disconnect");
  sensorFunctionsMap.insert_or_assign(SensorFunctions::eMeasure, "measure");
  sensorFunctionsMap.insert_or_assign(SensorFunctions::eStream, "stream");

  // fill connection types map

  // fill tool types map
  toolTypesMap.insert_or_assign(ToolTypes::eReportTool, "report");
  toolTypesMap.insert_or_assign(ToolTypes::eUnknownTool, "tool");

  // fill message types map
  messageTypesMap.insert_or_assign(MessageTypes::eInformationMessage,
                                   "information");
  messageTypesMap.insert_or_assign(MessageTypes::eWarningMessage, "warning");
  messageTypesMap.insert_or_assign(MessageTypes::eErrorMessage, "error");
  messageTypesMap.insert_or_assign(MessageTypes::eCriticalMessage, "critical");

  // fill geometry parameters map
  geometryParametersMap.insert_or_assign(GeometryParameters::eUnknownX, "x");
  geometryParametersMap.insert_or_assign(GeometryParameters::eUnknownY, "y");
  geometryParametersMap.insert_or_assign(GeometryParameters::eUnknownZ, "z");
  geometryParametersMap.insert_or_assign(GeometryParameters::eUnknownPrimaryI,
                                         "i");
  geometryParametersMap.insert_or_assign(GeometryParameters::eUnknownPrimaryJ,
                                         "j");
  geometryParametersMap.insert_or_assign(GeometryParameters::eUnknownPrimaryK,
                                         "k");
  geometryParametersMap.insert_or_assign(GeometryParameters::eUnknownSecondaryI,
                                         "i 2");
  geometryParametersMap.insert_or_assign(GeometryParameters::eUnknownSecondaryJ,
                                         "j 2");
  geometryParametersMap.insert_or_assign(GeometryParameters::eUnknownSecondaryK,
                                         "k 2");
  geometryParametersMap.insert_or_assign(GeometryParameters::eUnknownRadiusA,
                                         "radius");
  geometryParametersMap.insert_or_assign(GeometryParameters::eUnknownRadiusB,
                                         "radius 2");
  geometryParametersMap.insert_or_assign(GeometryParameters::eUnknownAperture,
                                         "aperture");
  geometryParametersMap.insert_or_assign(GeometryParameters::eUnknownA, "a");
  geometryParametersMap.insert_or_assign(GeometryParameters::eUnknownB, "b");
  geometryParametersMap.insert_or_assign(GeometryParameters::eUnknownC, "c");
  geometryParametersMap.insert_or_assign(GeometryParameters::eUnknownAngle,
                                         "angle");
  geometryParametersMap.insert_or_assign(GeometryParameters::eUnknownDistance,
                                         "distance");
  geometryParametersMap.insert_or_assign(
      GeometryParameters::eUnknownMeasurementSeries, "measurement series");
  geometryParametersMap.insert_or_assign(
      GeometryParameters::eUnknownTemperature, "temperature");
  geometryParametersMap.insert_or_assign(GeometryParameters::eUnknownLength,
                                         "length");

  // fill trafo param parameters map
  trafoParamParametersMap.insert_or_assign(TrafoParamParameters::eUnknownTX,
                                           "tx");
  trafoParamParametersMap.insert_or_assign(TrafoParamParameters::eUnknownTY,
                                           "ty");
  trafoParamParametersMap.insert_or_assign(TrafoParamParameters::eUnknownTZ,
                                           "tz");
  trafoParamParametersMap.insert_or_assign(TrafoParamParameters::eUnknownRX,
                                           "rx");
  trafoParamParametersMap.insert_or_assign(TrafoParamParameters::eUnknownRY,
                                           "ry");
  trafoParamParametersMap.insert_or_assign(TrafoParamParameters::eUnknownRZ,
                                           "rz");
  trafoParamParametersMap.insert_or_assign(TrafoParamParameters::eUnknownSX,
                                           "sx");
  trafoParamParametersMap.insert_or_assign(TrafoParamParameters::eUnknownSY,
                                           "sy");
  trafoParamParametersMap.insert_or_assign(TrafoParamParameters::eUnknownSZ,
                                           "sz");

  // fill actual nominal filter map
  actualNominalFilterMap.insert_or_assign(
      ActualNominalFilter::eFilterActualNominal, "All Features");
  actualNominalFilterMap.insert_or_assign(ActualNominalFilter::eFilterActual,
                                          "Actuals");
  actualNominalFilterMap.insert_or_assign(ActualNominalFilter::eFilterNominal,
                                          "Nominals");

  // fill material maps
  materialStringsMap.insert_or_assign(MaterialsTempComp::eMaterialSteel,
                                      "steel");
  materialStringsMap.insert_or_assign(MaterialsTempComp::eMaterialAluminum,
                                      "aluminum");
  materialStringsMap.insert_or_assign(MaterialsTempComp::eMaterialPlumb,
                                      "plumb");
  materialStringsMap.insert_or_assign(MaterialsTempComp::eMaterialIron, "iron");
  materialStringsMap.insert_or_assign(MaterialsTempComp::eMaterialGrayCastIron,
                                      "gray cast iron");
  materialStringsMap.insert_or_assign(MaterialsTempComp::eMaterialCopper,
                                      "copper");
  materialStringsMap.insert_or_assign(MaterialsTempComp::eMaterialBrass,
                                      "brass");
  materialStringsMap.insert_or_assign(MaterialsTempComp::eMaterialZinc, "zinc");
  materialStringsMap.insert_or_assign(MaterialsTempComp::eMaterialPlatinum,
                                      "platinum");
  materialStringsMap.insert_or_assign(MaterialsTempComp::eMaterialConcrete,
                                      "concrete");
  materialStringsMap.insert_or_assign(
      MaterialsTempComp::eMaterialReinforcedConcrete, "reinforced concrete");

  materialValuesMap.insert_or_assign(MaterialsTempComp::eMaterialSteel,
                                     kThermalExpansionSteel);
  materialValuesMap.insert_or_assign(MaterialsTempComp::eMaterialAluminum,
                                     kThermalExpansionAluminum);
  materialValuesMap.insert_or_assign(MaterialsTempComp::eMaterialPlumb,
                                     kThermalExpansionPlumb);
  materialValuesMap.insert_or_assign(MaterialsTempComp::eMaterialIron,
                                     kThermalExpansionIron);
  materialValuesMap.insert_or_assign(MaterialsTempComp::eMaterialGrayCastIron,
                                     kThermalExpansionGrayCastIron);
  materialValuesMap.insert_or_assign(MaterialsTempComp::eMaterialCopper,
                                     kThermalExpansionCopper);
  materialValuesMap.insert_or_assign(MaterialsTempComp::eMaterialBrass,
                                     kThermalExpansionBrass);
  materialValuesMap.insert_or_assign(MaterialsTempComp::eMaterialZinc,
                                     kThermalExpansionZinc);
  materialValuesMap.insert_or_assign(MaterialsTempComp::eMaterialPlatinum,
                                     kThermalExpansionPlatinum);
  materialValuesMap.insert_or_assign(MaterialsTempComp::eMaterialConcrete,
                                     kThermalExpansionConcrete);
  materialValuesMap.insert_or_assign(
      MaterialsTempComp::eMaterialReinforcedConcrete,
      kThermalExpansionReinforcedConcrete);

  // ########################################################################
  // DISPLAY NAMES
  //
  // Safe to edit. These are what the user reads; nothing here is ever
  // written to a project file or a settings key. Wording and capitalisation
  // can be changed freely without breaking anything.
  //
  // Seeded as title-case from the persisted tokens above. They are an
  // explicit table rather than a transform of the tokens precisely so that
  // editing one cannot change the other.
  // ########################################################################

  // elementTypesDisplayMap
  elementTypesDisplayMap.insert_or_assign(ElementTypes::eCircleElement,
                                          "Circle");
  elementTypesDisplayMap.insert_or_assign(ElementTypes::eConeElement, "Cone");
  elementTypesDisplayMap.insert_or_assign(ElementTypes::eCylinderElement,
                                          "Cylinder");
  elementTypesDisplayMap.insert_or_assign(ElementTypes::eEllipseElement,
                                          "Ellipse");
  elementTypesDisplayMap.insert_or_assign(ElementTypes::eEllipsoidElement,
                                          "Ellipsoid");
  elementTypesDisplayMap.insert_or_assign(ElementTypes::eHyperboloidElement,
                                          "Hyperboloid");
  elementTypesDisplayMap.insert_or_assign(ElementTypes::eLineElement, "Line");
  elementTypesDisplayMap.insert_or_assign(ElementTypes::eNurbsElement, "Nurbs");
  elementTypesDisplayMap.insert_or_assign(ElementTypes::eParaboloidElement,
                                          "Paraboloid");
  elementTypesDisplayMap.insert_or_assign(ElementTypes::ePlaneElement, "Plane");
  elementTypesDisplayMap.insert_or_assign(ElementTypes::ePointElement, "Point");
  elementTypesDisplayMap.insert_or_assign(ElementTypes::ePointCloudElement,
                                          "Point Cloud");
  elementTypesDisplayMap.insert_or_assign(
      ElementTypes::eScalarEntityAngleElement, "Angle");
  elementTypesDisplayMap.insert_or_assign(
      ElementTypes::eScalarEntityDistanceElement, "Distance");
  elementTypesDisplayMap.insert_or_assign(
      ElementTypes::eScalarEntityMeasurementSeriesElement,
      "Measurement Series");
  elementTypesDisplayMap.insert_or_assign(
      ElementTypes::eScalarEntityTemperatureElement, "Temperature");
  elementTypesDisplayMap.insert_or_assign(ElementTypes::eSlottedHoleElement,
                                          "Slotted Hole");
  elementTypesDisplayMap.insert_or_assign(ElementTypes::eSphereElement,
                                          "Sphere");
  elementTypesDisplayMap.insert_or_assign(ElementTypes::eTorusElement, "Torus");
  elementTypesDisplayMap.insert_or_assign(ElementTypes::eDirectionElement,
                                          "Direction");
  elementTypesDisplayMap.insert_or_assign(ElementTypes::ePositionElement,
                                          "Position");
  elementTypesDisplayMap.insert_or_assign(ElementTypes::eRadiusElement,
                                          "Radius");
  elementTypesDisplayMap.insert_or_assign(
      ElementTypes::eCoordinateSystemElement, "Coordinate System");
  elementTypesDisplayMap.insert_or_assign(ElementTypes::eStationElement,
                                          "Station");
  elementTypesDisplayMap.insert_or_assign(ElementTypes::eTrafoParamElement,
                                          "Transformation Parameters");
  elementTypesDisplayMap.insert_or_assign(ElementTypes::eObservationElement,
                                          "Observation");
  elementTypesDisplayMap.insert_or_assign(
      ElementTypes::eReadingCartesianElement, "Cartesian Reading");
  elementTypesDisplayMap.insert_or_assign(ElementTypes::eReadingPolarElement,
                                          "Polar Reading");
  elementTypesDisplayMap.insert_or_assign(ElementTypes::eReadingDistanceElement,
                                          "Distance Reading");
  elementTypesDisplayMap.insert_or_assign(
      ElementTypes::eReadingDirectionElement, "Direction Reading");
  elementTypesDisplayMap.insert_or_assign(
      ElementTypes::eReadingTemperatureElement, "Temperature Reading");
  elementTypesDisplayMap.insert_or_assign(ElementTypes::eReadingLevelElement,
                                          "Level Reading");
  elementTypesDisplayMap.insert_or_assign(ElementTypes::eLevelElement, "Level");
  elementTypesDisplayMap.insert_or_assign(ElementTypes::ePointCollectionElement,
                                          "Point Collection");

  // elementTypesDisplayPluralMap
  elementTypesDisplayPluralMap.insert_or_assign(ElementTypes::eCircleElement,
                                                "Circles");
  elementTypesDisplayPluralMap.insert_or_assign(ElementTypes::eConeElement,
                                                "Cones");
  elementTypesDisplayPluralMap.insert_or_assign(ElementTypes::eCylinderElement,
                                                "Cylinders");
  elementTypesDisplayPluralMap.insert_or_assign(ElementTypes::eEllipseElement,
                                                "Ellipses");
  elementTypesDisplayPluralMap.insert_or_assign(ElementTypes::eEllipsoidElement,
                                                "Ellipsoids");
  elementTypesDisplayPluralMap.insert_or_assign(
      ElementTypes::eHyperboloidElement, "Hyperboloids");
  elementTypesDisplayPluralMap.insert_or_assign(ElementTypes::eLineElement,
                                                "Lines");
  elementTypesDisplayPluralMap.insert_or_assign(ElementTypes::eNurbsElement,
                                                "Nurbs");
  elementTypesDisplayPluralMap.insert_or_assign(
      ElementTypes::eParaboloidElement, "Paraboloids");
  elementTypesDisplayPluralMap.insert_or_assign(ElementTypes::ePlaneElement,
                                                "Planes");
  elementTypesDisplayPluralMap.insert_or_assign(ElementTypes::ePointElement,
                                                "Points");
  elementTypesDisplayPluralMap.insert_or_assign(
      ElementTypes::ePointCloudElement, "Point Clouds");
  elementTypesDisplayPluralMap.insert_or_assign(
      ElementTypes::eScalarEntityAngleElement, "Angles");
  elementTypesDisplayPluralMap.insert_or_assign(
      ElementTypes::eScalarEntityDistanceElement, "Distances");
  elementTypesDisplayPluralMap.insert_or_assign(
      ElementTypes::eScalarEntityMeasurementSeriesElement,
      "Measurement Series");
  elementTypesDisplayPluralMap.insert_or_assign(
      ElementTypes::eScalarEntityTemperatureElement, "Temperatures");
  elementTypesDisplayPluralMap.insert_or_assign(
      ElementTypes::eSlottedHoleElement, "Slotted Holes");
  elementTypesDisplayPluralMap.insert_or_assign(ElementTypes::eSphereElement,
                                                "Spheres");
  elementTypesDisplayPluralMap.insert_or_assign(ElementTypes::eTorusElement,
                                                "Tori");
  elementTypesDisplayPluralMap.insert_or_assign(ElementTypes::eDirectionElement,
                                                "Directions");
  elementTypesDisplayPluralMap.insert_or_assign(ElementTypes::ePositionElement,
                                                "Positions");
  elementTypesDisplayPluralMap.insert_or_assign(ElementTypes::eRadiusElement,
                                                "Radii");
  elementTypesDisplayPluralMap.insert_or_assign(
      ElementTypes::eCoordinateSystemElement, "Coordinate Systems");
  elementTypesDisplayPluralMap.insert_or_assign(ElementTypes::eStationElement,
                                                "Stations");
  elementTypesDisplayPluralMap.insert_or_assign(
      ElementTypes::eTrafoParamElement, "Transformation Parameters");
  elementTypesDisplayPluralMap.insert_or_assign(
      ElementTypes::eObservationElement, "Observations");
  elementTypesDisplayPluralMap.insert_or_assign(
      ElementTypes::eReadingCartesianElement, "Cartesian Readings");
  elementTypesDisplayPluralMap.insert_or_assign(
      ElementTypes::eReadingPolarElement, "Polar Readings");
  elementTypesDisplayPluralMap.insert_or_assign(
      ElementTypes::eReadingDistanceElement, "Distance Readings");
  elementTypesDisplayPluralMap.insert_or_assign(
      ElementTypes::eReadingDirectionElement, "Direction Readings");
  elementTypesDisplayPluralMap.insert_or_assign(
      ElementTypes::eReadingTemperatureElement, "Temperature Readings");
  elementTypesDisplayPluralMap.insert_or_assign(
      ElementTypes::eReadingLevelElement, "Level Readings");
  elementTypesDisplayPluralMap.insert_or_assign(
      ElementTypes::ePointCollectionElement, "Point Collections");

  // featureTypesDisplayMap
  featureTypesDisplayMap.insert_or_assign(FeatureTypes::eCircleFeature,
                                          "Circle");
  featureTypesDisplayMap.insert_or_assign(FeatureTypes::eConeFeature, "Cone");
  featureTypesDisplayMap.insert_or_assign(FeatureTypes::eCylinderFeature,
                                          "Cylinder");
  featureTypesDisplayMap.insert_or_assign(FeatureTypes::eEllipseFeature,
                                          "Ellipse");
  featureTypesDisplayMap.insert_or_assign(FeatureTypes::eEllipsoidFeature,
                                          "Ellipsoid");
  featureTypesDisplayMap.insert_or_assign(FeatureTypes::eHyperboloidFeature,
                                          "Hyperboloid");
  featureTypesDisplayMap.insert_or_assign(FeatureTypes::eLineFeature, "Line");
  featureTypesDisplayMap.insert_or_assign(FeatureTypes::eNurbsFeature, "Nurbs");
  featureTypesDisplayMap.insert_or_assign(FeatureTypes::eParaboloidFeature,
                                          "Paraboloid");
  featureTypesDisplayMap.insert_or_assign(FeatureTypes::ePlaneFeature, "Plane");
  featureTypesDisplayMap.insert_or_assign(FeatureTypes::ePointFeature, "Point");
  featureTypesDisplayMap.insert_or_assign(FeatureTypes::ePointCloudFeature,
                                          "Point Cloud");
  featureTypesDisplayMap.insert_or_assign(
      FeatureTypes::eScalarEntityAngleFeature, "Angle");
  featureTypesDisplayMap.insert_or_assign(
      FeatureTypes::eScalarEntityDistanceFeature, "Distance");
  featureTypesDisplayMap.insert_or_assign(
      FeatureTypes::eScalarEntityMeasurementSeriesFeature,
      "Measurement Series");
  featureTypesDisplayMap.insert_or_assign(
      FeatureTypes::eScalarEntityTemperatureFeature, "Temperature");
  featureTypesDisplayMap.insert_or_assign(FeatureTypes::eSlottedHoleFeature,
                                          "Slotted Hole");
  featureTypesDisplayMap.insert_or_assign(FeatureTypes::eSphereFeature,
                                          "Sphere");
  featureTypesDisplayMap.insert_or_assign(FeatureTypes::eTorusFeature, "Torus");
  featureTypesDisplayMap.insert_or_assign(
      FeatureTypes::eCoordinateSystemFeature, "Coordinate System");
  featureTypesDisplayMap.insert_or_assign(FeatureTypes::eStationFeature,
                                          "Station");
  featureTypesDisplayMap.insert_or_assign(FeatureTypes::eTrafoParamFeature,
                                          "Transformation Parameters");
  featureTypesDisplayMap.insert_or_assign(FeatureTypes::eLevelFeature, "Level");
  featureTypesDisplayMap.insert_or_assign(FeatureTypes::eGroupFeature, "Group");
  featureTypesDisplayMap.insert_or_assign(FeatureTypes::ePointGroupFeature,
                                          "Point Group");

  // geometryTypesDisplayMap
  geometryTypesDisplayMap.insert_or_assign(GeometryTypes::eCircleGeometry,
                                           "Circle");
  geometryTypesDisplayMap.insert_or_assign(GeometryTypes::eConeGeometry,
                                           "Cone");
  geometryTypesDisplayMap.insert_or_assign(GeometryTypes::eCylinderGeometry,
                                           "Cylinder");
  geometryTypesDisplayMap.insert_or_assign(GeometryTypes::eEllipseGeometry,
                                           "Ellipse");
  geometryTypesDisplayMap.insert_or_assign(GeometryTypes::eEllipsoidGeometry,
                                           "Ellipsoid");
  geometryTypesDisplayMap.insert_or_assign(GeometryTypes::eHyperboloidGeometry,
                                           "Hyperboloid");
  geometryTypesDisplayMap.insert_or_assign(GeometryTypes::eLineGeometry,
                                           "Line");
  geometryTypesDisplayMap.insert_or_assign(GeometryTypes::eNurbsGeometry,
                                           "Nurbs");
  geometryTypesDisplayMap.insert_or_assign(GeometryTypes::eParaboloidGeometry,
                                           "Paraboloid");
  geometryTypesDisplayMap.insert_or_assign(GeometryTypes::ePlaneGeometry,
                                           "Plane");
  geometryTypesDisplayMap.insert_or_assign(GeometryTypes::ePointGeometry,
                                           "Point");
  geometryTypesDisplayMap.insert_or_assign(GeometryTypes::ePointCloudGeometry,
                                           "Point Cloud");
  geometryTypesDisplayMap.insert_or_assign(
      GeometryTypes::eScalarEntityAngleGeometry, "Angle");
  geometryTypesDisplayMap.insert_or_assign(
      GeometryTypes::eScalarEntityDistanceGeometry, "Distance");
  geometryTypesDisplayMap.insert_or_assign(
      GeometryTypes::eScalarEntityMeasurementSeriesGeometry,
      "Measurement Series");
  geometryTypesDisplayMap.insert_or_assign(
      GeometryTypes::eScalarEntityTemperatureGeometry, "Temperature");
  geometryTypesDisplayMap.insert_or_assign(GeometryTypes::eSlottedHoleGeometry,
                                           "Slotted Hole");
  geometryTypesDisplayMap.insert_or_assign(GeometryTypes::eSphereGeometry,
                                           "Sphere");
  geometryTypesDisplayMap.insert_or_assign(GeometryTypes::eTorusGeometry,
                                           "Torus");
  geometryTypesDisplayMap.insert_or_assign(GeometryTypes::ePlaneLevelGeometry,
                                           "Level");
}

const Tables& tables() {
  static const Tables instance;
  return instance;
}

}  // namespace internal

// #######################################
// global functions to query element types
// #######################################

/*!
 * \brief getAvailableElementTypes
 * \return
 */
std::vector<ElementTypes> getAvailableElementTypes() {
  return oi::keys(internal::tables().elementTypesMap);
}

/*!
 * \brief getAvailableFeatureTypes
 * \return
 */
std::vector<FeatureTypes> getAvailableFeatureTypes() {
  return oi::keys(internal::tables().featureTypesMap);
}

/*!
 * \brief getAvailableGeometryTypes
 * \return
 */
std::vector<GeometryTypes> getAvailableGeometryTypes() {
  return oi::keys(internal::tables().geometryTypesMap);
}

/*!
 * \brief getElementTypeName
 * \param type
 * \return
 */
const std::string& getElementTypeName(const ElementTypes& type) {
  // get the corresponding element name
  if (internal::tables().elementTypesMap.contains(type)) {
    return internal::tables().elementTypesMap.at(type);
  }
  return internal::undefined;
}

/*!
 * \brief getElementTypePluralName
 * \param type
 * \return
 */
const std::string& getElementTypePluralName(const ElementTypes& type) {
  // get the corresponding element name
  if (internal::tables().elementTypesPluralMap.contains(type)) {
    return internal::tables().elementTypesPluralMap.at(type);
  }
  return internal::undefined;
}

/*!
 * \brief getElementTypeEnum
 * \param name
 * \return
 */
ElementTypes getElementTypeEnum(const std::string& name) {
  // get the corresponding element enum value
  ElementTypes type = oi::key(internal::tables().elementTypesMap, name,
                              ElementTypes::eUndefinedElement);
  if (type == ElementTypes::eUndefinedElement) {
    type = oi::key(internal::tables().elementTypesPluralMap, name,
                   ElementTypes::eUndefinedElement);
    ;
  }

  return type;
}

/*!
 * \brief getElementTypeEnum
 * Converts FeatureTypes to ElementTypes
 * \param type
 * \return
 */
ElementTypes getElementTypeEnum(const FeatureTypes& type) {
  switch (type) {
    case FeatureTypes::eCircleFeature:
      return ElementTypes::eCircleElement;
    case FeatureTypes::eConeFeature:
      return ElementTypes::eConeElement;
    case FeatureTypes::eCylinderFeature:
      return ElementTypes::eCylinderElement;
    case FeatureTypes::eEllipseFeature:
      return ElementTypes::eEllipseElement;
    case FeatureTypes::eEllipsoidFeature:
      return ElementTypes::eEllipsoidElement;
    case FeatureTypes::eHyperboloidFeature:
      return ElementTypes::eHyperboloidElement;
    case FeatureTypes::eLineFeature:
      return ElementTypes::eLineElement;
    case FeatureTypes::eNurbsFeature:
      return ElementTypes::eNurbsElement;
    case FeatureTypes::eParaboloidFeature:
      return ElementTypes::eParaboloidElement;
    case FeatureTypes::ePlaneFeature:
      return ElementTypes::ePlaneElement;
    case FeatureTypes::ePointFeature:
      return ElementTypes::ePointElement;
    case FeatureTypes::ePointCloudFeature:
      return ElementTypes::ePointCloudElement;
    case FeatureTypes::ePointGroupFeature:
      // both a point cloud and a point group are collections of points, so
      // a function needing ePointCollectionElement accepts either. The
      // cloud keeps its own more specific element type for consumers that
      // genuinely need bulk scan data rather than any collection.
      return ElementTypes::ePointCollectionElement;
    case FeatureTypes::eScalarEntityAngleFeature:
      return ElementTypes::eScalarEntityAngleElement;
    case FeatureTypes::eScalarEntityDistanceFeature:
      return ElementTypes::eScalarEntityDistanceElement;
    case FeatureTypes::eScalarEntityMeasurementSeriesFeature:
      return ElementTypes::eScalarEntityMeasurementSeriesElement;
    case FeatureTypes::eScalarEntityTemperatureFeature:
      return ElementTypes::eScalarEntityTemperatureElement;
    case FeatureTypes::eSlottedHoleFeature:
      return ElementTypes::eSlottedHoleElement;
    case FeatureTypes::eSphereFeature:
      return ElementTypes::eSphereElement;
    case FeatureTypes::eTorusFeature:
      return ElementTypes::eTorusElement;
    case FeatureTypes::eCoordinateSystemFeature:
      return ElementTypes::eCoordinateSystemElement;
    case FeatureTypes::eStationFeature:
      return ElementTypes::eStationElement;
    case FeatureTypes::eTrafoParamFeature:
      return ElementTypes::eTrafoParamElement;
    default:
      return ElementTypes::eUndefinedElement;
  }
}

/*!
 * \brief getElementTypeEnum
 * convert ReadingTypes to ElementTypes
 * \param type
 * \return
 */
ElementTypes getElementTypeEnum(const ReadingTypes& type) {
  switch (type) {
    case ReadingTypes::eCartesianReading:
      return ElementTypes::eReadingCartesianElement;
    case ReadingTypes::eDistanceReading:
      return ElementTypes::eReadingDistanceElement;
    case ReadingTypes::ePolarReading:
      return ElementTypes::eReadingPolarElement;
    case ReadingTypes::eDirectionReading:
      return ElementTypes::eReadingDirectionElement;
    case ReadingTypes::eTemperatureReading:
      return ElementTypes::eReadingTemperatureElement;
    case ReadingTypes::eLevelReading:
      return ElementTypes::eReadingLevelElement;
    default:
      break;
  }

  return ElementTypes::eUndefinedElement;
}

/*!
 * \brief getFeatureTypeName
 * \param type
 * \return
 */
const std::string& getFeatureTypeName(const FeatureTypes& type) {
  // get the corresponding feature name
  if (internal::tables().featureTypesMap.contains(type)) {
    return internal::tables().featureTypesMap.at(type);
  }
  return internal::undefined;
}

/*!
 * \brief getFeatureTypeEnum
 * \param name
 * \return
 */
FeatureTypes getFeatureTypeEnum(const std::string& name) {
  // get the corresponding feature enum value
  return oi::key(internal::tables().featureTypesMap, name,
                 FeatureTypes::eUndefinedFeature);
}

/*!
 * \brief getGeometryTypeName
 * \param type
 * \return
 */
const std::string& getGeometryTypeName(const GeometryTypes& type) {
  // get the corresponding geometry name
  if (internal::tables().geometryTypesMap.contains(type)) {
    return internal::tables().geometryTypesMap.at(type);
  }
  return internal::undefined;
}

/*!
 * \brief getGeometryTypeEnum
 * \param name
 * \return
 */
GeometryTypes getGeometryTypeEnum(const std::string& name) {
  // get the corresponding geometry enum value
  return oi::key(internal::tables().geometryTypesMap, name,
                 GeometryTypes::eUndefinedGeometry);
}

/*!
 * \brief getGeometryTypeEnum
 * \param type
 * \return
 */
GeometryTypes getGeometryTypeEnum(const FeatureTypes& type) {
  switch (type) {
    case FeatureTypes::eCircleFeature:
      return GeometryTypes::eCircleGeometry;
    case FeatureTypes::eConeFeature:
      return GeometryTypes::eConeGeometry;
    case FeatureTypes::eCylinderFeature:
      return GeometryTypes::eCylinderGeometry;
    case FeatureTypes::eEllipseFeature:
      return GeometryTypes::eEllipseGeometry;
    case FeatureTypes::eEllipsoidFeature:
      return GeometryTypes::eEllipsoidGeometry;
    case FeatureTypes::eHyperboloidFeature:
      return GeometryTypes::eHyperboloidGeometry;
    case FeatureTypes::eLineFeature:
      return GeometryTypes::eLineGeometry;
    case FeatureTypes::eNurbsFeature:
      return GeometryTypes::eNurbsGeometry;
    case FeatureTypes::eParaboloidFeature:
      return GeometryTypes::eParaboloidGeometry;
    case FeatureTypes::ePlaneFeature:
      return GeometryTypes::ePlaneGeometry;
    case FeatureTypes::ePointFeature:
      return GeometryTypes::ePointGeometry;
    case FeatureTypes::ePointCloudFeature:
      return GeometryTypes::ePointCloudGeometry;
    case FeatureTypes::eScalarEntityAngleFeature:
      return GeometryTypes::eScalarEntityAngleGeometry;
    case FeatureTypes::eScalarEntityDistanceFeature:
      return GeometryTypes::eScalarEntityDistanceGeometry;
    case FeatureTypes::eScalarEntityMeasurementSeriesFeature:
      return GeometryTypes::eScalarEntityMeasurementSeriesGeometry;
    case FeatureTypes::eScalarEntityTemperatureFeature:
      return GeometryTypes::eScalarEntityTemperatureGeometry;
    case FeatureTypes::eSlottedHoleFeature:
      return GeometryTypes::eSlottedHoleGeometry;
    case FeatureTypes::eSphereFeature:
      return GeometryTypes::eSphereGeometry;
    case FeatureTypes::eTorusFeature:
      return GeometryTypes::eTorusGeometry;
    default:
      return GeometryTypes::eUndefinedGeometry;
  }
}

/*!
 * \brief getIsFeature
 * \param type
 * \return
 */
/*!
 * \brief getElementTypeDisplayName
 * The user-facing name of an element type.
 * Display name only - never persist this or feed it to a reverse lookup.
 * \param type
 * \return
 */
const std::string& getElementTypeDisplayName(const ElementTypes& type) {
  if (internal::tables().elementTypesDisplayMap.contains(type)) {
    return internal::tables().elementTypesDisplayMap.at(type);
  }
  return internal::undefined;
}

/*!
 * \brief getElementTypeDisplayPluralName
 * The user-facing plural name of an element type.
 * Display name only - never persist this or feed it to a reverse lookup.
 * \param type
 * \return
 */
const std::string& getElementTypeDisplayPluralName(const ElementTypes& type) {
  if (internal::tables().elementTypesDisplayPluralMap.contains(type)) {
    return internal::tables().elementTypesDisplayPluralMap.at(type);
  }
  return internal::undefined;
}

/*!
 * \brief getFeatureTypeDisplayName
 * The user-facing name of a feature type.
 * Display name only - never persist this or feed it to a reverse lookup.
 * \param type
 * \return
 */
const std::string& getFeatureTypeDisplayName(const FeatureTypes& type) {
  if (internal::tables().featureTypesDisplayMap.contains(type)) {
    return internal::tables().featureTypesDisplayMap.at(type);
  }
  return internal::undefined;
}

/*!
 * \brief getGeometryTypeDisplayName
 * The user-facing name of a geometry type.
 * Display name only - never persist this or feed it to a reverse lookup.
 * \param type
 * \return
 */
const std::string& getGeometryTypeDisplayName(const GeometryTypes& type) {
  if (internal::tables().geometryTypesDisplayMap.contains(type)) {
    return internal::tables().geometryTypesDisplayMap.at(type);
  }
  return internal::undefined;
}

bool getIsFeature(const ElementTypes& type) {
  if (type == ElementTypes::eObservationElement ||
      type == ElementTypes::eReadingCartesianElement ||
      type == ElementTypes::eReadingPolarElement ||
      type == ElementTypes::eReadingDistanceElement ||
      type == ElementTypes::eReadingDirectionElement ||
      type == ElementTypes::eReadingTemperatureElement ||
      type == ElementTypes::eReadingLevelElement ||
      type == ElementTypes::eDirectionElement ||
      type == ElementTypes::ePositionElement ||
      type == ElementTypes::eRadiusElement ||
      type == ElementTypes::eUndefinedElement) {
    return false;
  }
  return true;
}

/*!
 * \brief getIsReading
 * \param type
 * \return
 */
bool getIsReading(const ElementTypes& type) {
  if (type == ElementTypes::eReadingCartesianElement ||
      type == ElementTypes::eReadingDirectionElement ||
      type == ElementTypes::eReadingDistanceElement ||
      type == ElementTypes::eReadingLevelElement ||
      type == ElementTypes::eReadingPolarElement ||
      type == ElementTypes::eReadingTemperatureElement) {
    return true;
  }
  return false;
}

/*!
 * \brief getIsObservation
 * \param type
 * \return
 */
bool getIsObservation(const ElementTypes& type) {
  if (type == ElementTypes::eObservationElement) {
    return true;
  }
  return false;
}

/*!
 * \brief getIsGeometry
 * \param type
 * \return
 */
bool getIsGeometry(const FeatureTypes& type) {
  if (type == FeatureTypes::eCoordinateSystemFeature ||
      type == FeatureTypes::eStationFeature ||
      type == FeatureTypes::eTrafoParamFeature
      // Stage 7e: Group is a Feature, not a Geometry (domain-model.md
      // S5 - "it has no position, solves to nothing"). Every caller of
      // this function uses it to guard a feature->getGeometry() access,
      // which is null for a Group/PointGroup and would otherwise crash
      // the moment a Group actually reaches one of those call sites -
      // found via OiJob::checkAndSetUpNewFeature's identical getGeometry()
      // dereference while adding the first real Group to a job in
      // tst_project_exchange.cpp, not a hypothetical.
      || type == FeatureTypes::eGroupFeature ||
      type == FeatureTypes::ePointGroupFeature) {
    return false;
  }
  return true;
}

/*!
 * \brief getIsScalarEntity
 * \param type
 * \return
 */
bool getIsScalarEntity(const FeatureTypes& type) {
  if (type == FeatureTypes::eScalarEntityAngleFeature ||
      type == FeatureTypes::eScalarEntityDistanceFeature ||
      type == FeatureTypes::eScalarEntityMeasurementSeriesFeature ||
      type == FeatureTypes::eScalarEntityTemperatureFeature) {
    return true;
  }
  return false;
}

/*!
 * \brief getElementTypeIconPath
 * \param type
 * \return
 */
const std::string& getElementTypeIconPath(const ElementTypes& type) {
  // get the corresponding element type icon
  if (internal::tables().elementTypeIcons.contains(type)) {
    return internal::tables().elementTypeIcons.at(type);
  }
  return internal::undefined;
}

/*!
 * \brief getFeatureTypeIconPath
 * \param type
 * \return
 */
const std::string& getFeatureTypeIconPath(const FeatureTypes& type) {
  // get the corresponding feature type icon
  if (internal::tables().featureTypeIcons.contains(type)) {
    return internal::tables().featureTypeIcons.at(type);
  }
  return internal::undefined;
}

/*!
 * \brief getReadingTypeName
 * \param type
 * \return
 */
const std::string& getReadingTypeName(const ReadingTypes& type) {
  // get the corresponding reading name
  if (internal::tables().readingTypesMap.contains(type)) {
    return internal::tables().readingTypesMap.at(type);
  }
  return internal::undefined;
}

/*!
 * \brief getReadingTypeEnum
 * \param name
 * \return
 */
ReadingTypes getReadingTypeEnum(const std::string& name) {
  // get the corresponding reading enum value
  return oi::key(internal::tables().readingTypesMap, name,
                 ReadingTypes::eUndefinedReading);
}

/*!
 * \brief getFeatureDisplayAttributes
 * Returns a list of enum values (feature attributes and trafo param attributes)
 * \return
 */
const std::vector<int>& getFeatureDisplayAttributes() {
  return internal::tables().featureDisplayAttributes;
}

/*!
 * \brief getObservationDisplayAttributes
 * \return
 */
const std::vector<ObservationDisplayAttributes>&
getObservationDisplayAttributes() {
  return internal::tables().observationDisplayAttributes;
}

/*!
 * \brief getReadingDisplayAttributes
 * \return
 */
const std::vector<ReadingDisplayAttributes>& getReadingDisplayAttributes() {
  return internal::tables().readingDisplayAttributes;
}

/*!
 * \brief getIsFeatureDisplayAttribute
 * Returns true if the given attribute is a feature attribute
 * \param attr
 * \return
 */
bool getIsFeatureDisplayAttribute(const int& attr) {
  return (attr >= eFeatureDisplayType && attr <= eFeatureDisplayStDev) ||
         (attr >= eFeatureDisplayMeasurementConfig &&
          attr <= eFeatureDisplayIsActual) ||
         (attr >= eFeatureDisplayX && attr <= eFeatureDisplayFormError) ||
         (attr >= eFeatureDisplayExpansionOriginX &&
          attr <= eFeatureDisplayExpansionOriginZ);
}

/*!
 * \brief getIsTrafoParamDisplayAttribute
 * Returns true if the given attribute is a trafo param attribute
 * \param attr
 * \return
 */
bool getIsTrafoParamDisplayAttribute(const int& attr) {
  return (attr >= eTrafoParamDisplayType && attr <= eTrafoParamDisplayStDev) ||
         (attr >= eTrafoParamDisplayStartSystem &&
          attr <= eTrafoParamDisplayIsDatumTransformation);
}

/*!
 * \brief getDisplayAttributeName
 * Returns the name of the attribute represented by the given enum value
 * \param attr
 * \return
 */
const std::string& getFeatureDisplayAttributeName(const int& attr) {
  if (getIsFeatureDisplayAttribute(attr)) {  // feature display attribute
    return internal::nameOrUndefined(
        internal::tables().featureDisplayAttributesMap,
        static_cast<FeatureDisplayAttributes>(attr));
  } else if (getIsTrafoParamDisplayAttribute(
                 attr)) {  // trafo param display attribute
    return internal::nameOrUndefined(
        internal::tables().trafoParamDisplayAttributesMap,
        static_cast<TrafoParamDisplayAttributes>(attr));
  }

  return internal::undefined;
}

/*!
 * \brief getDisplayAttributeName
 * \param attr
 * \return
 */
const std::string& getFeatureDisplayAttributeName(
    const FeatureDisplayAttributes& attr) {
  return internal::tables().featureDisplayAttributesMap.at(attr);
}

/*!
 * \brief getDisplayAttributeName
 * \param attr
 * \return
 */
const std::string& getFeatureDisplayAttributeName(
    const TrafoParamDisplayAttributes& attr) {
  return internal::tables().trafoParamDisplayAttributesMap.at(attr);
}

/*!
 * \brief getReadingDisplayAttributeVisibility
 * \param attr
 * \param type
 * \return
 */
bool getReadingDisplayAttributeVisibility(const ReadingDisplayAttributes& attr,
                                          const ReadingTypes& type) {
  switch (attr) {
    case eReadingDisplayAzimuth:
      if (type != ReadingTypes::ePolarReading &&
          type != ReadingTypes::eDirectionReading) {
        return false;
      }
      break;
    case eReadingDisplayZenith:
      if (type != ReadingTypes::ePolarReading &&
          type != ReadingTypes::eDirectionReading) {
        return false;
      }
      break;
    case eReadingDisplayDistance:
      if (type != ReadingTypes::ePolarReading &&
          type != ReadingTypes::eDistanceReading) {
        return false;
      }
      break;
    case eReadingDisplayX:
      if (type != ReadingTypes::eCartesianReading &&
          type != ReadingTypes::eCartesianReading6D) {
        return false;
      }
      break;
    case eReadingDisplayY:
      if (type != ReadingTypes::eCartesianReading &&
          type != ReadingTypes::eCartesianReading6D) {
        return false;
      }
      break;
    case eReadingDisplayZ:
      if (type != ReadingTypes::eCartesianReading &&
          type != ReadingTypes::eCartesianReading6D) {
        return false;
      }
      break;
    case eReadingDisplayI:
      if (type != ReadingTypes::eLevelReading &&
          type != ReadingTypes::eCartesianReading6D) {
        return false;
      }
      break;
    case eReadingDisplayJ:
      if (type != ReadingTypes::eLevelReading &&
          type != ReadingTypes::eCartesianReading6D) {
        return false;
      }
      break;
    case eReadingDisplayK:
      if (type != ReadingTypes::eLevelReading &&
          type != ReadingTypes::eCartesianReading6D) {
        return false;
      }
      break;
    case eReadingDisplayTemperature:
      if (type != ReadingTypes::eTemperatureReading) {
        return false;
      }
      break;
    case eReadingDisplaySigmaAzimuth:
      if (type != ReadingTypes::ePolarReading &&
          type != ReadingTypes::eDirectionReading) {
        return false;
      }
      break;
    case eReadingDisplaySigmaZenith:
      if (type != ReadingTypes::ePolarReading &&
          type != ReadingTypes::eDirectionReading) {
        return false;
      }
      break;
    case eReadingDisplaySigmaDistance:
      if (type != ReadingTypes::ePolarReading &&
          type != ReadingTypes::eDistanceReading) {
        return false;
      }
      break;
    case eReadingDisplaySigmaX:
      if (type != ReadingTypes::eCartesianReading &&
          type != ReadingTypes::eCartesianReading6D) {
        return false;
      }
      break;
    case eReadingDisplaySigmaY:
      if (type != ReadingTypes::eCartesianReading &&
          type != ReadingTypes::eCartesianReading6D) {
        return false;
      }
      break;
    case eReadingDisplaySigmaZ:
      if (type != ReadingTypes::eCartesianReading &&
          type != ReadingTypes::eCartesianReading6D) {
        return false;
      }
      break;
    case eReadingDisplaySigmaI:
      if (type != ReadingTypes::eLevelReading) {
        return false;
      }
      break;
    case eReadingDisplaySigmaJ:
      if (type != ReadingTypes::eLevelReading) {
        return false;
      }
      break;
    case eReadingDisplaySigmaK:
      if (type != ReadingTypes::eLevelReading) {
        return false;
      }
      break;
    case eReadingDisplaySigmaTemperature:
      if (type != ReadingTypes::eTemperatureReading) {
        return false;
      }
      break;
    default:
      break;
  }

  return true;
}

/*!
 * \brief getObservationDisplayAttributesName
 * \param attr
 * \return
 */
const std::string& getObservationDisplayAttributesName(
    const ObservationDisplayAttributes& attr) {
  return internal::tables().observationDisplayAttributesMap.at(attr);
}

/*!
 * \brief getReadingDisplayAttributeName
 * \param attr
 * \return
 */
const std::string& getReadingDisplayAttributeName(
    const ReadingDisplayAttributes& attr) {
  return internal::tables().readingDisplayAttributesMap.at(attr);
}

/*!
 * \brief getObservationDisplayAttributeEnum
 * \param name
 * \return
 */
ObservationDisplayAttributes getObservationDisplayAttributeEnum(
    const std::string& name) {
  // get the corresponding observation enum value
  return oi::key(internal::tables().observationDisplayAttributesMap, name,
                 eObservationDisplayId);
}

/*!
 * \brief getReadingDisplayAttributeEnum
 * \param name
 * \return
 */
ReadingDisplayAttributes getReadingDisplayAttributeEnum(
    const std::string& name) {
  // get the corresponding reading enum value
  return oi::key(internal::tables().readingDisplayAttributesMap, name,
                 eReadingDisplayId);
}

/*!
 * \brief convertToDefault
 * Convert the given value of unit type to the default unit
 * \param value
 * \param type
 * \return
 */
double convertToDefault(const double& value, const UnitType& type) {
  switch (type) {
    case UnitType::eUnitMilliMeter:
      return value * (1.0 / kMillimetersPerMeter);
    case UnitType::eUnitInch:
      return value * (1.0 / kInchesPerMeter);
    case UnitType::eUnitMilliRadians:
      return value * (1.0 / kMilliradiansPerRadian);
    case UnitType::eUnitGon:
      return value * (1.0 / kRhoGon);
    case UnitType::eUnitArcSeconds:
      return value * (1.0 / kArcSecondsPerRadian);
    case UnitType::eUnitDecimalDegree:
      return value * (1.0 / kRhoDegree);
    case UnitType::eUnitFahrenheit:
      return (value - kFahrenheitAtZeroCelsius) / kFahrenheitPerCelsius;
    case UnitType::eUnitKelvin:
      return value - kKelvinAtZeroCelsius;
    default:
      return value;
  }
}

/*!
 * \brief convertFromDefault
 * Convert the given value of default unit to the specified unit type
 * \param value
 * \param type
 * \return
 */
double convertFromDefault(const double& value, const UnitType& type) {
  switch (type) {
    case UnitType::eUnitMilliMeter:
      return value * kMillimetersPerMeter;
    case UnitType::eUnitInch:
      return value * kInchesPerMeter;
    case UnitType::eUnitMilliRadians:
      return value * kMilliradiansPerRadian;
    case UnitType::eUnitGon:
      return value * kRhoGon;
    case UnitType::eUnitArcSeconds:
      return value * kArcSecondsPerRadian;
    case UnitType::eUnitDecimalDegree:
      return value * kRhoDegree;
    case UnitType::eUnitFahrenheit:
      return value * kFahrenheitPerCelsius + kFahrenheitAtZeroCelsius;
    case UnitType::eUnitKelvin:
      return value + kKelvinAtZeroCelsius;
    default:
      return value;
  }
}

/*!
 * \brief getUnitTypeName
 * \param type
 * \return
 */
const std::string& getUnitTypeName(const UnitType& type) {
  // get the corresponding reading name
  if (internal::tables().unitTypesMap.contains(type)) {
    return internal::tables().unitTypesMap.at(type);
  }
  return internal::undefined;
}

/*!
 * \brief getUnitTypeEnum
 * \param name
 * \return
 */
UnitType getUnitTypeEnum(const std::string& name) {
  // get the corresponding reading enum value
  return oi::key(internal::tables().unitTypesMap, name, UnitType::eNoUnit);
}

/*!
 * \brief almostEqual
 * \param a
 * \param b
 * \param digits
 * \return
 */
bool almostEqual(const double& a, const double& b, const int& digits) {
  // calculate difference
  double diff = std::abs(a - b);

  // calculate epsilon
  double epsilon = 1.0;
  for (int i = 0; i < digits; i++) {
    epsilon = epsilon / kDecimalBase;
  }

  // check difference
  return diff <= epsilon;
}

/*!
 * \brief getAvailableSensorFunctions
 * \return
 */
std::vector<SensorFunctions> getAvailableSensorFunctions() {
  return oi::keys(internal::tables().sensorFunctionsMap);
}

/*!
 * \brief getSensorFunctionName
 * \param type
 * \return
 */
const std::string& getSensorFunctionName(const SensorFunctions& type) {
  // get the corresponding sensor function name
  if (internal::tables().sensorFunctionsMap.contains(type)) {
    return internal::tables().sensorFunctionsMap.at(type);
  }
  return internal::undefined;
}

/*!
 * \brief getSensorFunctionEnum
 * \param name
 * \return
 */
SensorFunctions getSensorFunctionEnum(const std::string& name) {
  // get the corresponding sensor function enum value
  return oi::key(internal::tables().sensorFunctionsMap, name,
                 SensorFunctions::eInitialize);
}

/*!
 * \brief getAvailableToolTypes
 * \return
 */
std::vector<ToolTypes> getAvailableToolTypes() {
  return oi::keys(internal::tables().toolTypesMap);
}

/*!
 * \brief getToolTypeName
 * \param type
 * \return
 */
const std::string& getToolTypeName(const ToolTypes& type) {
  // get the corresponding tool type name
  if (internal::tables().toolTypesMap.contains(type)) {
    return internal::tables().toolTypesMap.at(type);
  }
  return internal::undefined;
}

/*!
 * \brief getToolTypeEnum
 * \param name
 * \return
 */
ToolTypes getToolTypeEnum(const std::string& name) {
  // get the corresponding tool type enum value
  return oi::key(internal::tables().toolTypesMap, name,
                 ToolTypes::eUnknownTool);
}

/*!
 * \brief getAvailableMessageTypes
 * \return
 */
std::vector<MessageTypes> getAvailableMessageTypes() {
  return oi::keys(internal::tables().messageTypesMap);
}

/*!
 * \brief getMessageTypeName
 * \param type
 * \return
 */
const std::string& getMessageTypeName(const MessageTypes& type) {
  // get the corresponding message type name
  if (internal::tables().messageTypesMap.contains(type)) {
    return internal::tables().messageTypesMap.at(type);
  }
  return internal::undefined;
}

/*!
 * \brief getMessageTypeEnum
 * \param name
 * \return
 */
MessageTypes getMessageTypeEnum(const std::string& name) {
  // get the corresponding message type enum value
  return oi::key(internal::tables().messageTypesMap, name,
                 MessageTypes::eInformationMessage);
}

/*!
 * \brief getAvailableUnknownParameters
 * \return
 */
std::vector<GeometryParameters> getAvailableGeometryParameters() {
  return oi::keys(internal::tables().geometryParametersMap);
}

/*!
 * \brief getUnknownParameterName
 * \param parameter
 * \return
 */
const std::string& getGeometryParameterName(
    const GeometryParameters& parameter) {
  // get the corresponding geometry parameter name
  if (internal::tables().geometryParametersMap.contains(parameter)) {
    return internal::tables().geometryParametersMap.at(parameter);
  }
  return internal::undefined;
}

/*!
 * \brief getUnknownParameterEnum
 * \param name
 * \return
 */
GeometryParameters getGeometryParameterEnum(const std::string& name) {
  // get the corresponding geometry parameter enum value
  return oi::key(internal::tables().geometryParametersMap, name,
                 GeometryParameters::eUnknownX);
}

/*!
 * \brief getUnknownParameters
 * \param type
 * \return
 */
std::vector<GeometryParameters> getGeometryParameters(
    const GeometryTypes& type) {
  std::vector<GeometryParameters> unknownParameters;

  switch (type) {
    case GeometryTypes::eCircleGeometry:
      unknownParameters.push_back(GeometryParameters::eUnknownX);
      unknownParameters.push_back(GeometryParameters::eUnknownY);
      unknownParameters.push_back(GeometryParameters::eUnknownZ);
      unknownParameters.push_back(GeometryParameters::eUnknownPrimaryI);
      unknownParameters.push_back(GeometryParameters::eUnknownPrimaryJ);
      unknownParameters.push_back(GeometryParameters::eUnknownPrimaryK);
      unknownParameters.push_back(GeometryParameters::eUnknownRadiusA);
      break;
    case GeometryTypes::eConeGeometry:
      unknownParameters.push_back(GeometryParameters::eUnknownX);
      unknownParameters.push_back(GeometryParameters::eUnknownY);
      unknownParameters.push_back(GeometryParameters::eUnknownZ);
      unknownParameters.push_back(GeometryParameters::eUnknownPrimaryI);
      unknownParameters.push_back(GeometryParameters::eUnknownPrimaryJ);
      unknownParameters.push_back(GeometryParameters::eUnknownPrimaryK);
      unknownParameters.push_back(GeometryParameters::eUnknownAperture);
      break;
    case GeometryTypes::eCylinderGeometry:
      unknownParameters.push_back(GeometryParameters::eUnknownX);
      unknownParameters.push_back(GeometryParameters::eUnknownY);
      unknownParameters.push_back(GeometryParameters::eUnknownZ);
      unknownParameters.push_back(GeometryParameters::eUnknownPrimaryI);
      unknownParameters.push_back(GeometryParameters::eUnknownPrimaryJ);
      unknownParameters.push_back(GeometryParameters::eUnknownPrimaryK);
      unknownParameters.push_back(GeometryParameters::eUnknownRadiusA);
      break;
    case GeometryTypes::eEllipseGeometry:
      unknownParameters.push_back(GeometryParameters::eUnknownX);
      unknownParameters.push_back(GeometryParameters::eUnknownY);
      unknownParameters.push_back(GeometryParameters::eUnknownZ);
      unknownParameters.push_back(GeometryParameters::eUnknownPrimaryI);
      unknownParameters.push_back(GeometryParameters::eUnknownPrimaryJ);
      unknownParameters.push_back(GeometryParameters::eUnknownPrimaryK);
      unknownParameters.push_back(GeometryParameters::eUnknownA);
      unknownParameters.push_back(GeometryParameters::eUnknownB);
      unknownParameters.push_back(GeometryParameters::eUnknownSecondaryI);
      unknownParameters.push_back(GeometryParameters::eUnknownSecondaryJ);
      unknownParameters.push_back(GeometryParameters::eUnknownSecondaryK);
      break;
    case GeometryTypes::eEllipsoidGeometry:
      unknownParameters.push_back(GeometryParameters::eUnknownX);
      unknownParameters.push_back(GeometryParameters::eUnknownY);
      unknownParameters.push_back(GeometryParameters::eUnknownZ);
      unknownParameters.push_back(GeometryParameters::eUnknownPrimaryI);
      unknownParameters.push_back(GeometryParameters::eUnknownPrimaryJ);
      unknownParameters.push_back(GeometryParameters::eUnknownPrimaryK);
      unknownParameters.push_back(GeometryParameters::eUnknownA);
      unknownParameters.push_back(GeometryParameters::eUnknownB);
      break;
    case GeometryTypes::eHyperboloidGeometry:
      unknownParameters.push_back(GeometryParameters::eUnknownX);
      unknownParameters.push_back(GeometryParameters::eUnknownY);
      unknownParameters.push_back(GeometryParameters::eUnknownZ);
      unknownParameters.push_back(GeometryParameters::eUnknownPrimaryI);
      unknownParameters.push_back(GeometryParameters::eUnknownPrimaryJ);
      unknownParameters.push_back(GeometryParameters::eUnknownPrimaryK);
      unknownParameters.push_back(GeometryParameters::eUnknownA);
      unknownParameters.push_back(GeometryParameters::eUnknownC);
      break;
    case GeometryTypes::eLineGeometry:
      unknownParameters.push_back(GeometryParameters::eUnknownX);
      unknownParameters.push_back(GeometryParameters::eUnknownY);
      unknownParameters.push_back(GeometryParameters::eUnknownZ);
      unknownParameters.push_back(GeometryParameters::eUnknownPrimaryI);
      unknownParameters.push_back(GeometryParameters::eUnknownPrimaryJ);
      unknownParameters.push_back(GeometryParameters::eUnknownPrimaryK);
      break;
    case GeometryTypes::eNurbsGeometry:
      break;
    case GeometryTypes::eParaboloidGeometry:
      unknownParameters.push_back(GeometryParameters::eUnknownX);
      unknownParameters.push_back(GeometryParameters::eUnknownY);
      unknownParameters.push_back(GeometryParameters::eUnknownZ);
      unknownParameters.push_back(GeometryParameters::eUnknownPrimaryI);
      unknownParameters.push_back(GeometryParameters::eUnknownPrimaryJ);
      unknownParameters.push_back(GeometryParameters::eUnknownPrimaryK);
      unknownParameters.push_back(GeometryParameters::eUnknownA);
      break;
    case GeometryTypes::ePlaneGeometry:
      unknownParameters.push_back(GeometryParameters::eUnknownX);
      unknownParameters.push_back(GeometryParameters::eUnknownY);
      unknownParameters.push_back(GeometryParameters::eUnknownZ);
      unknownParameters.push_back(GeometryParameters::eUnknownPrimaryI);
      unknownParameters.push_back(GeometryParameters::eUnknownPrimaryJ);
      unknownParameters.push_back(GeometryParameters::eUnknownPrimaryK);
      break;
    case GeometryTypes::ePointGeometry:
      unknownParameters.push_back(GeometryParameters::eUnknownX);
      unknownParameters.push_back(GeometryParameters::eUnknownY);
      unknownParameters.push_back(GeometryParameters::eUnknownZ);
      break;
    case GeometryTypes::ePointCloudGeometry:
      break;
    case GeometryTypes::eScalarEntityAngleGeometry:
      unknownParameters.push_back(GeometryParameters::eUnknownAngle);
      break;
    case GeometryTypes::eScalarEntityDistanceGeometry:
      unknownParameters.push_back(GeometryParameters::eUnknownDistance);
      break;
    case GeometryTypes::eScalarEntityMeasurementSeriesGeometry:
      unknownParameters.push_back(
          GeometryParameters::eUnknownMeasurementSeries);
      break;
    case GeometryTypes::eScalarEntityTemperatureGeometry:
      unknownParameters.push_back(GeometryParameters::eUnknownTemperature);
      break;
    case GeometryTypes::eSlottedHoleGeometry:
      unknownParameters.push_back(GeometryParameters::eUnknownX);
      unknownParameters.push_back(GeometryParameters::eUnknownY);
      unknownParameters.push_back(GeometryParameters::eUnknownZ);
      unknownParameters.push_back(GeometryParameters::eUnknownPrimaryI);
      unknownParameters.push_back(GeometryParameters::eUnknownPrimaryJ);
      unknownParameters.push_back(GeometryParameters::eUnknownPrimaryK);
      unknownParameters.push_back(GeometryParameters::eUnknownRadiusA);
      unknownParameters.push_back(GeometryParameters::eUnknownLength);
      unknownParameters.push_back(GeometryParameters::eUnknownSecondaryI);
      unknownParameters.push_back(GeometryParameters::eUnknownSecondaryJ);
      unknownParameters.push_back(GeometryParameters::eUnknownSecondaryK);
      break;
    case GeometryTypes::eSphereGeometry:
      unknownParameters.push_back(GeometryParameters::eUnknownX);
      unknownParameters.push_back(GeometryParameters::eUnknownY);
      unknownParameters.push_back(GeometryParameters::eUnknownZ);
      unknownParameters.push_back(GeometryParameters::eUnknownRadiusA);
      break;
    case GeometryTypes::eTorusGeometry:
      unknownParameters.push_back(GeometryParameters::eUnknownX);
      unknownParameters.push_back(GeometryParameters::eUnknownY);
      unknownParameters.push_back(GeometryParameters::eUnknownZ);
      unknownParameters.push_back(GeometryParameters::eUnknownPrimaryI);
      unknownParameters.push_back(GeometryParameters::eUnknownPrimaryJ);
      unknownParameters.push_back(GeometryParameters::eUnknownPrimaryK);
      unknownParameters.push_back(GeometryParameters::eUnknownRadiusA);
      unknownParameters.push_back(GeometryParameters::eUnknownRadiusB);
      break;
    default:
      break;
  }

  return unknownParameters;
}

/*!
 * \brief getAvailableTrafoParamParameters
 * \return
 */
std::vector<TrafoParamParameters> getAvailableTrafoParamParameters() {
  return oi::keys(internal::tables().trafoParamParametersMap);
}

/*!
 * \brief getTrafoParamParameterName
 * \param parameter
 * \return
 */
const std::string& getTrafoParamParameterName(
    const TrafoParamParameters& parameter) {
  // get the corresponding trafo param parameter name
  if (internal::tables().trafoParamParametersMap.contains(parameter)) {
    return internal::tables().trafoParamParametersMap.at(parameter);
  }
  return internal::undefined;
}

/*!
 * \brief getTrafoParamParameterEnum
 * \param name
 * \return
 */
TrafoParamParameters getTrafoParamParameterEnum(const std::string& name) {
  // get the corresponding trafo param parameter enum value
  return oi::key(internal::tables().trafoParamParametersMap, name,
                 TrafoParamParameters::eUnknownTX);
}

/*!
 * \brief getAvailableActualNominalFilters
 * \return
 */
std::vector<ActualNominalFilter> getAvailableActualNominalFilters() {
  return oi::keys(internal::tables().actualNominalFilterMap);
}

/*!
 * \brief getActualNominalFilterName
 * \param type
 * \return
 */
const std::string& getActualNominalFilterName(const ActualNominalFilter& type) {
  // get the corresponding actual nominal filter name
  if (internal::tables().actualNominalFilterMap.contains(type)) {
    return internal::tables().actualNominalFilterMap.at(type);
  }
  return internal::undefined;
}

/*!
 * \brief getActualNominalFilterEnum
 * \param name
 * \return
 */
ActualNominalFilter getActualNominalFilterEnum(const std::string& name) {
  // get the corresponding actual nominal filter enum value
  return oi::key(internal::tables().actualNominalFilterMap, name,
                 ActualNominalFilter::eFilterActualNominal);
}

/*!
 * \brief getMaterialName
 * \param material
 * \return
 */
const std::string& getMaterialName(const MaterialsTempComp& material) {
  // get the corresponding actual nominal filter name
  if (internal::tables().materialStringsMap.contains(material)) {
    return internal::tables().materialStringsMap.at(material);
  }
  return internal::undefined;
}

/*!
 * \brief getMaterialValue
 * \param material
 * \return
 */
double getMaterialValue(const MaterialsTempComp& material) {
  // get the corresponding actual nominal filter name
  if (internal::tables().materialValuesMap.contains(material)) {
    return internal::tables().materialValuesMap.at(material);
  }
  return 0.0;
}

/*!
 * \brief getMaterials
 * \return
 */
std::vector<std::string> getMaterials() {
  return oi::values(internal::tables().materialStringsMap);
}

/*!
 * \brief getTemperatureExpansion
 * \param material
 * \param actual
 * \param nominal
 * \return
 */
double getTemperatureExpansion(const std::string material, double actual,
                               double nominal) {
  double exp =
      oi::value(internal::tables().materialValuesMap,
                oi::key(internal::tables().materialStringsMap, material));
  double expansion = (actual - nominal) * exp;

  return 1.0 / (1.0 + expansion);
}

}  // namespace oi
