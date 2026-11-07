#ifndef OI_CODEC_CONVERT_H_
#define OI_CODEC_CONVERT_H_

// Internal: conversions between the core model and the protobuf messages.
// The public face of the codec is bytes in, bytes out (see project_codec.h
// once it exists). This header is internal to the OpenIndyCodec library and
// its tests.

#include <QPointer>
#include <optional>

#include "coordinatesystem.h"
#include "geometry.h"
#include "group.h"
#include "measurementconfig.h"
#include "observation.h"
#include "openindy/project.pb.h"
#include "plugin/function/function.h"
#include "plugin/networkAdjustment/bundleadjustment.h"
#include "reading.h"
#include "sensorconfiguration.h"
#include "station.h"
#include "trafoparam.h"

namespace oi::codec {

[[nodiscard]] openindy::v1::MeasurementConfig toProto(
    const MeasurementConfig& config);

/*! Empty if the message has no name, which no valid config can have. The
 *  caller decides the config type (user, project, standard). */
[[nodiscard]] std::optional<MeasurementConfig> fromProto(
    const openindy::v1::MeasurementConfig& message);

[[nodiscard]] openindy::v1::SensorConfiguration toProto(
    const SensorConfiguration& config);

/*! Empty if the message has no name, which no valid config can have. Values
 *  the model cannot represent (an unspecified enum, a baud rate that is not
 *  a standard one) come back as Qt's "unknown" values. */
[[nodiscard]] std::optional<SensorConfiguration> fromProto(
    const openindy::v1::SensorConfiguration& message);

// ---------------------------------------------------------------------------
// Entities
//
// toProto() writes an object's own state plus the ids of the objects it refers
// to. createX() builds a fresh object (the caller owns it) with that state
// restored but with NO references resolved: the linking pass reads the ids
// from the same message once every object exists. Ids stay as they were saved.
// ---------------------------------------------------------------------------

[[nodiscard]] openindy::v1::Reading toProto(const Reading& reading);
[[nodiscard]] QPointer<Reading> createReading(
    const openindy::v1::Reading& message);

[[nodiscard]] openindy::v1::Observation toProto(const Observation& observation);
/*! Creates the observation and the reading it owns, and points the reading
 *  back at the observation. */
[[nodiscard]] QPointer<Observation> createObservation(
    const openindy::v1::Observation& message);

[[nodiscard]] openindy::v1::TrafoParam toProto(const TrafoParam& trafoParam);
[[nodiscard]] QPointer<TrafoParam> createTrafoParam(
    const openindy::v1::TrafoParam& message);

[[nodiscard]] openindy::v1::CoordinateSystem toProto(
    const CoordinateSystem& system);
[[nodiscard]] QPointer<CoordinateSystem> createCoordinateSystem(
    const openindy::v1::CoordinateSystem& message);

[[nodiscard]] openindy::v1::Station toProto(const Station& station);
/*! The new station comes with its own default position and station system;
 *  the linking pass replaces them with the saved ones. */
[[nodiscard]] QPointer<Station> createStation(
    const openindy::v1::Station& message);

/*! Writes any of the 19 concrete geometry types (not an Observation, which
 *  has its own message). The shape is chosen from the object's real type. */
[[nodiscard]] openindy::v1::Geometry toProto(const Geometry& geometry);
/*! Null if the message has no shape. The geometry's references (observations,
 *  nominal system, realised nominal, owner) are left for the linking pass. */
[[nodiscard]] QPointer<Geometry> createGeometry(
    const openindy::v1::Geometry& message);

[[nodiscard]] openindy::v1::Group toProto(const Group& group);
/*! A plain Group or a PointGroup, as the message says. Members are left for
 *  the linking pass. */
[[nodiscard]] QPointer<Group> createGroup(const openindy::v1::Group& message);

/*! A function's identity and the user-facing state that makes a project
 *  reproducible: inputs, pipeline stages, parameters. Results are not stored;
 *  they are recomputed. */
[[nodiscard]] openindy::v1::FunctionRecord toProto(const Function& function);

/*! Restores a record onto a function the host created from the record's
 *  plugin reference: parameters, declared pipelines and input elements. The
 *  inputs come back with ids and flags but no object pointers; the linking
 *  pass resolves those. */
void applyRecord(const openindy::v1::FunctionRecord& record,
                 Function& function);

/*! The state a bundle adjustment plugin holds: participating stations and
 *  geometries, what to solve for, and what it solved. */
[[nodiscard]] openindy::v1::BundleAdjustmentRecord toProto(
    const BundleAdjustment& bundle);

/*! Restores a record onto a bundle adjustment the host created from the
 *  record's plugin reference. The bundle system is not part of the record: it
 *  is the coordinate system the bundle is attached to. */
void applyRecord(const openindy::v1::BundleAdjustmentRecord& record,
                 BundleAdjustment& bundle);

}  // namespace oi::codec

#endif  // OI_CODEC_CONVERT_H_
