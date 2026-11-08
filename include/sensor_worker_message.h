#ifndef OI_CORE_SENSOR_WORKER_MESSAGE_H_
#define OI_CORE_SENSOR_WORKER_MESSAGE_H_

#include <string>

#include "core_export.h"

struct OI_CORE_EXPORT SensorWorkerMessage {
  static const std::string NO_SENSOR_INSTANCE;
  static const std::string CONNECTION_WAS_BLOCKED;
  static const std::string MEASUREMENT_DATA_RECEIVED;
  static const std::string MEASUREMENT_DIT_NOT_DELIVER_ANY_RESULTS;

  static const std::string FAILED_TO_MEASURE;  // also used as suffix

  static const std::string MEASUREMENT_FINISHED;  // also used as suffix
  static const std::string SENSOR_IS_NOT_CONNECTED;
  static const std::string SENSOR_DISCONNECTED;
  static const std::string SENSOR_CONNECTED;
  static const std::string SENSOR_IS_ALREADY_CONNECTED;
  static const std::string FAILED_TO_CONNECT_SENSOR;
  static const std::string FAILED_TO_DISCONNECT_SENSOR;
  static const std::string FAILED_TO_MOVE_SENSOR;

  static const std::string MOVING_SENSOR_FINISHED_PREFIX;  // prefix
  static const std::string MOVING_SENSOR_FINISHED_MEASUREMENT_FINISHED;
  static const std::string MOVING_SENSOR_FINISHED_FAILED_TO_MEASURE;

  static const std::string FAILED_TO_INITIALIZE_SENSOR;
  static const std::string INITIALIZING_FINISHED;

  static const std::string FAILED_TO_CHANGE_MOTOR_STATE;
  static const std::string CHANGING_MOTOR_STATE_FINISHED;

  static const std::string FAILED_TO_SET_SENSOR_TO_HOME_POSITION;
  static const std::string SETTING_HOME_FINISHED;

  static const std::string FAILED_TO_TOGGLE_SIGHT_ORIENTATION;
  static const std::string TOGGLE_SIGHT_ORIENTATION_FINISHED;

  static const std::string FAILED_TO_START_COMPENSATION;
  static const std::string STARTING_COMPENSATION_FINISHED;

  static const std::string FAILED_TO_DO_SELF_DEFINED_ACTION;
  static const std::string SELF_DEFINED_ACTION_FINISHED;

  static const std::string SEARCH_FINISHED;
  static const std::string FAILED_TO_SEARCH;
};
#endif  // OI_CORE_SENSOR_WORKER_MESSAGE_H_
