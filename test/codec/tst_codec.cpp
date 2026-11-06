#include <google/protobuf/util/time_util.h>

#include <catch2/catch_test_macros.hpp>
#include <string>

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
