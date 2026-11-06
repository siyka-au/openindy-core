#include "convert.h"

#include "sensortype.h"

namespace oi::codec {

namespace oipb = openindy::v1;

namespace {

// The wire enums have their own numbering (0 is always "unspecified"), so
// every value is mapped explicitly instead of cast.

oipb::MeasurementMode toProto(MeasurementModes mode) {
  switch (mode) {
    case MeasurementModes::eFast_MeasurementMode:
      return oipb::MEASUREMENT_MODE_FAST;
    case MeasurementModes::eStandard_MeasurementMode:
      return oipb::MEASUREMENT_MODE_STANDARD;
    case MeasurementModes::ePrecise_MeasurementMode:
      return oipb::MEASUREMENT_MODE_PRECISE;
  }
  return oipb::MEASUREMENT_MODE_UNSPECIFIED;
}

MeasurementModes fromProto(oipb::MeasurementMode mode) {
  switch (mode) {
    case oipb::MEASUREMENT_MODE_FAST:
      return MeasurementModes::eFast_MeasurementMode;
    case oipb::MEASUREMENT_MODE_PRECISE:
      return MeasurementModes::ePrecise_MeasurementMode;
    default:
      return MeasurementModes::eStandard_MeasurementMode;
  }
}

oipb::MeasurementType toProto(MeasurementTypes type) {
  switch (type) {
    case MeasurementTypes::eSinglePoint_MeasurementType:
      return oipb::MEASUREMENT_TYPE_SINGLE_POINT;
    case MeasurementTypes::eScanTimeDependent_MeasurementType:
      return oipb::MEASUREMENT_TYPE_SCAN_TIME_DEPENDENT;
    case MeasurementTypes::eScanDistanceDependent_MeasurementType:
      return oipb::MEASUREMENT_TYPE_SCAN_DISTANCE_DEPENDENT;
    case MeasurementTypes::eLevel_MeasurementType:
      return oipb::MEASUREMENT_TYPE_LEVEL;
    case MeasurementTypes::eDirection_MeasurementType:
      return oipb::MEASUREMENT_TYPE_DIRECTION;
    case MeasurementTypes::eDistance_MeasurementType:
      return oipb::MEASUREMENT_TYPE_DISTANCE;
    case MeasurementTypes::eTemperature_MeasurementType:
      return oipb::MEASUREMENT_TYPE_TEMPERATURE;
    case MeasurementTypes::eUnknown_MeasurementType:
      break;
  }
  return oipb::MEASUREMENT_TYPE_UNSPECIFIED;
}

MeasurementTypes fromProto(oipb::MeasurementType type) {
  switch (type) {
    case oipb::MEASUREMENT_TYPE_SINGLE_POINT:
      return MeasurementTypes::eSinglePoint_MeasurementType;
    case oipb::MEASUREMENT_TYPE_SCAN_TIME_DEPENDENT:
      return MeasurementTypes::eScanTimeDependent_MeasurementType;
    case oipb::MEASUREMENT_TYPE_SCAN_DISTANCE_DEPENDENT:
      return MeasurementTypes::eScanDistanceDependent_MeasurementType;
    case oipb::MEASUREMENT_TYPE_LEVEL:
      return MeasurementTypes::eLevel_MeasurementType;
    case oipb::MEASUREMENT_TYPE_DIRECTION:
      return MeasurementTypes::eDirection_MeasurementType;
    case oipb::MEASUREMENT_TYPE_DISTANCE:
      return MeasurementTypes::eDistance_MeasurementType;
    case oipb::MEASUREMENT_TYPE_TEMPERATURE:
      return MeasurementTypes::eTemperature_MeasurementType;
    default:
      return MeasurementTypes::eUnknown_MeasurementType;
  }
}

}  // namespace

oipb::MeasurementConfig toProto(const MeasurementConfig& config) {
  oipb::MeasurementConfig message;
  message.set_name(config.getName().toStdString());
  message.set_mode(toProto(config.getMeasurementMode()));
  message.set_type(toProto(config.getMeasurementType()));
  message.set_measure_two_sides(config.getMeasureTwoSides());
  message.set_max_observations(config.getMaxObservations());
  message.set_time_interval(config.getTimeInterval());
  message.set_distance_interval(config.getDistanceInterval());
  if (config.getSensorType().isValid()) {
    message.set_sensor_type(config.getSensorType().id().toStdString());
  }
  return message;
}

std::optional<MeasurementConfig> fromProto(
    const oipb::MeasurementConfig& message) {
  if (message.name().empty()) {
    return std::nullopt;
  }
  MeasurementConfig config;
  config.setName(QString::fromStdString(message.name()));
  config.setMeasurementMode(fromProto(message.mode()));
  config.setMeasurementType(fromProto(message.type()));
  config.setMeasureTwoSides(message.measure_two_sides());
  config.setMaxObservations(message.max_observations());
  config.setTimeInterval(message.time_interval());
  config.setDistanceInterval(message.distance_interval());
  if (message.has_sensor_type()) {
    // an id this build does not know leaves the config ungated rather than
    // failing the whole project
    config.setSensorType(
        SensorTypeRegistry::byId(QString::fromStdString(message.sensor_type()))
            .value_or(SensorType()));
  }
  return config;
}

}  // namespace oi::codec
