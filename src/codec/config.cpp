// Config files: SensorConfiguration <-> protobuf, and the JSON text encoding of
// both config kinds (see config_codec.h).
#include <google/protobuf/json/json.h>

#include <map>
#include <string>

#include "config_codec.h"
#include "containers.h"
#include "wire.h"

namespace oi::codec {

namespace {

// The core's serial enums have their own values, the wire enums their own
// numbering (0 = unspecified). Every value is mapped explicitly. A value
// outside the known set (including whatever an uninitialised ConnectionConfig
// holds) goes to the wire as unspecified. The core enums have no "unknown"
// value, so unspecified comes back as the usual 9600 8N1 without flow control.

oipb::SerialParity toProto(oi::Parity v) {
  switch (v) {
    case oi::Parity::None:
      return oipb::SERIAL_PARITY_NONE;
    case oi::Parity::Even:
      return oipb::SERIAL_PARITY_EVEN;
    case oi::Parity::Odd:
      return oipb::SERIAL_PARITY_ODD;
    case oi::Parity::Space:
      return oipb::SERIAL_PARITY_SPACE;
    case oi::Parity::Mark:
      return oipb::SERIAL_PARITY_MARK;
    default:
      return oipb::SERIAL_PARITY_UNSPECIFIED;
  }
}

oi::Parity fromProto(oipb::SerialParity v) {
  switch (v) {
    case oipb::SERIAL_PARITY_NONE:
      return oi::Parity::None;
    case oipb::SERIAL_PARITY_EVEN:
      return oi::Parity::Even;
    case oipb::SERIAL_PARITY_ODD:
      return oi::Parity::Odd;
    case oipb::SERIAL_PARITY_SPACE:
      return oi::Parity::Space;
    case oipb::SERIAL_PARITY_MARK:
      return oi::Parity::Mark;
    default:
      return oi::Parity::None;
  }
}

oipb::SerialDataBits toProto(oi::DataBits v) {
  switch (v) {
    case oi::DataBits::Data5:
      return oipb::SERIAL_DATA_BITS_5;
    case oi::DataBits::Data6:
      return oipb::SERIAL_DATA_BITS_6;
    case oi::DataBits::Data7:
      return oipb::SERIAL_DATA_BITS_7;
    case oi::DataBits::Data8:
      return oipb::SERIAL_DATA_BITS_8;
    default:
      return oipb::SERIAL_DATA_BITS_UNSPECIFIED;
  }
}

oi::DataBits fromProto(oipb::SerialDataBits v) {
  switch (v) {
    case oipb::SERIAL_DATA_BITS_5:
      return oi::DataBits::Data5;
    case oipb::SERIAL_DATA_BITS_6:
      return oi::DataBits::Data6;
    case oipb::SERIAL_DATA_BITS_7:
      return oi::DataBits::Data7;
    case oipb::SERIAL_DATA_BITS_8:
      return oi::DataBits::Data8;
    default:
      return oi::DataBits::Data8;
  }
}

oipb::SerialStopBits toProto(oi::StopBits v) {
  switch (v) {
    case oi::StopBits::One:
      return oipb::SERIAL_STOP_BITS_ONE;
    case oi::StopBits::OneAndHalf:
      return oipb::SERIAL_STOP_BITS_ONE_AND_HALF;
    case oi::StopBits::Two:
      return oipb::SERIAL_STOP_BITS_TWO;
    default:
      return oipb::SERIAL_STOP_BITS_UNSPECIFIED;
  }
}

oi::StopBits fromProto(oipb::SerialStopBits v) {
  switch (v) {
    case oipb::SERIAL_STOP_BITS_ONE:
      return oi::StopBits::One;
    case oipb::SERIAL_STOP_BITS_ONE_AND_HALF:
      return oi::StopBits::OneAndHalf;
    case oipb::SERIAL_STOP_BITS_TWO:
      return oi::StopBits::Two;
    default:
      return oi::StopBits::One;
  }
}

oipb::SerialFlowControl toProto(oi::FlowControl v) {
  switch (v) {
    case oi::FlowControl::None:
      return oipb::SERIAL_FLOW_CONTROL_NONE;
    case oi::FlowControl::Hardware:
      return oipb::SERIAL_FLOW_CONTROL_HARDWARE;
    case oi::FlowControl::Software:
      return oipb::SERIAL_FLOW_CONTROL_SOFTWARE;
    default:
      return oipb::SERIAL_FLOW_CONTROL_UNSPECIFIED;
  }
}

oi::FlowControl fromProto(oipb::SerialFlowControl v) {
  switch (v) {
    case oipb::SERIAL_FLOW_CONTROL_NONE:
      return oi::FlowControl::None;
    case oipb::SERIAL_FLOW_CONTROL_HARDWARE:
      return oi::FlowControl::Hardware;
    case oipb::SERIAL_FLOW_CONTROL_SOFTWARE:
      return oi::FlowControl::Software;
    default:
      return oi::FlowControl::None;
  }
}

// A baud rate is a number, not a choice: the wire carries the rate itself,
// but only the standard rates are accepted.
int32_t toProto(oi::BaudRate v) {
  switch (v) {
    case oi::BaudRate::Baud1200:
    case oi::BaudRate::Baud2400:
    case oi::BaudRate::Baud4800:
    case oi::BaudRate::Baud9600:
    case oi::BaudRate::Baud19200:
    case oi::BaudRate::Baud38400:
    case oi::BaudRate::Baud57600:
    case oi::BaudRate::Baud115200:
      return static_cast<int32_t>(v);
    default:
      return 0;
  }
}

oi::BaudRate baudFromProto(int32_t v) {
  switch (v) {
    case 1200:
      return oi::BaudRate::Baud1200;
    case 2400:
      return oi::BaudRate::Baud2400;
    case 4800:
      return oi::BaudRate::Baud4800;
    case 9600:
      return oi::BaudRate::Baud9600;
    case 19200:
      return oi::BaudRate::Baud19200;
    case 38400:
      return oi::BaudRate::Baud38400;
    case 57600:
      return oi::BaudRate::Baud57600;
    case 115200:
      return oi::BaudRate::Baud115200;
    default:
      return oi::BaudRate::Baud9600;
  }
}

std::string str(const std::string& s) { return s; }
std::string qstr(const std::string& s) { return (s); }

template <typename Message>
std::string toJson(const Message& message) {
  google::protobuf::json::PrintOptions options;
  options.add_whitespace = true;
  options.always_print_fields_with_no_presence = true;
  options.preserve_proto_field_names = true;
  std::string out;
  if (!google::protobuf::json::MessageToJsonString(message, &out, options)
           .ok()) {
    return {};
  }
  out += '\n';
  return out;
}

template <typename Message>
std::optional<Message> parseJson(std::string_view json) {
  google::protobuf::json::ParseOptions options;
  options.ignore_unknown_fields = true;
  Message message;
  if (!google::protobuf::json::JsonStringToMessage(json, &message, options)
           .ok()) {
    return std::nullopt;
  }
  return message;
}

}  // namespace

oipb::SensorConfiguration toProto(const SensorConfiguration& config) {
  oipb::SensorConfiguration m;
  m.set_name(str(config.getName()));
  m.set_is_saved(config.getIsSaved());
  m.set_plugin_name(str(config.getPluginName()));
  m.set_sensor_name(str(config.getSensorName()));
  if (config.getTypeOfSensor().isValid()) {
    m.set_sensor_type(str(config.getTypeOfSensor().id()));
  }

  const ConnectionConfig& c = config.getConnectionConfig();
  oipb::SensorConnection* conn = m.mutable_connection();
  conn->set_kind(str(c.typeOfConnection.id()));
  conn->set_ip(str(c.ip));
  conn->set_port(str(c.port));
  conn->set_com_port(str(c.comPort));
  conn->set_baud_rate(toProto(c.baudRate));
  conn->set_data_bits(toProto(c.dataBits));
  conn->set_parity(toProto(c.parity));
  conn->set_stop_bits(toProto(c.stopBits));
  conn->set_flow_control(toProto(c.flowControl));

  const Accuracy& a = config.getAccuracy();
  oipb::SensorAccuracy* acc = m.mutable_accuracy();
  *acc->mutable_sigma_xyz() = vec3(a.sigmaXyz);
  acc->set_sigma_azimuth(a.sigmaAzimuth);
  acc->set_sigma_zenith(a.sigmaZenith);
  acc->set_sigma_distance(a.sigmaDistance);
  acc->set_sigma_temp(a.sigmaTemp);
  acc->set_sigma_i(a.sigmaI);
  acc->set_sigma_j(a.sigmaJ);
  acc->set_sigma_k(a.sigmaK);
  for (auto it = a.sigmaUndefined.cbegin(); it != a.sigmaUndefined.cend();
       ++it) {
    oipb::NamedDouble* entry = acc->add_sigma_undefined();
    entry->set_name(str(it->first));
    entry->set_value(it->second);
  }

  for (auto it = config.getIntegerParameter().cbegin();
       it != config.getIntegerParameter().cend(); ++it) {
    oipb::NamedInt* entry = m.add_integer_parameters();
    entry->set_name(str(it->first));
    entry->set_value(it->second);
  }
  for (auto it = config.getDoubleParameter().cbegin();
       it != config.getDoubleParameter().cend(); ++it) {
    oipb::NamedDouble* entry = m.add_double_parameters();
    entry->set_name(str(it->first));
    entry->set_value(it->second);
  }
  for (auto it = config.getStringParameter().cbegin();
       it != config.getStringParameter().cend(); ++it) {
    oipb::NamedString* entry = m.add_string_parameters();
    entry->set_name(str(it->first));
    entry->set_value(str(it->second));
  }
  const auto& available = config.getAvailableStringParameter();
  for (auto group = available.cbegin(); group != available.cend();
       group = available.upper_bound(group->first)) {
    const std::string& key = group->first;
    oipb::StringParameterOptions* entry = m.add_available_string_parameters();
    entry->set_name(str(key));
    for (const std::string& value : oi::values(available, key)) {
      entry->add_options(str(value));
    }
  }
  return m;
}

std::optional<SensorConfiguration> fromProto(
    const oipb::SensorConfiguration& m) {
  if (m.name().empty()) {
    return std::nullopt;
  }
  SensorConfiguration config;
  config.setName(qstr(m.name()));
  config.setIsSaved(m.is_saved());
  config.setPluginName(qstr(m.plugin_name()));
  config.setSensorName(qstr(m.sensor_name()));
  config.setTypeOfSensor(
      SensorTypeRegistry::byId(qstr(m.sensor_type())).value_or(SensorType()));

  ConnectionConfig c;
  const oipb::SensorConnection& conn = m.connection();
  c.typeOfConnection = ConnectionKind::byId(qstr(conn.kind()));
  c.ip = qstr(conn.ip());
  c.port = qstr(conn.port());
  c.comPort = qstr(conn.com_port());
  c.baudRate = baudFromProto(conn.baud_rate());
  c.dataBits = fromProto(conn.data_bits());
  c.parity = fromProto(conn.parity());
  c.stopBits = fromProto(conn.stop_bits());
  c.flowControl = fromProto(conn.flow_control());
  config.setConnectionConfig(c);

  Accuracy a;
  const oipb::SensorAccuracy& acc = m.accuracy();
  a.sigmaXyz = toVec(acc.sigma_xyz());
  a.sigmaAzimuth = acc.sigma_azimuth();
  a.sigmaZenith = acc.sigma_zenith();
  a.sigmaDistance = acc.sigma_distance();
  a.sigmaTemp = acc.sigma_temp();
  a.sigmaI = acc.sigma_i();
  a.sigmaJ = acc.sigma_j();
  a.sigmaK = acc.sigma_k();
  for (const auto& entry : acc.sigma_undefined()) {
    a.sigmaUndefined.insert_or_assign(qstr(entry.name()), entry.value());
  }
  config.setAccuracy(a);

  std::map<std::string, int> ints;
  for (const auto& entry : m.integer_parameters()) {
    ints.insert_or_assign(qstr(entry.name()), entry.value());
  }
  config.setIntegerParameter(ints);
  std::map<std::string, double> doubles;
  for (const auto& entry : m.double_parameters()) {
    doubles.insert_or_assign(qstr(entry.name()), entry.value());
  }
  config.setDoubleParameter(doubles);
  std::map<std::string, std::string> strings;
  for (const auto& entry : m.string_parameters()) {
    strings.insert_or_assign(qstr(entry.name()), qstr(entry.value()));
  }
  config.setStringParameter(strings);

  // oi::values(multimap, key) lists the most recently inserted value first,
  // so insert in reverse to get the options back in the order they were
  // written.
  std::multimap<std::string, std::string> available;
  for (const auto& entry : m.available_string_parameters()) {
    for (int i = entry.options_size() - 1; i >= 0; --i) {
      available.emplace(qstr(entry.name()), qstr(entry.options(i)));
    }
  }
  config.setAvailableStringParameter(available);
  return config;
}

std::string encodeMeasurementConfig(const MeasurementConfig& config) {
  return toJson(toProto(config));
}

std::optional<MeasurementConfig> decodeMeasurementConfig(
    std::string_view json) {
  const auto message = parseJson<oipb::MeasurementConfig>(json);
  if (!message) {
    return std::nullopt;
  }
  return fromProto(*message);
}

std::string encodeSensorConfiguration(const SensorConfiguration& config) {
  return toJson(toProto(config));
}

std::optional<SensorConfiguration> decodeSensorConfiguration(
    std::string_view json) {
  const auto message = parseJson<oipb::SensorConfiguration>(json);
  if (!message) {
    return std::nullopt;
  }
  return fromProto(*message);
}

}  // namespace oi::codec
