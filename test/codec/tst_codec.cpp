#include <google/protobuf/util/message_differencer.h>
#include <google/protobuf/util/time_util.h>

#include <algorithm>
#include <catch2/catch_test_macros.hpp>
#include <functional>
#include <map>
#include <string>
#include <vector>

#include "bundleadjustment.h"
#include "chooselalib.h"
#include "config_codec.h"
#include "constructfunction.h"
#include "containers.h"
#include "convert.h"
#include "digest.h"
#include "fitfunction.h"
#include "geometry/plane.h"
#include "geometry/point.h"
#include "group.h"
#include "network_convert.h"
#include "oijob.h"
#include "openindy/network.pb.h"
#include "openindy/project.pb.h"
#include "pointgroup.h"
#include "project_codec.h"
#include "sensortype.h"
#include "station.h"

using namespace oi;
namespace oipb = openindy::v1;

namespace {

MeasurementConfig sampleConfig() {
  MeasurementConfig config;
  config.setName("StdScan");
  config.setMeasurementMode(MeasurementModes::ePrecise_MeasurementMode);
  config.setMeasurementType(
      MeasurementTypes::eScanDistanceDependent_MeasurementType);
  config.setMeasureTwoSides(true);
  config.setMaxObservations(25);
  config.setTimeInterval(0.5);
  config.setDistanceInterval(2.25);
  return config;
}

}  // namespace

TEST_CASE("MeasurementConfig survives a protobuf round trip") {
  const MeasurementConfig in = sampleConfig();

  std::string bytes;
  REQUIRE(codec::toProto(in).SerializeToString(&bytes));
  oipb::MeasurementConfig message;
  REQUIRE(message.ParseFromString(bytes));

  const auto out = codec::fromProto(message);
  REQUIRE(out.has_value());
  CHECK(out->getName() == in.getName());
  CHECK(out->getMeasurementMode() == in.getMeasurementMode());
  CHECK(out->getMeasurementType() == in.getMeasurementType());
  CHECK(out->getMeasureTwoSides());
  CHECK(out->getMaxObservations() == 25);
  CHECK(out->getTimeInterval() == 0.5);
  CHECK(out->getDistanceInterval() == 2.25);
  CHECK_FALSE(out->getSensorType().isValid());
}

TEST_CASE("every measurement mode and type maps both ways") {
  for (const auto mode : {MeasurementModes::eFast_MeasurementMode,
                          MeasurementModes::eStandard_MeasurementMode,
                          MeasurementModes::ePrecise_MeasurementMode}) {
    MeasurementConfig in = sampleConfig();
    in.setMeasurementMode(mode);
    CHECK(codec::fromProto(codec::toProto(in))->getMeasurementMode() == mode);
  }
  for (const auto type :
       {MeasurementTypes::eSinglePoint_MeasurementType,
        MeasurementTypes::eScanTimeDependent_MeasurementType,
        MeasurementTypes::eScanDistanceDependent_MeasurementType,
        MeasurementTypes::eLevel_MeasurementType,
        MeasurementTypes::eDirection_MeasurementType,
        MeasurementTypes::eDistance_MeasurementType,
        MeasurementTypes::eTemperature_MeasurementType,
        MeasurementTypes::eUnknown_MeasurementType}) {
    MeasurementConfig in = sampleConfig();
    in.setMeasurementType(type);
    CHECK(codec::fromProto(codec::toProto(in))->getMeasurementType() == type);
  }
}

TEST_CASE("a known sensor type id survives, an unknown one is dropped") {
  const auto known = SensorTypeRegistry::all();
  REQUIRE_FALSE(known.empty());

  MeasurementConfig gated = sampleConfig();
  gated.setSensorType(known.front());
  const auto back = codec::fromProto(codec::toProto(gated));
  REQUIRE(back.has_value());
  CHECK(back->getSensorType().id() == known.front().id());

  oipb::MeasurementConfig message = codec::toProto(sampleConfig());
  message.set_sensor_type("noSuchSensorType");
  const auto unknown = codec::fromProto(message);
  REQUIRE(unknown.has_value());  // the project still loads
  CHECK_FALSE(unknown->getSensorType().isValid());
}

TEST_CASE("a measurement config without a name is rejected") {
  CHECK_FALSE(codec::fromProto(oipb::MeasurementConfig{}).has_value());
}

TEST_CASE("fnv1a64 matches the published test vectors") {
  CHECK(codec::fnv1a64("") == 0xcbf29ce484222325ULL);
  CHECK(codec::fnv1a64("a") == 0xaf63dc4c8601ec8cULL);
  CHECK(codec::fnv1a64("foobar") == 0x85944171f73967e8ULL);
  CHECK(codec::fnv1a64("foobar") != codec::fnv1a64("foobaz"));
}

TEST_CASE("the Project header round trips, including the timestamp") {
  oipb::Project in;
  in.set_format_version(1);
  in.set_name("Hall 3");
  in.set_app_version("1.2.3-4-gabcdef");
  in.set_next_id(4711);
  *in.mutable_saved_at() =
      google::protobuf::util::TimeUtil::SecondsToTimestamp(1730000000);
  *in.add_measurement_configs() = codec::toProto(sampleConfig());

  std::string bytes;
  REQUIRE(in.SerializeToString(&bytes));
  oipb::Project out;
  REQUIRE(out.ParseFromString(bytes));
  CHECK(out.name() == "Hall 3");
  CHECK(out.next_id() == 4711);
  CHECK(out.saved_at().seconds() == 1730000000);
  REQUIRE(out.measurement_configs_size() == 1);
  CHECK(out.measurement_configs(0).name() == "StdScan");

  // serialising the same message twice gives identical bytes, which is what
  // the digest relies on
  std::string again;
  REQUIRE(out.SerializeToString(&again));
  CHECK(codec::fnv1a64(again) == codec::fnv1a64(bytes));
}

// ---------------------------------------------------------------------------
// Entities: build a message by hand, create the object from it, convert the
// object back, and require the two messages to be identical. That checks
// every field in both directions at once.
// ---------------------------------------------------------------------------

namespace {

using google::protobuf::util::MessageDifferencer;

void useArmadillo() {
  math::ChooseLALib::setLinearAlgebra(
      math::ChooseLALib::LinearAlgebraLibrary::Armadillo);
}

oipb::Vec3 v3(double x, double y, double z) {
  oipb::Vec3 v;
  v.set_x(x);
  v.set_y(y);
  v.set_z(z);
  return v;
}

oipb::Reading readingBase(int id) {
  oipb::Reading m;
  m.set_id(id);
  m.mutable_measured_at()->set_seconds(1730000000);
  m.mutable_measured_at()->set_nanos(123456789);  // sub-second survives
  m.set_imported(true);
  m.set_face(oipb::SENSOR_FACE_BACK);
  m.set_sensor_config_name("Tracker A");
  m.mutable_measurement_config()->set_name("StdScan");
  return m;
}

void requireReadingRoundTrip(const oipb::Reading& in) {
  const std::unique_ptr<Reading> reading = codec::createReading(in);
  REQUIRE(reading != nullptr);
  const oipb::Reading out = codec::toProto(*reading);
  INFO(in.ShortDebugString());
  INFO(out.ShortDebugString());
  CHECK(MessageDifferencer::Equals(in, out));
}

}  // namespace

