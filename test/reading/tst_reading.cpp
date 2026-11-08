#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <format>
#include <stdexcept>
#include <string>

#include "catch_compat.h"
#include "choose_la_lib.h"
#include "convert.h"
#include "openindy/project.pb.h"
#include "reading.h"

#define COMPARE_DOUBLE(actual, expected, threshold) \
  OI_REQUIRE2(                                      \
      std::abs(actual - expected) < threshold,      \
      std::format("actual: {}, expected: {}", actual, expected).c_str());

using namespace oi;

class ReadingTest {
 public:
  ReadingTest();

 public:
  void initTestCase();
  void testCartesian6D();
  void testCartesian();
  void testDistanceReading();
  void testPolarReading();
  void testDirectionReading();
  void testTemperatureReading();
  void testLevelReading();
  void testMeasuredAtSurvivesRoundTrip();

 private:
  std::unique_ptr<Reading> roundTrip(const std::unique_ptr<Reading>& r);
};

void ReadingTest::initTestCase() {
  ChooseLALib::setLinearAlgebra(ChooseLALib::LinearAlgebraLibrary::Armadillo);
}

ReadingTest::ReadingTest() {}

// What a reading goes through when a project is saved and loaded again.
std::unique_ptr<Reading> ReadingTest::roundTrip(
    const std::unique_ptr<Reading>& r) {
  std::string bytes;
  if (!oi::codec::toProto(*r).SerializeToString(&bytes)) {
    throw std::runtime_error("failed to serialise the reading");
  }
  openindy::v1::Reading message;
  if (!message.ParseFromString(bytes)) {
    throw std::runtime_error("failed to parse the reading");
  }
  return oi::codec::createReading(message);
}

// The time a reading was taken is kept to the nanosecond.
void ReadingTest::testMeasuredAtSurvivesRoundTrip() {
  using namespace std::chrono;
  ReadingUndefined u;
  auto reading = std::make_unique<Reading>(u);
  const sys_time<nanoseconds> when = sys_days{year{2024} / 5 / 17} + hours{13} +
                                     minutes{45} + seconds{9} +
                                     nanoseconds{123456789};
  reading->setMeasuredAt(when);

  auto back = roundTrip(reading);
  OI_COMPARE(time_point_cast<nanoseconds>(back->getMeasuredAt())
                 .time_since_epoch()
                 .count(),
             when.time_since_epoch().count());
}

void ReadingTest::testCartesian6D() {
  ReadingCartesian6D reading;
  reading.isValid = true;
  reading.xyz.setAt(0, 1.);
  reading.xyz.setAt(1, 2.);
  reading.xyz.setAt(2, 3.);
  reading.ijk.setAt(0, 0.1);
  reading.ijk.setAt(1, 0.9);
  reading.ijk.setAt(2, 0.2);

  auto outReading = std::make_unique<Reading>(reading);
  auto inReading = this->roundTrip(outReading);

  ReadingCartesian6D in = inReading->getCartesianReading6D();

  REQUIRE(inReading->getTypeOfReading() == ReadingTypes::eCartesianReading6D);
  REQUIRE(in.isValid);
  COMPARE_DOUBLE(in.xyz.getAt(0), 1., 0.01);
  COMPARE_DOUBLE(in.xyz.getAt(1), 2., 0.01);
  COMPARE_DOUBLE(in.xyz.getAt(2), 3., 0.01);
  COMPARE_DOUBLE(in.ijk.getAt(0), .1, 0.01);
  COMPARE_DOUBLE(in.ijk.getAt(1), .9, 0.01);
  COMPARE_DOUBLE(in.ijk.getAt(2), .2, 0.01);
}

void ReadingTest::testCartesian() {
  ReadingCartesian reading;
  reading.isValid = true;
  reading.xyz.setAt(0, 1.);
  reading.xyz.setAt(1, 2.);
  reading.xyz.setAt(2, 3.);
  ;

  auto outReading = std::make_unique<Reading>(reading);
  auto inReading = this->roundTrip(outReading);

  ReadingCartesian in = inReading->getCartesianReading();

  REQUIRE(inReading->getTypeOfReading() == ReadingTypes::eCartesianReading);
  REQUIRE(in.isValid);
  COMPARE_DOUBLE(in.xyz.getAt(0), 1., 0.01);
  COMPARE_DOUBLE(in.xyz.getAt(1), 2., 0.01);
  COMPARE_DOUBLE(in.xyz.getAt(2), 3., 0.01);
}

