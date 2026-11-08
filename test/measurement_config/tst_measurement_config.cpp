#include <catch2/catch_test_macros.hpp>
#include <map>
#include <string>

#include "catch_compat.h"
#include "containers.h"
#include "measurement_config.h"
#include "sensor_type.h"

using namespace oi;

class MeasurementConfigTest {
 public:
  // MeasurementConfigKey used to be (name, configType) alone. Two drivers
  // publishing the same standard-config name - which every driver does,
  // e.g. "FastPoint" - collided in exactly this map shape
  //(MeasurementConfigManager::configs) and silently dropped one of them.
  // sensorType is now part of the key's identity for exactly the reason
  // file path is part of a plugin's identity: same display name, genuinely
  // different things.
  void sameNameDifferentSensorTypeDoNotCollide();

  // the reverse must also hold: two identically-typed, identically-named
  // standard configs really are the same key (so re-publishing/re-scanning
  // the same driver doesn't duplicate its own entries)
  void sameNameSameSensorTypeAreEqual();
};

void MeasurementConfigTest::sameNameDifferentSensorTypeDoNotCollide() {
  MeasurementConfig trackerFastPoint;
  trackerFastPoint.setName("FastPoint");
  trackerFastPoint.makeStandardConfig();
  trackerFastPoint.setSensorType(SensorType("laserTracker", "Laser Tracker"));

  MeasurementConfig totalStationFastPoint;
  totalStationFastPoint.setName("FastPoint");
  totalStationFastPoint.makeStandardConfig();
  totalStationFastPoint.setSensorType(
      SensorType("totalStation", "Total Station"));

  OI_REQUIRE2(
      !(trackerFastPoint.getKey() == totalStationFastPoint.getKey()),
      "same-named configs from different sensor types must not share a key");

  std::map<MeasurementConfigKey, MeasurementConfig> configs;
  configs.insert_or_assign(trackerFastPoint.getKey(), trackerFastPoint);
  configs.insert_or_assign(totalStationFastPoint.getKey(),
                           totalStationFastPoint);

  OI_COMPARE(configs.size(), 2);
  OI_COMPARE(oi::value(configs, trackerFastPoint.getKey()).getSensorType().id(),
             std::string("laserTracker"));
  OI_COMPARE(
      oi::value(configs, totalStationFastPoint.getKey()).getSensorType().id(),
      std::string("totalStation"));
}

void MeasurementConfigTest::sameNameSameSensorTypeAreEqual() {
  MeasurementConfig first;
  first.setName("StdPoint");
  first.makeStandardConfig();
  first.setSensorType(SensorType("laserTracker", "Laser Tracker"));

  MeasurementConfig second;
  second.setName("StdPoint");
  second.makeStandardConfig();
  second.setSensorType(SensorType("laserTracker", "Laser Tracker"));

  REQUIRE(first.getKey() == second.getKey());

  std::map<MeasurementConfigKey, MeasurementConfig> configs;
  configs.insert_or_assign(first.getKey(), first);
  configs.insert_or_assign(second.getKey(), second);

  OI_COMPARE(configs.size(), 1);
}

TEST_CASE("MeasurementConfigTest::sameNameDifferentSensorTypeDoNotCollide",
          "[MeasurementConfigTest]") {
  MeasurementConfigTest fixture;
  fixture.sameNameDifferentSensorTypeDoNotCollide();
}

TEST_CASE("MeasurementConfigTest::sameNameSameSensorTypeAreEqual",
          "[MeasurementConfigTest]") {
  MeasurementConfigTest fixture;
  fixture.sameNameSameSensorTypeAreEqual();
}