TEST_CASE("every reading variant round trips, sigmas included") {
  useArmadillo();

  oipb::Reading polar = readingBase(11);
  polar.mutable_polar()->set_azimuth(1.5);
  polar.mutable_polar()->set_zenith(1.6);
  polar.mutable_polar()->set_distance(12.25);
  polar.mutable_polar()->set_sigma_azimuth(0.001);
  polar.mutable_polar()->set_sigma_zenith(0.002);
  polar.mutable_polar()->set_sigma_distance(0.0005);
  requireReadingRoundTrip(polar);

  oipb::Reading cartesian = readingBase(12);
  *cartesian.mutable_cartesian()->mutable_xyz() = v3(1, 2, 3);
  *cartesian.mutable_cartesian()->mutable_sigma_xyz() = v3(.1, .2, .3);
  requireReadingRoundTrip(cartesian);

  oipb::Reading cartesian6d = readingBase(13);
  *cartesian6d.mutable_cartesian_6d()->mutable_xyz() = v3(1, 2, 3);
  *cartesian6d.mutable_cartesian_6d()->mutable_ijk() = v3(0, 0, 1);
  *cartesian6d.mutable_cartesian_6d()->mutable_sigma_xyz() = v3(.1, .2, .3);
  requireReadingRoundTrip(cartesian6d);

  oipb::Reading direction = readingBase(14);
  direction.mutable_direction()->set_azimuth(2.0);
  direction.mutable_direction()->set_zenith(1.0);
  direction.mutable_direction()->set_sigma_azimuth(0.01);
  direction.mutable_direction()->set_sigma_zenith(0.02);
  requireReadingRoundTrip(direction);

  oipb::Reading distance = readingBase(15);
  distance.mutable_distance()->set_distance(7.5);
  distance.mutable_distance()->set_sigma_distance(0.003);
  requireReadingRoundTrip(distance);

  oipb::Reading temperature = readingBase(16);
  temperature.mutable_temperature()->set_temperature(21.5);
  temperature.mutable_temperature()->set_sigma_temperature(0.1);
  requireReadingRoundTrip(temperature);

  oipb::Reading level = readingBase(17);
  level.mutable_level()->set_i(0.001);
  level.mutable_level()->set_j(-0.002);
  level.mutable_level()->set_k(0.999);
  level.mutable_level()->set_sigma_i(1e-5);
  level.mutable_level()->set_sigma_j(2e-5);
  level.mutable_level()->set_sigma_k(3e-5);
  requireReadingRoundTrip(level);

  oipb::Reading undefined = readingBase(18);
  (*undefined.mutable_undefined()->mutable_values())["pressure"] = 1013.25;
  (*undefined.mutable_undefined()->mutable_sigma_values())["pressure"] = 0.5;
  requireReadingRoundTrip(undefined);
}

TEST_CASE("a reading keeps its time to the nanosecond and its face") {
  useArmadillo();
  oipb::Reading m = readingBase(21);
  m.mutable_distance()->set_distance(1.0);
  const std::unique_ptr<Reading> reading = codec::createReading(m);
  REQUIRE(reading != nullptr);
  CHECK(reading->getFace() == SensorFaces::eBackSide);
  CHECK(reading->isImported());
  CHECK(std::chrono::duration_cast<std::chrono::nanoseconds>(
            reading->getMeasuredAt().time_since_epoch())
            .count() == 1730000000123456789LL);
}

TEST_CASE("a reading with no measurement is rejected") {
  useArmadillo();
  CHECK(codec::createReading(readingBase(30)) == nullptr);
}

TEST_CASE("an observation round trips, including the original coordinates") {
  useArmadillo();
  oipb::Observation in;
  in.set_id(40);
  in.set_valid(true);
  in.set_solved(true);
  in.set_dummy_point(false);
  in.set_has_direction(true);
  *in.mutable_xyz() = v3(1, 2, 3);
  *in.mutable_original_xyz() = v3(10, 20, 30);  // lost by the old XML format
  *in.mutable_sigma_xyz() = v3(.1, .1, .1);
  *in.mutable_original_sigma_xyz() = v3(.2, .2, .2);
  *in.mutable_ijk() = v3(0, 0, 1);
  *in.mutable_original_ijk() = v3(1, 0, 0);
  *in.mutable_sigma_ijk() = v3(.01, .01, .01);
  *in.mutable_original_sigma_ijk() = v3(.02, .02, .02);
  *in.mutable_reading() = readingBase(41);
  in.mutable_reading()->mutable_distance()->set_distance(3.0);
  // station / measured_target_geometry are references, resolved by the linker

  const std::unique_ptr<Observation> observation = codec::createObservation(in);
  REQUIRE(observation != nullptr);
  CHECK(MessageDifferencer::Equals(in, codec::toProto(*observation)));
  REQUIRE_FALSE(observation->getReading().isNull());
  CHECK(observation->getReading()->getObservation().data() ==
        observation.get());
}

TEST_CASE("an observation without its reading cannot be created") {
  useArmadillo();
  oipb::Observation in;
  in.set_id(50);
  CHECK(codec::createObservation(in) == nullptr);
}

TEST_CASE("transformation parameters round trip, matrix included") {
  useArmadillo();
  oipb::TrafoParam in;
  auto* f = in.mutable_feature();
  f->set_id(60);
  f->set_name("Bundle01_1");
  f->set_group("Trafos");
  f->set_comment("datum");
  f->set_solved(true);
  *in.mutable_translation() = v3(1.5, -2.5, 3.5);
  *in.mutable_rotation() = v3(0.1, 0.2, 0.3);
  *in.mutable_scale() = v3(1.0, 1.0, 1.0001);
  for (int i = 0; i < 16; ++i) {
    in.add_homogen_matrix(i % 5 == 0 ? 1.0 : 0.01 * i);
  }
  in.set_datum_trafo(true);
  in.set_used(true);
  // from/to are references, resolved by the linker

  const auto trafo = codec::createTrafoParam(in);
  REQUIRE(trafo);
  CHECK(MessageDifferencer::Equals(in, codec::toProto(*trafo)));
}

TEST_CASE("a coordinate system round trips, bundle template included") {
  useArmadillo();
  oipb::CoordinateSystem in;
  auto* f = in.mutable_feature();
  f->set_id(70);
  f->set_name("Bundle01");
  f->set_solved(true);
  in.set_active_system(true);
  in.set_bundle_system(true);
  *in.mutable_origin() = v3(1, 2, 3);
  *in.mutable_x_axis() = v3(1, 0, 0);
  *in.mutable_y_axis() = v3(0, 1, 0);
  *in.mutable_z_axis() = v3(0, 0, 1);
  *in.mutable_expansion_origin() = v3(4, 5, 6);
  auto* bt = in.mutable_bundle_template();
  bt->set_name("Standard");
  bt->set_level_station("STATION01");
  bt->set_max_iterations("25");
  bt->add_parameters("tx");
  bt->add_parameters("ry");  // the old XML format lost these
  bt->mutable_plugin()->set_name("BundleAdjustment");
  bt->mutable_plugin()->set_plugin("OpenIndy Default Plugin");
  // trafo_params / observations / nominal_geometries are references

  const auto system = codec::createCoordinateSystem(in);
  REQUIRE(system);
  CHECK(MessageDifferencer::Equals(in, codec::toProto(*system)));
}

TEST_CASE("a station round trips its own state") {
  useArmadillo();
  oipb::Station in;
  auto* f = in.mutable_feature();
  f->set_id(80);
  f->set_name("STATION01");
  f->set_solved(true);
  f->set_active(true);
  in.set_active_station(true);

  const auto station = codec::createStation(in);
  REQUIRE(station);
  const oipb::Station out = codec::toProto(*station);
  CHECK(MessageDifferencer::Equals(in.feature(), out.feature()));
  CHECK(out.active_station());
}

namespace {

// The state every geometry shares, with distinctive values so that a field
// swapped or dropped by a converter shows up. References are left out: the
// linking pass owns them.
oipb::Geometry geometryBase(int id) {
  oipb::Geometry m;
  auto* f = m.mutable_common();
  f->set_id(id);
  f->set_name("G" + std::to_string(id));
  f->set_group("walls");
  f->set_comment("measured on site");
  f->set_solved(true);
  f->set_active(true);
  m.set_is_common(true);
  *m.mutable_position() = v3(1.25, -2.5, 3.75);
  *m.mutable_direction() = v3(0.0, 0.6, 0.8);
  m.set_radius(4.5);
  auto* st = m.mutable_statistic();
  st->set_valid(true);
  st->set_s0_apriori(1.0);
  st->set_s0_aposteriori(1.1);
  st->set_stdev(0.002);
  st->set_form_error(0.0005);
  m.mutable_measurement_config()->set_name("FastPoint");
  return m;
}

}  // namespace

