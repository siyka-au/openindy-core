#ifndef OI_CONFIG_CODEC_H_
#define OI_CONFIG_CODEC_H_

// Config files: the human-editable measurement and sensor configurations,
// stored as the protobuf JSON mapping (messages MeasurementConfig and
// SensorConfiguration in proto/openindy/project.proto).
//
// Naming convention: JSON keys are the proto field names (snake_case, e.g.
// "measure_two_sides"), enums are their proto names ("MEASUREMENT_MODE_FAST").
// Writing is pretty printed and always prints every field, so a file shows
// everything a user can edit. Reading accepts both snake_case and camelCase
// keys and ignores keys it does not know, so files written by a newer version
// still load. Decoding never throws or crashes: any failure (bad JSON, wrong
// types, no name) is an empty optional.

#include <optional>
#include <string>
#include <string_view>

#include "measurement_config.h"
#include "sensor_configuration.h"

namespace oi::codec {

[[nodiscard]] std::string encodeMeasurementConfig(
    const MeasurementConfig& config);
[[nodiscard]] std::optional<MeasurementConfig> decodeMeasurementConfig(
    std::string_view json);

[[nodiscard]] std::string encodeSensorConfiguration(
    const SensorConfiguration& config);
[[nodiscard]] std::optional<SensorConfiguration> decodeSensorConfiguration(
    std::string_view json);

}  // namespace oi::codec

#endif  // OI_CONFIG_CODEC_H_
