#include <google/protobuf/util/message_differencer.h>
#include <google/protobuf/util/time_util.h>

#include <catch2/catch_test_macros.hpp>
#include <string>

#include "chooselalib.h"
#include "convert.h"
#include "digest.h"
#include "openindy/project.pb.h"
#include "sensortype.h"

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
  REQUIRE_FALSE(known.isEmpty());

  MeasurementConfig gated = sampleConfig();
  gated.setSensorType(known.first());
  const auto back = codec::fromProto(codec::toProto(gated));
  REQUIRE(back.has_value());
  CHECK(back->getSensorType().id() == known.first().id());

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
  const QPointer<Reading> reading = codec::createReading(in);
  REQUIRE_FALSE(reading.isNull());
  const oipb::Reading out = codec::toProto(*reading);
  INFO(in.ShortDebugString());
  INFO(out.ShortDebugString());
  CHECK(MessageDifferencer::Equals(in, out));
  delete reading.data();
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
  const QPointer<Reading> reading = codec::createReading(m);
  REQUIRE_FALSE(reading.isNull());
  CHECK(reading->getFace() == SensorFaces::eBackSide);
  CHECK(reading->isImported());
  CHECK(std::chrono::duration_cast<std::chrono::nanoseconds>(
            reading->getMeasuredAt().time_since_epoch())
            .count() == 1730000000123456789LL);
  delete reading.data();
}

TEST_CASE("a reading with no measurement is rejected") {
  useArmadillo();
  CHECK(codec::createReading(readingBase(30)).isNull());
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

  const QPointer<Observation> observation = codec::createObservation(in);
  REQUIRE_FALSE(observation.isNull());
  CHECK(MessageDifferencer::Equals(in, codec::toProto(*observation)));
  REQUIRE_FALSE(observation->getReading().isNull());
  CHECK(observation->getReading()->getObservation() == observation);
  delete observation->getReading().data();
  delete observation.data();
}

TEST_CASE("an observation without its reading cannot be created") {
  useArmadillo();
  oipb::Observation in;
  in.set_id(50);
  CHECK(codec::createObservation(in).isNull());
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

  const QPointer<TrafoParam> trafo = codec::createTrafoParam(in);
  REQUIRE_FALSE(trafo.isNull());
  CHECK(MessageDifferencer::Equals(in, codec::toProto(*trafo)));
  delete trafo.data();
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

  const QPointer<CoordinateSystem> system = codec::createCoordinateSystem(in);
  REQUIRE_FALSE(system.isNull());
  CHECK(MessageDifferencer::Equals(in, codec::toProto(*system)));
  delete system.data();
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

  const QPointer<Station> station = codec::createStation(in);
  REQUIRE_FALSE(station.isNull());
  const oipb::Station out = codec::toProto(*station);
  CHECK(MessageDifferencer::Equals(in.feature(), out.feature()));
  CHECK(out.active_station());
  delete station.data();
}
