#ifndef OI_CORE_UTIL_TYPES_H_
#define OI_CORE_UTIL_TYPES_H_
#include <map>
#include <string>
#include <vector>

#include "core_export.h"

namespace oi {

// #######################
// define scalar constants
// #######################

#define PI 3.141592653589793
#define RHO_DEGREE (180.0 / PI)
#define RHO_GON (200.0 / PI)

// #######################
// available message types
// #######################

//! all types of messages
enum class MessageTypes {
  eInformationMessage = 0,
  eWarningMessage,
  eErrorMessage,
  eCriticalMessage,
  eQuestionMessage
};

//! used to specify where to print a message
enum class MessageDestinations {
  eConsoleMessage = 0,
  eMessageBoxMessage,
  eStatusBarMessage
};

// #######################
// available element types
// #######################

//! all OpenIndy element types
enum class ElementTypes {

  // geometry types
  eCircleElement = 0,
  eConeElement,
  eCylinderElement,
  eEllipseElement,
  eEllipsoidElement,
  eHyperboloidElement,
  eLineElement,
  eNurbsElement,
  eParaboloidElement,
  ePlaneElement,
  ePointElement,
  ePointCloudElement,
  eScalarEntityAngleElement,
  eScalarEntityDistanceElement,
  eScalarEntityMeasurementSeriesElement,
  eScalarEntityTemperatureElement,
  eSlottedHoleElement,
  eSphereElement,
  eTorusElement,

  // geometric element types
  eDirectionElement,
  ePositionElement,
  eRadiusElement,

  // general feature types
  eCoordinateSystemElement,
  eStationElement,
  eTrafoParamElement,

  // observation and reading types
  eObservationElement,
  eReadingCartesianElement,
  eReadingPolarElement,
  eReadingDistanceElement,
  eReadingDirectionElement,
  eReadingTemperatureElement,
  eReadingLevelElement,

  eUndefinedElement,
  eLevelElement,

  //"a collection of points" - satisfied by a PointCloud or a PointGroup.
  // A fit function asks for this rather than for N individual points, so
  // bulk acquisition mutates the collection and never touches the
  // function's own inputs.
  ePointCollectionElement

};

//! all OpenIndy feature types
enum class FeatureTypes {
  // keep order !
  // geometry types
  eCircleFeature = 0,
  eConeFeature,
  eCylinderFeature,
  eEllipseFeature,
  eEllipsoidFeature,
  eHyperboloidFeature,
  eLineFeature,
  eNurbsFeature,
  eParaboloidFeature,
  ePlaneFeature,
  ePointFeature,
  ePointCloudFeature,
  eScalarEntityAngleFeature,
  eScalarEntityDistanceFeature,
  eScalarEntityMeasurementSeriesFeature,
  eScalarEntityTemperatureFeature,
  eSlottedHoleFeature,
  eSphereFeature,
  eTorusFeature,

  // general feature types
  eCoordinateSystemFeature,
  eStationFeature,
  eTrafoParamFeature,

  eUndefinedFeature,
  eLevelFeature,

  // organisational (not measured, not solved): a named collection of
  // references to other features. Appended deliberately - this enum is
  // order-sensitive and existing values must not shift.
  eGroupFeature,
  ePointGroupFeature,

};

//! all OpenIndy geometry types
enum class GeometryTypes {

  eCircleGeometry = 0,
  eConeGeometry,
  eCylinderGeometry,
  eEllipseGeometry,
  eEllipsoidGeometry,
  eHyperboloidGeometry,
  eLineGeometry,
  eNurbsGeometry,
  eParaboloidGeometry,
  ePlaneGeometry,
  ePointGeometry,
  ePointCloudGeometry,
  eScalarEntityAngleGeometry,
  eScalarEntityDistanceGeometry,
  eScalarEntityMeasurementSeriesGeometry,
  eScalarEntityTemperatureGeometry,
  eSlottedHoleGeometry,
  eSphereGeometry,
  eTorusGeometry,