TEST_CASE("every geometry type round trips, with its type-specific state") {
  useArmadillo();

  const std::vector<
      std::pair<const char*, std::function<void(oipb::Geometry&)>>>
      shapes = {
          {"circle", [](auto& m) { m.mutable_circle(); }},
          {"cone", [](auto& m) { m.mutable_cone()->set_aperture(0.35); }},
          {"cylinder", [](auto& m) { m.mutable_cylinder(); }},
          {"ellipse",
           [](auto& m) {
             auto* e = m.mutable_ellipse();
             e->set_a(7.0);
             e->set_b(3.0);
             *e->mutable_semi_major_axis() = v3(1.0, 0.0, 0.0);
           }},
          {"ellipsoid",
           [](auto& m) {
             m.mutable_ellipsoid()->set_a(5.0);
             m.mutable_ellipsoid()->set_b(2.0);
           }},
          {"hyperboloid",
           [](auto& m) {
             m.mutable_hyperboloid()->set_a(1.5);
             m.mutable_hyperboloid()->set_c(2.5);
           }},
          {"line", [](auto& m) { m.mutable_line(); }},
          {"nurbs", [](auto& m) { m.mutable_nurbs(); }},
          {"paraboloid", [](auto& m) { m.mutable_paraboloid()->set_a(0.75); }},
          {"plane",
           [](auto& m) { m.mutable_plane()->set_distance_from_origin(12.5); }},
          {"point",
           [](auto& m) {
             auto* i = m.mutable_point()->add_intents();
             i->set_feature_id(40);
             i->set_role("wall");
             i = m.mutable_point()->add_intents();
             i->set_feature_id(41);
             i->set_role("registration");
           }},
          {"point cloud",
           [](auto& m) {
             auto* c = m.mutable_point_cloud();
             for (float x : {1.f, 2.f, 3.f, 4.f, 5.f, 6.f}) {
               c->add_coordinates(x);
             }
             c->add_used(true);
             c->add_used(false);
             for (float x : {1.f, 2.f, 3.f}) c->add_bbox_min(x);
             for (float x : {4.f, 5.f, 6.f}) c->add_bbox_max(x);
           }},
          {"scalar angle",
           [](auto& m) { m.mutable_scalar_angle()->set_angle(1.2); }},
          {"scalar distance",
           [](auto& m) { m.mutable_scalar_distance()->set_distance(8.8); }},
          {"scalar measurement series",
           [](auto& m) {
             m.mutable_scalar_measurement_series()->set_series_value(3.3);
           }},
          {"scalar temperature",
           [](auto& m) {
             m.mutable_scalar_temperature()->set_temperature(21.5);
           }},
          {"slotted hole",
           [](auto& m) {
             auto* h = m.mutable_slotted_hole();
             *h->mutable_center() = v3(1, 2, 3);
             *h->mutable_normal() = v3(0, 0, 1);
             h->set_radius(2.0);
             h->set_length(9.0);
             *h->mutable_hole_axis() = v3(1, 0, 0);
             *h->mutable_circle_center_a() = v3(-3.5, 2, 3);
             *h->mutable_circle_center_b() = v3(5.5, 2, 3);
           }},
          {"sphere", [](auto& m) { m.mutable_sphere(); }},
          {"torus", [](auto& m) { m.mutable_torus()->set_radius_b(0.5); }},
  };
  REQUIRE(shapes.size() == 19);

  int id = 100;
  for (const auto& [name, fill] : shapes) {
    INFO(name);
    oipb::Geometry in = geometryBase(id++);
    fill(in);

    const auto geometry = codec::createGeometry(in);
    REQUIRE(geometry);
    std::string differences;
    MessageDifferencer differencer;
    differencer.ReportDifferencesToString(&differences);
    const bool same = differencer.Compare(in, codec::toProto(*geometry));
    INFO(differences);
    CHECK(same);
  }
}

TEST_CASE("a geometry message without a shape cannot be created") {
  useArmadillo();
  CHECK(codec::createGeometry(geometryBase(1)) == nullptr);
}

TEST_CASE("a geometry's references are not applied at creation") {
  useArmadillo();
  oipb::Geometry in = geometryBase(7);
  in.mutable_point();
  in.add_observations(31);
  in.set_nominal_system(5);
  in.set_realises(6);
  in.set_owner_feature(8);

  const auto geometry = codec::createGeometry(in);
  REQUIRE(geometry);
  const oipb::Geometry out = codec::toProto(*geometry);
  // left for the linking pass, which reads them from the message
  CHECK(out.observations_size() == 0);
  CHECK(out.nominal_system() == 0);
  CHECK_FALSE(geometry->getRealises().has_value());
  CHECK_FALSE(geometry->getOwnerFeature().has_value());
}

TEST_CASE("groups round trip, constraint and kind included") {
  useArmadillo();

  oipb::Group plain;
  plain.mutable_common()->set_id(200);
  plain.mutable_common()->set_name("walls");
  plain.set_constraint(oipb::FEATURE_KIND_PLANE);
  plain.add_members(1);  // references are left for the linking pass

  std::unique_ptr<Group> group = codec::createGroup(plain);
  REQUIRE(group);
  CHECK(dynamic_cast<PointGroup*>(group.get()) == nullptr);
  CHECK(group->getConstraint() == FeatureTypes::ePlaneFeature);
  oipb::Group out = codec::toProto(*group);
  CHECK(out.constraint() == oipb::FEATURE_KIND_PLANE);
  CHECK_FALSE(out.point_group());
  CHECK(out.common().name() == "walls");

  oipb::Group points;
  points.mutable_common()->set_id(201);
  points.mutable_common()->set_name("targets");
  points.set_constraint(oipb::FEATURE_KIND_POINT);
  points.set_point_group(true);

  group = codec::createGroup(points);
  REQUIRE(group);
  CHECK(dynamic_cast<PointGroup*>(group.get()) != nullptr);
  out = codec::toProto(*group);
  CHECK(out.point_group());
  CHECK(out.constraint() == oipb::FEATURE_KIND_POINT);

  oipb::Group unconstrained;
  unconstrained.mutable_common()->set_id(202);
  group = codec::createGroup(unconstrained);
  REQUIRE(group);
  CHECK_FALSE(group->getConstraint().has_value());
  CHECK(codec::toProto(*group).constraint() == oipb::FEATURE_KIND_UNSPECIFIED);
}

namespace {

// A function that does nothing but expose what the codec has to save: two
// input positions, parameters and a pipeline slot. (No Q_OBJECT: it adds no
// signals or slots.)
class TestFit : public FitFunction, public Producer<Point> {
 public:
  TestFit() {
    metaData.name = "TestFit";
    metaData.pluginName = "Test Plugin";
    NeededElement points;
    points.typeOfElement = ElementTypes::ePointElement;
    points.infinite = true;
    points.roleName = "points";
    neededElements.push_back(points);
    NeededElement plane;
    plane.typeOfElement = ElementTypes::ePlaneElement;
    plane.roleName = "plane";
    neededElements.push_back(plane);
    applicableFor.push_back(FeatureTypes::ePointFeature);
    integerParameters.insert_or_assign("iterations", 5);
    doubleParameters.insert_or_assign("tolerance", 0.001);
    stringParameters.emplace("method", "robust");
  }

  std::expected<Point, std::string> exec() override {
    return std::unexpected(std::string("not used"));
  }

  void declare(int position, std::vector<PipelineStage> stages) {
    for (const PipelineStage& stage : stages) {
      neededElements[position].pipeline.push_back(stage);
    }
  }
  void addInput(int position, InputElement input) {
    inputElements[position].push_back(input);
  }
  void setUserParameters() {
    scalarInputParams.intParameter.insert_or_assign("iterations", 12);
    scalarInputParams.doubleParameter.insert_or_assign("tolerance", 0.25);
    scalarInputParams.stringParameter.insert_or_assign("method", "plain");
    scalarInputParams.isValid = true;
  }
  void fix(GeometryParameters parameter, double value) {
    FixedParameter fixed(parameter);
    fixed.value = value;
    fixedParameters.push_back(fixed);
  }
};

}  // namespace

TEST_CASE("a function record keeps inputs, pipelines and parameters") {
  useArmadillo();

  TestFit original;
  original.setUserParameters();
  original.fix(GeometryParameters::eUnknownRadiusA, 2.5);

  InputElement a(31);
  a.typeOfElement = ElementTypes::ePointElement;
  a.label = "corner";
  a.shouldBeUsed = false;
  a.isUsed = false;
  a.ignoredDestinationParams = {GeometryParameters::eUnknownZ};
  a.ignoredTargetParams = {GeometryParameters::eUnknownRadiusA,
                           GeometryParameters::eUnknownPrimaryK};
  original.addInput(0, a);
  InputElement b(32);
  b.typeOfElement = ElementTypes::ePointElement;
  original.addInput(0, b);
  InputElement c(40);
  c.typeOfElement = ElementTypes::ePlaneElement;
  original.addInput(1, c);

  original.declare(
      0, {PipelineStage::explicitList({31, 32, 33}),
          PipelineStage::exclude({PipelineExclusion(33, "behind the wall"),
                                  PipelineExclusion(34, "outlier")}),
          PipelineStage::residualOutlierRejection(2.5)});

  const oipb::FunctionRecord record = codec::toProto(original);
  CHECK(record.plugin().name() == "TestFit");
  CHECK(record.plugin().plugin() == "Test Plugin");
  CHECK(record.inputs_size() == 3);
  CHECK(record.pipelines_size() == 1);

  // what the host does on load: make the function from the plugin reference,
  // then apply the record
  TestFit restored;
  codec::applyRecord(record, restored);
  CHECK(MessageDifferencer::Equals(record, codec::toProto(restored)));

  // and the bytes survive a trip over the wire
  std::string bytes;
  REQUIRE(record.SerializeToString(&bytes));
  oipb::FunctionRecord parsed;
  REQUIRE(parsed.ParseFromString(bytes));
  CHECK(MessageDifferencer::Equals(record, parsed));
}

