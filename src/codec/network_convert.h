#ifndef OI_CODEC_NETWORK_CONVERT_H_
#define OI_CODEC_NETWORK_CONVERT_H_

// Internal: mappings between the remote-control model's enums and the wire
// enums of proto/openindy/network.proto. Every value is mapped explicitly;
// the two numberings are independent.

#include <optional>

#include "oirequestresponse.h"
#include "openindy/network.pb.h"
#include "types.h"

namespace oi::codec {

[[nodiscard]] openindy::v1::ErrorCode toProto(OiRequestResponse::ErrorCode e);
/*! Empty for a number this version does not know. */
[[nodiscard]] std::optional<OiRequestResponse::ErrorCode> fromProto(
    openindy::v1::ErrorCode e);

/*! Notifications and the unknown request have no wire request kind: they map
 *  to unspecified. */
[[nodiscard]] openindy::v1::RequestKind toProto(
    OiRequestResponse::RequestType t);
/*! Unspecified (and unknown numbers) map to eUnknownRequest. */
[[nodiscard]] OiRequestResponse::RequestType fromProto(
    openindy::v1::RequestKind k);

[[nodiscard]] openindy::v1::ReadingKind toProto(ReadingTypes t);
/*! Empty for unspecified and for numbers this version does not know. */
[[nodiscard]] std::optional<ReadingTypes> fromProto(
    openindy::v1::ReadingKind k);

[[nodiscard]] openindy::v1::MessageKind toProto(MessageTypes t);
[[nodiscard]] std::optional<MessageTypes> fromProto(
    openindy::v1::MessageKind k);

}  // namespace oi::codec

#endif  // OI_CODEC_NETWORK_CONVERT_H_