  ePlaneLevelGeometry,  // special plane
  eUndefinedGeometry

};

// ################################
// all available unknown parameters
// ################################

// unknown parameters of all available geometry types
enum class GeometryParameters {

  eUnknownX = 0,  // point coordinates
  eUnknownY,
  eUnknownZ,
  eUnknownPrimaryI,  // primary vector
  eUnknownPrimaryJ,
  eUnknownPrimaryK,
  eUnknownSecondaryI,  // secondary vector
  eUnknownSecondaryJ,
  eUnknownSecondaryK,
  eUnknownRadiusA,   // radius
  eUnknownRadiusB,   //(torus)
  eUnknownAperture,  // opening angle (cone)
  eUnknownA,         // semi-major axis
  eUnknownB,         // semi-minor axis
  eUnknownC,         //(hyperboloid)
  eUnknownAngle,     // scalar entities
  eUnknownDistance,
  eUnknownMeasurementSeries,
  eUnknownTemperature,
  eUnknownLength  //(slotted hole)

};

// unknown parameters of trafo param features
enum class TrafoParamParameters {

  eUnknownTX = 0,  // translation
  eUnknownTY,
  eUnknownTZ,
  eUnknownRX,  // rotation
  eUnknownRY,
  eUnknownRZ,
  eUnknownSX,  // scale
  eUnknownSY,
  eUnknownSZ

};

// #######################
// available reading types
// #######################

enum class ReadingTypes {

  eDistanceReading = 0,
  eCartesianReading,
  ePolarReading,
  eDirectionReading,
  eTemperatureReading,
  eLevelReading,
  eUndefinedReading,
  eCartesianReading6D,

};

// ############################
// available display attributes
// ############################

enum FeatureDisplayAttributes {

  // general attributes
  eFeatureDisplayType = 0,
  eFeatureDisplayName,
  eFeatureDisplayComment,
  eFeatureDisplayGroup,
  eFeatureDisplayIsSolved,
  eFeatureDisplayIsUpdated,
  eFeatureDisplayFunctions,
  eFeatureDisplayUsedFor,
  eFeatureDisplayPreviouslyNeeded,
  eFeatureDisplayStDev,
  // for new enum check range in util.cpp getIsFeatureDisplayAttribute &&
  // featureDisplayAttributes.append(i)

  // geometry specific
  eFeatureDisplayMeasurementConfig = 100,
  eFeatureDisplayObservations,
  eFeatureDisplayIsCommon,
  eFeatureDisplayIsActual,
  // for new enum check range in util.cpp getIsFeatureDisplayAttribute  &&
  // featureDisplayAttributes.append(i)

  // unknown parameters
  eFeatureDisplayX = 200,
  eFeatureDisplayY,
  eFeatureDisplayZ,
  eFeatureDisplayPrimaryI,
  eFeatureDisplayPrimaryJ,
  eFeatureDisplayPrimaryK,
  eFeatureDisplayRadiusA,
  eFeatureDisplayRadiusB,
  eFeatureDisplaySecondaryI,
  eFeatureDisplaySecondaryJ,
  eFeatureDisplaySecondaryK,
  eFeatureDisplayAperture,
  eFeatureDisplayA,
  eFeatureDisplayB,
  eFeatureDisplayC,
  eFeatureDisplayAngle,
  eFeatureDisplayDistance,
  eFeatureDisplayMeasurementSeries,
  eFeatureDisplayTemperature,
  eFeatureDisplayLength,
  eFeatureDisplayFormError,
  // for new enum check range in util.cpp getIsFeatureDisplayAttribute &&
  // featureDisplayAttributes.append(i)

  // coordinate system specific
  eFeatureDisplayExpansionOriginX = 300,
  eFeatureDisplayExpansionOriginY,
  eFeatureDisplayExpansionOriginZ
  // for new enum check range in util.cpp getIsFeatureDisplayAttribute &&
  // featureDisplayAttributes.append(i)

};

enum TrafoParamDisplayAttributes {