TEST_CASE("an unedited function saves its default parameters") {
  useArmadillo();
  TestFit untouched;
  const oipb::FunctionRecord record = codec::toProto(untouched);
  REQUIRE(record.int_parameters_size() == 1);
  CHECK(record.int_parameters(0).name() == "iterations");
  CHECK(record.int_parameters(0).value() == 5);
  REQUIRE(record.double_parameters_size() == 1);
  CHECK(record.double_parameters(0).value() == 0.001);
  REQUIRE(record.string_parameters_size() == 1);
  CHECK(record.string_parameters(0).value() == "robust");
}

TEST_CASE(
    "pipelines for positions the function no longer declares are dropped") {
  useArmadillo();
  oipb::FunctionRecord record;
  record.mutable_plugin()->set_name("TestFit");
  auto* pipeline = record.add_pipelines();
  pipeline->set_position(7);  // TestFit declares positions 0 and 1
  pipeline->add_stages()->set_kind(oipb::PIPELINE_STAGE_KIND_EXCLUDE);
  auto* kept = record.add_pipelines();
  kept->set_position(1);
  kept->add_stages()->set_kind(
      oipb::PIPELINE_STAGE_KIND_RESIDUAL_OUTLIER_REJECTION);
  kept->mutable_stages(0)->set_sigma_multiplier(4.0);

  TestFit restored;
  codec::applyRecord(record, restored);
  const oipb::FunctionRecord out = codec::toProto(restored);
  REQUIRE(out.pipelines_size() == 1);
  CHECK(out.pipelines(0).position() == 1);
  CHECK(out.pipelines(0).stages(0).sigma_multiplier() == 4.0);
}

TEST_CASE("a feature's functions are part of its common record") {
  useArmadillo();
  Point point;
  auto owned_fit = std::make_unique<TestFit>();
  owned_fit->setUserParameters();
  point.addFunction(std::move(owned_fit));

  const oipb::Geometry out = codec::toProto(point);
  REQUIRE(out.common().functions_size() == 1);
  CHECK(out.common().functions(0).plugin().name() == "TestFit");
  CHECK(out.common().functions(0).int_parameters(0).value() == 12);
}

namespace {

class TestBundle : public BundleAdjustment {
 public:
  TestBundle() {
    metaData.name = "TestBundle";
    metaData.pluginName = "Test Plugin";
  }
};

}  // namespace

TEST_CASE("a bundle adjustment record round trips, base station included") {
  useArmadillo();

  oipb::BundleAdjustmentRecord in;
  in.mutable_plugin()->set_name("TestBundle");
  in.mutable_plugin()->set_plugin("Test Plugin");

  auto* a = in.add_stations();
  a->set_id(60);
  a->set_tx(true);
  a->set_ty(true);
  a->set_tz(false);
  a->set_rx(false);
  a->set_ry(true);
  a->set_rz(true);
  a->set_m(false);
  auto* ag = a->add_geometries();
  ag->set_id(90);
  auto* p = ag->add_parameters();
  p->set_parameter(oipb::GEOMETRY_PARAMETER_UNKNOWN_X);
  p->set_value(1.5);
  p = ag->add_parameters();
  p->set_parameter(oipb::GEOMETRY_PARAMETER_UNKNOWN_RADIUS_A);
  p->set_value(0.25);

  auto* b = in.add_stations();
  b->set_id(61);
  b->set_tx(true);
  b->set_ty(true);
  b->set_tz(true);
  b->set_rx(true);
  b->set_ry(true);
  b->set_rz(true);
  b->set_m(true);
  in.set_base_station(61);

  auto* g = in.add_geometries();
  g->set_id(91);
  auto* gp = g->add_parameters();
  gp->set_parameter(oipb::GEOMETRY_PARAMETER_UNKNOWN_Z);
  gp->set_value(-3.0);

  auto* t = in.add_transformations();
  t->set_id(70);
  auto* tp = t->add_parameters();
  tp->set_parameter(oipb::TRAFO_PARAMETER_ROTATION_Z);
  tp->set_value(0.01);
  tp = t->add_parameters();
  tp->set_parameter(oipb::TRAFO_PARAMETER_SCALE_X);
  tp->set_value(1.0001);

  TestBundle bundle;
  codec::applyRecord(in, bundle);

  std::string differences;
  MessageDifferencer differencer;
  differencer.ReportDifferencesToString(&differences);
  const bool same = differencer.Compare(in, codec::toProto(bundle));
  INFO(differences);
  CHECK(same);
}

TEST_CASE("a bundle system saves its attached bundle adjustment") {
  useArmadillo();
  CoordinateSystem system(true);
  system.setBundlePlugin(std::make_unique<TestBundle>());

  const oipb::CoordinateSystem out = codec::toProto(system);
  REQUIRE(out.has_bundle_adjustment());
  CHECK(out.bundle_adjustment().plugin().name() == "TestBundle");

  CoordinateSystem plain(false);
  CHECK_FALSE(codec::toProto(plain).has_bundle_adjustment());
}

// ---------------------------------------------------------------------------
// Whole projects
// ---------------------------------------------------------------------------

namespace {

class TestHost : public codec::ProjectHost {
 public:
  bool pluginsInstalled = true;

  std::unique_ptr<Function> createFunction(const std::string& plugin,
                                           const std::string& name) override {
    if (pluginsInstalled && plugin == "Test Plugin" && name == "TestFit") {
      return std::make_unique<TestFit>();
    }
    return nullptr;
  }
  std::unique_ptr<BundleAdjustment> createBundleAdjustment(
      const std::string& plugin, const std::string& name) override {
    if (pluginsInstalled && plugin == "Test Plugin" && name == "TestBundle") {
      return std::make_unique<TestBundle>();
    }
    return nullptr;
  }
  std::unique_ptr<Sensor> createSensor(const std::string&,
                                       const std::string&) override {
    return nullptr;
  }
  MeasurementConfig measurementConfig(
      const MeasurementConfigKey&) const override {
    return {};
  }
};

struct World {
  oi::Guarded<OiJob> job;
  oi::Guarded<Station> station;
  oi::Guarded<CoordinateSystem> part;
  oi::Guarded<Point> a;
  oi::Guarded<Point> b;
  oi::Guarded<Point> c;
  oi::Guarded<Plane> plane;
  oi::Guarded<Group> group;
  oi::Guarded<TrafoParam> trafo;
};

// The job takes over a feature the caller built.
template <class T>
bool addToJob(OiJob& job, const oi::Guarded<T>& feature,
              void (FeatureWrapper::*set)(const oi::Guarded<T>&)) {
  static_cast<void>(set);
  // the test built the feature with new; the job takes it over
  return !job.addFeature(std::unique_ptr<Feature>(feature.data())).isNull();
}

template <class T>
bool addToJob(OiJob& job, std::unique_ptr<T> feature) {
  return !job.addFeature(std::move(feature)).isNull();
}

oi::Guarded<Point> addPoint(OiJob& job, const std::string& name, double x) {
  auto owned_point = std::make_unique<Point>(Position(x, 2.0, 3.0));
  const oi::Guarded<Point> point(owned_point.get());
  point->setFeatureName(name);
  point->setIsSolved(true);
  REQUIRE(addToJob<Point>(job, std::move(owned_point)));
  return point;
}

oi::Guarded<FeatureWrapper> wrapperOf(OiJob& job, int id) {
  for (const auto& wrapper : job.getFeaturesList()) {
    if (!wrapper.isNull() && wrapper->getFeature()->getId() == id) {
      return wrapper;
    }
  }
  return {};
}

World buildWorld() {
  useArmadillo();
  World w;
  w.job = new OiJob();

  w.station = new Station();
  w.station->setFeatureName("STATION01");
  REQUIRE(addToJob<Station>(*w.job, w.station, &FeatureWrapper::setStation));
  w.part = new CoordinateSystem();
  w.part->setFeatureName("PART");
  REQUIRE(addToJob<CoordinateSystem>(*w.job, w.part,
                                     &FeatureWrapper::setCoordinateSystem));
  w.station->setActiveStationState(true);
  w.station->getCoordinateSystem()->setActiveCoordinateSystemState(true);

  auto owned_bundle = std::make_unique<CoordinateSystem>(true);

  const oi::Guarded<CoordinateSystem> bundle(owned_bundle.get());
  bundle->setFeatureName("Bundle01");
  bundle->setBundlePlugin(std::make_unique<TestBundle>());
  REQUIRE(addToJob<CoordinateSystem>(*w.job, std::move(owned_bundle)));

  w.a = addPoint(*w.job, "A", 1.0);
  w.b = addPoint(*w.job, "B", 2.0);

  // c is computed by a function from a and b
  w.c = new Point();
  w.c->setFeatureName("C");
  auto owned_fit = std::make_unique<TestFit>();
  TestFit* const fit = owned_fit.get();
  fit->setUserParameters();
  InputElement inA(w.a->getId());
  inA.typeOfElement = ElementTypes::ePointElement;
  inA.point = w.a;
  inA.geometry = w.a;
  fit->addInput(0, inA);
  InputElement inB(w.b->getId());
  inB.typeOfElement = ElementTypes::ePointElement;
  inB.point = w.b;
  inB.geometry = w.b;
  fit->addInput(0, inB);
  w.c->addFunction(std::move(owned_fit));
  REQUIRE(addToJob<Point>(*w.job, w.c, &FeatureWrapper::setPoint));

  w.plane = new Plane();
  w.plane->setFeatureName("P1");
  REQUIRE(addToJob<Plane>(*w.job, w.plane, &FeatureWrapper::setPlane));

  w.group = new Group();
  w.group->setFeatureName("Both");
  REQUIRE(w.group->addMember(wrapperOf(*w.job, w.a->getId())));
  REQUIRE(w.group->addMember(wrapperOf(*w.job, w.plane->getId())));
  REQUIRE(addToJob<Group>(*w.job, w.group, &FeatureWrapper::setGroup));

  w.trafo = new TrafoParam();
  w.trafo->setFeatureName("T1");
  w.trafo->setCoordinateSystems(w.station->getCoordinateSystem(), w.part);
  REQUIRE(
      addToJob<TrafoParam>(*w.job, w.trafo, &FeatureWrapper::setTrafoParam));
  // the systems register a transformation by id, which it only has now
  w.station->getCoordinateSystem()->addTransformationParameter(w.trafo);
  w.part->addTransformationParameter(w.trafo);
  return w;
}

oipb::Project parse(const std::string& bytes) {
  oipb::Project project;
  REQUIRE(project.ParseFromString(bytes));
  return project;
}

// the part of a saved project that is not the time it was saved
oipb::Project withoutTime(const std::string& bytes) {
  oipb::Project project = parse(bytes);
  project.clear_saved_at();
  return project;
}

}  // namespace

