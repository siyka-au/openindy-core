#ifndef UTIL_H
#define UTIL_H
#include <string>
#include <vector>

#include "core_export.h"
#include "types.h"

namespace oi {

class FeatureWrapper;

// #######################################
// global functions to query element types
// #######################################

std::vector<ElementTypes> OI_CORE_EXPORT getAvailableElementTypes();
std::vector<FeatureTypes> OI_CORE_EXPORT getAvailableFeatureTypes();
std::vector<GeometryTypes> OI_CORE_EXPORT getAvailableGeometryTypes();

// #############################################################################
// TYPE NAMES - two separate lookups, and the difference matters
// #############################################################################
//
//  *TypeName()        -> the PERSISTED TOKEN. Stable identifier. NOT for
//  display. *TypeDisplayName() -> what the user reads. Free to reword and
//  re-case.
//
//  The token functions below are what a saved project actually contains:
//
//      plane.setAttribute("type", getGeometryTypeName(ePlaneGeometry));
//
//  and they are read back with an exact, case-sensitive QMap::key() lookup, so
//  changing one does not fail loudly - the geometry simply becomes eUndefined
//  on load. Type names are also interpolated into settings keys
//  ("defaultFunction.%1", "defaultMeasurementConfig.%1"), so a changed token
//  silently orphans every saved default as well.
//
//  That is why these are two tables rather than one string transformed for
//  display: editing what a user reads must not be able to change what a file
//  contains. tst_typenames pins every token against a golden list and will
//  fail if one moves.
//
//  Rule of thumb: if the string reaches a file, a settings key, or a
//  reverse lookup, it is a token. If it reaches a widget, it is a display name.

const std::string OI_CORE_EXPORT& getElementTypeName(
    const ElementTypes& type);  // persisted token
const std::string OI_CORE_EXPORT& getElementTypePluralName(
    const ElementTypes&
        type);  // persisted token (getElementTypeEnum falls back to it)
ElementTypes OI_CORE_EXPORT getElementTypeEnum(
    const std::string& name);  // token -> enum; never accepts a display name
ElementTypes OI_CORE_EXPORT getElementTypeEnum(const FeatureTypes& type);
ElementTypes OI_CORE_EXPORT getElementTypeEnum(const ReadingTypes& type);

const std::string OI_CORE_EXPORT& getElementTypeDisplayName(
    const ElementTypes& type);
const std::string OI_CORE_EXPORT& getElementTypeDisplayPluralName(
    const ElementTypes& type);

const std::string OI_CORE_EXPORT& getFeatureTypeName(
    const FeatureTypes& type);  // persisted token
FeatureTypes OI_CORE_EXPORT
getFeatureTypeEnum(const std::string& name);  // token -> enum
const std::string OI_CORE_EXPORT& getFeatureTypeDisplayName(
    const FeatureTypes& type);

const std::string OI_CORE_EXPORT& getGeometryTypeName(
    const GeometryTypes& type);  // persisted token
GeometryTypes OI_CORE_EXPORT
getGeometryTypeEnum(const std::string& name);  // token -> enum
GeometryTypes OI_CORE_EXPORT getGeometryTypeEnum(const FeatureTypes& type);
const std::string OI_CORE_EXPORT& getGeometryTypeDisplayName(
    const GeometryTypes& type);

bool OI_CORE_EXPORT getIsFeature(const ElementTypes& type);
bool OI_CORE_EXPORT getIsReading(const ElementTypes& type);
bool OI_CORE_EXPORT getIsObservation(const ElementTypes& type);

bool OI_CORE_EXPORT getIsGeometry(const FeatureTypes& type);
bool OI_CORE_EXPORT getIsScalarEntity(const FeatureTypes& type);

// #########################################################
// global function to query icon resources for element types
// #########################################################

const std::string OI_CORE_EXPORT& getElementTypeIconPath(
    const ElementTypes& type);
const std::string OI_CORE_EXPORT& getFeatureTypeIconPath(
    const FeatureTypes& type);

// #######################################
// global functions to query reading types
// #######################################

const std::string OI_CORE_EXPORT& getReadingTypeName(const ReadingTypes& type);
ReadingTypes OI_CORE_EXPORT getReadingTypeEnum(const std::string& name);

// ############################################
// global functions to query display attributes
// ############################################

const std::vector<int> OI_CORE_EXPORT& getFeatureDisplayAttributes();
const std::vector<ObservationDisplayAttributes> OI_CORE_EXPORT&
getObservationDisplayAttributes();
const std::vector<ReadingDisplayAttributes> OI_CORE_EXPORT&
getReadingDisplayAttributes();

bool OI_CORE_EXPORT getIsFeatureDisplayAttribute(const int& attr);
bool OI_CORE_EXPORT getIsTrafoParamDisplayAttribute(const int& attr);

const std::string OI_CORE_EXPORT& getFeatureDisplayAttributeName(
    const int& attr);
const std::string OI_CORE_EXPORT& getFeatureDisplayAttributeName(
    const FeatureDisplayAttributes& attr);
const std::string OI_CORE_EXPORT& getFeatureDisplayAttributeName(
    const TrafoParamDisplayAttributes& attr);

bool OI_CORE_EXPORT getReadingDisplayAttributeVisibility(
    const ReadingDisplayAttributes& attr, const ReadingTypes& type);

const std::string OI_CORE_EXPORT& getObservationDisplayAttributesName(
    const ObservationDisplayAttributes& attr);
const std::string OI_CORE_EXPORT& getReadingDisplayAttributeName(
    const ReadingDisplayAttributes& attr);

ObservationDisplayAttributes OI_CORE_EXPORT
getObservationDisplayAttributeEnum(const std::string& name);
ReadingDisplayAttributes OI_CORE_EXPORT
getReadingDisplayAttributeEnum(const std::string& name);

// ###############################
// global functions to query units
// ###############################

double OI_CORE_EXPORT convertToDefault(const double& value,
                                       const UnitType& type);
double OI_CORE_EXPORT convertFromDefault(const double& value,
                                         const UnitType& type);

const std::string OI_CORE_EXPORT& getUnitTypeName(const UnitType& type);
UnitType OI_CORE_EXPORT getUnitTypeEnum(const std::string& name);

// #######################################
// global function for floating comparison
// #######################################

bool OI_CORE_EXPORT almostEqual(const double& a, const double& b,
                                const int& digits);

// ######################################
// global functions to query sensor types
// ######################################

// ##########################################
// global functions to query sensor functions
// ##########################################

std::vector<SensorFunctions> OI_CORE_EXPORT getAvailableSensorFunctions();
const std::string OI_CORE_EXPORT& getSensorFunctionName(
    const SensorFunctions& type);
SensorFunctions OI_CORE_EXPORT getSensorFunctionEnum(const std::string& name);

// #########################################
// global function to query connection types
// #########################################

// ####################################
// global functions to query tool types
// ####################################

std::vector<ToolTypes> OI_CORE_EXPORT getAvailableToolTypes();
const std::string OI_CORE_EXPORT& getToolTypeName(const ToolTypes& type);
ToolTypes OI_CORE_EXPORT getToolTypeEnum(const std::string& name);

// #######################################
// global functions to query message types
// #######################################

std::vector<MessageTypes> OI_CORE_EXPORT getAvailableMessageTypes();
const std::string OI_CORE_EXPORT& getMessageTypeName(const MessageTypes& type);
MessageTypes OI_CORE_EXPORT getMessageTypeEnum(const std::string& name);

// ############################################
// global functions to query unknown parameters
// ############################################

std::vector<GeometryParameters> OI_CORE_EXPORT getAvailableGeometryParameters();
const std::string OI_CORE_EXPORT& getGeometryParameterName(
    const GeometryParameters& parameter);
GeometryParameters OI_CORE_EXPORT
getGeometryParameterEnum(const std::string& name);

std::vector<GeometryParameters> OI_CORE_EXPORT
getGeometryParameters(const GeometryTypes& type);

std::vector<TrafoParamParameters> OI_CORE_EXPORT
getAvailableTrafoParamParameters();
const std::string OI_CORE_EXPORT& getTrafoParamParameterName(
    const TrafoParamParameters& parameter);
TrafoParamParameters OI_CORE_EXPORT
getTrafoParamParameterEnum(const std::string& name);

// #######################################################
// global functions to query actual nominal filter options
// #######################################################

std::vector<ActualNominalFilter> OI_CORE_EXPORT
getAvailableActualNominalFilters();
const std::string OI_CORE_EXPORT& getActualNominalFilterName(
    const ActualNominalFilter& type);
ActualNominalFilter OI_CORE_EXPORT
getActualNominalFilterEnum(const std::string& name);

const std::string OI_CORE_EXPORT& getMaterialName(
    const MaterialsTempComp& material);
double OI_CORE_EXPORT getMaterialValue(const MaterialsTempComp& material);
double OI_CORE_EXPORT getTemperatureExpansion(const std::string material,
                                              double actual, double nominal);
std::vector<std::string> OI_CORE_EXPORT getMaterials();

}  // namespace oi

#endif  // UTIL_H