  // general attributes
  eTrafoParamDisplayType = 0,
  eTrafoParamDisplayName,
  eTrafoParamDisplayComment,
  eTrafoParamDisplayGroup,
  eTrafoParamDisplayIsSolved,
  eTrafoParamDisplayIsUpdated,
  eTrafoParamDisplayFunctions,
  eTrafoParamDisplayUsedFor,
  eTrafoParamDisplayPreviouslyNeeded,
  eTrafoParamDisplayStDev,

  // trafo param specific attributes
  eTrafoParamDisplayStartSystem = 400,
  eTrafoParamDisplayDestinationSystem,
  eTrafoParamDisplayTranslationX,
  eTrafoParamDisplayTranslationY,
  eTrafoParamDisplayTranslationZ,
  eTrafoParamDisplayRotationX,
  eTrafoParamDisplayRotationY,
  eTrafoParamDisplayRotationZ,
  eTrafoParamDisplayScaleX,
  eTrafoParamDisplayScaleY,
  eTrafoParamDisplayScaleZ,
  eTrafoParamDisplayIsUsed,
  eTrafoParamDisplayValidTime,
  eTrafoParamDisplayIsMovement,
  eTrafoParamDisplayIsDatumTransformation
  // for new enum check range featureDisplayAttributes.append(i)
};

enum ObservationDisplayAttributes {

  eObservationDisplayId = 0,
  eObservationDisplayStation,
  eObservationDisplayTargetGeometries,
  eObservationDisplayX,
  eObservationDisplayY,
  eObservationDisplayZ,
  eObservationDisplayI,
  eObservationDisplayJ,
  eObservationDisplayK,
  eObservationDisplaySigmaX,
  eObservationDisplaySigmaY,
  eObservationDisplaySigmaZ,
  eObservationDisplaySigmaI,
  eObservationDisplaySigmaJ,
  eObservationDisplaySigmaK,
  eObservationDisplayIsValid,
  eObservationDisplayIsSolved,
  eObservationDisplayReadingTime,
  eObservationDisplayVX,
  eObservationDisplayVY,
  eObservationDisplayVZ,
  eObservationDisplayV,
  eObservationDisplayVR,
  eObservationDisplayVI,
  eObservationDisplayVJ,
  eObservationDisplayVK,
  eObservationDisplayIsUsed,
  eObservationDisplayIsDummyPoint

};

enum ReadingDisplayAttributes {
  // order corresponds to the display order
  eReadingDisplayId = 0,
  eReadingDisplayType,
  eReadingDisplayTime,
  eReadingDisplaySensorConfigName,
  eReadingDisplayMeasurementConfigName,
  eReadingDisplaySide,
  eReadingDisplayAzimuth,
  eReadingDisplayZenith,
  eReadingDisplayDistance,
  eReadingDisplayX,
  eReadingDisplayY,
  eReadingDisplayZ,
  eReadingDisplayI,
  eReadingDisplayJ,
  eReadingDisplayK,
  eReadingDisplayTemperature,
  eReadingDisplaySigmaAzimuth,
  eReadingDisplaySigmaZenith,
  eReadingDisplaySigmaDistance,
  eReadingDisplaySigmaX,
  eReadingDisplaySigmaY,
  eReadingDisplaySigmaZ,
  eReadingDisplaySigmaI,
  eReadingDisplaySigmaJ,
  eReadingDisplaySigmaK,
  eReadingDisplaySigmaTemperature,
  eReadingDisplayImported

};

// ###############
// available units
// ###############

enum class UnitType {

  // metric unit types
  eUnitMilliMeter = 0,
  eUnitMeter,
  eUnitInch,

  // angular unit types
  eUnitRadiant,
  eUnitMilliRadians,
  eUnitGon,
  eUnitDecimalDegree,
  eUnitArcSeconds,

  // temperature unit types
  eUnitGrad,
  eUnitKelvin,
  eUnitFahrenheit,

  eNoUnit

};

enum class DimensionType {

