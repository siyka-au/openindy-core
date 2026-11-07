// Saving and loading a whole project.
//
// Saving collects every entity of the job and converts it (the converters
// live in entities.cpp, geometry.cpp and functions.cpp).
//
// Loading is two passes, the way the model needs it. First every entity is
// created from its message with its own state but no references. Then the
// linking pass resolves the ids each message carries into pointers, creates
// the plugin objects (functions, bundle adjustments, sensors) through the
// host, and hands the finished features to the job.
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl_lite.h>

#include <algorithm>
#include <chrono>
#include <map>
#include <set>
#include <vector>

#include "access.h"
#include "containers.h"
#include "convert.h"
#include "featurewrapper.h"
#include "guarded.h"
#include "oijob.h"
#include "plugin/sensor/sensor.h"
#include "pointgroup.h"
#include "project_codec.h"
#include "sha256.h"
#include "wire.h"

namespace oi::codec {

namespace {

constexpr std::uint32_t kFormatVersion = 1;

std::string serialize(const oipb::Project& message) {
  std::string bytes;
  {
    google::protobuf::io::StringOutputStream stream(&bytes);
    google::protobuf::io::CodedOutputStream coded(&stream);
    coded.SetSerializationDeterministic(true);
    message.SerializeToCodedStream(&coded);
  }
  return bytes;
}

std::string digestOf(const oipb::Project& body) {
  const std::string bytes = serialize(body);
  return sha256Hex(bytes);
}

bool isScalarEntity(const FeatureWrapper& wrapper) {
  switch (wrapper.getFeatureTypeEnum()) {
    case FeatureTypes::eScalarEntityAngleFeature:
    case FeatureTypes::eScalarEntityDistanceFeature:
    case FeatureTypes::eScalarEntityTemperatureFeature:
    case FeatureTypes::eScalarEntityMeasurementSeriesFeature:
      return true;
    default:
      return false;
  }
}

// ---------------------------------------------------------------------------
// Encoding
// ---------------------------------------------------------------------------

// The entities of a job keyed by id, so that the output is in id order (and
// therefore the same however the job's own lists happen to be ordered) and
// nothing is written twice.
template <class T>
using ById = std::map<int, const T*>;

struct Collected {
  ById<Station> stations;
  ById<CoordinateSystem> systems;
  ById<TrafoParam> trafos;
  ById<Geometry> geometries;
  ById<Group> groups;
  ById<Observation> observations;
};

Collected collect(OiJob& job) {
  Collected out;

  for (const oi::Guarded<Station>& station : job.getStationsList()) {
    if (station.isNull()) {
      continue;
    }
    out.stations.emplace(station->getId(), station.data());
    if (!station->getCoordinateSystem().isNull()) {
      const CoordinateSystem* system = station->getCoordinateSystem().data();
      out.systems.emplace(system->getId(), system);
      for (const oi::Guarded<Observation>& observation :
           system->getObservations()) {
        if (!observation.isNull()) {
          out.observations.emplace(observation->getId(), observation.data());
        }
      }
    }
    if (!station->getPosition().isNull()) {
      out.geometries.emplace(station->getPosition()->getId(),
                             station->getPosition().data());
    }
  }

  for (const oi::Guarded<CoordinateSystem>& system :
       job.getCoordinateSystemsList()) {
    if (!system.isNull()) {
      out.systems.emplace(system->getId(), system.data());
    }
  }
  for (const oi::Guarded<TrafoParam>& trafo :
       job.getTransformationParametersList()) {
    if (!trafo.isNull()) {
      out.trafos.emplace(trafo->getId(), trafo.data());
    }
  }
  for (const oi::Guarded<FeatureWrapper>& wrapper : job.getGeometriesList()) {
    if (!wrapper.isNull() && !wrapper->getGeometry().isNull()) {
      out.geometries.emplace(wrapper->getGeometry()->getId(),
                             wrapper->getGeometry().data());
    }
  }
  for (const FeatureTypes kind :
       {FeatureTypes::eGroupFeature, FeatureTypes::ePointGroupFeature}) {
    for (const oi::Guarded<FeatureWrapper>& wrapper :
         job.getFeaturesByType(kind)) {
      if (!wrapper.isNull() && !wrapper->getGroup().isNull()) {
        out.groups.emplace(wrapper->getGroup()->getId(),
                           wrapper->getGroup().data());
      }
    }
  }
  return out;
}

// The configs the project defines itself: every config its geometries use
// that the application does not ship as a standard config.
std::map<MeasurementConfigKey, MeasurementConfig> projectConfigsOf(
    const Collected& collected, const ProjectHost& host) {
  std::map<MeasurementConfigKey, MeasurementConfig> configs;
  for (const auto& [id, geometry] : collected.geometries) {
    const MeasurementConfigKey key = geometry->getMeasurementConfig();
    if (!key.isValid() || key.isStandardConfig() || configs.contains(key)) {
      continue;
    }
    const MeasurementConfig config = host.measurementConfig(key);
    if (config.isValid()) {
      configs.insert_or_assign(key, config);
    }
  }
  return configs;
}

// ---------------------------------------------------------------------------
// Decoding
// ---------------------------------------------------------------------------

// Everything created while loading. Until the job has taken it over, the
// staging area owns it, so a load that fails half way leaves nothing behind.
struct Staging {
  std::map<int, oi::Guarded<Observation>> observations;
  std::map<int, oi::Guarded<Reading>> readings;
  std::map<int, oi::Guarded<FeatureWrapper>> stations;
  std::map<int, oi::Guarded<FeatureWrapper>> systems;
  std::map<int, oi::Guarded<FeatureWrapper>> trafos;
  std::map<int, oi::Guarded<FeatureWrapper>> geometries;
  std::map<int, oi::Guarded<FeatureWrapper>> groups;
  std::vector<oi::Guarded<Function>>
      functions;  // not yet attached to a feature