TEST_CASE("a project saves, loads and saves again to the same content") {
  World w = buildWorld();
  TestHost host;

  const std::string first = codec::encodeProject(*w.job, host, "1.2.3");
  codec::DecodeResult loaded = codec::decodeProject(first, host);
  INFO(loaded.error);
  REQUIRE(loaded.ok());
  const std::string second = codec::encodeProject(*loaded.job, host, "1.2.3");

  std::string differences;
  MessageDifferencer differencer;
  differencer.ReportDifferencesToString(&differences);
  const bool same =
      differencer.Compare(withoutTime(first), withoutTime(second));
  INFO(differences);
  CHECK(same);
  CHECK(loaded.job->getDigest() == w.job->getDigest());

  delete w.job.data();
}

TEST_CASE("loading links the references the file only holds as ids") {
  World w = buildWorld();
  TestHost host;
  codec::DecodeResult loaded =
      codec::decodeProject(codec::encodeProject(*w.job, host, "x"), host);
  REQUIRE(loaded.ok());
  OiJob& job = *loaded.job;

  CHECK(job.getStationsList().size() == 1);
  const oi::Guarded<Station> station = job.getActiveStation();
  REQUIRE(station);
  CHECK(station->getFeatureName() == "STATION01");
  CHECK_FALSE(station->getPosition().isNull());
  REQUIRE_FALSE(station->getCoordinateSystem().isNull());
  CHECK(station->getCoordinateSystem()->getIsStationSystem());
  CHECK(station->getCoordinateSystem()->getStation() == station);
  CHECK(job.getActiveCoordinateSystem() == station->getCoordinateSystem());

  // the function came back through the host, with its inputs pointing at the
  // loaded points
  const auto c = job.getFeaturesByName("C");
  REQUIRE(c.size() == 1);
  const auto functions = c.front()->getFeature()->getFunctions();
  REQUIRE(functions.size() == 1);
  const auto& inputs = functions.front()->getInputElements();
  REQUIRE(inputs.contains(0));
  REQUIRE(oi::value(inputs, 0).size() == 2);
  const auto a = job.getFeaturesByName("A");
  REQUIRE(a.size() == 1);
  CHECK(oi::value(inputs, 0).at(0).point == a.front()->getPoint());
  CHECK(oi::value(inputs, 0).at(0).geometry == a.front()->getGeometry());

  // the group's members are the loaded features, not the saved ones
  const auto group = job.getFeaturesByName("Both");
  REQUIRE(group.size() == 1);
  CHECK(group.front()->getGroup()->getMembers().size() == 2);
  CHECK(oi::contains(group.front()->getGroup()->getMembers(), a.front()));

  // the transformation knows its two systems
  const auto trafo = job.getFeaturesByName("T1");
  REQUIRE(trafo.size() == 1);
  CHECK(trafo.front()->getTrafoParam()->getStartSystem() ==
        station->getCoordinateSystem());

  // the bundle adjustment is attached to its system and knows it
  const auto bundle = job.getFeaturesByName("Bundle01");
  REQUIRE(bundle.size() == 1);
  REQUIRE_FALSE(
      bundle.front()->getCoordinateSystem()->getBundlePlugin().isNull());

  // ids carry on from where the saved job left off
  CHECK(job.generateUniqueId() == w.job->generateUniqueId());

  delete w.job.data();
}

TEST_CASE("a project that needs a plugin that is not installed does not load") {
  World w = buildWorld();
  TestHost host;
  const std::string bytes = codec::encodeProject(*w.job, host, "x");

  host.pluginsInstalled = false;
  const codec::DecodeResult loaded = codec::decodeProject(bytes, host);
  CHECK_FALSE(loaded.ok());
  CHECK(loaded.job == nullptr);
  CHECK(loaded.missingPlugins ==
        std::vector<std::string>{"Test Plugin/TestBundle",
                                 "Test Plugin/TestFit"});
  CHECK_FALSE(loaded.error.empty());

  delete w.job.data();
}

TEST_CASE("a load that fails midway leaves nothing behind") {
  // run under AddressSanitizer with detect_leaks=1: everything the decoder
  // built (features, station position and system, observations and their
  // readings, functions, bundle adjustments) must be freed when the project
  // then fails on a plugin that is not installed
  World w = buildWorld();
  std::vector<std::unique_ptr<Reading>> readings;
  for (int i = 0; i < 3; ++i) {
    ReadingCartesian r;
    r.isValid = true;
    r.xyz.setAt(0, 1.0 + i);
    r.xyz.setAt(1, 2.0);
    r.xyz.setAt(2, 3.0);
    readings.push_back(std::make_unique<Reading>(r));
  }
  w.job->addMeasurementResults(w.a->getId(), std::move(readings));
  TestHost host;
  const std::string bytes = codec::encodeProject(*w.job, host, "x");
  REQUIRE(parse(bytes).observations_size() == 3);

  host.pluginsInstalled = false;
  const codec::DecodeResult failed = codec::decodeProject(bytes, host);
  CHECK_FALSE(failed.ok());
  CHECK(failed.job == nullptr);
  CHECK_FALSE(failed.missingPlugins.empty());

  // and a load that works with the same observations frees them with the job
  host.pluginsInstalled = true;
  const codec::DecodeResult loaded = codec::decodeProject(bytes, host);
  REQUIRE(loaded.ok());
  CHECK(
      parse(codec::encodeProject(*loaded.job, host, "x")).observations_size() ==
      3);

  delete w.job.data();
}

