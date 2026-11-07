// Conversions for the model's entities: station, coordinate system,
// transformation parameters, observation and reading.
//
// Access is the friend declared in the model classes (Element, Feature,
// Station, ...). It exists so the codec can restore private state directly,
// the way the classes' own fromOpenIndyXML used to.

#include <QJsonArray>
#include <QJsonObject>
#include <chrono>
#include <cmath>

#include "access.h"
#include "convert.h"
#include "featurewrapper.h"
#include "plugin/sensor/sensor.h"
#include "wire.h"

namespace oi::codec {

namespace oipb = openindy::v1;

namespace {

oipb::SensorFace toProto(SensorFaces face) {
  switch (face) {
    case SensorFaces::eFrontSide:
      return oipb::SENSOR_FACE_FRONT;
    case SensorFaces::eBackSide:
      return oipb::SENSOR_FACE_BACK;
    case SensorFaces::eUndefinedSide:
      break;
  }
  return oipb::SENSOR_FACE_UNDEFINED;
}

SensorFaces fromProto(oipb::SensorFace face) {
  switch (face) {
    case oipb::SENSOR_FACE_FRONT:
      return SensorFaces::eFrontSide;
    case oipb::SENSOR_FACE_BACK:
      return SensorFaces::eBackSide;
    default:
      return SensorFaces::eUndefinedSide;
  }
}

// The bundle template is user-supplied JSON; its values are kept as text.
std::string jsonText(const QJsonValue& value) {
  if (value.isString()) {
    return value.toString().toStdString();
  }
  if (value.isDouble()) {
    return QString::number(value.toDouble(), 'g', 17).toStdString();
  }
  return {};
}

oipb::BundleTemplate bundleTemplateOf(const QJsonObject& json) {
  oipb::BundleTemplate out;
  out.set_name(jsonText(json.value("name")));
  out.set_level_station(jsonText(json.value("levelStation")));
  out.set_max_iterations(jsonText(json.value("maxIterations")));
  for (const QJsonValue& parameter : json.value("parameters").toArray()) {
    out.add_parameters(jsonText(parameter));
  }
  const QJsonObject plugin = json.value("plugin").toObject();
  out.mutable_plugin()->set_name(jsonText(plugin.value("name")));
  out.mutable_plugin()->set_plugin(jsonText(plugin.value("pluginName")));
  return out;
}

QJsonObject bundleTemplateToJson(const oipb::BundleTemplate& m) {
  QJsonObject json;
  json["name"] = QString::fromStdString(m.name());
  json["levelStation"] = QString::fromStdString(m.level_station());
  json["maxIterations"] = QString::fromStdString(m.max_iterations());
  QJsonArray parameters;
  for (const std::string& p : m.parameters()) {
    parameters.append(QString::fromStdString(p));
  }
  json["parameters"] = parameters;
  QJsonObject plugin;
  plugin["name"] = QString::fromStdString(m.plugin().name());
  plugin["pluginName"] = QString::fromStdString(m.plugin().plugin());
  json["plugin"] = plugin;
  return json;
}

}  // namespace

// ---------------------------------------------------------------------------
// Reading
// ---------------------------------------------------------------------------

void Access::encodeReading(const Reading& r, oipb::Reading* m) {
  m->set_id(r.id);
  *m->mutable_measured_at() = toTimestamp(r.measuredAt);
  m->set_imported(r.isImported());
  m->set_face(toProto(r.face));
  m->set_sensor_config_name(r.sensorConfigName.toStdString());
  if (!r.mConfig.getName().isEmpty()) {
    *m->mutable_measurement_config() = configRefOf(r.mConfig);
  }

  switch (r.typeOfReading) {
    case ReadingTypes::ePolarReading: {
      auto* p = m->mutable_polar();
      p->set_azimuth(r.rPolar.azimuth);
      p->set_zenith(r.rPolar.zenith);
      p->set_distance(r.rPolar.distance);
      p->set_sigma_azimuth(r.rPolar.sigmaAzimuth);
      p->set_sigma_zenith(r.rPolar.sigmaZenith);
      p->set_sigma_distance(r.rPolar.sigmaDistance);
      break;
    }
    case ReadingTypes::eCartesianReading:
      *m->mutable_cartesian()->mutable_xyz() = vec3(r.rCartesian.xyz);
      *m->mutable_cartesian()->mutable_sigma_xyz() =
          vec3(r.rCartesian.sigmaXyz);
      break;
    case ReadingTypes::eCartesianReading6D:
      *m->mutable_cartesian_6d()->mutable_xyz() = vec3(r.rCartesian6D.xyz);
      *m->mutable_cartesian_6d()->mutable_ijk() = vec3(r.rCartesian6D.ijk);
      *m->mutable_cartesian_6d()->mutable_sigma_xyz() =
          vec3(r.rCartesian6D.sigmaXyz);
      break;
    case ReadingTypes::eDirectionReading: {
      auto* d = m->mutable_direction();
      d->set_azimuth(r.rDirection.azimuth);
      d->set_zenith(r.rDirection.zenith);
      d->set_sigma_azimuth(r.rDirection.sigmaAzimuth);
      d->set_sigma_zenith(r.rDirection.sigmaZenith);
      break;
    }
    case ReadingTypes::eDistanceReading:
      m->mutable_distance()->set_distance(r.rDistance.distance);
      m->mutable_distance()->set_sigma_distance(r.rDistance.sigmaDistance);
      break;
    case ReadingTypes::eTemperatureReading:
      m->mutable_temperature()->set_temperature(r.rTemperature.temperature);
      m->mutable_temperature()->set_sigma_temperature(
          r.rTemperature.sigmaTemperature);
      break;
    case ReadingTypes::eLevelReading: {
      auto* l = m->mutable_level();
      l->set_i(r.rLevel.i);
      l->set_j(r.rLevel.j);
      l->set_k(r.rLevel.k);
      l->set_sigma_i(r.rLevel.sigmaI);
      l->set_sigma_j(r.rLevel.sigmaJ);
      l->set_sigma_k(r.rLevel.sigmaK);
      break;
    }
    case ReadingTypes::eUndefinedReading: {
      auto* u = m->mutable_undefined();
      for (auto it = r.rUndefined.values.cbegin();
           it != r.rUndefined.values.cend(); ++it) {
        (*u->mutable_values())[it.key().toStdString()] = it.value();
      }
      for (auto it = r.rUndefined.sigmaValues.cbegin();
           it != r.rUndefined.sigmaValues.cend(); ++it) {
        (*u->mutable_sigma_values())[it.key().toStdString()] = it.value();
      }
      break;
    }
  }
}

oi::Guarded<Reading> Access::decodeReading(const oipb::Reading& m) {
  oi::Guarded<Reading> reading;
  switch (m.data_case()) {
    case oipb::Reading::kPolar: {
      ReadingPolar v;
      v.azimuth = m.polar().azimuth();
      v.zenith = m.polar().zenith();
      v.distance = m.polar().distance();
      v.sigmaAzimuth = m.polar().sigma_azimuth();
      v.sigmaZenith = m.polar().sigma_zenith();
      v.sigmaDistance = m.polar().sigma_distance();
      v.isValid = true;
      reading = new Reading(v);
      break;
    }
    case oipb::Reading::kCartesian: {
      ReadingCartesian v;
      v.xyz = toVec(m.cartesian().xyz());
      v.sigmaXyz = toVec(m.cartesian().sigma_xyz());
      v.isValid = true;
      reading = new Reading(v);
      break;
    }
    case oipb::Reading::kCartesian6D: {
      ReadingCartesian6D v;
      v.xyz = toVec(m.cartesian_6d().xyz());
      v.ijk = toVec(m.cartesian_6d().ijk());
      v.sigmaXyz = toVec(m.cartesian_6d().sigma_xyz());
      v.isValid = true;
      reading = new Reading(v);
      break;
    }
    case oipb::Reading::kDirection: {
      ReadingDirection v;
      v.azimuth = m.direction().azimuth();
      v.zenith = m.direction().zenith();
      v.sigmaAzimuth = m.direction().sigma_azimuth();
      v.sigmaZenith = m.direction().sigma_zenith();
      v.isValid = true;
      reading = new Reading(v);
      break;
    }
    case oipb::Reading::kDistance: {
      ReadingDistance v;
      v.distance = m.distance().distance();
      v.sigmaDistance = m.distance().sigma_distance();
      v.isValid = true;
      reading = new Reading(v);
      break;
    }
    case oipb::Reading::kTemperature: {
      ReadingTemperature v;
      v.temperature = m.temperature().temperature();
      v.sigmaTemperature = m.temperature().sigma_temperature();
      v.isValid = true;
      reading = new Reading(v);
      break;
    }
    case oipb::Reading::kLevel: {
      ReadingLevel v;
      v.i = m.level().i();
      v.j = m.level().j();
      v.k = m.level().k();
      v.sigmaI = m.level().sigma_i();
      v.sigmaJ = m.level().sigma_j();
      v.sigmaK = m.level().sigma_k();
      v.isValid = true;
      reading = new Reading(v);
      break;
    }
    case oipb::Reading::kUndefined: {
      ReadingUndefined v;
      for (const auto& [key, value] : m.undefined().values()) {
        v.values.insert(QString::fromStdString(key), value);
      }
      for (const auto& [key, value] : m.undefined().sigma_values()) {
        v.sigmaValues.insert(QString::fromStdString(key), value);
      }
      v.isValid = true;
      reading = new Reading(v);
      break;
    }
    case oipb::Reading::DATA_NOT_SET:
      // a reading with no measurement is not meaningful
      return {};
  }

  reading->id = m.id();
  reading->measuredAt = fromTimestamp(m.measured_at());
  reading->setImported(m.imported());
  reading->face = fromProto(m.face());
  reading->sensorConfigName = QString::fromStdString(m.sensor_config_name());
  if (m.has_measurement_config()) {
    reading->mConfig = configKeyOf(m.measurement_config());
  }
  return reading;
}

oipb::Reading toProto(const Reading& reading) {
  oipb::Reading m;
  Access::encodeReading(reading, &m);
  return m;
}

oi::Guarded<Reading> createReading(const oipb::Reading& message) {
  return Access::decodeReading(message);
}

// ---------------------------------------------------------------------------
// Observation
// ---------------------------------------------------------------------------

void Access::encodeObservation(const Observation& o, oipb::Observation* m) {
  m->set_id(o.id);
  m->set_valid(o.isValid);
  m->set_solved(o.isSolved);
  m->set_dummy_point(o.isDummyPoint);
  m->set_has_direction(o.hasDirection);
  *m->mutable_xyz() = vec3(o.xyz);
  *m->mutable_original_xyz() = vec3(o.originalXyz);
  *m->mutable_sigma_xyz() = vec3(o.sigmaXyz);
  *m->mutable_original_sigma_xyz() = vec3(o.originalSigmaXyz);
  *m->mutable_ijk() = vec3(o.ijk);
  *m->mutable_original_ijk() = vec3(o.originalIjk);
  *m->mutable_sigma_ijk() = vec3(o.sigmaIjk);
  *m->mutable_original_sigma_ijk() = vec3(o.originalSigmaIjk);
  m->set_station(idOf(o.station.data()));
  m->set_measured_target_geometry(idOf(o.measuredTargetGeometry.data()));
  if (!o.reading.isNull()) {
    encodeReading(*o.reading, m->mutable_reading());
  }
}

oipb::Observation toProto(const Observation& observation) {
  oipb::Observation m;
  Access::encodeObservation(observation, &m);
  return m;
}

oi::Guarded<Observation> createObservation(const oipb::Observation& message) {
  if (!message.has_reading()) {
    return {};  // an observation without its reading is not usable
  }
  oi::Guarded<Reading> reading = createReading(message.reading());
  if (reading.isNull()) {
    return {};
  }
  oi::Guarded<Observation> observation = new Observation();
  Access::restoreObservation(*observation, message);
  Access::setReading(*observation, reading);
  reading->setObservation(observation);
  return observation;
}

// ---------------------------------------------------------------------------
// TrafoParam
// ---------------------------------------------------------------------------

oipb::TrafoParam toProto(const TrafoParam& trafo) {
  oipb::TrafoParam m;
  *m.mutable_feature() = commonOf(trafo);
  *m.mutable_translation() = vec3(trafo.getTranslation());
  *m.mutable_rotation() = vec3(trafo.getRotation());
  *m.mutable_scale() = vec3(trafo.getScale());
  const OiMat& matrix = trafo.getHomogenMatrix();
  if (matrix.getRowCount() == 4 && matrix.getColCount() == 4) {
    for (int row = 0; row < 4; ++row) {
      for (int col = 0; col < 4; ++col) {
        m.add_homogen_matrix(matrix.getAt(row, col));
      }
    }
  }
  m.set_datum_trafo(trafo.getIsDatumTrafo());
  m.set_used(trafo.getIsUsed());
  m.set_from(Access::fromId(trafo));
  m.set_to(Access::toId(trafo));
  return m;
}

oi::Guarded<TrafoParam> createTrafoParam(const oipb::TrafoParam& message) {
  oi::Guarded<TrafoParam> trafo = new TrafoParam();
  Access::restoreCommon(*trafo, message.feature());
  Access::restoreTrafo(*trafo, message);
  return trafo;
}

// ---------------------------------------------------------------------------
// CoordinateSystem
// ---------------------------------------------------------------------------

oipb::CoordinateSystem toProto(const CoordinateSystem& system) {
  oipb::CoordinateSystem m;
  *m.mutable_feature() = commonOf(system);
  m.set_active_system(system.getIsActiveCoordinateSystem());
  m.set_bundle_system(system.getIsBundleSystem());
  *m.mutable_origin() = vec3(system.getOrigin().getVector());
  *m.mutable_x_axis() = vec3(system.getXAxis().getVector());
  *m.mutable_y_axis() = vec3(system.getYAxis().getVector());
  *m.mutable_z_axis() = vec3(system.getZAxis().getVector());
  *m.mutable_expansion_origin() = vec3(system.getExpansionOrigin().getVector());
  if (!system.getBundlePlugin().isNull()) {
    *m.mutable_bundle_adjustment() = toProto(*system.getBundlePlugin());
  }
  const QJsonObject bundleTemplate = system.getBundleTemplate();
  if (!bundleTemplate.isEmpty()) {
    *m.mutable_bundle_template() = bundleTemplateOf(bundleTemplate);
  }
  for (const auto& trafo : system.getTransformationParameters()) {
    if (!trafo.isNull()) {
      m.add_trafo_params(trafo->getId());
    }
  }
  for (const auto& observation : system.getObservations()) {
    if (!observation.isNull()) {
      m.add_observations(observation->getId());
    }
  }
  for (const auto& nominal : system.getNominals()) {
    if (!nominal.isNull() && !nominal->getFeature().isNull()) {
      m.add_nominal_geometries(nominal->getFeature()->getId());
    }
  }
  return m;
}

oi::Guarded<CoordinateSystem> createCoordinateSystem(
    const oipb::CoordinateSystem& message) {
  oi::Guarded<CoordinateSystem> system = new CoordinateSystem();
  Access::restoreCommon(*system, message.feature());
  Access::restoreSystem(*system, message);
  if (message.has_bundle_template()) {
    Access::setBundleTemplate(*system,
                              bundleTemplateToJson(message.bundle_template()));
  }
  return system;
}

// ---------------------------------------------------------------------------
// Station
// ---------------------------------------------------------------------------

oipb::Station toProto(const Station& station) {
  oipb::Station m;
  *m.mutable_feature() = commonOf(station);
  m.set_active_station(station.getIsActiveStation());
  for (const Sensor& sensor : station.getUsedSensors()) {
    *m.add_used_sensors() = pluginRef(sensor.getMetaData());
  }
  const Sensor active = station.getSensor();
  if (!active.getMetaData().name.isEmpty()) {
    *m.mutable_active_sensor() = pluginRef(active.getMetaData());
  }
  m.set_position(idOf(station.getPosition().data()));
  m.set_coordinate_system(idOf(station.getCoordinateSystem().data()));
  return m;
}

oi::Guarded<Station> createStation(const oipb::Station& message) {
  oi::Guarded<Station> station = new Station();
  Access::restoreCommon(*station, message.feature());
  Access::restoreStation(*station, message);
  return station;
}

}  // namespace oi::codec