  // The owner of every staged feature until it is handed on (to the job,
  // or to the station that owns its position and system). The maps above
  // are lookup handles only, so a decode that fails midway frees whatever
  // was built by construction.
  std::vector<std::unique_ptr<Feature>> owned;
  bool released = false;  // functions and observations have been adopted

  Staging() = default;
  Staging(const Staging&) = delete;
  Staging& operator=(const Staging&) = delete;
  Staging(Staging&&) = delete;
  Staging& operator=(Staging&&) = delete;

  ~Staging() {
    if (released) {
      return;
    }
    for (const auto& function : functions) {
      delete function.data();
    }
    for (const auto& [id, observation] : observations) {
      delete observation.data();
    }
  }

  // Hands a staged feature on; null when it was already taken.
  [[nodiscard]] std::unique_ptr<Feature> take(
      const oi::Guarded<FeatureWrapper>& wrapper) {
    if (wrapper.isNull()) {
      return nullptr;
    }
    const Feature* const raw = wrapper->getFeature().data();
    const auto it = std::find_if(
        owned.begin(), owned.end(),
        [raw](const std::unique_ptr<Feature>& f) { return f.get() == raw; });
    if (it == owned.end()) {
      return nullptr;
    }
    std::unique_ptr<Feature> result = std::move(*it);
    owned.erase(it);
    return result;
  }

  // Any feature by id, restricted to the kinds a given reference may name.
  enum Kind : unsigned {
    kGeometry = 1,
    kSystem = 2,
    kTrafo = 4,
    kStation = 8,
    kGroup = 16,
  };

  [[nodiscard]] oi::Guarded<FeatureWrapper> feature(int id,
                                                    unsigned kinds) const {
    if ((kinds & kGeometry) != 0 && geometries.contains(id)) {
      return oi::value(geometries, id);
    }
    if ((kinds & kSystem) != 0 && systems.contains(id)) {
      return oi::value(systems, id);
    }
    if ((kinds & kTrafo) != 0 && trafos.contains(id)) {
      return oi::value(trafos, id);
    }
    if ((kinds & kStation) != 0 && stations.contains(id)) {
      return oi::value(stations, id);
    }
    if ((kinds & kGroup) != 0 && groups.contains(id)) {
      return oi::value(groups, id);
    }
    return {};
  }
};

// The messages the linking pass reads the references from, next to the
// objects created from them.
template <class Message, class Object>
struct Created {
  const Message* message;
  oi::Guarded<Object> object;
};

class Linker {
 public:
  Linker(const oipb::Project& project, ProjectHost& host, Staging& staging,
         DecodeResult& result)
      : project_(project), host_(host), staging_(staging), result_(result) {}

  bool run();