  eMetric = 0,
  eAngular,
  eTemperature,
  eDimensionless

};

// #####################
// sensor specific enums
// #####################

enum class SensorFaces {

  eFrontSide = 0,
  eBackSide,
  eUndefinedSide  // e.g. not measured

};

enum class SensorFunctions {

  eMoveAngle = 0,
  eMoveXYZ,
  eMotorState,
  eToggleSight,
  eInitialize,
  eHome,
  eCompensation,
  eMeasure,
  eConnect,
  eDisconnect,
  eStream,
  eSearch
};

/*!
 * \brief The AcquisitionMode enum
 * How a sensor produces data, which is independent of what type of sensor it
 * is and of how many points one acquisition yields.
 *
 * eDiscrete   - start, wait, take the result. Photogrammetry emits thousands
 *               of points from an instant snapshot; a terrestrial scan yields
 *               a whole cloud from one sweep; a touch probe yields one point.
 *               All discrete: there is no live stream to subscribe to.
 * eContinuous - a running, interruptible stream: target tracking, a scanning
 *               probe head, a handheld scanner. Only these can drive live
 *               workflows such as stakeout guidance or averaging windows.
 */
enum class AcquisitionMode {

  eDiscrete = 0,
  eContinuous

};

/*!
 * \brief The AcquisitionResultShape enum
 * D2/D15a - how many Points one acquisition action yields, declared by the
 * driver as a capability. This is orthogonal to AcquisitionMode and to
 * sensor type: it says nothing about *what* a Point means (that never
 * varies, domain-model.md S2), only how many of them one action produces.
 *
 * eOnePoint   - one physical target, however many readings it took
 *               (a TS15 10-count config, a laser tracker 100-sample
 *               average). Every driver in this repo today.
 * eManyPoints - one action yields many distinct targets, one reading each
 *               (a V-STARS snapshot).
 * eCloud      - one action yields a PointCloud, not Points at all (a
 *               terrestrial scan).
 *
 * Only eOnePoint is wired end to end yet (Stage 7f); the others exist so
 * the capability is expressible ahead of the driver that needs it, per D2 -
 * swapping instruments must never change what a Point means.
 */
enum class AcquisitionResultShape {

  eOnePoint = 0,
  eManyPoints,
  eCloud

};

// ################################
// tool types (categories of tools)
// ################################

enum class ToolTypes {

  eReportTool = 0,
  eUnknownTool

};

// #############################
// actual nominal filter options
// #############################

enum class ActualNominalFilter {

  eFilterActualNominal = 0,
  eFilterActual,
  eFilterNominal

};

// ######################################
// materials for temperature compensation
// ######################################
enum class MaterialsTempComp {

  eMaterialSteel = 0,
  eMaterialAluminum,
  eMaterialPlumb,
  eMaterialIron,
  eMaterialGrayCastIron,
  eMaterialCopper,
  eMaterialBrass,
  eMaterialZinc,
  eMaterialPlatinum,
  eMaterialConcrete,
  eMaterialReinforcedConcrete
};

// ##############
// helper classes
// ##############

/*!
 * \brief The ScalarInputParams class
 * Save user specified non-element scalar input parameters
 */
class ScalarInputParams {
 public:
  ScalarInputParams() {}

  bool isValid{false};
  std::map<std::string, double> doubleParameter;
  std::map<std::string, int> intParameter;
  std::map<std::string, std::string> stringParameter;
};

enum class SensorStatus {

  eUnknown = 0,
  eReadyForMeasurement,  // target present, device ready, measurement arm ready
  eNotReadyForMeasurement,  // target not present or device not ready or
                            // measurement arm not ready
  eSensorActionInProgress,
  eClearStatus  // TODO later: define behaviour !!!
};

enum class SensorAction {
  eSensorActionUndefind = 0,  // TODO implement all SensorActions
  eSensorActionMove,          // aim or move
  eSensorActionMeasure,
  eSensorActionAll,
};

}  // namespace oi

#endif  // OI_CORE_UTIL_TYPES_H_
