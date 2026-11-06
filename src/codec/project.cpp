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

#include <QCryptographicHash>
#include <QMap>
#include <QPointer>
#include <algorithm>
#include <chrono>
#include <map>
#include <set>

#include "access.h"
#include "convert.h"
#include "featurewrapper.h"
#include "oijob.h"
#include "plugin/sensor/sensor.h"
#include "pointgroup.h"
#include "project_codec.h"
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
  QCryptographicHash hash(QCryptographicHash::Sha256);
  hash.addData(
      QByteArrayView(bytes.data(), static_cast<qsizetype>(bytes.size())));
  return hash.result().toHex().toStdString();
}

// The job holds features through FeatureWrapper, one wrapper per feature. (A
// feature also carries a wrapper of its own, but the job and the features'
// reference lists have always used separately created ones.)
template <class T>
bool wrapAs(QPointer<FeatureWrapper>& wrapper, Feature* feature,
            void (FeatureWrapper::*set)(const QPointer<T>&)) {
  if (auto* typed = dynamic_cast<T*>(feature)) {
    wrapper = new FeatureWrapper();
    (wrapper.data()->*set)(typed);
    return true;
  }
  return false;
}

QPointer<FeatureWrapper> wrap(Feature* feature) {
  QPointer<FeatureWrapper> wrapper;
  // a point group is a group, so it has to be asked first
  const bool known =
      wrapAs<Station>(wrapper, feature, &FeatureWrapper::setStation) ||
      wrapAs<CoordinateSystem>(wrapper, feature,
                               &FeatureWrapper::setCoordinateSystem) ||
      wrapAs<TrafoParam>(wrapper, feature, &FeatureWrapper::setTrafoParam) ||
      wrapAs<PointGroup>(wrapper, feature, &FeatureWrapper::setPointGroup) ||
      wrapAs<Group>(wrapper, feature, &FeatureWrapper::setGroup) ||
      wrapAs<Circle>(wrapper, feature, &FeatureWrapper::setCircle) ||
      wrapAs<Cone>(wrapper, feature, &FeatureWrapper::setCone) ||
      wrapAs<Cylinder>(wrapper, feature, &FeatureWrapper::setCylinder) ||
      wrapAs<Ellipse>(wrapper, feature, &FeatureWrapper::setEllipse) ||
      wrapAs<Ellipsoid>(wrapper, feature, &FeatureWrapper::setEllipsoid) ||
      wrapAs<Hyperboloid>(wrapper, feature, &FeatureWrapper::setHyperboloid) ||
      wrapAs<Line>(wrapper, feature, &FeatureWrapper::setLine) ||
      wrapAs<Nurbs>(wrapper, feature, &FeatureWrapper::setNurbs) ||
      wrapAs<Paraboloid>(wrapper, feature, &FeatureWrapper::setParaboloid) ||
      wrapAs<Plane>(wrapper, feature, &FeatureWrapper::setPlane) ||
      wrapAs<Point>(wrapper, feature, &FeatureWrapper::setPoint) ||
      wrapAs<PointCloud>(wrapper, feature, &FeatureWrapper::setPointCloud) ||
      wrapAs<ScalarEntityAngle>(wrapper, feature,
                                &FeatureWrapper::setScalarEntityAngle) ||
      wrapAs<ScalarEntityDistance>(wrapper, feature,
                                   &FeatureWrapper::setScalarEntityDistance) ||
      wrapAs<ScalarEntityMeasurementSeries>(
          wrapper, feature,
          &FeatureWrapper::setScalarEntityMeasurementSeries) ||
      wrapAs<ScalarEntityTemperature>(
          wrapper, feature, &FeatureWrapper::setScalarEntityTemperature) ||
      wrapAs<SlottedHole>(wrapper, feature, &FeatureWrapper::setSlottedHole) ||
      wrapAs<Sphere>(wrapper, feature, &FeatureWrapper::setSphere) ||
      wrapAs<Torus>(wrapper, feature, &FeatureWrapper::setTorus);
  return known ? wrapper : QPointer<FeatureWrapper>();
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

  for (const QPointer<Station>& station : job.getStationsList()) {
    if (station.isNull()) {
      continue;
    }
    out.stations.emplace(station->getId(), station.data());
    if (!station->getCoordinateSystem().isNull()) {
      const CoordinateSystem* system = station->getCoordinateSystem().data();
      out.systems.emplace(system->getId(), system);
      for (const QPointer<Observation>& observation :
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

  for (const QPointer<CoordinateSystem>& system :
       job.getCoordinateSystemsList()) {
    if (!system.isNull()) {
      out.systems.emplace(system->getId(), system.data());
    }
  }
  for (const QPointer<TrafoParam>& trafo :
       job.getTransformationParametersList()) {
    if (!trafo.isNull()) {
      out.trafos.emplace(trafo->getId(), trafo.data());
    }
  }
  for (const QPointer<FeatureWrapper>& wrapper : job.getGeometriesList()) {
    if (!wrapper.isNull() && !wrapper->getGeometry().isNull()) {
      out.geometries.emplace(wrapper->getGeometry()->getId(),
                             wrapper->getGeometry().data());
    }
  }
  for (const FeatureTypes kind :
       {FeatureTypes::eGroupFeature, FeatureTypes::ePointGroupFeature}) {
    for (const QPointer<FeatureWrapper>& wrapper :
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
QMap<MeasurementConfigKey, MeasurementConfig> projectConfigsOf(
    const Collected& collected, const ProjectHost& host) {
  QMap<MeasurementConfigKey, MeasurementConfig> configs;
  for (const auto& [id, geometry] : collected.geometries) {
    const MeasurementConfigKey key = geometry->getMeasurementConfig();
    if (!key.isValid() || key.isStandardConfig() || configs.contains(key)) {
      continue;
    }
    const MeasurementConfig config = host.measurementConfig(key);
    if (config.isValid()) {
      configs.insert(key, config);
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
  QMap<int, QPointer<Observation>> observations;
  QMap<int, QPointer<Reading>> readings;
  QMap<int, QPointer<FeatureWrapper>> stations;
  QMap<int, QPointer<FeatureWrapper>> systems;
  QMap<int, QPointer<FeatureWrapper>> trafos;
  QMap<int, QPointer<FeatureWrapper>> geometries;
  QMap<int, QPointer<FeatureWrapper>> groups;
  QList<QPointer<Function>> functions;  // not yet attached to a feature
  bool released = false;

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
    for (const auto& observation : observations) {
      delete observation.data();
    }
    for (const auto* map :
         {&stations, &systems, &trafos, &geometries, &groups}) {
      for (const auto& wrapper : *map) {
        if (wrapper.isNull()) {
          continue;
        }
        const QPointer<FeatureWrapper> keep = wrapper;
        delete keep->getFeature().data();
        delete keep.data();
      }
    }
  }

  // Any feature by id, restricted to the kinds a given reference may name.
  enum Kind : unsigned {
    kGeometry = 1,
    kSystem = 2,
    kTrafo = 4,
    kStation = 8,
    kGroup = 16,
  };

  [[nodiscard]] QPointer<FeatureWrapper> feature(int id, unsigned kinds) const {
    if ((kinds & kGeometry) != 0 && geometries.contains(id)) {
      return geometries.value(id);
    }
    if ((kinds & kSystem) != 0 && systems.contains(id)) {
      return systems.value(id);
    }
    if ((kinds & kTrafo) != 0 && trafos.contains(id)) {
      return trafos.value(id);
    }
    if ((kinds & kStation) != 0 && stations.contains(id)) {
      return stations.value(id);
    }
    if ((kinds & kGroup) != 0 && groups.contains(id)) {
      return groups.value(id);
    }
    return {};
  }
};

// The messages the linking pass reads the references from, next to the
// objects created from them.
template <class Message, class Object>
struct Created {
  const Message* message;
  QPointer<Object> object;
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
                  const QPointer<FeatureWrapper>& wrapper);
  void attachFunctions(const oipb::FeatureCommon& common, Feature& feature);
  [[nodiscard]] QPointer<Function> buildFunction(
      const oipb::FunctionRecord& record);
  [[nodiscard]] bool resolveInput(InputElement& element) const;
  [[nodiscard]] QPointer<BundleAdjustment> buildBundle(
      const oipb::BundleAdjustmentRecord& record,
      const QPointer<CoordinateSystem>& system);
  void linkMeasurementConfig(const oipb::Geometry& message, Geometry& geometry);

  const oipb::Project& project_;
  ProjectHost& host_;
  Staging& staging_;
  DecodeResult& result_;

  QMap<MeasurementConfigKey, MeasurementConfig> projectConfigs_;
};

bool Linker::create() {
  for (const auto& message : project_.observations()) {
    QPointer<Observation> observation = createObservation(message);
    if (observation.isNull() || staging_.observations.contains(message.id())) {
      delete observation.data();
      continue;
    }
    staging_.observations.insert(message.id(), observation);
    staging_.readings.insert(observation->getReading()->getId(),
                             observation->getReading());
  }

  // Every duplicate id is dropped (the first one wins) rather than silently
  // replacing an entity other entities already refer to.
  const auto add = [](QMap<int, QPointer<FeatureWrapper>>& into, int id,
                      const QPointer<Feature>& feature) {
    if (feature.isNull()) {
      return;
    }
    if (into.contains(id)) {
      delete feature.data();
      return;
    }
    const QPointer<FeatureWrapper> wrapper = wrap(feature);
    if (wrapper.isNull()) {
      delete feature.data();
      return;
    }
    into.insert(id, wrapper);
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

  if (staging_.stations.isEmpty()) {
    result_.error = "the project has no station";
    return false;
  }
  if (staging_.systems.isEmpty()) {
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
      projectConfigs_.insert(config->getKey(), *config);
    }
  }
  return true;
}

// ---- shared pieces ---------------------------------------------------------

void Linker::linkCommon(const oipb::FeatureCommon& common,
                        const QPointer<FeatureWrapper>& wrapper) {
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
    QPointer<Function> function = buildFunction(record);
    if (function.isNull()) {
      continue;  // already reported as missing
    }
    staging_.functions.removeOne(function);
    feature.addFunction(function);
  }
}

QPointer<Function> Linker::buildFunction(const oipb::FunctionRecord& record) {
  std::unique_ptr<Function> created =
      host_.createFunction(record.plugin().plugin(), record.plugin().name());
  if (!created) {
    missing(record.plugin());
    return {};
  }
  QPointer<Function> function = created.release();
  staging_.functions.append(function);
  applyRecord(record, *function);

  // point the inputs at the objects they name; one that no longer exists is
  // dropped, since a function must never be left holding a null input
  auto& inputs = Access::inputs(*function);
  for (auto it = inputs.begin(); it != inputs.end(); ++it) {
    QList<InputElement>& list = it.value();
    for (qsizetype i = list.size() - 1; i >= 0; --i) {
      if (!resolveInput(list[i])) {
        list.removeAt(i);
      }
    }
  }
  return function;
}

bool Linker::resolveInput(InputElement& element) const {
  const int id = element.id;

  if (staging_.stations.contains(id)) {
    element.station = staging_.stations.value(id)->getStation();
    element.typeOfElement = ElementTypes::eStationElement;
    return true;
  }
  if (staging_.systems.contains(id)) {
    element.coordSystem = staging_.systems.value(id)->getCoordinateSystem();
    element.typeOfElement = ElementTypes::eCoordinateSystemElement;
    return true;
  }
  if (staging_.trafos.contains(id)) {
    element.trafoParam = staging_.trafos.value(id)->getTrafoParam();
    element.typeOfElement = ElementTypes::eTrafoParamElement;
    return true;
  }
  if (staging_.geometries.contains(id)) {
    const QPointer<FeatureWrapper> wrapper = staging_.geometries.value(id);
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
    element.observation = staging_.observations.value(id);
    element.typeOfElement = ElementTypes::eObservationElement;
    return true;
  }
  if (staging_.readings.contains(id)) {
    const QPointer<Reading> reading = staging_.readings.value(id);
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

QPointer<BundleAdjustment> Linker::buildBundle(
    const oipb::BundleAdjustmentRecord& record,
    const QPointer<CoordinateSystem>& system) {
  std::unique_ptr<BundleAdjustment> created = host_.createBundleAdjustment(
      record.plugin().plugin(), record.plugin().name());
  if (!created) {
    missing(record.plugin());
    return {};
  }
  QPointer<BundleAdjustment> bundle = created.release();
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
    const QPointer<Observation> observation =
        staging_.observations.value(message.id());

    if (staging_.stations.contains(message.station())) {
      const QPointer<Station> station =
          staging_.stations.value(message.station())->getStation();
      observation->setStation(station);
      if (station->getIsSensorSet()) {
        observation->getReading()->setSensorConfiguration(
            station->getSensorConfiguration());
      }
      Access::addStationReading(*station, observation->getReading());
    }
    if (staging_.geometries.contains(message.measured_target_geometry())) {
      observation->setMeasuredTargetGeometry(
          staging_.geometries.value(message.measured_target_geometry())
              ->getGeometry());
    }
  }
}

void Linker::linkStations() {
  for (const auto& message : project_.stations()) {
    const QPointer<FeatureWrapper> wrapper =
        staging_.stations.value(message.feature().id());
    if (wrapper.isNull()) {
      continue;
    }
    const QPointer<Station> station = wrapper->getStation();

    QList<Sensor> usedSensors;
    for (const auto& ref : message.used_sensors()) {
      std::unique_ptr<Sensor> sensor =
          host_.createSensor(ref.plugin(), ref.name());
      if (!sensor) {
        missing(ref);
        continue;
      }
      usedSensors.append(Sensor(*sensor));
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
      const QPointer<Point> point =
          staging_.geometries.value(message.position())->getPoint();
      if (!point.isNull()) {
        Access::setStationPosition(*station, point);
      }
    }
    if (staging_.systems.contains(message.coordinate_system())) {
      const QPointer<CoordinateSystem> system =
          staging_.systems.value(message.coordinate_system())
              ->getCoordinateSystem();
      if (!system.isNull()) {
        Access::setStationSystem(*station, system);
      }
    }

    linkCommon(message.feature(), wrapper);
  }
}

void Linker::linkSystems() {
  for (const auto& message : project_.coordinate_systems()) {
    const QPointer<FeatureWrapper> wrapper =
        staging_.systems.value(message.feature().id());
    if (wrapper.isNull()) {
      continue;
    }
    const QPointer<CoordinateSystem> system = wrapper->getCoordinateSystem();

    if (message.has_bundle_adjustment()) {
      const QPointer<BundleAdjustment> bundle =
          buildBundle(message.bundle_adjustment(), system);
      if (!bundle.isNull()) {
        system->setBundlePlugin(bundle);
      }
    }

    for (const int id : message.observations()) {
      if (staging_.observations.contains(id)) {
        Access::addSystemObservation(*system, staging_.observations.value(id));
      }
    }
    for (const int id : message.nominal_geometries()) {
      if (staging_.geometries.contains(id)) {
        const QPointer<FeatureWrapper> nominal = staging_.geometries.value(id);
        // a geometry owned by another feature is regenerated, not listed
        if (!nominal->getGeometry()->getOwnerDescription().has_value()) {
          Access::addSystemNominal(*system, nominal);
        }
      }
    }
    for (const int id : message.trafo_params()) {
      if (staging_.trafos.contains(id)) {
        system->addTransformationParameter(
            staging_.trafos.value(id)->getTrafoParam());
      }
    }

    linkCommon(message.feature(), wrapper);
  }
}

void Linker::linkTrafos() {
  for (const auto& message : project_.trafo_params()) {
    const QPointer<FeatureWrapper> wrapper =
        staging_.trafos.value(message.feature().id());
    if (wrapper.isNull()) {
      continue;
    }
    if (staging_.systems.contains(message.from()) &&
        staging_.systems.contains(message.to())) {
      wrapper->getTrafoParam()->setCoordinateSystems(
          staging_.systems.value(message.from())->getCoordinateSystem(),
          staging_.systems.value(message.to())->getCoordinateSystem());
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
  const auto project = projectConfigs_.constFind(projectKey);
  if (project != projectConfigs_.constEnd() && project->isValid()) {
    geometry.setMeasurementConfig(project->getKey());
  }
}

void Linker::linkGeometries() {
  for (const auto& message : project_.geometries()) {
    const QPointer<FeatureWrapper> wrapper =
        staging_.geometries.value(message.common().id());
    if (wrapper.isNull()) {
      continue;
    }
    const QPointer<Geometry> geometry = wrapper->getGeometry();

    for (const int id : message.observations()) {
      if (staging_.observations.contains(id)) {
        geometry->addObservation(staging_.observations.value(id));
      }
    }

    if (message.realises() != 0 &&
        staging_.geometries.contains(message.realises())) {
      geometry->setRealises(
          staging_.geometries.value(message.realises())->getGeometry());
    }

    // A geometry owned by another feature was computed from it: once the
    // owner is resolved and the geometry is solved there is nothing to
    // recompute, so mark it clean rather than re-run its construction chain
    // on open. A later real change to the owner dirties it again.
    if (message.owner_feature() != 0 &&
        staging_.geometries.contains(message.owner_feature())) {
      if (geometry->setOwnerFeature(
              staging_.geometries.value(message.owner_feature())
                  ->getGeometry()) &&
          geometry->getIsSolved()) {
        geometry->setIsUpdated(true);
      }
    }

    if (staging_.systems.contains(message.nominal_system())) {
      geometry->setNominalSystem(
          staging_.systems.value(message.nominal_system())
              ->getCoordinateSystem());
    }

    if (!isScalarEntity(*wrapper)) {
      linkMeasurementConfig(message, *geometry);
    }

    if (message.has_point_cloud()) {
      const QPointer<PointCloud> cloud = wrapper->getPointCloud();
      for (const int id : message.point_cloud().segments()) {
        if (!cloud.isNull() && staging_.geometries.contains(id)) {
          Access::addSegment(*cloud, staging_.geometries.value(id));
        }
      }
    }

    linkCommon(message.common(), wrapper);
  }
}

void Linker::linkGroups() {
  for (const auto& message : project_.groups()) {
    const QPointer<FeatureWrapper> wrapper =
        staging_.groups.value(message.common().id());
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
  for (const auto& config : projectConfigsOf(collected, host)) {
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
  job.setDigest(QString::fromStdString(digest));

  oipb::Project project = std::move(body);
  project.set_format_version(kFormatVersion);
  project.set_name(job.getJobName().toStdString());
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
  QList<QPointer<FeatureWrapper>> geometries;
  std::set<int> stationPoints;
  for (const auto& wrapper : staging.stations) {
    const QPointer<Point> point = wrapper->getStation()->getPosition();
    if (!point.isNull()) {
      stationPoints.insert(point->getId());
    }
  }
  for (auto it = staging.geometries.cbegin(); it != staging.geometries.cend();
       ++it) {
    if (stationPoints.count(it.key()) == 0) {
      geometries.append(it.value());
    }
  }

  QPointer<OiJob> job = new OiJob();
  Access::adopt(*job, staging.stations.values());
  Access::adopt(*job, staging.systems.values());
  Access::adopt(*job, staging.trafos.values());
  Access::adopt(*job, geometries);
  Access::adopt(*job, staging.groups.values());
  staging.released = true;

  if (project.next_id() > 0) {
    Access::setNextId(*job, project.next_id());
  }
  job->setDigest(QString::fromStdString(project.digest()));
  job->setLoadedProjectVersion(QString::fromStdString(project.app_version()));

  staging.stations.value(project.active_station())
      ->getStation()
      ->setActiveStationState(true);
  staging.systems.value(project.active_coordinate_system())
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
