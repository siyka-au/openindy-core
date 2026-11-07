#include <QtTest>
#include <chrono>
#include <format>
#include <string>

#include "chooselalib.h"
#include "convert.h"
#include "openindy/project.pb.h"
#include "reading.h"

#define COMPARE_DOUBLE(actual, expected, threshold) \
  QVERIFY2(std::abs(actual - expected) < threshold, \
           std::format("actual: {}, expected: {}", actual, expected).c_str());

using namespace oi;

class ReadingTest : public QObject {
  Q_OBJECT

 public:
  ReadingTest();

 private Q_SLOTS:
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
  oi::Guarded<Reading> roundTrip(oi::Guarded<Reading> r);
};

void ReadingTest::initTestCase() {
  ChooseLALib::setLinearAlgebra(ChooseLALib::LinearAlgebraLibrary::Armadillo);
}

ReadingTest::ReadingTest() {}

// What a reading goes through when a project is saved and loaded again.
oi::Guarded<Reading> ReadingTest::roundTrip(oi::Guarded<Reading> r) {
  std::string bytes;
  if (!oi::codec::toProto(*r).SerializeToString(&bytes)) {
    qFatal("failed to serialise the reading");
  }
  openindy::v1::Reading message;
  if (!message.ParseFromString(bytes)) {
    qFatal("failed to parse the reading");
  }
  return oi::codec::createReading(message);
}

// The time a reading was taken is kept to the nanosecond.
void ReadingTest::testMeasuredAtSurvivesRoundTrip() {
  using namespace std::chrono;
  ReadingUndefined u;
  oi::Guarded<Reading> reading = new Reading(u);
  const sys_time<nanoseconds> when = sys_days{year{2024} / 5 / 17} + hours{13} +
                                     minutes{45} + seconds{9} +
                                     nanoseconds{123456789};
  reading->setMeasuredAt(when);

  oi::Guarded<Reading> back = roundTrip(reading);
  QCOMPARE(time_point_cast<nanoseconds>(back->getMeasuredAt())
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

  oi::Guarded<Reading> outReading = new Reading(reading);
  oi::Guarded<Reading> inReading = this->roundTrip(outReading);

  ReadingCartesian6D in = inReading->getCartesianReading6D();

  QVERIFY(inReading->getTypeOfReading() == ReadingTypes::eCartesianReading6D);
  QVERIFY(in.isValid);
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

  oi::Guarded<Reading> outReading = new Reading(reading);
  oi::Guarded<Reading> inReading = this->roundTrip(outReading);

  ReadingCartesian in = inReading->getCartesianReading();

  QVERIFY(inReading->getTypeOfReading() == ReadingTypes::eCartesianReading);
  QVERIFY(in.isValid);
  COMPARE_DOUBLE(in.xyz.getAt(0), 1., 0.01);
  COMPARE_DOUBLE(in.xyz.getAt(1), 2., 0.01);
  COMPARE_DOUBLE(in.xyz.getAt(2), 3., 0.01);
}

void ReadingTest::testDistanceReading() {
  ReadingDistance reading;
  reading.isValid = true;
  reading.distance = 123.4;

  oi::Guarded<Reading> outReading = new Reading(reading);
  oi::Guarded<Reading> inReading = this->roundTrip(outReading);

  ReadingDistance in = inReading->getDistanceReading();

  QVERIFY(inReading->getTypeOfReading() == ReadingTypes::eDistanceReading);
  QVERIFY(in.isValid);
  COMPARE_DOUBLE(in.distance, 123.4, 0.01);
}

void ReadingTest::testPolarReading() {
  ReadingPolar reading;
  reading.isValid = true;
  reading.azimuth = 12.2;
  reading.zenith = 23.3;
  reading.distance = 56.8;

  oi::Guarded<Reading> outReading = new Reading(reading);
  oi::Guarded<Reading> inReading = this->roundTrip(outReading);

  ReadingPolar in = inReading->getPolarReading();

  QVERIFY(inReading->getTypeOfReading() == ReadingTypes::ePolarReading);
  QVERIFY(in.isValid);
  COMPARE_DOUBLE(in.azimuth, 12.2, 0.01);
  COMPARE_DOUBLE(in.zenith, 23.3, 0.01);
  COMPARE_DOUBLE(in.distance, 56.8, 0.01);
}

void ReadingTest::testDirectionReading() {
  ReadingDirection reading;
  reading.isValid = true;
  reading.azimuth = 12.2;
  reading.zenith = 23.3;

  oi::Guarded<Reading> outReading = new Reading(reading);
  oi::Guarded<Reading> inReading = this->roundTrip(outReading);

  ReadingDirection in = inReading->getDirectionReading();

  QVERIFY(inReading->getTypeOfReading() == ReadingTypes::eDirectionReading);
  QVERIFY(in.isValid);
  COMPARE_DOUBLE(in.azimuth, 12.2, 0.01);
  COMPARE_DOUBLE(in.zenith, 23.3, 0.01);
}

void ReadingTest::testTemperatureReading() {
  ReadingTemperature reading;
  reading.isValid = true;
  reading.temperature = 27.1;

  oi::Guarded<Reading> outReading = new Reading(reading);
  oi::Guarded<Reading> inReading = this->roundTrip(outReading);

  ReadingTemperature in = inReading->getTemperatureReading();

  QVERIFY(inReading->getTypeOfReading() == ReadingTypes::eTemperatureReading);
  QVERIFY(in.isValid);
  COMPARE_DOUBLE(in.temperature, 27.1, 0.01);
}

void ReadingTest::testLevelReading() {
  ReadingLevel reading;
  reading.isValid = true;
  reading.i = 0.1;
  reading.j = 0.9;
  reading.k = 0.2;

  oi::Guarded<Reading> outReading = new Reading(reading);
  oi::Guarded<Reading> inReading = this->roundTrip(outReading);

  ReadingLevel in = inReading->getLevelReading();

  QVERIFY(inReading->getTypeOfReading() == ReadingTypes::eLevelReading);
  QVERIFY(in.isValid);
  COMPARE_DOUBLE(in.i, .1, 0.01);
  COMPARE_DOUBLE(in.j, .9, 0.01);
  COMPARE_DOUBLE(in.k, .2, 0.01);
}

QTEST_APPLESS_MAIN(ReadingTest)

#include "tst_reading.moc"