TEST_CASE("a file that is not a usable project is refused with a reason") {
  useArmadillo();
  TestHost host;

  SECTION("not protobuf at all") {
    const auto loaded = codec::decodeProject("this is not a project", host);
    CHECK_FALSE(loaded.ok());
    CHECK_FALSE(loaded.error.empty());
  }
  SECTION("a format version this program does not know") {
    World w = buildWorld();
    oipb::Project project = parse(codec::encodeProject(*w.job, host, "x"));
    project.set_format_version(999);
    const auto loaded = codec::decodeProject(project.SerializeAsString(), host);
    CHECK_FALSE(loaded.ok());
    CHECK(loaded.error.find("999") != std::string::npos);
    delete w.job.data();
  }
  SECTION("an active station that is not in the project") {
    World w = buildWorld();
    oipb::Project project = parse(codec::encodeProject(*w.job, host, "x"));
    project.set_active_station(123456);
    const auto loaded = codec::decodeProject(project.SerializeAsString(), host);
    CHECK_FALSE(loaded.ok());
    delete w.job.data();
  }
  SECTION("no stations") {
    World w = buildWorld();
    oipb::Project project = parse(codec::encodeProject(*w.job, host, "x"));
    project.clear_stations();
    const auto loaded = codec::decodeProject(project.SerializeAsString(), host);
    CHECK_FALSE(loaded.ok());
    delete w.job.data();
  }
}

TEST_CASE("the digest follows the content, not the save time or name") {
  World w = buildWorld();
  TestHost host;

  codec::encodeProject(*w.job, host, "1.0");
  const std::string before = w.job->getDigest();
  (void)codec::encodeProject(*w.job, host, "2.0");
  CHECK(w.job->getDigest() == before);

  w.a->setFeatureName("A-renamed");
  (void)codec::encodeProject(*w.job, host, "2.0");
  CHECK(w.job->getDigest() != before);

  delete w.job.data();
}

// ---------------------------------------------------------------------------
// Ownership references
// ---------------------------------------------------------------------------

namespace {

class TestConstruct : public ConstructFunction, public Producer<Point> {
 public:
  TestConstruct() { metaData.name = "TestConstruct"; }
  std::expected<Point, std::string> exec() override { return Point(); }
};

}  // namespace

TEST_CASE("an actual writes the nominal it realises, a nominal writes none") {
  useArmadillo();
  Point nominal(101, Position());
  Point actual(202, Position());
  actual.addFunction(std::make_unique<TestFit>());
  REQUIRE(actual.setRealises(nominal.getFeatureWrapper()->getGeometry()));

  CHECK(codec::toProto(actual).realises() == 101);
  CHECK(codec::toProto(nominal).realises() == 0);
}

TEST_CASE("a geometry writes the feature that owns it, an owner writes none") {
  useArmadillo();
  Point owner(303, Position());
  Point child(404, Position());
  child.addFunction(std::make_unique<TestConstruct>());
  REQUIRE(child.setOwnerFeature(owner.getFeatureWrapper()->getGeometry()));
  child.setIsSolved(true);

  CHECK(codec::toProto(child).owner_feature() == 303);
  CHECK(codec::toProto(owner).owner_feature() == 0);
}

// ---------------------------------------------------------------------------
// Config files (JSON)
// ---------------------------------------------------------------------------

namespace {

SensorConfiguration sampleSensorConfig() {
  SensorConfiguration config;
  config.setName("Tracker \"A\" \xC3\xA4");
  config.setIsSaved(true);
  config.setPluginName("Some Plugin");
  config.setSensorName("Sensor 1");
  config.setTypeOfSensor(SensorTypeRegistry::byId("laserTracker").value());

  ConnectionConfig connection;
  connection.typeOfConnection = ConnectionKind::serial();
  connection.ip = "192.168.0.7";
  connection.port = "700";
  connection.comPort = "/dev/ttyUSB0";
  connection.baudRate = oi::BaudRate::Baud115200;
  connection.dataBits = oi::DataBits::Data7;
  connection.parity = oi::Parity::Odd;
  connection.stopBits = oi::StopBits::OneAndHalf;
  connection.flowControl = oi::FlowControl::Hardware;
  config.setConnectionConfig(connection);

  Accuracy accuracy;
  accuracy.sigmaXyz.setAt(0, 0.1);
  accuracy.sigmaXyz.setAt(1, 0.2);
  accuracy.sigmaXyz.setAt(2, 0.3);
  accuracy.sigmaAzimuth = 1.5e-6;
  accuracy.sigmaZenith = 2.5e-6;
  accuracy.sigmaDistance = 3.5e-5;
  accuracy.sigmaTemp = 0.5;
  accuracy.sigmaI = 0.01;
  accuracy.sigmaJ = 0.02;
  accuracy.sigmaK = 0.03;
  accuracy.sigmaUndefined.insert_or_assign("custom sigma", 7.25);
  accuracy.sigmaUndefined.insert_or_assign("other", -1.0);
  config.setAccuracy(accuracy);

  config.setIntegerParameter({{"count", 3}, {"negative", -42}});
  config.setDoubleParameter({{"gain", 0.125}, {"huge", 1e300}});
  config.setStringParameter({{"probe", "1.5''"}, {"empty", ""}});
  std::multimap<std::string, std::string> ordered;
  for (const std::string& v :
       {std::string("third"), std::string("second"), std::string("first")}) {
    ordered.emplace("probe", v);
  }
  config.setAvailableStringParameter(ordered);
  return config;
}

}  // namespace

TEST_CASE("MeasurementConfig survives a JSON round trip") {
  MeasurementConfig in = sampleConfig();
  in.setSensorType(SensorTypeRegistry::byId("totalStation").value());

  const std::string json = codec::encodeMeasurementConfig(in);
  CHECK(json.find("\"measure_two_sides\": true") != std::string::npos);
  CHECK(json.find("MEASUREMENT_MODE_PRECISE") != std::string::npos);

  const auto out = codec::decodeMeasurementConfig(json);
  REQUIRE(out.has_value());
  CHECK(out->getName() == in.getName());
  CHECK(out->getMeasurementMode() == in.getMeasurementMode());
  CHECK(out->getMeasurementType() == in.getMeasurementType());
  CHECK(out->getMeasureTwoSides());
  CHECK(out->getMaxObservations() == 25);
  CHECK(out->getTimeInterval() == 0.5);
  CHECK(out->getDistanceInterval() == 2.25);
  CHECK(out->getSensorType().id() == "totalStation");
}

TEST_CASE("a default-valued MeasurementConfig prints every field") {
  MeasurementConfig in;
  in.setName("Zero");
  in.setMeasureTwoSides(false);
  in.setMaxObservations(0);
  in.setTimeInterval(0);
  in.setDistanceInterval(0);
  const std::string json = codec::encodeMeasurementConfig(in);
  for (const char* key :
       {"name", "mode", "type", "measure_two_sides", "max_observations",
        "time_interval", "distance_interval"}) {
    INFO(key);
    CHECK(json.find(std::string("\"") + key + "\"") != std::string::npos);
  }
  // not gated: no sensor_type key
  CHECK(json.find("sensor_type") == std::string::npos);
  const auto out = codec::decodeMeasurementConfig(json);
  REQUIRE(out.has_value());
  CHECK_FALSE(out->getMeasureTwoSides());
  CHECK(out->getMaxObservations() == 0);
  CHECK_FALSE(out->getSensorType().isValid());
}

TEST_CASE(
    "measurement config decoding ignores unknown keys and accepts camelCase") {
  const auto out = codec::decodeMeasurementConfig(R"({
    "name": "FromTheFuture",
    "measureTwoSides": true,
    "max_observations": 9,
    "added_in_v9": {"nested": [1, 2, 3]},
    "mode": "MEASUREMENT_MODE_THAT_DOES_NOT_EXIST_YET"
  })");
  REQUIRE(out.has_value());
  CHECK(out->getName() == "FromTheFuture");
  CHECK(out->getMeasureTwoSides());
  CHECK(out->getMaxObservations() == 9);
}

TEST_CASE("garbage is never a measurement config") {
  for (const std::string_view garbage :
       {std::string_view{""}, std::string_view{"   "},
        std::string_view{"not json"}, std::string_view{"{"},
        std::string_view{"[]"}, std::string_view{"null"},
        std::string_view{"42"}, std::string_view{"{}"},
        std::string_view{R"({"name": ""})"}, std::string_view{R"({"name": 5})"},
        std::string_view{R"({"name": "x", "max_observations": "many"})"},
        std::string_view{R"({"name": "x", "max_observations": 1e30})"},
        std::string_view{R"({"name": "x", "time_interval": [1]})"},
        std::string_view{"<measurementConfig name=\"x\"/>"},
        std::string_view{"\x00\x01\xff\xfe", 4}}) {
    INFO(std::string(garbage));
    CHECK_FALSE(codec::decodeMeasurementConfig(garbage).has_value());
  }
}

