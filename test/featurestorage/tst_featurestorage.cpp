#include <catch2/catch_test_macros.hpp>
#include <memory>
#include <vector>

#include "chooselalib.h"
#include "coordinatesystem.h"
#include "featurewrapper.h"
#include "group.h"
#include "guarded.h"
#include "oijob.h"
#include "plane.h"
#include "point.h"
#include "pointgroup.h"
#include "station.h"
#include "trafoparam.h"

using namespace oi;

namespace {

std::unique_ptr<Point> namedPoint(const char* name) {
  auto point = std::make_unique<Point>();
  point->setFeatureName(name);
  return point;
}

}  // namespace

// The job owns its features through the FeatureContainer; a feature owns its
// one FeatureWrapper, and everything else holds oi::Guarded handles.

TEST_CASE("the job destroys every feature it owns, whatever its type") {
  ChooseLALib::setLinearAlgebra(ChooseLALib::LinearAlgebraLibrary::Armadillo);
  oi::Guarded<Point> point;
  oi::Guarded<Plane> plane;
  oi::Guarded<Group> group;
  oi::Guarded<FeatureWrapper> wrapper;
  {
    OiJob job;
    auto p = namedPoint("P1");
    point = p.get();
    wrapper = job.addFeature(std::move(p));
    REQUIRE_FALSE(wrapper.isNull());

    auto pl = std::make_unique<Plane>();
    pl->setFeatureName("Pl1");
    plane = pl.get();
    REQUIRE(job.addFeature(std::move(pl)));

    auto g = std::make_unique<Group>("G1");
    group = g.get();
    REQUIRE(job.addFeature(std::move(g)));
    REQUIRE_FALSE(point.isNull());
  }
  // a plain geometry used to outlive its job (leaked); now nothing does
  CHECK(point.isNull());
  CHECK(plane.isNull());
  CHECK(group.isNull());
  CHECK(wrapper.isNull());
}

TEST_CASE("a feature the job rejects is destroyed, not leaked") {
  ChooseLALib::setLinearAlgebra(ChooseLALib::LinearAlgebraLibrary::Armadillo);
  OiJob job;
  REQUIRE(job.addFeature(namedPoint("P1")));

  // same name and type: rejected
  auto duplicate = namedPoint("P1");
  oi::Guarded<Point> guard = duplicate.get();
  CHECK(job.addFeature(std::move(duplicate)).isNull());
  CHECK(guard.isNull());

  // no name at all: rejected
  auto unnamed = std::make_unique<Point>();
  guard = unnamed.get();
  CHECK(job.addFeature(std::move(unnamed)).isNull());
  CHECK(guard.isNull());

  CHECK(job.addFeature(std::unique_ptr<Feature>()).isNull());
  CHECK(job.getFeatureCount() == 1);
}

TEST_CASE("the job's handle for a feature is the feature's own wrapper") {
  ChooseLALib::setLinearAlgebra(ChooseLALib::LinearAlgebraLibrary::Armadillo);
  OiJob job;
  auto p = namedPoint("P1");
  Point* point = p.get();
  const oi::Guarded<FeatureWrapper> wrapper = job.addFeature(std::move(p));
  REQUIRE_FALSE(wrapper.isNull());
  CHECK(wrapper.data() == point->getFeatureWrapper().data());
  CHECK(job.getFeatureById(point->getId()).data() == wrapper.data());
  CHECK(wrapper->getPoint().data() == point);
  CHECK(wrapper->getFeatureTypeEnum() == FeatureTypes::ePointFeature);
}

TEST_CASE("removing a feature destroys it and nulls every handle") {
  ChooseLALib::setLinearAlgebra(ChooseLALib::LinearAlgebraLibrary::Armadillo);
  OiJob job;
  auto p = namedPoint("P1");
  oi::Guarded<Point> point = p.get();
  const oi::Guarded<FeatureWrapper> wrapper = job.addFeature(std::move(p));
  const int id = point->getId();

  REQUIRE(job.removeFeature(id));
  CHECK(point.isNull());
  CHECK(wrapper.isNull());
  CHECK(job.getFeatureById(id).isNull());
  CHECK(job.getFeatureCount() == 0);
}

TEST_CASE("groups carry a typed wrapper of their own") {
  ChooseLALib::setLinearAlgebra(ChooseLALib::LinearAlgebraLibrary::Armadillo);
  Group group("plain");
  CHECK(group.getFeatureWrapper()->getFeatureTypeEnum() ==
        FeatureTypes::eGroupFeature);
  CHECK(group.getFeatureWrapper()->getGroup().data() == &group);

  PointGroup points("points");
  CHECK(points.getFeatureWrapper()->getFeatureTypeEnum() ==
        FeatureTypes::ePointGroupFeature);
  CHECK(points.getFeatureWrapper()->getPointGroup().data() == &points);
  CHECK(points.getFeatureWrapper()->getGroup().data() == &points);
}

TEST_CASE("removing a station takes its system's id entry with it") {
  ChooseLALib::setLinearAlgebra(ChooseLALib::LinearAlgebraLibrary::Armadillo);
  OiJob job;
  auto s = std::make_unique<Station>("S1");
  const oi::Guarded<Station> station = s.get();
  REQUIRE(job.addFeature(std::move(s)));
  const int systemId = station->getCoordinateSystem()->getId();
  // the station system is found by id through its own wrapper
  const oi::Guarded<FeatureWrapper> found = job.getFeatureById(systemId);
  REQUIRE_FALSE(found.isNull());
  CHECK(found->getCoordinateSystem().data() ==
        station->getCoordinateSystem().data());

  REQUIRE(job.removeFeature(station->getId()));
  CHECK(job.getFeatureById(systemId).isNull());
}

TEST_CASE(
    "removing a coordinate system removes its transformation parameters") {
  ChooseLALib::setLinearAlgebra(ChooseLALib::LinearAlgebraLibrary::Armadillo);
  OiJob job;
  auto s = std::make_unique<Station>("S1");
  Station* station = s.get();
  REQUIRE(job.addFeature(std::move(s)));
  auto c = std::make_unique<CoordinateSystem>();
  c->setFeatureName("PART");
  CoordinateSystem* part = c.get();
  REQUIRE(job.addFeature(std::move(c)));

  auto t = std::make_unique<TrafoParam>();
  t->setFeatureName("T1");
  REQUIRE(t->setCoordinateSystems(station->getCoordinateSystem(), part));
  oi::Guarded<TrafoParam> trafo = t.get();
  REQUIRE(job.addFeature(std::move(t)));
  // the systems register a transformation by id, which it only has now
  station->getCoordinateSystem()->addTransformationParameter(trafo);
  part->addTransformationParameter(trafo);
  const int count = job.getFeatureCount();

  REQUIRE(job.removeFeature(part->getId()));
  CHECK(trafo.isNull());
  CHECK(job.getFeatureCount() == count - 2);
}
