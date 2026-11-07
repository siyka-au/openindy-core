#ifndef OI_CODEC_WIRE_H_
#define OI_CODEC_WIRE_H_

// Internal helpers shared by the codec's translation units.

#include <google/protobuf/repeated_field.h>
#include <google/protobuf/timestamp.pb.h>

#include <chrono>

#include "convert.h"
#include "feature.h"
#include "featurewrapper.h"
#include "measurementconfig.h"
#include "openindy/project.pb.h"
#include "pluginmetadata.h"
#include "sensortype.h"

namespace oi::codec {

namespace oipb = openindy::v1;

inline oipb::Vec3 vec3(const OiVec& v) {
  oipb::Vec3 out;
  if (v.getSize() >= 3) {
    out.set_x(v.getAt(0));
    out.set_y(v.getAt(1));
    out.set_z(v.getAt(2));
  }
  return out;
}

inline oipb::Vec3 vec3(double x, double y, double z) {
  oipb::Vec3 out;
  out.set_x(x);
  out.set_y(y);
  out.set_z(z);
  return out;
}

inline OiVec toVec(const oipb::Vec3& v) {
  OiVec out(3);
  out.setAt(0, v.x());
  out.setAt(1, v.y());
  out.setAt(2, v.z());
  return out;
}

inline int idOf(const Feature* f) { return f != nullptr ? f->getId() : 0; }

inline void appendIds(const QList<oi::Guarded<FeatureWrapper>>& wrappers,
                      google::protobuf::RepeatedField<int32_t>* out) {
  for (const auto& wrapper : wrappers) {
    if (!wrapper.isNull() && !wrapper->getFeature().isNull()) {
      out->Add(wrapper->getFeature()->getId());
    }
  }
}

inline oipb::PluginObjectRef pluginRef(const PluginMetaData& meta) {
  oipb::PluginObjectRef ref;
  ref.set_name(meta.name.toStdString());
  ref.set_plugin(meta.pluginName.toStdString());
  return ref;
}

inline oipb::FeatureCommon commonOf(const Feature& f) {
  oipb::FeatureCommon c;
  c.set_id(f.getId());
  c.set_name(f.getFeatureName().toStdString());
  c.set_group(f.getGroupName().toStdString());
  c.set_comment(f.getComment().toStdString());
  c.set_solved(f.getIsSolved());
  c.set_active(f.getIsActiveFeature());
  appendIds(f.getUsedFor(), c.mutable_used_for());
  appendIds(f.getPreviouslyNeeded(), c.mutable_previously_needed());
  for (const auto& function : f.getFunctions()) {
    if (!function.isNull()) {
      *c.add_functions() = toProto(*function);
    }
  }
  return c;
}

inline google::protobuf::Timestamp toTimestamp(
    std::chrono::system_clock::time_point tp) {
  using namespace std::chrono;
  const auto whole = floor<seconds>(tp);
  google::protobuf::Timestamp out;
  out.set_seconds(whole.time_since_epoch().count());
  out.set_nanos(
      static_cast<int32_t>(duration_cast<nanoseconds>(tp - whole).count()));
  return out;
}

inline std::chrono::system_clock::time_point fromTimestamp(
    const google::protobuf::Timestamp& ts) {
  using namespace std::chrono;
  return system_clock::time_point{} + seconds{ts.seconds()} +
         duration_cast<system_clock::duration>(nanoseconds{ts.nanos()});
}

/*! How a geometry or reading names its measurement config. The config type
 *  (project, user, standard) is not stored: the linking pass finds the config
 *  by name and sensor type. */
inline oipb::MeasurementConfigRef configRefOf(const MeasurementConfigKey& key) {
  oipb::MeasurementConfigRef ref;
  ref.set_name(key.getName().toStdString());
  if (key.getSensorType().isValid()) {
    ref.set_sensor_type(key.getSensorType().id().toStdString());
  }
  return ref;
}

inline MeasurementConfigKey configKeyOf(const oipb::MeasurementConfigRef& ref) {
  SensorType sensorType;
  if (ref.has_sensor_type()) {
    sensorType =
        SensorTypeRegistry::byId(QString::fromStdString(ref.sensor_type()))
            .value_or(SensorType());
  }
  return MeasurementConfigKey(QString::fromStdString(ref.name()),
                              ConfigTypes::eUndefinded, sensorType);
}

}  // namespace oi::codec

#endif  // OI_CODEC_WIRE_H_
