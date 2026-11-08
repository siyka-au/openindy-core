#ifndef OI_CORE_OI_REQUEST_RESPONSE_H_
#define OI_CORE_OI_REQUEST_RESPONSE_H_

#include <string>

#include "types.h"

namespace oi {

/*!
 * \brief The OiRequestResponse class
 * Carries one remote-control request and its response between the websocket
 * layer and the request handler. The request and response are serialized
 * protocol messages (see proto/openindy/network.proto); they are plain bytes
 * here so that this header, which plugins see, stays free of protobuf.
 */
class OiRequestResponse {
 public:
  enum class RequestType {

    eGetProject = 0,
    eGetActiveFeature,
    eSetActiveFeature,
    eGetActiveStation,
    eSetActiveStation,
    eGetActiveCoordinateSystem,
    eSetActiveCoordinateSystem,
    eAim,
    eMeasure,
    eStartWatchwindow,
    eStopWatchwindow,
    eOiToolRequest,
    eGetFeatures,
    eAddFeatures,
    eGetObservations,
    eRemoveObservations,
    eGetParameters,
    eGetMeasurementConfigs,
    eGetMeasurementConfig,
    eSetMeasurementConfig,
    eGetCoordinateSystems,

    eUnknownRequest = 999,

    eSensorActionStarted = 1001,
    eSensorActionFinished,
    eMessageBox,
    eRealTimeReading,
    eActiveFeatureChanged,
    eActiveStationChanged,
    eActiveCoordinateSystemChanged,
    eFeatureSetChanged,
    eFeatureAttributesChanged

  };

  enum class ErrorCode {

    eNoError = 0,
    eNoJob,
    eWrongFormat,
    eUnknownRequestType,
    eNoActiveFeature,
    eNoActiveStation,
    eNoActiveCoordinateSystem,
    eNoFeatureWithId,
    eNoTransformationParameters,
    eTaskInProcess,
    eNoTask,
    eNoSensor,
    eNoTaskWithId,
    eCannotMeasureNominal,
    eMeasurementError,
    eNoSensorConnected,
    eFeatureNotSolved,
    eCreateFeatureError,
    eNoMeasurementConfigManager,
    eNoSensorConfigManager,
    eNoMeasurementConfig

  };

  RequestType myRequestType{
      RequestType::eUnknownRequest};         // defines the type of request
  ErrorCode errorCode{ErrorCode::eNoError};  // outcome of the request
  std::string request;   // the serialized request message (bytes)
  std::string response;  // the serialized response message (bytes)
  int requesterId{};  // identifies the requester so that the response is send
                      // only to him
};

}  // namespace oi

#endif  // OI_CORE_OI_REQUEST_RESPONSE_H_
