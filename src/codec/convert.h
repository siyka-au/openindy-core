#ifndef OI_CODEC_CONVERT_H_
#define OI_CODEC_CONVERT_H_

// Internal: conversions between the core model and the protobuf messages.
// The public face of the codec is bytes in, bytes out (see project_codec.h
// once it exists); nothing outside core includes this header. It is exported
// only so the unit tests can reach it.

#include <optional>

#include "core_export.h"
#include "measurementconfig.h"
#include "openindy/project.pb.h"

namespace oi::codec {

[[nodiscard]] OI_CORE_EXPORT openindy::v1::MeasurementConfig toProto(
    const MeasurementConfig& config);

/*! Empty if the message has no name, which no valid config can have. The
 *  caller decides the config type (user, project, standard). */
[[nodiscard]] OI_CORE_EXPORT std::optional<MeasurementConfig> fromProto(
    const openindy::v1::MeasurementConfig& message);

}  // namespace oi::codec

#endif  // OI_CODEC_CONVERT_H_