TEST_CASE("SensorConfiguration survives a protobuf round trip") {
  const SensorConfiguration in = sampleSensorConfig();
  const auto out = codec::fromProto(codec::toProto(in));
  REQUIRE(out.has_value());
  CHECK(out->getName() == in.getName());
  CHECK(out->getIsSaved());
  CHECK(out->getPluginName() == "Some Plugin");
  CHECK(out->getSensorName() == "Sensor 1");
  CHECK(out->getTypeOfSensor().id() == "laserTracker");

  const ConnectionConfig& c = out->getConnectionConfig();
  CHECK(c.typeOfConnection == ConnectionKind::serial());
  CHECK(c.ip == "192.168.0.7");
  CHECK(c.port == "700");
  CHECK(c.comPort == "/dev/ttyUSB0");
  CHECK(c.baudRate == oi::BaudRate::Baud115200);
  CHECK(c.dataBits == oi::DataBits::Data7);
  CHECK(c.parity == oi::Parity::Odd);
  CHECK(c.stopBits == oi::StopBits::OneAndHalf);
  CHECK(c.flowControl == oi::FlowControl::Hardware);

  const Accuracy& a = out->getAccuracy();
  CHECK(a.sigmaXyz.getAt(0) == 0.1);
  CHECK(a.sigmaXyz.getAt(1) == 0.2);
  CHECK(a.sigmaXyz.getAt(2) == 0.3);
  CHECK(a.sigmaAzimuth == 1.5e-6);
  CHECK(a.sigmaZenith == 2.5e-6);
  CHECK(a.sigmaDistance == 3.5e-5);
  CHECK(a.sigmaTemp == 0.5);
  CHECK(a.sigmaI == 0.01);
  CHECK(a.sigmaJ == 0.02);
  CHECK(a.sigmaK == 0.03);
  CHECK(a.sigmaUndefined.size() == 2);
  CHECK(oi::value(a.sigmaUndefined, "custom sigma") == 7.25);
  CHECK(oi::value(a.sigmaUndefined, "other") == -1.0);

  CHECK(out->getIntegerParameter() == in.getIntegerParameter());
  CHECK(out->getDoubleParameter() == in.getDoubleParameter());
  CHECK(out->getStringParameter() == in.getStringParameter());
  CHECK(oi::values(out->getAvailableStringParameter(), "probe") ==
        oi::values(in.getAvailableStringParameter(), "probe"));
}

TEST_CASE("SensorConfiguration survives a JSON round trip, repeatedly") {
  const SensorConfiguration in = sampleSensorConfig();
  const std::string once = codec::encodeSensorConfiguration(in);
  const auto out = codec::decodeSensorConfiguration(once);
  REQUIRE(out.has_value());
  CHECK(out->getName() == in.getName());
  CHECK(oi::value(out->getDoubleParameter(), "huge") == 1e300);
  CHECK(oi::values(out->getAvailableStringParameter(), "probe") ==
        std::vector<std::string>({"first", "second", "third"}));
  // the option order must not flip on every save
  CHECK(codec::encodeSensorConfiguration(*out) == once);
}

TEST_CASE("an empty SensorConfiguration with just a name round trips") {
  SensorConfiguration in;
  in.setName("Bare");
  const std::string json = codec::encodeSensorConfiguration(in);
  const auto out = codec::decodeSensorConfiguration(json);
  REQUIRE(out.has_value());
  CHECK(out->getName() == "Bare");
  CHECK_FALSE(out->getIsSaved());
  CHECK_FALSE(out->getTypeOfSensor().isValid());
  CHECK_FALSE(out->getConnectionConfig().typeOfConnection.isValid());
  CHECK(out->getIntegerParameter().empty());
  CHECK(out->getDoubleParameter().empty());
  CHECK(out->getStringParameter().empty());
  CHECK(out->getAvailableStringParameter().empty());
  CHECK(out->getAccuracy().sigmaUndefined.empty());
  CHECK(out->getAccuracy().sigmaXyz.getAt(2) == 0.0);
}

TEST_CASE("every serial setting maps both ways") {
  for (const auto baud : {oi::BaudRate::Baud1200, oi::BaudRate::Baud2400,
                          oi::BaudRate::Baud4800, oi::BaudRate::Baud9600,
                          oi::BaudRate::Baud19200, oi::BaudRate::Baud38400,
                          oi::BaudRate::Baud57600, oi::BaudRate::Baud115200}) {
    SensorConfiguration in = sampleSensorConfig();
    ConnectionConfig c = in.getConnectionConfig();
    c.baudRate = baud;
    in.setConnectionConfig(c);
    CHECK(
        codec::fromProto(codec::toProto(in))->getConnectionConfig().baudRate ==
        baud);
  }
  for (const auto parity : {oi::Parity::None, oi::Parity::Even, oi::Parity::Odd,
                            oi::Parity::Space, oi::Parity::Mark}) {
    SensorConfiguration in = sampleSensorConfig();
    ConnectionConfig c = in.getConnectionConfig();
    c.parity = parity;
    in.setConnectionConfig(c);
    CHECK(codec::fromProto(codec::toProto(in))->getConnectionConfig().parity ==
          parity);
  }
  for (const auto bits : {oi::DataBits::Data5, oi::DataBits::Data6,
                          oi::DataBits::Data7, oi::DataBits::Data8}) {
    SensorConfiguration in = sampleSensorConfig();
    ConnectionConfig c = in.getConnectionConfig();
    c.dataBits = bits;
    in.setConnectionConfig(c);
    CHECK(
        codec::fromProto(codec::toProto(in))->getConnectionConfig().dataBits ==
        bits);
  }
  for (const auto stop :
       {oi::StopBits::One, oi::StopBits::OneAndHalf, oi::StopBits::Two}) {
    SensorConfiguration in = sampleSensorConfig();
    ConnectionConfig c = in.getConnectionConfig();
    c.stopBits = stop;
    in.setConnectionConfig(c);
    CHECK(
        codec::fromProto(codec::toProto(in))->getConnectionConfig().stopBits ==
        stop);
  }
  for (const auto flow : {oi::FlowControl::None, oi::FlowControl::Hardware,
                          oi::FlowControl::Software}) {
    SensorConfiguration in = sampleSensorConfig();
    ConnectionConfig c = in.getConnectionConfig();
    c.flowControl = flow;
    in.setConnectionConfig(c);
    CHECK(codec::fromProto(codec::toProto(in))
              ->getConnectionConfig()
              .flowControl == flow);
  }
}

TEST_CASE(
    "an unknown sensor type or connection kind does not fail the config") {
  const auto out = codec::decodeSensorConfiguration(R"({
    "name": "Odd", "sensor_type": "noSuchType",
    "connection": {"kind": "carrierPigeon", "baud_rate": 31337},
    "unknown_future_field": true
  })");
  REQUIRE(out.has_value());
  CHECK_FALSE(out->getTypeOfSensor().isValid());
  CHECK_FALSE(out->getConnectionConfig().typeOfConnection.isValid());
  // unspecified or unknown values come back as 9600 8N1, no flow control
  CHECK(out->getConnectionConfig().baudRate == oi::BaudRate::Baud9600);
  CHECK(out->getConnectionConfig().dataBits == oi::DataBits::Data8);
  CHECK(out->getConnectionConfig().parity == oi::Parity::None);
  CHECK(out->getConnectionConfig().stopBits == oi::StopBits::One);
  CHECK(out->getConnectionConfig().flowControl == oi::FlowControl::None);
}

TEST_CASE("garbage is never a sensor configuration") {
  for (
      const std::string_view garbage :
      {std::string_view{""}, std::string_view{"nope"},
       std::string_view{"{\"name\": \"x\""}, std::string_view{"[1,2]"},
       std::string_view{"{}"}, std::string_view{R"({"name": ""})"},
       std::string_view{R"({"name": "x", "connection": 5})"},
       std::string_view{R"({"name": "x", "double_parameters": [1]})"},
       std::string_view{
           R"({"name": "x", "integer_parameters": [{"name": "a", "value": "b"}]})"},
       std::string_view{R"({"name": "x", "accuracy": {"sigma_xyz": "no"}})"},
       std::string_view{"<sensorConfig name=\"x\"/>"},
       std::string_view{"\xff\xfe\x00\x01", 4}}) {
    INFO(std::string(garbage));
    CHECK_FALSE(codec::decodeSensorConfiguration(garbage).has_value());
  }
}

// ---------------------------------------------------------------------------
// Remote-control protocol (network.proto)
// ---------------------------------------------------------------------------

