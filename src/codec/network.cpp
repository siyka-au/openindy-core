// Remote-control protocol enum mappings (see network_convert.h).

#include "network_convert.h"

namespace oi::codec {

namespace oipb = openindy::v1;
using Err = OiRequestResponse::ErrorCode;
using Req = OiRequestResponse::RequestType;

oipb::ErrorCode toProto(Err e) {
  switch (e) {
    case Err::eNoError:
      return oipb::ERROR_CODE_NONE;
    case Err::eNoJob:
      return oipb::ERROR_CODE_NO_JOB;
    case Err::eWrongFormat:
      return oipb::ERROR_CODE_WRONG_FORMAT;
    case Err::eUnknownRequestType:
      return oipb::ERROR_CODE_UNKNOWN_REQUEST_TYPE;
    case Err::eNoActiveFeature:
      return oipb::ERROR_CODE_NO_ACTIVE_FEATURE;
    case Err::eNoActiveStation:
      return oipb::ERROR_CODE_NO_ACTIVE_STATION;
    case Err::eNoActiveCoordinateSystem:
      return oipb::ERROR_CODE_NO_ACTIVE_COORDINATE_SYSTEM;
    case Err::eNoFeatureWithId:
      return oipb::ERROR_CODE_NO_FEATURE_WITH_ID;
    case Err::eNoTransformationParameters:
      return oipb::ERROR_CODE_NO_TRANSFORMATION_PARAMETERS;
    case Err::eTaskInProcess:
      return oipb::ERROR_CODE_TASK_IN_PROCESS;
    case Err::eNoTask:
      return oipb::ERROR_CODE_NO_TASK;
    case Err::eNoSensor:
      return oipb::ERROR_CODE_NO_SENSOR;
    case Err::eNoTaskWithId:
      return oipb::ERROR_CODE_NO_TASK_WITH_ID;
    case Err::eCannotMeasureNominal:
      return oipb::ERROR_CODE_CANNOT_MEASURE_NOMINAL;
    case Err::eMeasurementError:
      return oipb::ERROR_CODE_MEASUREMENT_ERROR;
    case Err::eNoSensorConnected:
      return oipb::ERROR_CODE_NO_SENSOR_CONNECTED;
    case Err::eFeatureNotSolved:
      return oipb::ERROR_CODE_FEATURE_NOT_SOLVED;
    case Err::eCreateFeatureError:
      return oipb::ERROR_CODE_CREATE_FEATURE_ERROR;
    case Err::eNoMeasurementConfigManager:
      return oipb::ERROR_CODE_NO_MEASUREMENT_CONFIG_MANAGER;
    case Err::eNoSensorConfigManager:
      return oipb::ERROR_CODE_NO_SENSOR_CONFIG_MANAGER;
    case Err::eNoMeasurementConfig:
      return oipb::ERROR_CODE_NO_MEASUREMENT_CONFIG;
  }
  // not reachable for a valid enumerator; report the most generic failure
  return oipb::ERROR_CODE_WRONG_FORMAT;
}

std::optional<Err> fromProto(oipb::ErrorCode e) {
  switch (e) {
    case oipb::ERROR_CODE_NONE:
      return Err::eNoError;
    case oipb::ERROR_CODE_NO_JOB:
      return Err::eNoJob;
    case oipb::ERROR_CODE_WRONG_FORMAT:
      return Err::eWrongFormat;
    case oipb::ERROR_CODE_UNKNOWN_REQUEST_TYPE:
      return Err::eUnknownRequestType;
    case oipb::ERROR_CODE_NO_ACTIVE_FEATURE:
      return Err::eNoActiveFeature;
    case oipb::ERROR_CODE_NO_ACTIVE_STATION:
      return Err::eNoActiveStation;
    case oipb::ERROR_CODE_NO_ACTIVE_COORDINATE_SYSTEM:
      return Err::eNoActiveCoordinateSystem;
    case oipb::ERROR_CODE_NO_FEATURE_WITH_ID:
      return Err::eNoFeatureWithId;
    case oipb::ERROR_CODE_NO_TRANSFORMATION_PARAMETERS:
      return Err::eNoTransformationParameters;
    case oipb::ERROR_CODE_TASK_IN_PROCESS:
      return Err::eTaskInProcess;
    case oipb::ERROR_CODE_NO_TASK:
      return Err::eNoTask;
    case oipb::ERROR_CODE_NO_SENSOR:
      return Err::eNoSensor;
    case oipb::ERROR_CODE_NO_TASK_WITH_ID:
      return Err::eNoTaskWithId;
    case oipb::ERROR_CODE_CANNOT_MEASURE_NOMINAL:
      return Err::eCannotMeasureNominal;
    case oipb::ERROR_CODE_MEASUREMENT_ERROR:
      return Err::eMeasurementError;
    case oipb::ERROR_CODE_NO_SENSOR_CONNECTED:
      return Err::eNoSensorConnected;
    case oipb::ERROR_CODE_FEATURE_NOT_SOLVED:
      return Err::eFeatureNotSolved;
    case oipb::ERROR_CODE_CREATE_FEATURE_ERROR:
      return Err::eCreateFeatureError;
    case oipb::ERROR_CODE_NO_MEASUREMENT_CONFIG_MANAGER:
      return Err::eNoMeasurementConfigManager;
    case oipb::ERROR_CODE_NO_SENSOR_CONFIG_MANAGER:
      return Err::eNoSensorConfigManager;
    case oipb::ERROR_CODE_NO_MEASUREMENT_CONFIG:
      return Err::eNoMeasurementConfig;
    default:
      break;
  }
  return std::nullopt;
}

oipb::RequestKind toProto(Req t) {
  switch (t) {
    case Req::eGetProject:
      return oipb::REQUEST_KIND_GET_PROJECT;
    case Req::eGetActiveFeature:
      return oipb::REQUEST_KIND_GET_ACTIVE_FEATURE;
    case Req::eSetActiveFeature:
      return oipb::REQUEST_KIND_SET_ACTIVE_FEATURE;
    case Req::eGetActiveStation:
      return oipb::REQUEST_KIND_GET_ACTIVE_STATION;
    case Req::eSetActiveStation:
      return oipb::REQUEST_KIND_SET_ACTIVE_STATION;
    case Req::eGetActiveCoordinateSystem:
      return oipb::REQUEST_KIND_GET_ACTIVE_COORDINATE_SYSTEM;
    case Req::eSetActiveCoordinateSystem:
      return oipb::REQUEST_KIND_SET_ACTIVE_COORDINATE_SYSTEM;
    case Req::eAim:
      return oipb::REQUEST_KIND_AIM;
    case Req::eMeasure:
      return oipb::REQUEST_KIND_MEASURE;
    case Req::eStartWatchwindow:
      return oipb::REQUEST_KIND_START_WATCHWINDOW;
    case Req::eStopWatchwindow:
      return oipb::REQUEST_KIND_STOP_WATCHWINDOW;
    case Req::eOiToolRequest:
      return oipb::REQUEST_KIND_TOOL;
    case Req::eGetFeatures:
      return oipb::REQUEST_KIND_GET_FEATURES;
    case Req::eAddFeatures:
      return oipb::REQUEST_KIND_ADD_FEATURES;
    case Req::eGetObservations:
      return oipb::REQUEST_KIND_GET_OBSERVATIONS;
    case Req::eRemoveObservations:
      return oipb::REQUEST_KIND_REMOVE_OBSERVATIONS;
    case Req::eGetParameters:
      return oipb::REQUEST_KIND_GET_PARAMETERS;
    case Req::eGetMeasurementConfigs:
      return oipb::REQUEST_KIND_GET_MEASUREMENT_CONFIGS;
    case Req::eGetMeasurementConfig:
      return oipb::REQUEST_KIND_GET_MEASUREMENT_CONFIG;
    case Req::eSetMeasurementConfig:
      return oipb::REQUEST_KIND_SET_MEASUREMENT_CONFIG;
    case Req::eGetCoordinateSystems:
      return oipb::REQUEST_KIND_GET_COORDINATE_SYSTEMS;
    default:
      break;
  }
  return oipb::REQUEST_KIND_UNSPECIFIED;
}

Req fromProto(oipb::RequestKind k) {
  switch (k) {
    case oipb::REQUEST_KIND_GET_PROJECT:
      return Req::eGetProject;
    case oipb::REQUEST_KIND_GET_ACTIVE_FEATURE:
      return Req::eGetActiveFeature;
    case oipb::REQUEST_KIND_SET_ACTIVE_FEATURE:
      return Req::eSetActiveFeature;
    case oipb::REQUEST_KIND_GET_ACTIVE_STATION:
      return Req::eGetActiveStation;
    case oipb::REQUEST_KIND_SET_ACTIVE_STATION:
      return Req::eSetActiveStation;
    case oipb::REQUEST_KIND_GET_ACTIVE_COORDINATE_SYSTEM:
      return Req::eGetActiveCoordinateSystem;
    case oipb::REQUEST_KIND_SET_ACTIVE_COORDINATE_SYSTEM:
      return Req::eSetActiveCoordinateSystem;
    case oipb::REQUEST_KIND_AIM:
      return Req::eAim;
    case oipb::REQUEST_KIND_MEASURE:
      return Req::eMeasure;
    case oipb::REQUEST_KIND_START_WATCHWINDOW:
      return Req::eStartWatchwindow;
    case oipb::REQUEST_KIND_STOP_WATCHWINDOW:
      return Req::eStopWatchwindow;
    case oipb::REQUEST_KIND_TOOL:
      return Req::eOiToolRequest;
    case oipb::REQUEST_KIND_GET_FEATURES:
      return Req::eGetFeatures;
    case oipb::REQUEST_KIND_ADD_FEATURES:
      return Req::eAddFeatures;
    case oipb::REQUEST_KIND_GET_OBSERVATIONS:
      return Req::eGetObservations;
    case oipb::REQUEST_KIND_REMOVE_OBSERVATIONS:
      return Req::eRemoveObservations;
    case oipb::REQUEST_KIND_GET_PARAMETERS:
      return Req::eGetParameters;
    case oipb::REQUEST_KIND_GET_MEASUREMENT_CONFIGS:
      return Req::eGetMeasurementConfigs;
    case oipb::REQUEST_KIND_GET_MEASUREMENT_CONFIG:
      return Req::eGetMeasurementConfig;
    case oipb::REQUEST_KIND_SET_MEASUREMENT_CONFIG:
      return Req::eSetMeasurementConfig;
    case oipb::REQUEST_KIND_GET_COORDINATE_SYSTEMS:
      return Req::eGetCoordinateSystems;
    default:
      break;
  }
  return Req::eUnknownRequest;
}

oipb::ReadingKind toProto(ReadingTypes t) {
  switch (t) {
    case ReadingTypes::eDistanceReading:
      return oipb::READING_KIND_DISTANCE;
    case ReadingTypes::eCartesianReading:
      return oipb::READING_KIND_CARTESIAN;
    case ReadingTypes::ePolarReading:
      return oipb::READING_KIND_POLAR;
    case ReadingTypes::eDirectionReading:
      return oipb::READING_KIND_DIRECTION;
    case ReadingTypes::eTemperatureReading:
      return oipb::READING_KIND_TEMPERATURE;
    case ReadingTypes::eLevelReading:
      return oipb::READING_KIND_LEVEL;
    case ReadingTypes::eCartesianReading6D:
      return oipb::READING_KIND_CARTESIAN_6D;
    case ReadingTypes::eUndefinedReading:
      break;
  }
  return oipb::READING_KIND_UNSPECIFIED;
}

std::optional<ReadingTypes> fromProto(oipb::ReadingKind k) {
  switch (k) {
    case oipb::READING_KIND_DISTANCE:
      return ReadingTypes::eDistanceReading;
    case oipb::READING_KIND_CARTESIAN:
      return ReadingTypes::eCartesianReading;
    case oipb::READING_KIND_POLAR:
      return ReadingTypes::ePolarReading;
    case oipb::READING_KIND_DIRECTION:
      return ReadingTypes::eDirectionReading;
    case oipb::READING_KIND_TEMPERATURE:
      return ReadingTypes::eTemperatureReading;
    case oipb::READING_KIND_LEVEL:
      return ReadingTypes::eLevelReading;
    case oipb::READING_KIND_CARTESIAN_6D:
      return ReadingTypes::eCartesianReading6D;
    default:
      break;
  }
  return std::nullopt;
}

oipb::MessageKind toProto(MessageTypes t) {
  switch (t) {
    case MessageTypes::eInformationMessage:
      return oipb::MESSAGE_KIND_INFORMATION;
    case MessageTypes::eWarningMessage:
      return oipb::MESSAGE_KIND_WARNING;
    case MessageTypes::eErrorMessage:
      return oipb::MESSAGE_KIND_ERROR;
    case MessageTypes::eCriticalMessage:
      return oipb::MESSAGE_KIND_CRITICAL;
    case MessageTypes::eQuestionMessage:
      return oipb::MESSAGE_KIND_QUESTION;
  }
  return oipb::MESSAGE_KIND_UNSPECIFIED;
}

std::optional<MessageTypes> fromProto(oipb::MessageKind k) {
  switch (k) {
    case oipb::MESSAGE_KIND_INFORMATION:
      return MessageTypes::eInformationMessage;
    case oipb::MESSAGE_KIND_WARNING:
      return MessageTypes::eWarningMessage;
    case oipb::MESSAGE_KIND_ERROR:
      return MessageTypes::eErrorMessage;
    case oipb::MESSAGE_KIND_CRITICAL:
      return MessageTypes::eCriticalMessage;
    case oipb::MESSAGE_KIND_QUESTION:
      return MessageTypes::eQuestionMessage;
    default:
      break;
  }
  return std::nullopt;
}

}  // namespace oi::codec