void ReadingTest::testDistanceReading() {
  ReadingDistance reading;
  reading.isValid = true;
  reading.distance = 123.4;

  auto outReading = std::make_unique<Reading>(reading);
  auto inReading = this->roundTrip(outReading);

  ReadingDistance in = inReading->getDistanceReading();

  REQUIRE(inReading->getTypeOfReading() == ReadingTypes::eDistanceReading);
  REQUIRE(in.isValid);
  COMPARE_DOUBLE(in.distance, 123.4, 0.01);
}

void ReadingTest::testPolarReading() {
  ReadingPolar reading;
  reading.isValid = true;
  reading.azimuth = 12.2;
  reading.zenith = 23.3;
  reading.distance = 56.8;

  auto outReading = std::make_unique<Reading>(reading);
  auto inReading = this->roundTrip(outReading);

  ReadingPolar in = inReading->getPolarReading();

  REQUIRE(inReading->getTypeOfReading() == ReadingTypes::ePolarReading);
  REQUIRE(in.isValid);
  COMPARE_DOUBLE(in.azimuth, 12.2, 0.01);
  COMPARE_DOUBLE(in.zenith, 23.3, 0.01);
  COMPARE_DOUBLE(in.distance, 56.8, 0.01);
}

void ReadingTest::testDirectionReading() {
  ReadingDirection reading;
  reading.isValid = true;
  reading.azimuth = 12.2;
  reading.zenith = 23.3;

  auto outReading = std::make_unique<Reading>(reading);
  auto inReading = this->roundTrip(outReading);

  ReadingDirection in = inReading->getDirectionReading();

  REQUIRE(inReading->getTypeOfReading() == ReadingTypes::eDirectionReading);
  REQUIRE(in.isValid);
  COMPARE_DOUBLE(in.azimuth, 12.2, 0.01);
  COMPARE_DOUBLE(in.zenith, 23.3, 0.01);
}

void ReadingTest::testTemperatureReading() {
  ReadingTemperature reading;
  reading.isValid = true;
  reading.temperature = 27.1;

  auto outReading = std::make_unique<Reading>(reading);
  auto inReading = this->roundTrip(outReading);

  ReadingTemperature in = inReading->getTemperatureReading();

  REQUIRE(inReading->getTypeOfReading() == ReadingTypes::eTemperatureReading);
  REQUIRE(in.isValid);
  COMPARE_DOUBLE(in.temperature, 27.1, 0.01);
}

void ReadingTest::testLevelReading() {
  ReadingLevel reading;
  reading.isValid = true;
  reading.i = 0.1;
  reading.j = 0.9;
  reading.k = 0.2;

  auto outReading = std::make_unique<Reading>(reading);
  auto inReading = this->roundTrip(outReading);

  ReadingLevel in = inReading->getLevelReading();

  REQUIRE(inReading->getTypeOfReading() == ReadingTypes::eLevelReading);
  REQUIRE(in.isValid);
  COMPARE_DOUBLE(in.i, .1, 0.01);
  COMPARE_DOUBLE(in.j, .9, 0.01);
  COMPARE_DOUBLE(in.k, .2, 0.01);
}

TEST_CASE("ReadingTest::testCartesian6D", "[ReadingTest]") {
  ReadingTest fixture;
  fixture.initTestCase();
  fixture.testCartesian6D();
}

TEST_CASE("ReadingTest::testCartesian", "[ReadingTest]") {
  ReadingTest fixture;
  fixture.initTestCase();
  fixture.testCartesian();
}

TEST_CASE("ReadingTest::testDistanceReading", "[ReadingTest]") {
  ReadingTest fixture;
  fixture.initTestCase();
  fixture.testDistanceReading();
}

TEST_CASE("ReadingTest::testPolarReading", "[ReadingTest]") {
  ReadingTest fixture;
  fixture.initTestCase();
  fixture.testPolarReading();
}

TEST_CASE("ReadingTest::testDirectionReading", "[ReadingTest]") {
  ReadingTest fixture;
  fixture.initTestCase();
  fixture.testDirectionReading();
}

TEST_CASE("ReadingTest::testTemperatureReading", "[ReadingTest]") {
  ReadingTest fixture;
  fixture.initTestCase();
  fixture.testTemperatureReading();
}

TEST_CASE("ReadingTest::testLevelReading", "[ReadingTest]") {
  ReadingTest fixture;
  fixture.initTestCase();
  fixture.testLevelReading();
}

TEST_CASE("ReadingTest::testMeasuredAtSurvivesRoundTrip", "[ReadingTest]") {
  ReadingTest fixture;
  fixture.initTestCase();
  fixture.testMeasuredAtSurvivesRoundTrip();
}
