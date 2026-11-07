#include "sensorworkermessage.h"

const std::string SensorWorkerMessage::NO_SENSOR_INSTANCE =
    "no sensor instance";
const std::string SensorWorkerMessage::CONNECTION_WAS_BLOCKED =
    "connection was blocked - please try again";
const std::string SensorWorkerMessage::MEASUREMENT_DATA_RECEIVED =
    "measurement data received";
const std::string SensorWorkerMessage::MEASUREMENT_DIT_NOT_DELIVER_ANY_RESULTS =
    "measurement did not deliver any results";

const std::string SensorWorkerMessage::FAILED_TO_MEASURE =
    "failed to measure";  // also used as suffix

const std::string SensorWorkerMessage::MEASUREMENT_FINISHED =
    "measurement finished";
const std::string SensorWorkerMessage::SENSOR_IS_NOT_CONNECTED =
    "sensor is not connected";
const std::string SensorWorkerMessage::SENSOR_DISCONNECTED =
    "sensor disconnected";
const std::string SensorWorkerMessage::SENSOR_CONNECTED = "sensor connected";
const std::string SensorWorkerMessage::SENSOR_IS_ALREADY_CONNECTED =
    "sensor is already connected";
const std::string SensorWorkerMessage::FAILED_TO_CONNECT_SENSOR =
    "failed to connect sensor";
const std::string SensorWorkerMessage::FAILED_TO_DISCONNECT_SENSOR =
    "failed to disconnect sensor";
const std::string SensorWorkerMessage::FAILED_TO_MOVE_SENSOR =
    "failed to move sensor";

const std::string SensorWorkerMessage::MOVING_SENSOR_FINISHED_PREFIX =
    "moving sensor finished";
const std::string
    SensorWorkerMessage::MOVING_SENSOR_FINISHED_MEASUREMENT_FINISHED =
        SensorWorkerMessage::MOVING_SENSOR_FINISHED_PREFIX + ", " +
        SensorWorkerMessage::MEASUREMENT_FINISHED;
const std::string
    SensorWorkerMessage::MOVING_SENSOR_FINISHED_FAILED_TO_MEASURE =
        SensorWorkerMessage::MOVING_SENSOR_FINISHED_PREFIX + ", " +
        SensorWorkerMessage::FAILED_TO_MEASURE;

const std::string SensorWorkerMessage::FAILED_TO_INITIALIZE_SENSOR =
    "failed to initialize sensor";
const std::string SensorWorkerMessage::INITIALIZING_FINISHED =
    "initializing finished";

const std::string SensorWorkerMessage::FAILED_TO_CHANGE_MOTOR_STATE =
    "failed to change motor state";
const std::string SensorWorkerMessage::CHANGING_MOTOR_STATE_FINISHED =
    "changing motor state finished";

const std::string SensorWorkerMessage::FAILED_TO_SET_SENSOR_TO_HOME_POSITION =
    "failed to set sensor to home position";
const std::string SensorWorkerMessage::SETTING_HOME_FINISHED =
    "setting home finished";

const std::string SensorWorkerMessage::FAILED_TO_TOGGLE_SIGHT_ORIENTATION =
    "failed to toggle sight orientation";
const std::string SensorWorkerMessage::TOGGLE_SIGHT_ORIENTATION_FINISHED =
    "toggle sight orientation finished";

const std::string SensorWorkerMessage::FAILED_TO_START_COMPENSATION =
    "failed to start compensation";
const std::string SensorWorkerMessage::STARTING_COMPENSATION_FINISHED =
    "starting compensation finished";

const std::string SensorWorkerMessage::FAILED_TO_DO_SELF_DEFINED_ACTION =
    "failed to do self defined action";
const std::string SensorWorkerMessage::SELF_DEFINED_ACTION_FINISHED =
    "self defined action finished";

const std::string SensorWorkerMessage::SEARCH_FINISHED = "search finished";
const std::string SensorWorkerMessage::FAILED_TO_SEARCH = "failed to search";