 private:
  void missing(const oipb::PluginObjectRef& ref) {
    const std::string name = ref.plugin() + "/" + ref.name();
    if (std::find(result_.missingPlugins.begin(), result_.missingPlugins.end(),
                  name) == result_.missingPlugins.end()) {
      result_.missingPlugins.push_back(name);
    }
  }

  bool create();
  void linkObservations();
  void linkStations();
  void linkSystems();
  void linkTrafos();
  void linkGeometries();
  void linkGroups();

  void linkCommon(const oipb::FeatureCommon& common,
                  const oi::Guarded<FeatureWrapper>& wrapper);
  void attachFunctions(const oipb::FeatureCommon& common, Feature& feature);
  [[nodiscard]] oi::Guarded<Function> buildFunction(
      const oipb::FunctionRecord& record);
  [[nodiscard]] bool resolveInput(InputElement& element) const;
  [[nodiscard]] oi::Guarded<BundleAdjustment> buildBundle(
      const oipb::BundleAdjustmentRecord& record,
      const oi::Guarded<CoordinateSystem>& system);
  void linkMeasurementConfig(const oipb::Geometry& message, Geometry& geometry);

  const oipb::Project& project_;
  ProjectHost& host_;
  Staging& staging_;
  DecodeResult& result_;

  std::map<MeasurementConfigKey, MeasurementConfig> projectConfigs_;
};

bool Linker::create() {
  for (const auto& message : project_.observations()) {
    oi::Guarded<Observation> observation = createObservation(message);
    if (observation.isNull() || staging_.observations.contains(message.id())) {
      delete observation.data();
      continue;
    }
    staging_.observations.insert_or_assign(message.id(), observation);
    staging_.readings.insert_or_assign(observation->getReading()->getId(),
                                       observation->getReading());
  }

  // Every duplicate id is dropped (the first one wins) rather than silently
  // replacing an entity other entities already refer to.
  const auto add = [this](std::map<int, oi::Guarded<FeatureWrapper>>& into,
                          int id, std::unique_ptr<Feature> feature) {
    if (!feature || into.contains(id)) {
      return;  // the duplicate is freed here
    }
    const oi::Guarded<FeatureWrapper> wrapper = feature->getFeatureWrapper();
    if (wrapper.isNull() ||
        wrapper->getFeatureTypeEnum() == FeatureTypes::eUndefinedFeature) {
      return;
    }
    into.insert_or_assign(id, wrapper);
    staging_.owned.push_back(std::move(feature));
  };

  for (const auto& message : project_.stations()) {
    add(staging_.stations, message.feature().id(), createStation(message));
  }
  for (const auto& message : project_.coordinate_systems()) {
    add(staging_.systems, message.feature().id(),
        createCoordinateSystem(message));
  }
  for (const auto& message : project_.trafo_params()) {
    add(staging_.trafos, message.feature().id(), createTrafoParam(message));
  }
  for (const auto& message : project_.geometries()) {
    add(staging_.geometries, message.common().id(), createGeometry(message));
  }
  for (const auto& message : project_.groups()) {
    add(staging_.groups, message.common().id(), createGroup(message));
  }

  if (staging_.stations.empty()) {
    result_.error = "the project has no station";
    return false;
  }
  if (staging_.systems.empty()) {
    result_.error = "the project has no coordinate system";
    return false;
  }
  if (!staging_.stations.contains(project_.active_station())) {
    result_.error = "the active station is not in the project";
    return false;
  }
  if (!staging_.systems.contains(project_.active_coordinate_system())) {
    result_.error = "the active coordinate system is not in the project";
    return false;
  }

  for (const auto& message : project_.measurement_configs()) {
    if (auto config = fromProto(message)) {
      config->makeProjectConfig();
      projectConfigs_.insert_or_assign(config->getKey(), *config);
    }
  }
  return true;
}

// ---- shared pieces ---------------------------------------------------------

void Linker::linkCommon(const oipb::FeatureCommon& common,
                        const oi::Guarded<FeatureWrapper>& wrapper) {
  Feature& feature = *wrapper->getFeature();
  attachFunctions(common, feature);

  // what a feature feeds is any feature but a station or group; what it
  // needed can be a station as well
  for (const int id : common.used_for()) {
    if (auto other = staging_.feature(
            id, Staging::kGeometry | Staging::kSystem | Staging::kTrafo)) {
      feature.addUsedFor(other);
    }
  }
  for (const int id : common.previously_needed()) {
    if (auto other =
            staging_.feature(id, Staging::kGeometry | Staging::kSystem |
                                     Staging::kTrafo | Staging::kStation)) {
      feature.addPreviouslyNeeded(other);
    }
  }
}

void Linker::attachFunctions(const oipb::FeatureCommon& common,
                             Feature& feature) {
  for (const auto& record : common.functions()) {
    oi::Guarded<Function> function = buildFunction(record);
    if (function.isNull()) {
      continue;  // already reported as missing
    }
    oi::removeOne(staging_.functions, function);
    feature.addFunction(function);
  }
}

oi::Guarded<Function> Linker::buildFunction(
    const oipb::FunctionRecord& record) {
  std::unique_ptr<Function> created =
      host_.createFunction(record.plugin().plugin(), record.plugin().name());
  if (!created) {
    missing(record.plugin());
    return {};
  }
  oi::Guarded<Function> function = created.release();
  staging_.functions.push_back(function);
  applyRecord(record, *function);

  // point the inputs at the objects they name; one that no longer exists is
  // dropped, since a function must never be left holding a null input
  auto& inputs = Access::inputs(*function);
  for (auto it = inputs.begin(); it != inputs.end(); ++it) {
    std::vector<InputElement>& list = it->second;
    for (std::ptrdiff_t i = std::ssize(list) - 1; i >= 0; --i) {
      if (!resolveInput(list[i])) {
        oi::removeAt(list, i);
      }
    }
  }
  return function;
}

bool Linker::resolveInput(InputElement& element) const {
  const int id = element.id;

  if (staging_.stations.contains(id)) {
    element.station = oi::value(staging_.stations, id)->getStation();
    element.typeOfElement = ElementTypes::eStationElement;
    return true;
  }
  if (staging_.systems.contains(id)) {
    element.coordSystem =
        oi::value(staging_.systems, id)->getCoordinateSystem();
    element.typeOfElement = ElementTypes::eCoordinateSystemElement;
    return true;
  }
  if (staging_.trafos.contains(id)) {
    element.trafoParam = oi::value(staging_.trafos, id)->getTrafoParam();
    element.typeOfElement = ElementTypes::eTrafoParamElement;
    return true;
  }
  if (staging_.geometries.contains(id)) {
    const oi::Guarded<FeatureWrapper> wrapper =
        oi::value(staging_.geometries, id);
    element.geometry = wrapper->getGeometry();
    switch (wrapper->getFeatureTypeEnum()) {
      case FeatureTypes::eCircleFeature:
        element.circle = wrapper->getCircle();
        element.typeOfElement = ElementTypes::eCircleElement;
        break;
      case FeatureTypes::eConeFeature:
        element.cone = wrapper->getCone();
        element.typeOfElement = ElementTypes::eConeElement;
        break;
      case FeatureTypes::eCylinderFeature:
        element.cylinder = wrapper->getCylinder();
        element.typeOfElement = ElementTypes::eCylinderElement;
        break;
      case FeatureTypes::eEllipseFeature:
        element.ellipse = wrapper->getEllipse();
        element.typeOfElement = ElementTypes::eEllipseElement;
        break;
      case FeatureTypes::eEllipsoidFeature:
        element.ellipsoid = wrapper->getEllipsoid();
        element.typeOfElement = ElementTypes::eEllipsoidElement;
        break;
      case FeatureTypes::eHyperboloidFeature:
        element.hyperboloid = wrapper->getHyperboloid();
        element.typeOfElement = ElementTypes::eHyperboloidElement;
        break;
      case FeatureTypes::eLineFeature:
        element.line = wrapper->getLine();
        element.typeOfElement = ElementTypes::eLineElement;
        break;
      case FeatureTypes::eNurbsFeature:
        element.nurbs = wrapper->getNurbs();
        element.typeOfElement = ElementTypes::eNurbsElement;
        break;
      case FeatureTypes::eParaboloidFeature:
        element.paraboloid = wrapper->getParaboloid();
        element.typeOfElement = ElementTypes::eParaboloidElement;
        break;
      case FeatureTypes::ePlaneFeature:
        element.plane = wrapper->getPlane();
        element.typeOfElement = ElementTypes::ePlaneElement;
        break;
      case FeatureTypes::ePointFeature:
        element.point = wrapper->getPoint();
        element.typeOfElement = ElementTypes::ePointElement;
        break;
      case FeatureTypes::ePointCloudFeature:
        element.pointCloud = wrapper->getPointCloud();
        element.typeOfElement = ElementTypes::ePointCloudElement;
        break;
      case FeatureTypes::eScalarEntityAngleFeature:
        element.scalarEntityAngle = wrapper->getScalarEntityAngle();
        element.typeOfElement = ElementTypes::eScalarEntityAngleElement;
        break;
      case FeatureTypes::eScalarEntityDistanceFeature:
        element.scalarEntityDistance = wrapper->getScalarEntityDistance();
        element.typeOfElement = ElementTypes::eScalarEntityDistanceElement;
        break;
      case FeatureTypes::eScalarEntityMeasurementSeriesFeature:
        element.scalarEntityMeasurementSeries =
            wrapper->getScalarEntityMeasurementSeries();
        element.typeOfElement =
            ElementTypes::eScalarEntityMeasurementSeriesElement;
        break;
      case FeatureTypes::eScalarEntityTemperatureFeature:
        element.scalarEntityTemperature = wrapper->getScalarEntityTemperature();
        element.typeOfElement = ElementTypes::eScalarEntityTemperatureElement;
        break;
      case FeatureTypes::eSlottedHoleFeature:
        element.slottedHole = wrapper->getSlottedHole();
        element.typeOfElement = ElementTypes::eSlottedHoleElement;
        break;
      case FeatureTypes::eSphereFeature:
        element.sphere = wrapper->getSphere();
        element.typeOfElement = ElementTypes::eSphereElement;
        break;
      case FeatureTypes::eTorusFeature:
        element.torus = wrapper->getTorus();
        element.typeOfElement = ElementTypes::eTorusElement;
        break;
      default:
        return false;
    }
    return true;
  }
  if (staging_.observations.contains(id)) {
    element.observation = oi::value(staging_.observations, id);
    element.typeOfElement = ElementTypes::eObservationElement;
    return true;
  }
  if (staging_.readings.contains(id)) {
    const oi::Guarded<Reading> reading = oi::value(staging_.readings, id);
    switch (reading->getTypeOfReading()) {
      case ReadingTypes::eDistanceReading:
        element.distanceReading = reading;
        element.typeOfElement = ElementTypes::eReadingDistanceElement;
        return true;
      case ReadingTypes::eCartesianReading:
        element.cartesianReading = reading;
        element.typeOfElement = ElementTypes::eReadingCartesianElement;
        return true;
      case ReadingTypes::ePolarReading:
        element.polarReading = reading;
        element.typeOfElement = ElementTypes::eReadingPolarElement;
        return true;
      case ReadingTypes::eDirectionReading:
        element.directionReading = reading;
        element.typeOfElement = ElementTypes::eReadingDirectionElement;
        return true;
      case ReadingTypes::eTemperatureReading:
        element.temperatureReading = reading;
        element.typeOfElement = ElementTypes::eReadingTemperatureElement;
        return true;
      case ReadingTypes::eLevelReading:
        element.levelReading = reading;
        element.typeOfElement = ElementTypes::eReadingLevelElement;
        return true;
      default:
        return false;
    }
  }
  return false;
}

oi::Guarded<BundleAdjustment> Linker::buildBundle(
    const oipb::BundleAdjustmentRecord& record,
    const oi::Guarded<CoordinateSystem>& system) {
  std::unique_ptr<BundleAdjustment> created = host_.createBundleAdjustment(
      record.plugin().plugin(), record.plugin().name());
  if (!created) {
    missing(record.plugin());
    return {};
  }
  oi::Guarded<BundleAdjustment> bundle = created.release();
  applyRecord(record, *bundle);
  bundle->setBundleSystem(system);
  return bundle;
}

// ---- per entity ------------------------------------------------------------

void Linker::linkObservations() {
  for (const auto& message : project_.observations()) {
    if (!staging_.observations.contains(message.id())) {
      continue;
    }
    const oi::Guarded<Observation> observation =
        oi::value(staging_.observations, message.id());

    if (staging_.stations.contains(message.station())) {
      const oi::Guarded<Station> station =
          oi::value(staging_.stations, message.station())->getStation();
      observation->setStation(station);
      if (station->getIsSensorSet()) {
        observation->getReading()->setSensorConfiguration(
            station->getSensorConfiguration());
      }
      Access::addStationReading(*station, observation->getReading());
    }
    if (staging_.geometries.contains(message.measured_target_geometry())) {
      observation->setMeasuredTargetGeometry(
          oi::value(staging_.geometries, message.measured_target_geometry())
              ->getGeometry());
    }
  }
}

void Linker::linkStations() {
  for (const auto& message : project_.stations()) {
    const oi::Guarded<FeatureWrapper> wrapper =
        oi::value(staging_.stations, message.feature().id());
    if (wrapper.isNull()) {
      continue;
    }
    const oi::Guarded<Station> station = wrapper->getStation();

    std::vector<Sensor> usedSensors;
    for (const auto& ref : message.used_sensors()) {
      std::unique_ptr<Sensor> sensor =
          host_.createSensor(ref.plugin(), ref.name());
      if (!sensor) {
        missing(ref);
        continue;
      }
      usedSensors.push_back(Sensor(*sensor));
    }
    Access::setUsedSensors(*station, usedSensors);

    if (message.has_active_sensor()) {
      std::unique_ptr<Sensor> sensor = host_.createSensor(
          message.active_sensor().plugin(), message.active_sensor().name());
      if (sensor) {
        station->setSensor(sensor.release());
      } else {
        missing(message.active_sensor());
      }
    }

    if (staging_.geometries.contains(message.position())) {
      const oi::Guarded<Point> point =
          oi::value(staging_.geometries, message.position())->getPoint();
      if (!point.isNull()) {
        // the station owns its position from here on (it deletes it)
        Access::setStationPosition(*station, point);
        [[maybe_unused]] Feature* const handedOver =
            staging_.take(oi::value(staging_.geometries, message.position()))
                .release();
      }
    }
    if (staging_.systems.contains(message.coordinate_system())) {
      const oi::Guarded<CoordinateSystem> system =
          oi::value(staging_.systems, message.coordinate_system())
              ->getCoordinateSystem();
      if (!system.isNull()) {
        // the station owns its system from here on (it deletes it)
        Access::setStationSystem(*station, system);
        [[maybe_unused]] Feature* const handedOver =
            staging_
                .take(oi::value(staging_.systems, message.coordinate_system()))
                .release();
      }
    }

    linkCommon(message.feature(), wrapper);
  }
}

void Linker::linkSystems() {
  for (const auto& message : project_.coordinate_systems()) {
    const oi::Guarded<FeatureWrapper> wrapper =
        oi::value(staging_.systems, message.feature().id());
    if (wrapper.isNull()) {
      continue;
    }
    const oi::Guarded<CoordinateSystem> system = wrapper->getCoordinateSystem();

    if (message.has_bundle_adjustment()) {
      const oi::Guarded<BundleAdjustment> bundle =
          buildBundle(message.bundle_adjustment(), system);
      if (!bundle.isNull()) {
        system->setBundlePlugin(bundle);
      }
    }

    for (const int id : message.observations()) {
      if (staging_.observations.contains(id)) {
        Access::addSystemObservation(*system,
                                     oi::value(staging_.observations, id));
      }
    }
    for (const int id : message.nominal_geometries()) {
      if (staging_.geometries.contains(id)) {
        const oi::Guarded<FeatureWrapper> nominal =
            oi::value(staging_.geometries, id);
        // a geometry owned by another feature is regenerated, not listed
        if (!nominal->getGeometry()->getOwnerDescription().has_value()) {
          Access::addSystemNominal(*system, nominal);
        }
      }
    }
    for (const int id : message.trafo_params()) {
      if (staging_.trafos.contains(id)) {
        system->addTransformationParameter(
            oi::value(staging_.trafos, id)->getTrafoParam());
      }
    }

    linkCommon(message.feature(), wrapper);
  }
}

void Linker::linkTrafos() {
  for (const auto& message : project_.trafo_params()) {
    const oi::Guarded<FeatureWrapper> wrapper =
        oi::value(staging_.trafos, message.feature().id());
    if (wrapper.isNull()) {
      continue;
    }
    if (staging_.systems.contains(message.from()) &&
        staging_.systems.contains(message.to())) {
      wrapper->getTrafoParam()->setCoordinateSystems(
          oi::value(staging_.systems, message.from())->getCoordinateSystem(),
          oi::value(staging_.systems, message.to())->getCoordinateSystem());
    }
    linkCommon(message.feature(), wrapper);
  }
}

void Linker::linkMeasurementConfig(const oipb::Geometry& message,
                                   Geometry& geometry) {
  if (!message.has_measurement_config() ||
      message.measurement_config().name().empty()) {
    return;
  }
  const MeasurementConfigKey saved = configKeyOf(message.measurement_config());
  // a standard config of that name wins over a project copy
  const MeasurementConfigKey standardKey(
      saved.getName(), ConfigTypes::eStandardConfig, saved.getSensorType());
  const MeasurementConfigKey projectKey(
      saved.getName(), ConfigTypes::eProjectConfig, saved.getSensorType());

  const MeasurementConfig standard = host_.measurementConfig(standardKey);
  if (standard.isValid()) {
    geometry.setMeasurementConfig(standard.getKey());
    return;
  }
  const auto project = projectConfigs_.find(projectKey);
  if (project != projectConfigs_.cend() && project->second.isValid()) {
    geometry.setMeasurementConfig(project->second.getKey());
  }
}

void Linker::linkGeometries() {
  for (const auto& message : project_.geometries()) {
    const oi::Guarded<FeatureWrapper> wrapper =
        oi::value(staging_.geometries, message.common().id());
    if (wrapper.isNull()) {
      continue;
    }
    const oi::Guarded<Geometry> geometry = wrapper->getGeometry();

    for (const int id : message.observations()) {
      if (staging_.observations.contains(id)) {
        geometry->addObservation(oi::value(staging_.observations, id));
      }
    }

    if (message.realises() != 0 &&
        staging_.geometries.contains(message.realises())) {
      geometry->setRealises(
          oi::value(staging_.geometries, message.realises())->getGeometry());
    }

    // A geometry owned by another feature was computed from it: once the
    // owner is resolved and the geometry is solved there is nothing to
    // recompute, so mark it clean rather than re-run its construction chain
    // on open. A later real change to the owner dirties it again.
    if (message.owner_feature() != 0 &&
        staging_.geometries.contains(message.owner_feature())) {
      if (geometry->setOwnerFeature(
              oi::value(staging_.geometries, message.owner_feature())
                  ->getGeometry()) &&
          geometry->getIsSolved()) {
        geometry->setIsUpdated(true);
      }
    }

    if (staging_.systems.contains(message.nominal_system())) {
      geometry->setNominalSystem(
          oi::value(staging_.systems, message.nominal_system())
              ->getCoordinateSystem());
    }

    if (!isScalarEntity(*wrapper)) {
      linkMeasurementConfig(message, *geometry);
    }

    if (message.has_point_cloud()) {
      const oi::Guarded<PointCloud> cloud = wrapper->getPointCloud();
      for (const int id : message.point_cloud().segments()) {
        if (!cloud.isNull() && staging_.geometries.contains(id)) {
          Access::addSegment(*cloud, oi::value(staging_.geometries, id));
        }
      }
    }

    linkCommon(message.common(), wrapper);
  }
}

void Linker::linkGroups() {
  for (const auto& message : project_.groups()) {
    const oi::Guarded<FeatureWrapper> wrapper =
        oi::value(staging_.groups, message.common().id());
    if (wrapper.isNull()) {
      continue;
    }
    for (const int id : message.members()) {
      if (auto member = staging_.feature(
              id, Staging::kGeometry | Staging::kSystem | Staging::kTrafo |
                      Staging::kStation | Staging::kGroup)) {
        wrapper->getGroup()->addMember(member);
      }
    }
    linkCommon(message.common(), wrapper);
  }
}

bool Linker::run() {
  if (!create()) {
    return false;
  }

  linkObservations();
  linkStations();
  linkSystems();
  linkTrafos();
  linkGeometries();
  linkGroups();

  if (!result_.missingPlugins.empty()) {
    std::sort(result_.missingPlugins.begin(), result_.missingPlugins.end());
    result_.error = "the project needs plugins that are not installed";
    return false;
  }
  return true;
}

}  // namespace

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

std::string encodeProject(OiJob& job, const ProjectHost& host,
                          const std::string& appVersion) {
  const Collected collected = collect(job);

  // The part of the project that decides whether it changed: the entities
  // and the active selection, but not the save time or version.
  oipb::Project body;
  if (!job.getActiveStation().isNull()) {
    body.set_active_station(job.getActiveStation()->getId());
  }
  if (!job.getActiveCoordinateSystem().isNull()) {
    body.set_active_coordinate_system(job.getActiveCoordinateSystem()->getId());
  }
  for (const auto& [key, config] : projectConfigsOf(collected, host)) {
    *body.add_measurement_configs() = toProto(config);
  }
  for (const auto& [id, station] : collected.stations) {
    *body.add_stations() = toProto(*station);
  }
  for (const auto& [id, system] : collected.systems) {
    *body.add_coordinate_systems() = toProto(*system);
  }
  for (const auto& [id, trafo] : collected.trafos) {
    *body.add_trafo_params() = toProto(*trafo);
  }
  for (const auto& [id, geometry] : collected.geometries) {
    *body.add_geometries() = toProto(*geometry);
  }
  for (const auto& [id, group] : collected.groups) {
    *body.add_groups() = toProto(*group);
  }
  for (const auto& [id, observation] : collected.observations) {
    *body.add_observations() = toProto(*observation);
  }

  const std::string digest = digestOf(body);
  job.setDigest((digest));

  oipb::Project project = std::move(body);
  project.set_format_version(kFormatVersion);
  project.set_name(job.getJobName());
  *project.mutable_saved_at() = toTimestamp(std::chrono::system_clock::now());
  project.set_app_version(appVersion);
  project.set_next_id(Access::nextId(job));
  project.set_digest(digest);
  return serialize(project);
}

DecodeResult decodeProject(std::string_view bytes, ProjectHost& host) {
  DecodeResult result;

  oipb::Project project;
  if (!project.ParseFromArray(bytes.data(), static_cast<int>(bytes.size()))) {
    result.error = "not a valid OpenIndy project file";
    return result;
  }
  if (project.format_version() != kFormatVersion) {
    result.error = "unsupported project format version " +
                   std::to_string(project.format_version());
    return result;
  }

  Staging staging;
  Linker linker(project, host, staging, result);
  if (!linker.run()) {
    return result;
  }

  // The station's own point is part of the station; only the station and its
  // point reach the job through the station, not through the geometry list.
  std::vector<oi::Guarded<FeatureWrapper>> geometries;
  std::set<int> stationPoints;
  for (const auto& [id, wrapper] : staging.stations) {
    const oi::Guarded<Point> point = wrapper->getStation()->getPosition();
    if (!point.isNull()) {
      stationPoints.insert(point->getId());
    }
  }
  for (auto it = staging.geometries.cbegin(); it != staging.geometries.cend();
       ++it) {
    if (stationPoints.count(it->first) == 0) {
      geometries.push_back(it->second);
    }
  }

  oi::Guarded<OiJob> job = new OiJob();
  const auto adoptAll = [&](const auto& wrappers) {
    std::vector<std::unique_ptr<Feature>> features;
    for (const oi::Guarded<FeatureWrapper>& wrapper : wrappers) {
      if (auto feature = staging.take(wrapper)) {
        features.push_back(std::move(feature));
      }
    }
    Access::adopt(*job, std::move(features));
  };
  adoptAll(oi::values(staging.stations));
  adoptAll(oi::values(staging.systems));
  adoptAll(oi::values(staging.trafos));
  adoptAll(geometries);
  adoptAll(oi::values(staging.groups));
  staging.released = true;

  if (project.next_id() > 0) {
    Access::setNextId(*job, project.next_id());
  }
  job->setDigest((project.digest()));
  job->setLoadedProjectVersion((project.app_version()));

  oi::value(staging.stations, project.active_station())
      ->getStation()
      ->setActiveStationState(true);
  oi::value(staging.systems, project.active_coordinate_system())
      ->getCoordinateSystem()
      ->setActiveCoordinateSystemState(true);

  for (const auto& message : project.measurement_configs()) {
    if (auto config = fromProto(message)) {
      config->makeProjectConfig();
      result.projectConfigs.push_back(*config);
    }
  }

  result.job = job;
  return result;
}

}  // namespace oi::codec
