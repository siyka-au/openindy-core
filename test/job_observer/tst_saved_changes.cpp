// Every change of what the project file stores reaches a JobObserver: the host
// decides "has the project changed since it was saved?" from these events
// alone (there is no content digest). One section per saved fact; the last
// test lists what must NOT be reported as a change.
#include <catch2/catch_test_macros.hpp>
#include <memory>
#include <string>
#include <vector>

#include "bundle_adjustment.h"
#include "catch_compat.h"
#include "choose_la_lib.h"
#include "coordinate_system.h"
#include "fit_function.h"
#include "geometry/plane.h"
#include "geometry/point.h"
#include "group.h"
#include "oi_job.h"
#include "reading.h"
#include "recording_job_observer.h"
#include "station.h"
#include "trafo_param.h"

using namespace oi;

namespace {

class SavedFit : public FitFunction, public Producer<Point> {
 public:
  SavedFit() {
    metaData.name = "SavedFit";
    metaData.pluginName = "Test Plugin";
    NeededElement points;
    points.typeOfElement = ElementTypes::ePointElement;
    points.infinite = true;
    points.roleName = "points";
    neededElements.push_back(points);
    applicableFor.push_back(FeatureTypes::ePointFeature);
  }
  std::expected<Point, std::string> exec() override {
    return std::unexpected(std::string("not used"));
  }
};

class SavedBundle : public BundleAdjustment {
 public:
  SavedBundle() {
    metaData.name = "SavedBundle";
    metaData.pluginName = "Test Plugin";
  }
};

struct Fixture {
  OiJob job;
  RecordingJobObserver rec;
  Station* station = nullptr;
  CoordinateSystem* part = nullptr;
  CoordinateSystem* bundle = nullptr;
  Point* a = nullptr;
  Point* b = nullptr;
  Point* c = nullptr;  // has a function
  Plane* plane = nullptr;
  Group* group = nullptr;
  TrafoParam* trafo = nullptr;

  Fixture() {
    math::ChooseLALib::setLinearAlgebra(
        math::ChooseLALib::LinearAlgebraLibrary::Armadillo);
    auto s = std::make_unique<Station>("S1");
    station = s.get();
    REQUIRE(job.addFeature(std::move(s)));
    auto p = std::make_unique<CoordinateSystem>();
    part = p.get();
    part->setFeatureName("PART");
    REQUIRE(job.addFeature(std::move(p)));
    auto bs = std::make_unique<CoordinateSystem>(true);
    bundle = bs.get();
    bundle->setFeatureName("Bundle01");
    REQUIRE(job.addFeature(std::move(bs)));
    a = addPoint("A");
    b = addPoint("B");
    auto cp = std::make_unique<Point>();
    c = cp.get();
    c->setFeatureName("C");
    REQUIRE(c->addFunction(std::make_unique<SavedFit>()));
    REQUIRE(job.addFeature(std::move(cp)));
    auto pl = std::make_unique<Plane>();
    plane = pl.get();
    plane->setFeatureName("P1");
    REQUIRE(job.addFeature(std::move(pl)));
    auto g = std::make_unique<Group>();
    group = g.get();
    group->setFeatureName("G");
    REQUIRE(job.addFeature(std::move(g)));
    auto t = std::make_unique<TrafoParam>();
    trafo = t.get();
    trafo->setFeatureName("T1");
    REQUIRE(trafo->setCoordinateSystems(station->getCoordinateSystem(), part));
    REQUIRE(job.addFeature(std::move(t)));
    rec.attach(&job);
  }

  Point* addPoint(const std::string& name) {
    auto p = std::make_unique<Point>(Position(1.0, 2.0, 3.0));
    Point* raw = p.get();
    raw->setFeatureName(name);
    REQUIRE(job.addFeature(std::move(p)));
    return raw;
  }

  Function* function() const { return c->getFunctions().front().data(); }

  // runs `action` and reports whether the observer was told `event`
  template <typename Action>
  bool reports(const std::string& event, Action&& action) {
    rec.calls.clear();
    action();
    return rec.count(event) > 0;
  }
};

}  // namespace

