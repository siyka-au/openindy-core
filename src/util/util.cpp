#include "util.h"

#include "featurewrapper.h"

namespace oi {

// #################################################
// maps with element enums and corresponding strings
// #################################################

namespace internal {

// ############################################################################
// PERSISTED TOKENS - these strings are written into saved projects and into
// settings keys. Changing one silently breaks loading. See util.h.
// ############################################################################

QMap<ElementTypes, QString> elementTypesMap;
QMap<ElementTypes, QString> elementTypesPluralMap;
QMap<FeatureTypes, QString> featureTypesMap;
QMap<GeometryTypes, QString> geometryTypesMap;

// ############################################################################
// DISPLAY NAMES - shown to the user, never persisted, safe to edit freely.
// Deliberately separate maps rather than a transform of the tokens above, so
// that changing what a user reads can never change what a file contains.
// ############################################################################

QMap<ElementTypes, QString> elementTypesDisplayMap;
QMap<ElementTypes, QString> elementTypesDisplayPluralMap;
QMap<FeatureTypes, QString> featureTypesDisplayMap;
QMap<GeometryTypes, QString> geometryTypesDisplayMap;

QMap<ElementTypes, QString> elementTypeIcons;
QMap<FeatureTypes, QString> featureTypeIcons;

QMap<ReadingTypes, QString> readingTypesMap;

QMap<FeatureDisplayAttributes, QString> featureDisplayAttributesMap;
QMap<TrafoParamDisplayAttributes, QString> trafoParamDisplayAttributesMap;
QMap<ObservationDisplayAttributes, QString> observationDisplayAttributesMap;
QMap<ReadingDisplayAttributes, QString> readingDisplayAttributesMap;

QList<int> featureDisplayAttributes;
QList<ObservationDisplayAttributes> observationDisplayAttributes;
QList<ReadingDisplayAttributes> readingDisplayAttributes;

QMap<UnitType, QString> unitTypesMap;

QMap<SensorFunctions, QString> sensorFunctionsMap;

QMap<ToolTypes, QString> toolTypesMap;

QMap<MessageTypes, QString> messageTypesMap;

QMap<GeometryParameters, QString> geometryParametersMap;
QMap<TrafoParamParameters, QString> trafoParamParametersMap;

QMap<ActualNominalFilter, QString> actualNominalFilterMap;

QString undefined = "";

QMap<MaterialsTempComp, QString> materialStringsMap;
QMap<MaterialsTempComp, double> materialValuesMap;

bool isInit = false;

void init() {
  // clear maps
  elementTypesMap.clear();
  elementTypesPluralMap.clear();
  elementTypesDisplayMap.clear();
  elementTypesDisplayPluralMap.clear();
  featureTypesDisplayMap.clear();
  geometryTypesDisplayMap.clear();
  featureTypesMap.clear();
  geometryTypesMap.clear();
  readingTypesMap.clear();
  featureDisplayAttributesMap.clear();
  observationDisplayAttributesMap.clear();
  readingDisplayAttributesMap.clear();
  trafoParamDisplayAttributesMap.clear();
  featureDisplayAttributes.clear();
  observationDisplayAttributes.clear();
  readingDisplayAttributes.clear();
  unitTypesMap.clear();
  toolTypesMap.clear();
  messageTypesMap.clear();
  geometryParametersMap.clear();
  actualNominalFilterMap.clear();
  materialStringsMap.clear();
  materialValuesMap.clear();

  // fill element map
  elementTypesMap.insert(ElementTypes::eCircleElement, "circle");
  elementTypesMap.insert(ElementTypes::eConeElement, "cone");
  elementTypesMap.insert(ElementTypes::eCylinderElement, "cylinder");
  elementTypesMap.insert(ElementTypes::eEllipseElement, "ellipse");
  elementTypesMap.insert(ElementTypes::eEllipsoidElement, "ellipsoid");
  elementTypesMap.insert(ElementTypes::eHyperboloidElement, "hyperboloid");
  elementTypesMap.insert(ElementTypes::eLineElement, "line");
  elementTypesMap.insert(ElementTypes::eNurbsElement, "nurbs");
  elementTypesMap.insert(ElementTypes::eParaboloidElement, "paraboloid");
  elementTypesMap.insert(ElementTypes::ePlaneElement, "plane");
  elementTypesMap.insert(ElementTypes::ePointElement, "point");
  elementTypesMap.insert(ElementTypes::ePointCloudElement, "point cloud");
  elementTypesMap.insert(ElementTypes::eScalarEntityAngleElement, "angle");
  elementTypesMap.insert(ElementTypes::eScalarEntityDistanceElement,
                         "distance");
  elementTypesMap.insert(ElementTypes::eScalarEntityMeasurementSeriesElement,
                         "measurement series");
  elementTypesMap.insert(ElementTypes::eScalarEntityTemperatureElement,
                         "temperature");
  elementTypesMap.insert(ElementTypes::eSlottedHoleElement, "slotted hole");
  elementTypesMap.insert(ElementTypes::eSphereElement, "sphere");
  elementTypesMap.insert(ElementTypes::eTorusElement, "torus");
  elementTypesMap.insert(ElementTypes::eDirectionElement, "direction");
  elementTypesMap.insert(ElementTypes::ePositionElement, "position");
  elementTypesMap.insert(ElementTypes::eRadiusElement, "radius");
  elementTypesMap.insert(ElementTypes::eCoordinateSystemElement,
                         "coordinate system");
  elementTypesMap.insert(ElementTypes::eStationElement, "station");
  elementTypesMap.insert(ElementTypes::eTrafoParamElement,
                         "transformation parameters");
  elementTypesMap.insert(ElementTypes::eObservationElement, "observation");
  elementTypesMap.insert(ElementTypes::eReadingCartesianElement,
                         "cartesian reading");
  elementTypesMap.insert(ElementTypes::eReadingPolarElement, "polar reading");
  elementTypesMap.insert(ElementTypes::eReadingDistanceElement,
                         "distance reading");
  elementTypesMap.insert(ElementTypes::eReadingDirectionElement,
                         "direction reading");
  elementTypesMap.insert(ElementTypes::eReadingTemperatureElement,
                         "temperature reading");
  elementTypesMap.insert(ElementTypes::eReadingLevelElement, "level reading");
  elementTypesMap.insert(ElementTypes::eLevelElement, "level");
  elementTypesMap.insert(ElementTypes::ePointCollectionElement,
                         "point collection");

  // fill element plural map
  elementTypesPluralMap.insert(ElementTypes::eCircleElement, "circles");
  elementTypesPluralMap.insert(ElementTypes::eConeElement, "cones");
  elementTypesPluralMap.insert(ElementTypes::eCylinderElement, "cylinders");
  elementTypesPluralMap.insert(ElementTypes::eEllipseElement, "ellipses");
  elementTypesPluralMap.insert(ElementTypes::eEllipsoidElement, "ellipsoids");
  elementTypesPluralMap.insert(ElementTypes::eHyperboloidElement,
                               "hyperboloids");
  elementTypesPluralMap.insert(ElementTypes::eLineElement, "lines");
  elementTypesPluralMap.insert(ElementTypes::eNurbsElement, "nurbs");
  elementTypesPluralMap.insert(ElementTypes::eParaboloidElement, "paraboloids");
  elementTypesPluralMap.insert(ElementTypes::ePlaneElement, "planes");
  elementTypesPluralMap.insert(ElementTypes::ePointElement, "points");
  elementTypesPluralMap.insert(ElementTypes::ePointCloudElement,
                               "point clouds");
  elementTypesPluralMap.insert(ElementTypes::eScalarEntityAngleElement,
                               "angles");
  elementTypesPluralMap.insert(ElementTypes::eScalarEntityDistanceElement,
                               "distances");
  elementTypesPluralMap.insert(
      ElementTypes::eScalarEntityMeasurementSeriesElement,
      "measurement series");
  elementTypesPluralMap.insert(ElementTypes::eScalarEntityTemperatureElement,
                               "temperatures");
  elementTypesPluralMap.insert(ElementTypes::eSlottedHoleElement,
                               "slotted holes");
  elementTypesPluralMap.insert(ElementTypes::eSphereElement, "spheres");
  elementTypesPluralMap.insert(ElementTypes::eTorusElement, "tori");
  elementTypesPluralMap.insert(ElementTypes::eDirectionElement, "directions");
  elementTypesPluralMap.insert(ElementTypes::ePositionElement, "positions");
  elementTypesPluralMap.insert(ElementTypes::eRadiusElement, "radii");
  elementTypesPluralMap.insert(ElementTypes::eCoordinateSystemElement,
                               "coordinate systems");
  elementTypesPluralMap.insert(ElementTypes::eStationElement, "stations");
  elementTypesPluralMap.insert(ElementTypes::eTrafoParamElement,
                               "transformation parameters");
  elementTypesPluralMap.insert(ElementTypes::eObservationElement,
                               "observations");
  elementTypesPluralMap.insert(ElementTypes::eReadingCartesianElement,
                               "cartesian readings");
  elementTypesPluralMap.insert(ElementTypes::eReadingPolarElement,
                               "polar readings");
  elementTypesPluralMap.insert(ElementTypes::eReadingDistanceElement,
                               "distance readings");
  elementTypesPluralMap.insert(ElementTypes::eReadingDirectionElement,
                               "direction readings");
  elementTypesPluralMap.insert(ElementTypes::eReadingTemperatureElement,
                               "temperature readings");
  elementTypesPluralMap.insert(ElementTypes::eReadingLevelElement,
                               "level readings");
  elementTypesPluralMap.insert(ElementTypes::ePointCollectionElement,
                               "point collections");

  // fill feature map
  featureTypesMap.insert(FeatureTypes::eCircleFeature, "circle");
  featureTypesMap.insert(FeatureTypes::eConeFeature, "cone");
  featureTypesMap.insert(FeatureTypes::eCylinderFeature, "cylinder");
  featureTypesMap.insert(FeatureTypes::eEllipseFeature, "ellipse");
  featureTypesMap.insert(FeatureTypes::eEllipsoidFeature, "ellipsoid");
  featureTypesMap.insert(FeatureTypes::eHyperboloidFeature, "hyperboloid");
  featureTypesMap.insert(FeatureTypes::eLineFeature, "line");
  featureTypesMap.insert(FeatureTypes::eNurbsFeature, "nurbs");
  featureTypesMap.insert(FeatureTypes::eParaboloidFeature, "paraboloid");
  featureTypesMap.insert(FeatureTypes::ePlaneFeature, "plane");
  featureTypesMap.insert(FeatureTypes::ePointFeature, "point");
  featureTypesMap.insert(FeatureTypes::ePointCloudFeature, "point cloud");
  featureTypesMap.insert(FeatureTypes::eScalarEntityAngleFeature, "angle");
  featureTypesMap.insert(FeatureTypes::eScalarEntityDistanceFeature,
                         "distance");
  featureTypesMap.insert(FeatureTypes::eScalarEntityMeasurementSeriesFeature,
                         "measurement series");
  featureTypesMap.insert(FeatureTypes::eScalarEntityTemperatureFeature,
                         "temperature");
  featureTypesMap.insert(FeatureTypes::eSlottedHoleFeature, "slotted hole");
  featureTypesMap.insert(FeatureTypes::eSphereFeature, "sphere");
  featureTypesMap.insert(FeatureTypes::eTorusFeature, "torus");
  featureTypesMap.insert(FeatureTypes::eCoordinateSystemFeature,
                         "coordinate system");
  featureTypesMap.insert(FeatureTypes::eStationFeature, "station");
  featureTypesMap.insert(FeatureTypes::eTrafoParamFeature,
                         "transformation parameters");
  featureTypesMap.insert(FeatureTypes::eLevelFeature, "level");
  featureTypesMap.insert(FeatureTypes::eGroupFeature, "group");
  featureTypesMap.insert(FeatureTypes::ePointGroupFeature, "point group");

  // fill geometry map
  geometryTypesMap.insert(GeometryTypes::eCircleGeometry, "circle");
  geometryTypesMap.insert(GeometryTypes::eConeGeometry, "cone");
  geometryTypesMap.insert(GeometryTypes::eCylinderGeometry, "cylinder");
  geometryTypesMap.insert(GeometryTypes::eEllipseGeometry, "ellipse");
  geometryTypesMap.insert(GeometryTypes::eEllipsoidGeometry, "ellipsoid");
  geometryTypesMap.insert(GeometryTypes::eHyperboloidGeometry, "hyperboloid");
  geometryTypesMap.insert(GeometryTypes::eLineGeometry, "line");
  geometryTypesMap.insert(GeometryTypes::eNurbsGeometry, "nurbs");
  geometryTypesMap.insert(GeometryTypes::eParaboloidGeometry, "paraboloid");
  geometryTypesMap.insert(GeometryTypes::ePlaneGeometry, "plane");
  geometryTypesMap.insert(GeometryTypes::ePointGeometry, "point");
  geometryTypesMap.insert(GeometryTypes::ePointCloudGeometry, "point cloud");
  geometryTypesMap.insert(GeometryTypes::eScalarEntityAngleGeometry, "angle");
  geometryTypesMap.insert(GeometryTypes::eScalarEntityDistanceGeometry,
                          "distance");
  geometryTypesMap.insert(GeometryTypes::eScalarEntityMeasurementSeriesGeometry,
                          "measurement series");
  geometryTypesMap.insert(GeometryTypes::eScalarEntityTemperatureGeometry,
                          "temperature");
  geometryTypesMap.insert(GeometryTypes::eSlottedHoleGeometry, "slotted hole");
  geometryTypesMap.insert(GeometryTypes::eSphereGeometry, "sphere");
  geometryTypesMap.insert(GeometryTypes::eTorusGeometry, "torus");
  geometryTypesMap.insert(GeometryTypes::ePlaneLevelGeometry, "level");

  // fill element type icons
  elementTypeIcons.insert(ElementTypes::eCircleElement,
                          ":/icons/icons/toolbars/feature/Circle.png");
  elementTypeIcons.insert(ElementTypes::eConeElement,
                          ":/icons/icons/toolbars/feature/Cone.png");
  elementTypeIcons.insert(ElementTypes::eCylinderElement,
                          ":/icons/icons/toolbars/feature/Cylinder.png");
  elementTypeIcons.insert(ElementTypes::eEllipseElement,
                          ":/icons/icons/toolbars/feature/Ellipse.png");
  elementTypeIcons.insert(ElementTypes::eEllipsoidElement,
                          ":/icons/icons/toolbars/feature/Ellipsoid.png");
  elementTypeIcons.insert(ElementTypes::eHyperboloidElement,
                          ":/icons/icons/toolbars/feature/Hyperboloid.png");
  elementTypeIcons.insert(ElementTypes::eLineElement,
                          ":/icons/icons/toolbars/feature/Line.png");
  elementTypeIcons.insert(ElementTypes::eNurbsElement,
                          ":/icons/icons/toolbars/feature/Nurbs Surface.png");
  elementTypeIcons.insert(ElementTypes::eParaboloidElement,
                          ":/icons/icons/toolbars/feature/Paraboloid.png");
  elementTypeIcons.insert(ElementTypes::ePlaneElement,
                          ":/icons/icons/toolbars/feature/Plane.png");
  elementTypeIcons.insert(ElementTypes::ePointElement,
                          ":/icons/icons/toolbars/feature/Point.png");
  elementTypeIcons.insert(ElementTypes::ePointCloudElement,
                          ":/icons/icons/toolbars/feature/Point Cloud.png");
  elementTypeIcons.insert(ElementTypes::eScalarEntityAngleElement,
                          ":/icons/icons/toolbars/feature/Scalar Entities.png");
  elementTypeIcons.insert(ElementTypes::eScalarEntityDistanceElement,
                          ":/icons/icons/toolbars/feature/Scalar Entities.png");
  elementTypeIcons.insert(ElementTypes::eScalarEntityMeasurementSeriesElement,
                          ":/icons/icons/toolbars/feature/Scalar Entities.png");
  elementTypeIcons.insert(ElementTypes::eScalarEntityTemperatureElement,
                          ":/icons/icons/toolbars/feature/Scalar Entities.png");
  elementTypeIcons.insert(ElementTypes::eSlottedHoleElement,
                          ":/icons/icons/toolbars/feature/Slotted Hole.png");
  elementTypeIcons.insert(ElementTypes::eSphereElement,
                          ":/icons/icons/toolbars/feature/Sphere.png");
  elementTypeIcons.insert(ElementTypes::eTorusElement,
                          ":/icons/icons/toolbars/feature/Torus.png");
  elementTypeIcons.insert(ElementTypes::eDirectionElement, "");
  elementTypeIcons.insert(ElementTypes::ePositionElement, "");
  elementTypeIcons.insert(ElementTypes::eRadiusElement, "");
  elementTypeIcons.insert(
      ElementTypes::eCoordinateSystemElement,
      ":/icons/icons/toolbars/feature/Coordinatesystem.png");
  elementTypeIcons.insert(ElementTypes::eStationElement,
                          ":/icons/icons/toolbars/feature/Station.png");
  elementTypeIcons.insert(ElementTypes::eTrafoParamElement,
                          ":/icons/icons/toolbars/feature/TrafoParam.png");
  elementTypeIcons.insert(ElementTypes::eObservationElement, "");
  elementTypeIcons.insert(ElementTypes::eReadingCartesianElement, "");
  elementTypeIcons.insert(ElementTypes::eReadingPolarElement, "");
  elementTypeIcons.insert(ElementTypes::eReadingDistanceElement, "");
  elementTypeIcons.insert(ElementTypes::eReadingDirectionElement, "");
  elementTypeIcons.insert(ElementTypes::eReadingTemperatureElement, "");
  elementTypeIcons.insert(ElementTypes::eReadingLevelElement, "");

  // fill feature type icons
  featureTypeIcons.insert(FeatureTypes::eCircleFeature,
                          ":/icons/icons/toolbars/feature/Circle.png");
  featureTypeIcons.insert(FeatureTypes::eConeFeature,
                          ":/icons/icons/toolbars/feature/Cone.png");
  featureTypeIcons.insert(FeatureTypes::eCylinderFeature,
                          ":/icons/icons/toolbars/feature/Cylinder.png");
  featureTypeIcons.insert(FeatureTypes::eEllipseFeature,
                          ":/icons/icons/toolbars/feature/Ellipse.png");
  featureTypeIcons.insert(FeatureTypes::eEllipsoidFeature,
                          ":/icons/icons/toolbars/feature/Ellipsoid.png");
  featureTypeIcons.insert(FeatureTypes::eHyperboloidFeature,
                          ":/icons/icons/toolbars/feature/Hyperboloid.png");
  featureTypeIcons.insert(FeatureTypes::eLineFeature,
                          ":/icons/icons/toolbars/feature/Line.png");
  featureTypeIcons.insert(FeatureTypes::eNurbsFeature,
                          ":/icons/icons/toolbars/feature/Nurbs Surface.png");
  featureTypeIcons.insert(FeatureTypes::eParaboloidFeature,
                          ":/icons/icons/toolbars/feature/Paraboloid.png");
  featureTypeIcons.insert(FeatureTypes::ePlaneFeature,
                          ":/icons/icons/toolbars/feature/Plane.png");
  featureTypeIcons.insert(FeatureTypes::ePointFeature,
                          ":/icons/icons/toolbars/feature/Point.png");
  featureTypeIcons.insert(FeatureTypes::ePointCloudFeature,
                          ":/icons/icons/toolbars/feature/Point Cloud.png");
  featureTypeIcons.insert(FeatureTypes::eScalarEntityAngleFeature,
                          ":/icons/icons/toolbars/feature/Scalar Entities.png");
  featureTypeIcons.insert(FeatureTypes::eScalarEntityDistanceFeature,
                          ":/icons/icons/toolbars/feature/Scalar Entities.png");
  featureTypeIcons.insert(FeatureTypes::eScalarEntityMeasurementSeriesFeature,
                          ":/icons/icons/toolbars/feature/Scalar Entities.png");
  featureTypeIcons.insert(FeatureTypes::eScalarEntityTemperatureFeature,
                          ":/icons/icons/toolbars/feature/Scalar Entities.png");
  featureTypeIcons.insert(FeatureTypes::eSlottedHoleFeature,
                          ":/icons/icons/toolbars/feature/Slotted Hole.png");
  featureTypeIcons.insert(FeatureTypes::eSphereFeature,
                          ":/icons/icons/toolbars/feature/Sphere.png");
  featureTypeIcons.insert(FeatureTypes::eTorusFeature,
                          ":/icons/icons/toolbars/feature/Torus.png");
  featureTypeIcons.insert(
      FeatureTypes::eCoordinateSystemFeature,
      ":/icons/icons/toolbars/feature/Coordinatesystem.png");
  featureTypeIcons.insert(FeatureTypes::eStationFeature,
                          ":/icons/icons/toolbars/feature/Station.png");
  featureTypeIcons.insert(FeatureTypes::eTrafoParamFeature,
                          ":/icons/icons/toolbars/feature/TrafoParam.png");

  // fill reading map
  readingTypesMap.insert(ReadingTypes::eDistanceReading, "distance");
  readingTypesMap.insert(ReadingTypes::eCartesianReading, "cartesian");
  readingTypesMap.insert(ReadingTypes::eCartesianReading6D, "cartesian6D");
  readingTypesMap.insert(ReadingTypes::ePolarReading, "polar");
  readingTypesMap.insert(ReadingTypes::eDirectionReading, "direction");
  readingTypesMap.insert(ReadingTypes::eTemperatureReading, "temperature");
  readingTypesMap.insert(ReadingTypes::eLevelReading, "level");

  // fill feature display attributes map
  featureDisplayAttributesMap.insert(eFeatureDisplayType, "type");
  featureDisplayAttributesMap.insert(eFeatureDisplayName, "name");
  featureDisplayAttributesMap.insert(eFeatureDisplayComment, "comment");
  featureDisplayAttributesMap.insert(eFeatureDisplayGroup, "group");
  featureDisplayAttributesMap.insert(eFeatureDisplayIsSolved, "solved");
  featureDisplayAttributesMap.insert(eFeatureDisplayIsUpdated, "updated");
  featureDisplayAttributesMap.insert(eFeatureDisplayFunctions, "functions");
  featureDisplayAttributesMap.insert(eFeatureDisplayIsCommon, "common");
  featureDisplayAttributesMap.insert(eFeatureDisplayUsedFor, "used for");
  featureDisplayAttributesMap.insert(eFeatureDisplayPreviouslyNeeded,
                                     "previously needed");
  featureDisplayAttributesMap.insert(eFeatureDisplayStDev, "stdev");
  featureDisplayAttributesMap.insert(eFeatureDisplayMeasurementConfig,
                                     "measurement config");
  featureDisplayAttributesMap.insert(eFeatureDisplayObservations,
                                     "observations");
  featureDisplayAttributesMap.insert(eFeatureDisplayX, "x");
  featureDisplayAttributesMap.insert(eFeatureDisplayY, "y");
  featureDisplayAttributesMap.insert(eFeatureDisplayZ, "z");
  featureDisplayAttributesMap.insert(eFeatureDisplayPrimaryI, "i");
  featureDisplayAttributesMap.insert(eFeatureDisplayPrimaryJ, "j");
  featureDisplayAttributesMap.insert(eFeatureDisplayPrimaryK, "k");
  featureDisplayAttributesMap.insert(eFeatureDisplayRadiusA, "radius");
  featureDisplayAttributesMap.insert(eFeatureDisplayRadiusB, "radius 2");
  featureDisplayAttributesMap.insert(eFeatureDisplaySecondaryI, "i 2");
  featureDisplayAttributesMap.insert(eFeatureDisplaySecondaryJ, "j 2");
  featureDisplayAttributesMap.insert(eFeatureDisplaySecondaryK, "k 2");
  featureDisplayAttributesMap.insert(eFeatureDisplayAperture, "aperture");
  featureDisplayAttributesMap.insert(eFeatureDisplayA, "a");
  featureDisplayAttributesMap.insert(eFeatureDisplayB, "b");
  featureDisplayAttributesMap.insert(eFeatureDisplayC, "c");
  featureDisplayAttributesMap.insert(eFeatureDisplayAngle, "angle");
  featureDisplayAttributesMap.insert(eFeatureDisplayDistance, "distance");
  featureDisplayAttributesMap.insert(eFeatureDisplayMeasurementSeries,
                                     "measurement series");
  featureDisplayAttributesMap.insert(eFeatureDisplayTemperature, "temperature");
  featureDisplayAttributesMap.insert(eFeatureDisplayLength, "length");
  featureDisplayAttributesMap.insert(eFeatureDisplayExpansionOriginX,
                                     "expansion x");
  featureDisplayAttributesMap.insert(eFeatureDisplayExpansionOriginY,
                                     "expansion y");
  featureDisplayAttributesMap.insert(eFeatureDisplayExpansionOriginZ,
                                     "expansion z");
  featureDisplayAttributesMap.insert(eFeatureDisplayIsActual, "actual feature");
  featureDisplayAttributesMap.insert(eFeatureDisplayFormError, "form error");

  // fill observation display attributes map
  observationDisplayAttributesMap.insert(eObservationDisplayId, "id");
  observationDisplayAttributesMap.insert(eObservationDisplayStation, "station");
  observationDisplayAttributesMap.insert(eObservationDisplayTargetGeometries,
                                         "target geometries");
  observationDisplayAttributesMap.insert(eObservationDisplayX, "x");
  observationDisplayAttributesMap.insert(eObservationDisplayY, "y");
  observationDisplayAttributesMap.insert(eObservationDisplayZ, "z");
  observationDisplayAttributesMap.insert(eObservationDisplayI, "i");
  observationDisplayAttributesMap.insert(eObservationDisplayJ, "j");
  observationDisplayAttributesMap.insert(eObservationDisplayK, "k");
  observationDisplayAttributesMap.insert(eObservationDisplaySigmaX, "sigma x");
  observationDisplayAttributesMap.insert(eObservationDisplaySigmaY, "sigma y");
  observationDisplayAttributesMap.insert(eObservationDisplaySigmaZ, "sigma z");
  observationDisplayAttributesMap.insert(eObservationDisplaySigmaI, "sigma i");
  observationDisplayAttributesMap.insert(eObservationDisplaySigmaJ, "sigma j");
  observationDisplayAttributesMap.insert(eObservationDisplaySigmaK, "sigma k");
  observationDisplayAttributesMap.insert(eObservationDisplayIsValid, "valid");
  observationDisplayAttributesMap.insert(eObservationDisplayIsSolved, "solved");
  observationDisplayAttributesMap.insert(eObservationDisplayVX, "vx");
  observationDisplayAttributesMap.insert(eObservationDisplayVY, "vy");
  observationDisplayAttributesMap.insert(eObservationDisplayVZ, "vz");
  observationDisplayAttributesMap.insert(eObservationDisplayV, "v");
  observationDisplayAttributesMap.insert(eObservationDisplayVR, "vr");
  observationDisplayAttributesMap.insert(eObservationDisplayVI, "vi");
  observationDisplayAttributesMap.insert(eObservationDisplayVJ, "vj");
  observationDisplayAttributesMap.insert(eObservationDisplayVK, "vk");
  observationDisplayAttributesMap.insert(eObservationDisplayIsUsed, "used");
  observationDisplayAttributesMap.insert(eObservationDisplayIsDummyPoint,
                                         "dummy point");
  observationDisplayAttributesMap.insert(eObservationDisplayReadingTime,
                                         "time");

  // fill reading display attributes map
  readingDisplayAttributesMap.insert(eReadingDisplayId, "id");
  readingDisplayAttributesMap.insert(eReadingDisplayType, "type");
  readingDisplayAttributesMap.insert(eReadingDisplayTime, "time");
  readingDisplayAttributesMap.insert(eReadingDisplaySensorConfigName,
                                     "sensor config");
  readingDisplayAttributesMap.insert(eReadingDisplayMeasurementConfigName,
                                     "measurement config");
  readingDisplayAttributesMap.insert(eReadingDisplaySide, "side");
  readingDisplayAttributesMap.insert(eReadingDisplayAzimuth, "azimuth");
  readingDisplayAttributesMap.insert(eReadingDisplayZenith, "zenith");
  readingDisplayAttributesMap.insert(eReadingDisplayDistance, "distance");
  readingDisplayAttributesMap.insert(eReadingDisplayX, "x");
  readingDisplayAttributesMap.insert(eReadingDisplayY, "y");
  readingDisplayAttributesMap.insert(eReadingDisplayZ, "z");
  readingDisplayAttributesMap.insert(eReadingDisplayI, "i");
  readingDisplayAttributesMap.insert(eReadingDisplayJ, "j");
  readingDisplayAttributesMap.insert(eReadingDisplayK, "k");
  readingDisplayAttributesMap.insert(eReadingDisplayTemperature, "temperature");
  readingDisplayAttributesMap.insert(eReadingDisplaySigmaAzimuth,
                                     "sigma azimuth");
  readingDisplayAttributesMap.insert(eReadingDisplaySigmaZenith,
                                     "sigma zenith");
  readingDisplayAttributesMap.insert(eReadingDisplaySigmaDistance,
                                     "sigma distance");
  readingDisplayAttributesMap.insert(eReadingDisplaySigmaX, "sigma x");
  readingDisplayAttributesMap.insert(eReadingDisplaySigmaY, "sigma y");
  readingDisplayAttributesMap.insert(eReadingDisplaySigmaZ, "sigma z");
  readingDisplayAttributesMap.insert(eReadingDisplaySigmaI, "sigma i");
  readingDisplayAttributesMap.insert(eReadingDisplaySigmaJ, "sigma j");
  readingDisplayAttributesMap.insert(eReadingDisplaySigmaK, "sigma k");
  readingDisplayAttributesMap.insert(eReadingDisplaySigmaTemperature,
                                     "sigma temperature");
  readingDisplayAttributesMap.insert(eReadingDisplayImported, "imported");

  // fill trafo param display attributes map
  trafoParamDisplayAttributesMap.insert(eTrafoParamDisplayType, "type");
  trafoParamDisplayAttributesMap.insert(eTrafoParamDisplayName, "name");
  trafoParamDisplayAttributesMap.insert(eTrafoParamDisplayComment, "comment");
  trafoParamDisplayAttributesMap.insert(eTrafoParamDisplayGroup, "group");
  trafoParamDisplayAttributesMap.insert(eTrafoParamDisplayIsSolved, "solved");
  trafoParamDisplayAttributesMap.insert(eTrafoParamDisplayIsUpdated, "updated");
  trafoParamDisplayAttributesMap.insert(eTrafoParamDisplayFunctions,
                                        "functions");
  trafoParamDisplayAttributesMap.insert(eTrafoParamDisplayUsedFor, "used for");
  trafoParamDisplayAttributesMap.insert(eTrafoParamDisplayPreviouslyNeeded,
                                        "previously needed");
  trafoParamDisplayAttributesMap.insert(eTrafoParamDisplayStDev, "stdev");
  trafoParamDisplayAttributesMap.insert(eTrafoParamDisplayStartSystem, "start");
  trafoParamDisplayAttributesMap.insert(eTrafoParamDisplayDestinationSystem,
                                        "destination");
  trafoParamDisplayAttributesMap.insert(eTrafoParamDisplayTranslationX, "tx");
  trafoParamDisplayAttributesMap.insert(eTrafoParamDisplayTranslationY, "ty");
  trafoParamDisplayAttributesMap.insert(eTrafoParamDisplayTranslationZ, "tz");
  trafoParamDisplayAttributesMap.insert(eTrafoParamDisplayRotationX, "rx");
  trafoParamDisplayAttributesMap.insert(eTrafoParamDisplayRotationY, "ry");
  trafoParamDisplayAttributesMap.insert(eTrafoParamDisplayRotationZ, "rz");
  trafoParamDisplayAttributesMap.insert(eTrafoParamDisplayScaleX, "sx");
  trafoParamDisplayAttributesMap.insert(eTrafoParamDisplayScaleY, "sy");
  trafoParamDisplayAttributesMap.insert(eTrafoParamDisplayScaleZ, "sz");
  trafoParamDisplayAttributesMap.insert(eTrafoParamDisplayIsUsed, "used");
  trafoParamDisplayAttributesMap.insert(eTrafoParamDisplayValidTime, "time");
  trafoParamDisplayAttributesMap.insert(eTrafoParamDisplayIsMovement,
                                        "movement");
  trafoParamDisplayAttributesMap.insert(eTrafoParamDisplayIsDatumTransformation,
                                        "datum transformation");

  // fill available display attributes
  for (int i = eFeatureDisplayType; i <= eFeatureDisplayStDev;
       i++) {  // general feature attributes
    featureDisplayAttributes.append(i);
  }
  for (int i = eFeatureDisplayMeasurementConfig; i <= eFeatureDisplayIsActual;
       i++) {  // geometry specific attributes
    featureDisplayAttributes.append(i);
  }
  for (int i = eFeatureDisplayX; i <= eFeatureDisplayFormError;
       i++) {  // unknown geometry parameters
    featureDisplayAttributes.append(i);
  }
  for (int i = eFeatureDisplayExpansionOriginX;
       i <= eFeatureDisplayExpansionOriginZ;
       i++) {  // coordinate system specific
    featureDisplayAttributes.append(i);
  }
  for (int i = eTrafoParamDisplayStartSystem;
       i <= eTrafoParamDisplayIsDatumTransformation;
       i++) {  // trafo param specific
    featureDisplayAttributes.append(i);
  }

  // fill available observation display attributes
  observationDisplayAttributes = observationDisplayAttributesMap.keys();

  // fill available reading display attributes
  readingDisplayAttributes = readingDisplayAttributesMap.keys();

  // fill unit types map
  unitTypesMap.insert(UnitType::eUnitArcSeconds, "[arcsec]");
  unitTypesMap.insert(UnitType::eUnitDecimalDegree, "[°]");
  unitTypesMap.insert(UnitType::eUnitFahrenheit, "[°F]");
  unitTypesMap.insert(UnitType::eUnitGon, "[gon]");
  unitTypesMap.insert(UnitType::eUnitGrad, "[°C]");
  unitTypesMap.insert(UnitType::eUnitInch, "[inch]");
  unitTypesMap.insert(UnitType::eUnitKelvin, "[°K]");
  unitTypesMap.insert(UnitType::eUnitMeter, "[m]");
  unitTypesMap.insert(UnitType::eUnitMilliMeter, "[mm]");
  unitTypesMap.insert(UnitType::eUnitMilliRadians, "[mrad]");
  unitTypesMap.insert(UnitType::eUnitRadiant, "[rad]");

  // fill sensor functions map
  sensorFunctionsMap.insert(SensorFunctions::eMoveAngle, "move (polar)");
  sensorFunctionsMap.insert(SensorFunctions::eMoveXYZ, "move (cartesian)");
  sensorFunctionsMap.insert(SensorFunctions::eMotorState, "change motor state");
  sensorFunctionsMap.insert(SensorFunctions::eToggleSight,
                            "toggle sight orientation");
  sensorFunctionsMap.insert(SensorFunctions::eInitialize, "initialize");
  sensorFunctionsMap.insert(SensorFunctions::eHome, "home");
  sensorFunctionsMap.insert(SensorFunctions::eCompensation, "compensation");
  sensorFunctionsMap.insert(SensorFunctions::eConnect, "connect");
  sensorFunctionsMap.insert(SensorFunctions::eDisconnect, "disconnect");
  sensorFunctionsMap.insert(SensorFunctions::eMeasure, "measure");
  sensorFunctionsMap.insert(SensorFunctions::eStream, "stream");

  // fill connection types map

  // fill tool types map
  toolTypesMap.insert(ToolTypes::eReportTool, "report");
  toolTypesMap.insert(ToolTypes::eUnknownTool, "tool");

  // fill message types map
  messageTypesMap.insert(MessageTypes::eInformationMessage, "information");
  messageTypesMap.insert(MessageTypes::eWarningMessage, "warning");
  messageTypesMap.insert(MessageTypes::eErrorMessage, "error");
  messageTypesMap.insert(MessageTypes::eCriticalMessage, "critical");

  // fill geometry parameters map
  geometryParametersMap.insert(GeometryParameters::eUnknownX, "x");
  geometryParametersMap.insert(GeometryParameters::eUnknownY, "y");
  geometryParametersMap.insert(GeometryParameters::eUnknownZ, "z");
  geometryParametersMap.insert(GeometryParameters::eUnknownPrimaryI, "i");
  geometryParametersMap.insert(GeometryParameters::eUnknownPrimaryJ, "j");
  geometryParametersMap.insert(GeometryParameters::eUnknownPrimaryK, "k");
  geometryParametersMap.insert(GeometryParameters::eUnknownSecondaryI, "i 2");
  geometryParametersMap.insert(GeometryParameters::eUnknownSecondaryJ, "j 2");
  geometryParametersMap.insert(GeometryParameters::eUnknownSecondaryK, "k 2");
  geometryParametersMap.insert(GeometryParameters::eUnknownRadiusA, "radius");
  geometryParametersMap.insert(GeometryParameters::eUnknownRadiusB, "radius 2");
  geometryParametersMap.insert(GeometryParameters::eUnknownAperture,
                               "aperture");
  geometryParametersMap.insert(GeometryParameters::eUnknownA, "a");
  geometryParametersMap.insert(GeometryParameters::eUnknownB, "b");
  geometryParametersMap.insert(GeometryParameters::eUnknownC, "c");
  geometryParametersMap.insert(GeometryParameters::eUnknownAngle, "angle");
  geometryParametersMap.insert(GeometryParameters::eUnknownDistance,
                               "distance");
  geometryParametersMap.insert(GeometryParameters::eUnknownMeasurementSeries,
                               "measurement series");
  geometryParametersMap.insert(GeometryParameters::eUnknownTemperature,
                               "temperature");
  geometryParametersMap.insert(GeometryParameters::eUnknownLength, "length");

  // fill trafo param parameters map
  trafoParamParametersMap.insert(TrafoParamParameters::eUnknownTX, "tx");
  trafoParamParametersMap.insert(TrafoParamParameters::eUnknownTY, "ty");
  trafoParamParametersMap.insert(TrafoParamParameters::eUnknownTZ, "tz");
  trafoParamParametersMap.insert(TrafoParamParameters::eUnknownRX, "rx");
  trafoParamParametersMap.insert(TrafoParamParameters::eUnknownRY, "ry");
  trafoParamParametersMap.insert(TrafoParamParameters::eUnknownRZ, "rz");
  trafoParamParametersMap.insert(TrafoParamParameters::eUnknownSX, "sx");
  trafoParamParametersMap.insert(TrafoParamParameters::eUnknownSY, "sy");
  trafoParamParametersMap.insert(TrafoParamParameters::eUnknownSZ, "sz");

  // fill actual nominal filter map
  actualNominalFilterMap.insert(ActualNominalFilter::eFilterActualNominal,
                                "All Features");
  actualNominalFilterMap.insert(ActualNominalFilter::eFilterActual, "Actuals");
  actualNominalFilterMap.insert(ActualNominalFilter::eFilterNominal,
                                "Nominals");

  // fill material maps
  materialStringsMap.insert(MaterialsTempComp::eMaterialSteel, "steel");
  materialStringsMap.insert(MaterialsTempComp::eMaterialAluminum, "aluminum");
  materialStringsMap.insert(MaterialsTempComp::eMaterialPlumb, "plumb");
  materialStringsMap.insert(MaterialsTempComp::eMaterialIron, "iron");
  materialStringsMap.insert(MaterialsTempComp::eMaterialGrayCastIron,
                            "gray cast iron");
  materialStringsMap.insert(MaterialsTempComp::eMaterialCopper, "copper");
  materialStringsMap.insert(MaterialsTempComp::eMaterialBrass, "brass");
  materialStringsMap.insert(MaterialsTempComp::eMaterialZinc, "zinc");
  materialStringsMap.insert(MaterialsTempComp::eMaterialPlatinum, "platinum");
  materialStringsMap.insert(MaterialsTempComp::eMaterialConcrete, "concrete");
  materialStringsMap.insert(MaterialsTempComp::eMaterialReinforcedConcrete,
                            "reinforced concrete");

  materialValuesMap.insert(MaterialsTempComp::eMaterialSteel, 0.00001139);
  materialValuesMap.insert(MaterialsTempComp::eMaterialAluminum, 0.000024);
  materialValuesMap.insert(MaterialsTempComp::eMaterialPlumb, 0.000030);
  materialValuesMap.insert(MaterialsTempComp::eMaterialIron, 0.000012);
  materialValuesMap.insert(MaterialsTempComp::eMaterialGrayCastIron, 0.000009);
  materialValuesMap.insert(MaterialsTempComp::eMaterialCopper, 0.000017);
  materialValuesMap.insert(MaterialsTempComp::eMaterialBrass, 0.000018);
  materialValuesMap.insert(MaterialsTempComp::eMaterialZinc, 0.000027);
  materialValuesMap.insert(MaterialsTempComp::eMaterialPlatinum, 0.000009);
  materialValuesMap.insert(MaterialsTempComp::eMaterialConcrete, 0.000012);
  materialValuesMap.insert(MaterialsTempComp::eMaterialReinforcedConcrete,
                           0.000013);

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
  elementTypesDisplayMap.insert(ElementTypes::eCircleElement, "Circle");
  elementTypesDisplayMap.insert(ElementTypes::eConeElement, "Cone");
  elementTypesDisplayMap.insert(ElementTypes::eCylinderElement, "Cylinder");
  elementTypesDisplayMap.insert(ElementTypes::eEllipseElement, "Ellipse");
  elementTypesDisplayMap.insert(ElementTypes::eEllipsoidElement, "Ellipsoid");
  elementTypesDisplayMap.insert(ElementTypes::eHyperboloidElement,
                                "Hyperboloid");
  elementTypesDisplayMap.insert(ElementTypes::eLineElement, "Line");
  elementTypesDisplayMap.insert(ElementTypes::eNurbsElement, "Nurbs");
  elementTypesDisplayMap.insert(ElementTypes::eParaboloidElement, "Paraboloid");
  elementTypesDisplayMap.insert(ElementTypes::ePlaneElement, "Plane");
  elementTypesDisplayMap.insert(ElementTypes::ePointElement, "Point");
  elementTypesDisplayMap.insert(ElementTypes::ePointCloudElement,
                                "Point Cloud");
  elementTypesDisplayMap.insert(ElementTypes::eScalarEntityAngleElement,
                                "Angle");
  elementTypesDisplayMap.insert(ElementTypes::eScalarEntityDistanceElement,
                                "Distance");
  elementTypesDisplayMap.insert(
      ElementTypes::eScalarEntityMeasurementSeriesElement,
      "Measurement Series");
  elementTypesDisplayMap.insert(ElementTypes::eScalarEntityTemperatureElement,
                                "Temperature");
  elementTypesDisplayMap.insert(ElementTypes::eSlottedHoleElement,
                                "Slotted Hole");
  elementTypesDisplayMap.insert(ElementTypes::eSphereElement, "Sphere");
  elementTypesDisplayMap.insert(ElementTypes::eTorusElement, "Torus");
  elementTypesDisplayMap.insert(ElementTypes::eDirectionElement, "Direction");
  elementTypesDisplayMap.insert(ElementTypes::ePositionElement, "Position");
  elementTypesDisplayMap.insert(ElementTypes::eRadiusElement, "Radius");
  elementTypesDisplayMap.insert(ElementTypes::eCoordinateSystemElement,
                                "Coordinate System");
  elementTypesDisplayMap.insert(ElementTypes::eStationElement, "Station");
  elementTypesDisplayMap.insert(ElementTypes::eTrafoParamElement,
                                "Transformation Parameters");
  elementTypesDisplayMap.insert(ElementTypes::eObservationElement,
                                "Observation");
  elementTypesDisplayMap.insert(ElementTypes::eReadingCartesianElement,
                                "Cartesian Reading");
  elementTypesDisplayMap.insert(ElementTypes::eReadingPolarElement,
                                "Polar Reading");
  elementTypesDisplayMap.insert(ElementTypes::eReadingDistanceElement,
                                "Distance Reading");
  elementTypesDisplayMap.insert(ElementTypes::eReadingDirectionElement,
                                "Direction Reading");
  elementTypesDisplayMap.insert(ElementTypes::eReadingTemperatureElement,
                                "Temperature Reading");
  elementTypesDisplayMap.insert(ElementTypes::eReadingLevelElement,
                                "Level Reading");
  elementTypesDisplayMap.insert(ElementTypes::eLevelElement, "Level");
  elementTypesDisplayMap.insert(ElementTypes::ePointCollectionElement,
                                "Point Collection");

  // elementTypesDisplayPluralMap
  elementTypesDisplayPluralMap.insert(ElementTypes::eCircleElement, "Circles");
  elementTypesDisplayPluralMap.insert(ElementTypes::eConeElement, "Cones");
  elementTypesDisplayPluralMap.insert(ElementTypes::eCylinderElement,
                                      "Cylinders");
  elementTypesDisplayPluralMap.insert(ElementTypes::eEllipseElement,
                                      "Ellipses");
  elementTypesDisplayPluralMap.insert(ElementTypes::eEllipsoidElement,
                                      "Ellipsoids");
  elementTypesDisplayPluralMap.insert(ElementTypes::eHyperboloidElement,
                                      "Hyperboloids");
  elementTypesDisplayPluralMap.insert(ElementTypes::eLineElement, "Lines");
  elementTypesDisplayPluralMap.insert(ElementTypes::eNurbsElement, "Nurbs");
  elementTypesDisplayPluralMap.insert(ElementTypes::eParaboloidElement,
                                      "Paraboloids");
  elementTypesDisplayPluralMap.insert(ElementTypes::ePlaneElement, "Planes");
  elementTypesDisplayPluralMap.insert(ElementTypes::ePointElement, "Points");
  elementTypesDisplayPluralMap.insert(ElementTypes::ePointCloudElement,
                                      "Point Clouds");
  elementTypesDisplayPluralMap.insert(ElementTypes::eScalarEntityAngleElement,
                                      "Angles");
  elementTypesDisplayPluralMap.insert(
      ElementTypes::eScalarEntityDistanceElement, "Distances");
  elementTypesDisplayPluralMap.insert(
      ElementTypes::eScalarEntityMeasurementSeriesElement,
      "Measurement Series");
  elementTypesDisplayPluralMap.insert(
      ElementTypes::eScalarEntityTemperatureElement, "Temperatures");
  elementTypesDisplayPluralMap.insert(ElementTypes::eSlottedHoleElement,
                                      "Slotted Holes");
  elementTypesDisplayPluralMap.insert(ElementTypes::eSphereElement, "Spheres");
  elementTypesDisplayPluralMap.insert(ElementTypes::eTorusElement, "Tori");
  elementTypesDisplayPluralMap.insert(ElementTypes::eDirectionElement,
                                      "Directions");
  elementTypesDisplayPluralMap.insert(ElementTypes::ePositionElement,
                                      "Positions");
  elementTypesDisplayPluralMap.insert(ElementTypes::eRadiusElement, "Radii");
  elementTypesDisplayPluralMap.insert(ElementTypes::eCoordinateSystemElement,
                                      "Coordinate Systems");
  elementTypesDisplayPluralMap.insert(ElementTypes::eStationElement,
                                      "Stations");
  elementTypesDisplayPluralMap.insert(ElementTypes::eTrafoParamElement,
                                      "Transformation Parameters");
  elementTypesDisplayPluralMap.insert(ElementTypes::eObservationElement,
                                      "Observations");
  elementTypesDisplayPluralMap.insert(ElementTypes::eReadingCartesianElement,
                                      "Cartesian Readings");
  elementTypesDisplayPluralMap.insert(ElementTypes::eReadingPolarElement,
                                      "Polar Readings");
  elementTypesDisplayPluralMap.insert(ElementTypes::eReadingDistanceElement,
                                      "Distance Readings");
  elementTypesDisplayPluralMap.insert(ElementTypes::eReadingDirectionElement,
                                      "Direction Readings");
  elementTypesDisplayPluralMap.insert(ElementTypes::eReadingTemperatureElement,
                                      "Temperature Readings");
  elementTypesDisplayPluralMap.insert(ElementTypes::eReadingLevelElement,
                                      "Level Readings");
  elementTypesDisplayPluralMap.insert(ElementTypes::ePointCollectionElement,
                                      "Point Collections");

  // featureTypesDisplayMap
  featureTypesDisplayMap.insert(FeatureTypes::eCircleFeature, "Circle");
  featureTypesDisplayMap.insert(FeatureTypes::eConeFeature, "Cone");
  featureTypesDisplayMap.insert(FeatureTypes::eCylinderFeature, "Cylinder");
  featureTypesDisplayMap.insert(FeatureTypes::eEllipseFeature, "Ellipse");
  featureTypesDisplayMap.insert(FeatureTypes::eEllipsoidFeature, "Ellipsoid");
  featureTypesDisplayMap.insert(FeatureTypes::eHyperboloidFeature,
                                "Hyperboloid");
  featureTypesDisplayMap.insert(FeatureTypes::eLineFeature, "Line");
  featureTypesDisplayMap.insert(FeatureTypes::eNurbsFeature, "Nurbs");
  featureTypesDisplayMap.insert(FeatureTypes::eParaboloidFeature, "Paraboloid");
  featureTypesDisplayMap.insert(FeatureTypes::ePlaneFeature, "Plane");
  featureTypesDisplayMap.insert(FeatureTypes::ePointFeature, "Point");
  featureTypesDisplayMap.insert(FeatureTypes::ePointCloudFeature,
                                "Point Cloud");
  featureTypesDisplayMap.insert(FeatureTypes::eScalarEntityAngleFeature,
                                "Angle");
  featureTypesDisplayMap.insert(FeatureTypes::eScalarEntityDistanceFeature,
                                "Distance");
  featureTypesDisplayMap.insert(
      FeatureTypes::eScalarEntityMeasurementSeriesFeature,
      "Measurement Series");
  featureTypesDisplayMap.insert(FeatureTypes::eScalarEntityTemperatureFeature,
                                "Temperature");
  featureTypesDisplayMap.insert(FeatureTypes::eSlottedHoleFeature,
                                "Slotted Hole");
  featureTypesDisplayMap.insert(FeatureTypes::eSphereFeature, "Sphere");
  featureTypesDisplayMap.insert(FeatureTypes::eTorusFeature, "Torus");
  featureTypesDisplayMap.insert(FeatureTypes::eCoordinateSystemFeature,
                                "Coordinate System");
  featureTypesDisplayMap.insert(FeatureTypes::eStationFeature, "Station");
  featureTypesDisplayMap.insert(FeatureTypes::eTrafoParamFeature,
                                "Transformation Parameters");
  featureTypesDisplayMap.insert(FeatureTypes::eLevelFeature, "Level");
  featureTypesDisplayMap.insert(FeatureTypes::eGroupFeature, "Group");
  featureTypesDisplayMap.insert(FeatureTypes::ePointGroupFeature,
                                "Point Group");

  // geometryTypesDisplayMap
  geometryTypesDisplayMap.insert(GeometryTypes::eCircleGeometry, "Circle");
  geometryTypesDisplayMap.insert(GeometryTypes::eConeGeometry, "Cone");
  geometryTypesDisplayMap.insert(GeometryTypes::eCylinderGeometry, "Cylinder");
  geometryTypesDisplayMap.insert(GeometryTypes::eEllipseGeometry, "Ellipse");
  geometryTypesDisplayMap.insert(GeometryTypes::eEllipsoidGeometry,
                                 "Ellipsoid");
  geometryTypesDisplayMap.insert(GeometryTypes::eHyperboloidGeometry,
                                 "Hyperboloid");
  geometryTypesDisplayMap.insert(GeometryTypes::eLineGeometry, "Line");
  geometryTypesDisplayMap.insert(GeometryTypes::eNurbsGeometry, "Nurbs");
  geometryTypesDisplayMap.insert(GeometryTypes::eParaboloidGeometry,
                                 "Paraboloid");
  geometryTypesDisplayMap.insert(GeometryTypes::ePlaneGeometry, "Plane");
  geometryTypesDisplayMap.insert(GeometryTypes::ePointGeometry, "Point");
  geometryTypesDisplayMap.insert(GeometryTypes::ePointCloudGeometry,
                                 "Point Cloud");
  geometryTypesDisplayMap.insert(GeometryTypes::eScalarEntityAngleGeometry,
                                 "Angle");
  geometryTypesDisplayMap.insert(GeometryTypes::eScalarEntityDistanceGeometry,
                                 "Distance");
  geometryTypesDisplayMap.insert(
      GeometryTypes::eScalarEntityMeasurementSeriesGeometry,
      "Measurement Series");
  geometryTypesDisplayMap.insert(
      GeometryTypes::eScalarEntityTemperatureGeometry, "Temperature");
  geometryTypesDisplayMap.insert(GeometryTypes::eSlottedHoleGeometry,
                                 "Slotted Hole");
  geometryTypesDisplayMap.insert(GeometryTypes::eSphereGeometry, "Sphere");
  geometryTypesDisplayMap.insert(GeometryTypes::eTorusGeometry, "Torus");
  geometryTypesDisplayMap.insert(GeometryTypes::ePlaneLevelGeometry, "Level");

  isInit = true;
}

}  // namespace internal

// #######################################
// global functions to query element types
// #######################################

/*!
 * \brief getAvailableElementTypes
 * \return
 */
QList<ElementTypes> getAvailableElementTypes() {
  // fill helper maps if not yet done
  if (!internal::isInit) {
    internal::init();
  }

  return internal::elementTypesMap.keys();
}

/*!
 * \brief getAvailableFeatureTypes
 * \return
 */
QList<FeatureTypes> getAvailableFeatureTypes() {
  // fill helper maps if not yet done
  if (!internal::isInit) {
    internal::init();
  }

  return internal::featureTypesMap.keys();
}

/*!
 * \brief getAvailableGeometryTypes
 * \return
 */
QList<GeometryTypes> getAvailableGeometryTypes() {
  // fill helper maps if not yet done
  if (!internal::isInit) {
    internal::init();
  }

  return internal::geometryTypesMap.keys();
}

/*!
 * \brief getElementTypeName
 * \param type
 * \return
 */
const QString& getElementTypeName(const ElementTypes& type) {
  // fill helper maps if not yet done
  if (!internal::isInit) {
    internal::init();
  }

  // get the corresponding element name
  if (internal::elementTypesMap.contains(type)) {
    return internal::elementTypesMap[type];
  }
  return internal::undefined;
}

/*!
 * \brief getElementTypePluralName
 * \param type
 * \return
 */
const QString& getElementTypePluralName(const ElementTypes& type) {
  // fill helper maps if not yet done
  if (!internal::isInit) {
    internal::init();
  }

  // get the corresponding element name
  if (internal::elementTypesPluralMap.contains(type)) {
    return internal::elementTypesPluralMap[type];
  }
  return internal::undefined;
}

/*!
 * \brief getElementTypeEnum
 * \param name
 * \return
 */
ElementTypes getElementTypeEnum(const QString& name) {
  // fill helper maps if not yet done
  if (!internal::isInit) {
    internal::init();
  }

  // get the corresponding element enum value
  ElementTypes type =
      internal::elementTypesMap.key(name, ElementTypes::eUndefinedElement);
  if (type == ElementTypes::eUndefinedElement) {
    type = internal::elementTypesPluralMap.key(name,
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
const QString& getFeatureTypeName(const FeatureTypes& type) {
  // fill helper maps if not yet done
  if (!internal::isInit) {
    internal::init();
  }

  // get the corresponding feature name
  if (internal::featureTypesMap.contains(type)) {
    return internal::featureTypesMap[type];
  }
  return internal::undefined;
}

/*!
 * \brief getFeatureTypeEnum
 * \param name
 * \return
 */
FeatureTypes getFeatureTypeEnum(const QString& name) {
  // fill helper maps if not yet done
  if (!internal::isInit) {
    internal::init();
  }

  // get the corresponding feature enum value
  return internal::featureTypesMap.key(name, FeatureTypes::eUndefinedFeature);
}

/*!
 * \brief getGeometryTypeName
 * \param type
 * \return
 */
const QString& getGeometryTypeName(const GeometryTypes& type) {
  // fill helper maps if not yet done
  if (!internal::isInit) {
    internal::init();
  }

  // get the corresponding geometry name
  if (internal::geometryTypesMap.contains(type)) {
    return internal::geometryTypesMap[type];
  }
  return internal::undefined;
}

/*!
 * \brief getGeometryTypeEnum
 * \param name
 * \return
 */
GeometryTypes getGeometryTypeEnum(const QString& name) {
  // fill helper maps if not yet done
  if (!internal::isInit) {
    internal::init();
  }

  // get the corresponding geometry enum value
  return internal::geometryTypesMap.key(name,
                                        GeometryTypes::eUndefinedGeometry);
}

/*!
 * \brief getGeometryTypeEnum
 * \param type
 * \return
 */
GeometryTypes getGeometryTypeEnum(const FeatureTypes& type) {
  // fill helper maps if not yet done
  if (!internal::isInit) {
    internal::init();
  }

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
const QString& getElementTypeDisplayName(const ElementTypes& type) {
  // fill helper maps if not yet done
  if (!internal::isInit) {
    internal::init();
  }

  if (internal::elementTypesDisplayMap.contains(type)) {
    return internal::elementTypesDisplayMap[type];
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
const QString& getElementTypeDisplayPluralName(const ElementTypes& type) {
  // fill helper maps if not yet done
  if (!internal::isInit) {
    internal::init();
  }

  if (internal::elementTypesDisplayPluralMap.contains(type)) {
    return internal::elementTypesDisplayPluralMap[type];
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
const QString& getFeatureTypeDisplayName(const FeatureTypes& type) {
  // fill helper maps if not yet done
  if (!internal::isInit) {
    internal::init();
  }

  if (internal::featureTypesDisplayMap.contains(type)) {
    return internal::featureTypesDisplayMap[type];
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
const QString& getGeometryTypeDisplayName(const GeometryTypes& type) {
  // fill helper maps if not yet done
  if (!internal::isInit) {
    internal::init();
  }

  if (internal::geometryTypesDisplayMap.contains(type)) {
    return internal::geometryTypesDisplayMap[type];
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
      // tst_projectexchange.cpp, not a hypothetical.
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
const QString& getElementTypeIconPath(const ElementTypes& type) {
  // fill helper maps if not yet done
  if (!internal::isInit) {
    internal::init();
  }

  // get the corresponding element type icon
  if (internal::elementTypeIcons.contains(type)) {
    return internal::elementTypeIcons[type];
  }
  return internal::undefined;
}

/*!
 * \brief getFeatureTypeIconPath
 * \param type
 * \return
 */
const QString& getFeatureTypeIconPath(const FeatureTypes& type) {
  // fill helper maps if not yet done
  if (!internal::isInit) {
    internal::init();
  }

  // get the corresponding feature type icon
  if (internal::featureTypeIcons.contains(type)) {
    return internal::featureTypeIcons[type];
  }
  return internal::undefined;
}

/*!
 * \brief getReadingTypeName
 * \param type
 * \return
 */
const QString& getReadingTypeName(const ReadingTypes& type) {
  // fill helper maps if not yet done
  if (!internal::isInit) {
    internal::init();
  }

  // get the corresponding reading name
  if (internal::readingTypesMap.contains(type)) {
    return internal::readingTypesMap[type];
  }
  return internal::undefined;
}

/*!
 * \brief getReadingTypeEnum
 * \param name
 * \return
 */
ReadingTypes getReadingTypeEnum(const QString& name) {
  // fill helper maps if not yet done
  if (!internal::isInit) {
    internal::init();
  }

  // get the corresponding reading enum value
  return internal::readingTypesMap.key(name, ReadingTypes::eUndefinedReading);
}

/*!
 * \brief getFeatureDisplayAttributes
 * Returns a list of enum values (feature attributes and trafo param attributes)
 * \return
 */
const QList<int>& getFeatureDisplayAttributes() {
  // fill helper maps if not yet done
  if (!internal::isInit) {
    internal::init();
  }

  return internal::featureDisplayAttributes;
}

/*!
 * \brief getObservationDisplayAttributes
 * \return
 */
const QList<ObservationDisplayAttributes>& getObservationDisplayAttributes() {
  // fill helper maps if not yet done
  if (!internal::isInit) {
    internal::init();
  }

  return internal::observationDisplayAttributes;
}

/*!
 * \brief getReadingDisplayAttributes
 * \return
 */
const QList<ReadingDisplayAttributes>& getReadingDisplayAttributes() {
  // fill helper maps if not yet done
  if (!internal::isInit) {
    internal::init();
  }

  return internal::readingDisplayAttributes;
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
  if ((attr >= 0 && attr < 10) || (attr >= 400 && attr < 415)) {
    return true;
  }
  return false;
}

/*!
 * \brief getDisplayAttributeName
 * Returns the name of the attribute represented by the given enum value
 * \param attr
 * \return
 */
const QString& getFeatureDisplayAttributeName(const int& attr) {
  // fill helper maps if not yet done
  if (!internal::isInit) {
    internal::init();
  }

  if (getIsFeatureDisplayAttribute(attr)) {  // feature display attribute
    return internal::featureDisplayAttributesMap[(
        FeatureDisplayAttributes)attr];
  } else if (getIsTrafoParamDisplayAttribute(
                 attr)) {  // trafo param display attribute
    return internal::trafoParamDisplayAttributesMap[(
        TrafoParamDisplayAttributes)attr];
  }

  return internal::undefined;
}

/*!
 * \brief getDisplayAttributeName
 * \param attr
 * \return
 */
const QString& getFeatureDisplayAttributeName(
    const FeatureDisplayAttributes& attr) {
  // fill helper maps if not yet done
  if (!internal::isInit) {
    internal::init();
  }

  return internal::featureDisplayAttributesMap[attr];
}

/*!
 * \brief getDisplayAttributeName
 * \param attr
 * \return
 */
const QString& getFeatureDisplayAttributeName(
    const TrafoParamDisplayAttributes& attr) {
  // fill helper maps if not yet done
  if (!internal::isInit) {
    internal::init();
  }

  return internal::trafoParamDisplayAttributesMap[attr];
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
const QString& getObservationDisplayAttributesName(
    const ObservationDisplayAttributes& attr) {
  // fill helper maps if not yet done
  if (!internal::isInit) {
    internal::init();
  }

  return internal::observationDisplayAttributesMap[attr];
}

/*!
 * \brief getReadingDisplayAttributeName
 * \param attr
 * \return
 */
const QString& getReadingDisplayAttributeName(
    const ReadingDisplayAttributes& attr) {
  // fill helper maps if not yet done
  if (!internal::isInit) {
    internal::init();
  }

  return internal::readingDisplayAttributesMap[attr];
}

/*!
 * \brief getObservationDisplayAttributeEnum
 * \param name
 * \return
 */
ObservationDisplayAttributes getObservationDisplayAttributeEnum(
    const QString& name) {
  // fill helper maps if not yet done
  if (!internal::isInit) {
    internal::init();
  }

  // get the corresponding observation enum value
  return internal::observationDisplayAttributesMap.key(name,
                                                       eObservationDisplayId);
}

/*!
 * \brief getReadingDisplayAttributeEnum
 * \param name
 * \return
 */
ReadingDisplayAttributes getReadingDisplayAttributeEnum(const QString& name) {
  // fill helper maps if not yet done
  if (!internal::isInit) {
    internal::init();
  }

  // get the corresponding reading enum value
  return internal::readingDisplayAttributesMap.key(name, eReadingDisplayId);
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
      return value * (1.0 / 1000.0);
    case UnitType::eUnitInch:
      return value * (1.0 / 39.37007874);
    case UnitType::eUnitMilliRadians:
      return value * (1.0 / 1000.0);
    case UnitType::eUnitGon:
      return value * (1.0 / RHO_GON);
    case UnitType::eUnitArcSeconds:
      return value * (1.0 / 206264.8062471);
    case UnitType::eUnitDecimalDegree:
      return value * (1.0 / RHO_DEGREE);
    case UnitType::eUnitFahrenheit:
      return (value - 32.0) / 1.8;
    case UnitType::eUnitKelvin:
      return value - 273.15;
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
      return value * 1000.0;
    case UnitType::eUnitInch:
      return value * 39.37007874;
    case UnitType::eUnitMilliRadians:
      return value * 1000.0;
    case UnitType::eUnitGon:
      return value * RHO_GON;
    case UnitType::eUnitArcSeconds:
      return value * 206264.8062471;
    case UnitType::eUnitDecimalDegree:
      return value * RHO_DEGREE;
    case UnitType::eUnitFahrenheit:
      return value * 1.8 + 32.0;
    case UnitType::eUnitKelvin:
      return value + 273.15;
    default:
      return value;
  }
}

/*!
 * \brief getUnitTypeName
 * \param type
 * \return
 */
const QString& getUnitTypeName(const UnitType& type) {
  // fill helper maps if not yet done
  if (!internal::isInit) {
    internal::init();
  }

  // get the corresponding reading name
  if (internal::unitTypesMap.contains(type)) {
    return internal::unitTypesMap[type];
  }
  return internal::undefined;
}

/*!
 * \brief getUnitTypeEnum
 * \param name
 * \return
 */
UnitType getUnitTypeEnum(const QString& name) {
  // fill helper maps if not yet done
  if (!internal::isInit) {
    internal::init();
  }

  // get the corresponding reading enum value
  return internal::unitTypesMap.key(name, UnitType::eNoUnit);
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
  double diff = qAbs(a - b);

  // calculate epsilon
  double epsilon = 1.0;
  for (int i = 0; i < digits; i++) {
    epsilon = epsilon / 10.0;
  }

  // check difference
  return diff <= epsilon;
}

/*!
 * \brief getAvailableSensorFunctions
 * \return
 */
QList<SensorFunctions> getAvailableSensorFunctions() {
  // fill helper maps if not yet done
  if (!internal::isInit) {
    internal::init();
  }

  return internal::sensorFunctionsMap.keys();
}

/*!
 * \brief getSensorFunctionName
 * \param type
 * \return
 */
const QString& getSensorFunctionName(const SensorFunctions& type) {
  // fill helper maps if not yet done
  if (!internal::isInit) {
    internal::init();
  }

  // get the corresponding sensor function name
  if (internal::sensorFunctionsMap.contains(type)) {
    return internal::sensorFunctionsMap[type];
  }
  return internal::undefined;
}

/*!
 * \brief getSensorFunctionEnum
 * \param name
 * \return
 */
SensorFunctions getSensorFunctionEnum(const QString& name) {
  // fill helper maps if not yet done
  if (!internal::isInit) {
    internal::init();
  }

  // get the corresponding sensor function enum value
  return internal::sensorFunctionsMap.key(name, SensorFunctions::eInitialize);
}

/*!
 * \brief getAvailableToolTypes
 * \return
 */
QList<ToolTypes> getAvailableToolTypes() {
  // fill helper maps if not yet done
  if (!internal::isInit) {
    internal::init();
  }

  return internal::toolTypesMap.keys();
}

/*!
 * \brief getToolTypeName
 * \param type
 * \return
 */
const QString& getToolTypeName(const ToolTypes& type) {
  // fill helper maps if not yet done
  if (!internal::isInit) {
    internal::init();
  }

  // get the corresponding tool type name
  if (internal::toolTypesMap.contains(type)) {
    return internal::toolTypesMap[type];
  }
  return internal::undefined;
}

/*!
 * \brief getToolTypeEnum
 * \param name
 * \return
 */
ToolTypes getToolTypeEnum(const QString& name) {
  // fill helper maps if not yet done
  if (!internal::isInit) {
    internal::init();
  }

  // get the corresponding tool type enum value
  return internal::toolTypesMap.key(name, ToolTypes::eUnknownTool);
}

/*!
 * \brief getAvailableMessageTypes
 * \return
 */
QList<MessageTypes> getAvailableMessageTypes() {
  // fill helper maps if not yet done
  if (!internal::isInit) {
    internal::init();
  }

  return internal::messageTypesMap.keys();
}

/*!
 * \brief getMessageTypeName
 * \param type
 * \return
 */
const QString& getMessageTypeName(const MessageTypes& type) {
  // fill helper maps if not yet done
  if (!internal::isInit) {
    internal::init();
  }

  // get the corresponding message type name
  if (internal::messageTypesMap.contains(type)) {
    return internal::messageTypesMap[type];
  }
  return internal::undefined;
}

/*!
 * \brief getMessageTypeEnum
 * \param name
 * \return
 */
MessageTypes getMessageTypeEnum(const QString& name) {
  // fill helper maps if not yet done
  if (!internal::isInit) {
    internal::init();
  }

  // get the corresponding message type enum value
  return internal::messageTypesMap.key(name, MessageTypes::eInformationMessage);
}

/*!
 * \brief getAvailableUnknownParameters
 * \return
 */
QList<GeometryParameters> getAvailableGeometryParameters() {
  // fill helper maps if not yet done
  if (!internal::isInit) {
    internal::init();
  }

  return internal::geometryParametersMap.keys();
}

/*!
 * \brief getUnknownParameterName
 * \param parameter
 * \return
 */
const QString& getGeometryParameterName(const GeometryParameters& parameter) {
  // fill helper maps if not yet done
  if (!internal::isInit) {
    internal::init();
  }

  // get the corresponding geometry parameter name
  if (internal::geometryParametersMap.contains(parameter)) {
    return internal::geometryParametersMap[parameter];
  }
  return internal::undefined;
}

/*!
 * \brief getUnknownParameterEnum
 * \param name
 * \return
 */
GeometryParameters getGeometryParameterEnum(const QString& name) {
  // fill helper maps if not yet done
  if (!internal::isInit) {
    internal::init();
  }

  // get the corresponding geometry parameter enum value
  return internal::geometryParametersMap.key(name,
                                             GeometryParameters::eUnknownX);
}

/*!
 * \brief getUnknownParameters
 * \param type
 * \return
 */
QList<GeometryParameters> getGeometryParameters(const GeometryTypes& type) {
  QList<GeometryParameters> unknownParameters;

  switch (type) {
    case GeometryTypes::eCircleGeometry:
      unknownParameters.append(GeometryParameters::eUnknownX);
      unknownParameters.append(GeometryParameters::eUnknownY);
      unknownParameters.append(GeometryParameters::eUnknownZ);
      unknownParameters.append(GeometryParameters::eUnknownPrimaryI);
      unknownParameters.append(GeometryParameters::eUnknownPrimaryJ);
      unknownParameters.append(GeometryParameters::eUnknownPrimaryK);
      unknownParameters.append(GeometryParameters::eUnknownRadiusA);
      break;
    case GeometryTypes::eConeGeometry:
      unknownParameters.append(GeometryParameters::eUnknownX);
      unknownParameters.append(GeometryParameters::eUnknownY);
      unknownParameters.append(GeometryParameters::eUnknownZ);
      unknownParameters.append(GeometryParameters::eUnknownPrimaryI);
      unknownParameters.append(GeometryParameters::eUnknownPrimaryJ);
      unknownParameters.append(GeometryParameters::eUnknownPrimaryK);
      unknownParameters.append(GeometryParameters::eUnknownAperture);
      break;
    case GeometryTypes::eCylinderGeometry:
      unknownParameters.append(GeometryParameters::eUnknownX);
      unknownParameters.append(GeometryParameters::eUnknownY);
      unknownParameters.append(GeometryParameters::eUnknownZ);
      unknownParameters.append(GeometryParameters::eUnknownPrimaryI);
      unknownParameters.append(GeometryParameters::eUnknownPrimaryJ);
      unknownParameters.append(GeometryParameters::eUnknownPrimaryK);
      unknownParameters.append(GeometryParameters::eUnknownRadiusA);
      break;
    case GeometryTypes::eEllipseGeometry:
      unknownParameters.append(GeometryParameters::eUnknownX);
      unknownParameters.append(GeometryParameters::eUnknownY);
      unknownParameters.append(GeometryParameters::eUnknownZ);
      unknownParameters.append(GeometryParameters::eUnknownPrimaryI);
      unknownParameters.append(GeometryParameters::eUnknownPrimaryJ);
      unknownParameters.append(GeometryParameters::eUnknownPrimaryK);
      unknownParameters.append(GeometryParameters::eUnknownA);
      unknownParameters.append(GeometryParameters::eUnknownB);
      unknownParameters.append(GeometryParameters::eUnknownSecondaryI);
      unknownParameters.append(GeometryParameters::eUnknownSecondaryJ);
      unknownParameters.append(GeometryParameters::eUnknownSecondaryK);
      break;
    case GeometryTypes::eEllipsoidGeometry:
      unknownParameters.append(GeometryParameters::eUnknownX);
      unknownParameters.append(GeometryParameters::eUnknownY);
      unknownParameters.append(GeometryParameters::eUnknownZ);
      unknownParameters.append(GeometryParameters::eUnknownPrimaryI);
      unknownParameters.append(GeometryParameters::eUnknownPrimaryJ);
      unknownParameters.append(GeometryParameters::eUnknownPrimaryK);
      unknownParameters.append(GeometryParameters::eUnknownA);
      unknownParameters.append(GeometryParameters::eUnknownB);
      break;
    case GeometryTypes::eHyperboloidGeometry:
      unknownParameters.append(GeometryParameters::eUnknownX);
      unknownParameters.append(GeometryParameters::eUnknownY);
      unknownParameters.append(GeometryParameters::eUnknownZ);
      unknownParameters.append(GeometryParameters::eUnknownPrimaryI);
      unknownParameters.append(GeometryParameters::eUnknownPrimaryJ);
      unknownParameters.append(GeometryParameters::eUnknownPrimaryK);
      unknownParameters.append(GeometryParameters::eUnknownA);
      unknownParameters.append(GeometryParameters::eUnknownC);
      break;
    case GeometryTypes::eLineGeometry:
      unknownParameters.append(GeometryParameters::eUnknownX);
      unknownParameters.append(GeometryParameters::eUnknownY);
      unknownParameters.append(GeometryParameters::eUnknownZ);
      unknownParameters.append(GeometryParameters::eUnknownPrimaryI);
      unknownParameters.append(GeometryParameters::eUnknownPrimaryJ);
      unknownParameters.append(GeometryParameters::eUnknownPrimaryK);
      break;
    case GeometryTypes::eNurbsGeometry:
      break;
    case GeometryTypes::eParaboloidGeometry:
      unknownParameters.append(GeometryParameters::eUnknownX);
      unknownParameters.append(GeometryParameters::eUnknownY);
      unknownParameters.append(GeometryParameters::eUnknownZ);
      unknownParameters.append(GeometryParameters::eUnknownPrimaryI);
      unknownParameters.append(GeometryParameters::eUnknownPrimaryJ);
      unknownParameters.append(GeometryParameters::eUnknownPrimaryK);
      unknownParameters.append(GeometryParameters::eUnknownA);
      break;
    case GeometryTypes::ePlaneGeometry:
      unknownParameters.append(GeometryParameters::eUnknownX);
      unknownParameters.append(GeometryParameters::eUnknownY);
      unknownParameters.append(GeometryParameters::eUnknownZ);
      unknownParameters.append(GeometryParameters::eUnknownPrimaryI);
      unknownParameters.append(GeometryParameters::eUnknownPrimaryJ);
      unknownParameters.append(GeometryParameters::eUnknownPrimaryK);
      break;
    case GeometryTypes::ePointGeometry:
      unknownParameters.append(GeometryParameters::eUnknownX);
      unknownParameters.append(GeometryParameters::eUnknownY);
      unknownParameters.append(GeometryParameters::eUnknownZ);
      break;
    case GeometryTypes::ePointCloudGeometry:
      break;
    case GeometryTypes::eScalarEntityAngleGeometry:
      unknownParameters.append(GeometryParameters::eUnknownAngle);
      break;
    case GeometryTypes::eScalarEntityDistanceGeometry:
      unknownParameters.append(GeometryParameters::eUnknownDistance);
      break;
    case GeometryTypes::eScalarEntityMeasurementSeriesGeometry:
      unknownParameters.append(GeometryParameters::eUnknownMeasurementSeries);
      break;
    case GeometryTypes::eScalarEntityTemperatureGeometry:
      unknownParameters.append(GeometryParameters::eUnknownTemperature);
      break;
    case GeometryTypes::eSlottedHoleGeometry:
      unknownParameters.append(GeometryParameters::eUnknownX);
      unknownParameters.append(GeometryParameters::eUnknownY);
      unknownParameters.append(GeometryParameters::eUnknownZ);
      unknownParameters.append(GeometryParameters::eUnknownPrimaryI);
      unknownParameters.append(GeometryParameters::eUnknownPrimaryJ);
      unknownParameters.append(GeometryParameters::eUnknownPrimaryK);
      unknownParameters.append(GeometryParameters::eUnknownRadiusA);
      unknownParameters.append(GeometryParameters::eUnknownLength);
      unknownParameters.append(GeometryParameters::eUnknownSecondaryI);
      unknownParameters.append(GeometryParameters::eUnknownSecondaryJ);
      unknownParameters.append(GeometryParameters::eUnknownSecondaryK);
      break;
    case GeometryTypes::eSphereGeometry:
      unknownParameters.append(GeometryParameters::eUnknownX);
      unknownParameters.append(GeometryParameters::eUnknownY);
      unknownParameters.append(GeometryParameters::eUnknownZ);
      unknownParameters.append(GeometryParameters::eUnknownRadiusA);
      break;
    case GeometryTypes::eTorusGeometry:
      unknownParameters.append(GeometryParameters::eUnknownX);
      unknownParameters.append(GeometryParameters::eUnknownY);
      unknownParameters.append(GeometryParameters::eUnknownZ);
      unknownParameters.append(GeometryParameters::eUnknownPrimaryI);
      unknownParameters.append(GeometryParameters::eUnknownPrimaryJ);
      unknownParameters.append(GeometryParameters::eUnknownPrimaryK);
      unknownParameters.append(GeometryParameters::eUnknownRadiusA);
      unknownParameters.append(GeometryParameters::eUnknownRadiusB);
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
QList<TrafoParamParameters> getAvailableTrafoParamParameters() {
  // fill helper maps if not yet done
  if (!internal::isInit) {
    internal::init();
  }

  return internal::trafoParamParametersMap.keys();
}

/*!
 * \brief getTrafoParamParameterName
 * \param parameter
 * \return
 */
const QString& getTrafoParamParameterName(
    const TrafoParamParameters& parameter) {
  // fill helper maps if not yet done
  if (!internal::isInit) {
    internal::init();
  }

  // get the corresponding trafo param parameter name
  if (internal::trafoParamParametersMap.contains(parameter)) {
    return internal::trafoParamParametersMap[parameter];
  }
  return internal::undefined;
}

/*!
 * \brief getTrafoParamParameterEnum
 * \param name
 * \return
 */
TrafoParamParameters getTrafoParamParameterEnum(const QString& name) {
  // fill helper maps if not yet done
  if (!internal::isInit) {
    internal::init();
  }

  // get the corresponding trafo param parameter enum value
  return internal::trafoParamParametersMap.key(
      name, TrafoParamParameters::eUnknownTX);
}

/*!
 * \brief getAvailableActualNominalFilters
 * \return
 */
QList<ActualNominalFilter> getAvailableActualNominalFilters() {
  // fill helper maps if not yet done
  if (!internal::isInit) {
    internal::init();
  }

  return internal::actualNominalFilterMap.keys();
}

/*!
 * \brief getActualNominalFilterName
 * \param type
 * \return
 */
const QString& getActualNominalFilterName(const ActualNominalFilter& type) {
  // fill helper maps if not yet done
  if (!internal::isInit) {
    internal::init();
  }

  // get the corresponding actual nominal filter name
  if (internal::actualNominalFilterMap.contains(type)) {
    return internal::actualNominalFilterMap[type];
  }
  return internal::undefined;
}

/*!
 * \brief getActualNominalFilterEnum
 * \param name
 * \return
 */
ActualNominalFilter getActualNominalFilterEnum(const QString& name) {
  // fill helper maps if not yet done
  if (!internal::isInit) {
    internal::init();
  }

  // get the corresponding actual nominal filter enum value
  return internal::actualNominalFilterMap.key(
      name, ActualNominalFilter::eFilterActualNominal);
}

/*!
 * \brief getMaterialName
 * \param material
 * \return
 */
const QString& getMaterialName(const MaterialsTempComp& material) {
  // fill helper maps if not yet done
  if (!internal::isInit) {
    internal::init();
  }

  // get the corresponding actual nominal filter name
  if (internal::materialStringsMap.contains(material)) {
    return internal::materialStringsMap[material];
  }
  return internal::undefined;
}

/*!
 * \brief getMaterialValue
 * \param material
 * \return
 */
double getMaterialValue(const MaterialsTempComp& material) {
  // fill helper maps if not yet done
  if (!internal::isInit) {
    internal::init();
  }

  // get the corresponding actual nominal filter name
  if (internal::materialValuesMap.contains(material)) {
    return internal::materialValuesMap[material];
  }
  return 0.0;
}

/*!
 * \brief getMaterials
 * \return
 */
QList<QString> getMaterials() {
  // fill helper maps if not yet done
  if (!internal::isInit) {
    internal::init();
  }

  return internal::materialStringsMap.values();
}

/*!
 * \brief getTemperatureExpansion
 * \param material
 * \param actual
 * \param nominal
 * \return
 */
double getTemperatureExpansion(const QString material, double actual,
                               double nominal) {
  // fill helper maps if not yet done
  if (!internal::isInit) {
    internal::init();
  }

  double exp = internal::materialValuesMap.value(
      internal::materialStringsMap.key(material));
  double expansion = (actual - nominal) * exp;

  return 1.0 / (1.0 + expansion);
}

}  // namespace oi