namespace {

template <class M>
M reparse(const M& in) {
  std::string bytes;
  REQUIRE(in.SerializeToString(&bytes));
  M out;
  REQUIRE(out.ParseFromString(bytes));
  return out;
}

/*! Every member of the message's "payload" oneof, set to an empty payload,
 *  survives serialisation with its case intact. */
template <class M>
void everyPayloadVariantRoundTrips(std::size_t expectedVariants) {
  const auto* oneof = M::descriptor()->FindOneofByName("payload");
  REQUIRE(oneof != nullptr);
  CHECK(static_cast<std::size_t>(oneof->field_count()) == expectedVariants);
  const auto* reflection = M::GetReflection();
  for (int i = 0; i < oneof->field_count(); ++i) {
    const auto* field = oneof->field(i);
    INFO(field->name());
    M in;
    reflection->MutableMessage(&in, field);
    const M out = reparse(in);
    CHECK(google::protobuf::util::MessageDifferencer::Equals(in, out));
    CHECK(reflection->GetOneofFieldDescriptor(out, oneof) == field);
  }
}

}  // namespace

TEST_CASE("every request variant round trips") {
  everyPayloadVariantRoundTrips<oipb::Request>(21);
}

TEST_CASE("every response variant round trips") {
  everyPayloadVariantRoundTrips<oipb::Response>(20);
}

TEST_CASE("every notification variant round trips") {
  everyPayloadVariantRoundTrips<oipb::Notification>(9);
}

TEST_CASE("every request has a response of the same name, except the tool") {
  const auto* requests =
      oipb::Request::descriptor()->FindOneofByName("payload");
  const auto* responses =
      oipb::Response::descriptor()->FindOneofByName("payload");
  for (int i = 0; i < requests->field_count(); ++i) {
    const std::string name(requests->field(i)->name());
    INFO(name);
    const bool hasResponse =
        oipb::Response::descriptor()->FindFieldByName(name) != nullptr;
    CHECK(hasResponse == (name != "tool"));
  }
  CHECK(responses->field_count() == requests->field_count() - 1);
}

TEST_CASE("an envelope carries the request id and its content") {
  oipb::Envelope in;
  in.set_request_id(42);
  auto* set = in.mutable_request()->mutable_set_measurement_config();
  set->set_id(7);
  set->set_config_name("StdScan");
  set->set_saved(true);
  const auto out = reparse(in);
  CHECK(google::protobuf::util::MessageDifferencer::Equals(in, out));
  CHECK(out.request_id() == 42);
  REQUIRE(out.content_case() == oipb::Envelope::kRequest);
  REQUIRE(out.request().payload_case() == oipb::Request::kSetMeasurementConfig);
  CHECK(out.request().set_measurement_config().config_name() == "StdScan");
  CHECK(out.request().set_measurement_config().has_saved());
}

TEST_CASE("absent optional fields stay absent on the wire") {
  oipb::Envelope in;
  in.mutable_request()->mutable_set_active_feature();  // no id
  const auto out = reparse(in);
  REQUIRE(out.request().payload_case() == oipb::Request::kSetActiveFeature);
  CHECK_FALSE(out.request().set_active_feature().has_id());

  in.mutable_request()->mutable_set_active_feature()->set_id(0);
  CHECK(reparse(in).request().set_active_feature().has_id());
}

TEST_CASE("a response with a populated payload round trips") {
  oipb::Envelope in;
  in.set_request_id(3);
  auto* response = in.mutable_response();
  response->set_request(oipb::REQUEST_KIND_GET_OBSERVATIONS);
  response->set_error(oipb::ERROR_CODE_NONE);
  auto* payload = response->mutable_get_observations();
  payload->set_id(5);
  auto* obs = payload->add_observations();
  obs->set_id(9);
  obs->mutable_xyz()->set_x(1.5);
  obs->set_valid(true);
  obs->mutable_deviation()->set_v(0.25);
  obs->set_used(false);
  payload->add_observations()->set_id(10);

  const auto out = reparse(in);
  CHECK(google::protobuf::util::MessageDifferencer::Equals(in, out));
  const auto& got = out.response().get_observations();
  REQUIRE(got.observations_size() == 2);
  CHECK(got.observations(0).has_deviation());
  CHECK(got.observations(0).has_used());
  CHECK_FALSE(got.observations(0).used());
  CHECK_FALSE(got.observations(1).has_deviation());
  CHECK_FALSE(got.observations(1).has_used());
}

TEST_CASE("a project travels inside a getProject response as bytes") {
  oipb::Envelope in;
  in.mutable_response()->mutable_get_project()->set_project(
      std::string("\0\1\2 binary", 10));
  const auto out = reparse(in);
  CHECK(out.response().get_project().project().size() == 10);
  CHECK(google::protobuf::util::MessageDifferencer::Equals(in, out));
}

TEST_CASE("every error code maps both ways") {
  using Err = OiRequestResponse::ErrorCode;
  std::vector<Err> all;
  for (int i = static_cast<int>(Err::eNoError);
       i <= static_cast<int>(Err::eNoMeasurementConfig); ++i) {
    all.push_back(static_cast<Err>(i));
  }
  REQUIRE(all.size() == 21);
  std::vector<int> seen;
  for (const auto e : all) {
    const auto wire = codec::toProto(e);
    CHECK(oipb::ErrorCode_IsValid(wire));
    const auto back = codec::fromProto(wire);
    REQUIRE(back.has_value());
    CHECK(*back == e);
    CHECK(std::find(seen.begin(), seen.end(), static_cast<int>(wire)) ==
          seen.end());
    seen.push_back(static_cast<int>(wire));
  }
  // every wire value is reachable from some model value
  const auto* values = oipb::ErrorCode_descriptor();
  for (int i = 0; i < values->value_count(); ++i) {
    CHECK(codec::fromProto(
              static_cast<oipb::ErrorCode>(values->value(i)->number()))
              .has_value());
  }
  CHECK(codec::toProto(Err::eNoError) == oipb::ERROR_CODE_NONE);
  CHECK(codec::toProto(Err::eNoJob) == oipb::ERROR_CODE_NO_JOB);
  CHECK(codec::toProto(Err::eWrongFormat) == oipb::ERROR_CODE_WRONG_FORMAT);
  // a number from a newer version is not guessed at
  CHECK_FALSE(codec::fromProto(static_cast<oipb::ErrorCode>(9999)).has_value());
}

TEST_CASE("every request type maps both ways") {
  using Req = OiRequestResponse::RequestType;
  int count = 0;
  for (int i = static_cast<int>(Req::eGetProject);
       i <= static_cast<int>(Req::eGetCoordinateSystems); ++i, ++count) {
    const auto type = static_cast<Req>(i);
    const auto wire = codec::toProto(type);
    CHECK(wire != oipb::REQUEST_KIND_UNSPECIFIED);
    CHECK(codec::fromProto(wire) == type);
  }
  CHECK(count == 21);
  // notifications and unknowns have no request kind
  CHECK(codec::toProto(Req::eUnknownRequest) == oipb::REQUEST_KIND_UNSPECIFIED);
  CHECK(codec::toProto(Req::eRealTimeReading) ==
        oipb::REQUEST_KIND_UNSPECIFIED);
  CHECK(codec::fromProto(oipb::REQUEST_KIND_UNSPECIFIED) ==
        Req::eUnknownRequest);
  CHECK(codec::fromProto(static_cast<oipb::RequestKind>(9999)) ==
        Req::eUnknownRequest);
}

TEST_CASE("reading and message kinds map both ways") {
  for (const auto type :
       {ReadingTypes::eDistanceReading, ReadingTypes::eCartesianReading,
        ReadingTypes::ePolarReading, ReadingTypes::eDirectionReading,
        ReadingTypes::eTemperatureReading, ReadingTypes::eLevelReading,
        ReadingTypes::eCartesianReading6D}) {
    CHECK(codec::fromProto(codec::toProto(type)) == type);
  }
  CHECK(codec::toProto(ReadingTypes::eUndefinedReading) ==
        oipb::READING_KIND_UNSPECIFIED);
  CHECK_FALSE(codec::fromProto(oipb::READING_KIND_UNSPECIFIED).has_value());

  for (const auto type :
       {MessageTypes::eInformationMessage, MessageTypes::eWarningMessage,
        MessageTypes::eErrorMessage, MessageTypes::eCriticalMessage,
        MessageTypes::eQuestionMessage}) {
    CHECK(codec::fromProto(codec::toProto(type)) == type);
  }
  CHECK_FALSE(codec::fromProto(oipb::MESSAGE_KIND_UNSPECIFIED).has_value());
}