TEST_CASE("saved job structure is reported") {
  Fixture f;
  SECTION("adding a feature (features, ids, stations, systems, trafos)") {
    CHECK(f.reports("featureSetChanged", [&] { f.addPoint("N"); }));
    CHECK(f.reports("stationSetChanged", [&] {
      REQUIRE(f.job.addFeature(std::make_unique<Station>("S2")));
    }));
  }
  SECTION("removing a feature") {
    CHECK(f.reports("featureSetChanged",
                    [&] { REQUIRE(f.job.removeFeature(f.b->getId())); }));
  }
  SECTION("name, group and comment") {
    CHECK(f.reports("featureNameChanged", [&] { f.a->setFeatureName("X"); }));
    CHECK(f.reports("featureGroupChanged", [&] { f.a->setGroupName("G2"); }));
    CHECK(f.reports("featureCommentChanged", [&] { f.a->setComment("hi"); }));
  }
  SECTION("active feature, station and coordinate system") {
    CHECK(f.reports("activeFeatureChanged",
                    [&] { f.a->setActiveFeatureState(true); }));
    CHECK(f.reports("activeStationChanged",
                    [&] { f.station->setActiveStationState(true); }));
    CHECK(f.reports("activeCoordinateSystemChanged",
                    [&] { f.part->setActiveCoordinateSystemState(true); }));
  }
  SECTION("group members") {
    CHECK(f.reports("groupMembersChanged", [&] {
      REQUIRE(f.group->addMember(f.job.getFeatureById(f.a->getId())));
    }));
  }
}

TEST_CASE("saved geometry values are reported") {
  Fixture f;
  CHECK(f.reports("geometryParametersChanged",
                  [&] { f.a->setPoint(Position(4.0, 5.0, 6.0)); }));
  CHECK(f.reports("geometryParametersChanged", [&] {
    f.plane->setPlane(Position(0.0, 0.0, 1.0), Direction(0.0, 0.0, 1.0));
  }));
  CHECK(f.reports("geometryParametersChanged", [&] {
    f.a->addIntent(MeasurementIntent(f.c->getId(), "points"));
  }));
  CHECK(f.reports("geometryParametersChanged", [&] {
    REQUIRE(f.a->removeIntent(MeasurementIntent(f.c->getId(), "points")));
  }));
  CHECK(
      f.reports("geometryIsCommonChanged", [&] { f.a->setCommonState(true); }));
  CHECK(f.reports("geometryStatisticChanged", [&] {
    Statistic statistic;
    statistic.setIsValid(true);
    f.a->setStatistic(statistic);
  }));
  CHECK(f.reports("geometryMeasurementConfigChanged", [&] {
    f.a->setMeasurementConfig(
        MeasurementConfigKey("cfg", ConfigTypes::eProjectConfig));
  }));
  CHECK(f.reports("geometryNominalSystemChanged",
                  [&] { REQUIRE(f.a->setNominalSystem(f.part)); }));
}

TEST_CASE("saved functions are reported") {
  Fixture f;
  Function* fn = f.function();
  InputElement input(f.a->getId());
  input.typeOfElement = ElementTypes::ePointElement;
  input.point = f.a;
  input.geometry = f.a;

  CHECK(f.reports("functionContentChanged",
                  [&] { fn->addInputElement(input, 0); }));
  CHECK(f.reports("functionContentChanged",
                  [&] { fn->setShouldBeUsed(0, f.a->getId(), false); }));
  CHECK(f.reports("functionContentChanged",
                  [&] { fn->setPipeline(0, std::vector<PipelineStage>{}); }));
  CHECK(f.reports("functionContentChanged", [&] {
    fn->fixParameter(FixedParameter(GeometryParameters::eUnknownX));
  }));
  CHECK(f.reports("functionContentChanged",
                  [&] { fn->unfixParameter(GeometryParameters::eUnknownX); }));
  CHECK(f.reports("functionContentChanged", [&] { fn->unfixAllParameters(); }));
  CHECK(f.reports("functionContentChanged",
                  [&] { fn->removeInputElement(f.a->getId(), 0); }));
  CHECK(f.reports("featureFunctionsChanged", [&] {
    REQUIRE(f.b->addFunction(std::make_unique<SavedFit>()));
  }));
}

TEST_CASE("saved transformations and systems are reported") {
  Fixture f;
  SECTION("trafo params") {
    CHECK(f.reports("trafoParamIsUsedChanged",
                    [&] { f.trafo->setIsUsed(true); }));
    CHECK(f.reports("trafoParamIsDatumChanged",
                    [&] { f.trafo->setIsDatumTrafo(true); }));
    CHECK(f.reports("trafoParamParametersChanged", [&] {
      OiVec rotation(3);
      OiVec translation(3);
      OiVec scale(3);
      scale.setAt(0, 1.0);
      scale.setAt(1, 1.0);
      scale.setAt(2, 1.0);
      REQUIRE(
          f.trafo->setTransformationParameters(rotation, translation, scale));
    }));
  }
  SECTION("coordinate systems") {
    CHECK(f.reports("systemFrameChanged", [&] {
      f.part->setCoordinateSystem(Position(1, 2, 3), Direction(1, 0, 0),
                                  Direction(0, 1, 0), Direction(0, 0, 1));
    }));
    CHECK(f.reports("systemFrameChanged",
                    [&] { f.part->setExpansionOrigin(Position(1, 1, 1)); }));
    CHECK(f.reports("systemTrafoParamsChanged", [&] {
      f.part->addTransformationParameter(
          f.job.getFeatureById(f.trafo->getId())->getTrafoParam());
    }));
  }
  SECTION("bundle systems") {
    CHECK(f.reports("systemBundleChanged",
                    [&] { f.bundle->setBundleTemplate("{}"); }));
    CHECK(f.reports("systemBundleChanged", [&] {
      REQUIRE(f.bundle->setBundlePlugin(std::make_unique<SavedBundle>()));
    }));
    CHECK(f.reports("systemBundleChanged", [&] {
      f.bundle->getBundlePlugin()->setInputStations({});
    }));
    CHECK(f.reports("systemBundleChanged", [&] {
      f.bundle->getBundlePlugin()->setBaseStation(BundleStation());
    }));
  }
}

TEST_CASE("saved observations and the station sensor are reported") {
  Fixture f;
  f.station->setActiveStationState(true);
  // a measurement adds observations to the station's system and to the new
  // point (the point itself is created without a feature-set notification;
  // the observation events are what report it)
  f.rec.calls.clear();
  ReadingCartesian r;
  r.isValid = true;
  std::vector<std::unique_ptr<Reading>> readings;
  readings.push_back(std::make_unique<Reading>(r));
  f.job.addMeasurementResults(f.a->getId(), std::move(readings));
  INFO(f.rec.joined());
  CHECK(f.rec.count("systemObservationsChanged") > 0);
  CHECK(f.rec.count("geometryObservationsChanged") > 0);

  // and removing them again
  f.rec.calls.clear();
  f.job.removeObservations(f.a->getId());
  CHECK(f.rec.count("geometryObservationsChanged") > 0);
  CHECK(f.reports("stationSensorChanged", [&] { f.station->resetSensor(); }));
}

TEST_CASE("changes that are not saved are not reported as saved content") {
  Fixture f;
  // none of these appear in the project file: the working group and the
  // recalculation bookkeeping
  const std::vector<std::string> saved = {
      "featureSetChanged",         "featureNameChanged",
      "geometryParametersChanged", "functionContentChanged",
      "systemBundleChanged",       "systemFrameChanged"};
  f.rec.calls.clear();
  f.job.setActiveGroup("whatever");
  f.job.featureRecalculated(f.a->getId());
  f.job.featuresRecalculated();
  f.job.sendMessage("hello", MessageTypes::eInformationMessage);
  f.a->setIsUpdated(true);
  for (const std::string& event : saved) {
    CHECK(f.rec.count(event) == 0);
  }
}
