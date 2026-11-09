#include <catch2/catch_test_macros.hpp>
#include <cmath>

#include "choose_la_lib.h"
#include "function.h"

using namespace oi;

namespace {

OiVec vec3(double x, double y, double z) {
  OiVec v(3);
  v.setAt(0, x);
  v.setAt(1, y);
  v.setAt(2, z);
  return v;
}

// exposes the protected helper
class Rectifier : public FunctionUtil {
 public:
  void rectify(OiVec& normal, OiVec& direction) {
    this->rectifyNormalToDirection(normal, direction);
  }
};

}  // namespace

TEST_CASE("opposesDirection decides by the sign of the dot product") {
  ChooseLALib::setLinearAlgebra(ChooseLALib::LinearAlgebraLibrary::Armadillo);

  SECTION("ordinary inputs") {
    CHECK_FALSE(fit::opposesDirection(vec3(0, 0, 1), vec3(0, 0.3, 1)));
    CHECK(fit::opposesDirection(vec3(0, 0, 1), vec3(0, 0.3, -1)));
    CHECK_FALSE(fit::opposesDirection(vec3(0, 0, 1), vec3(1, 0, 0)));
  }
  SECTION("exactly antiparallel") {
    CHECK(fit::opposesDirection(vec3(0, 0, 1), vec3(0, 0, -1)));
  }
  SECTION("zero direction never flips") {
    CHECK_FALSE(fit::opposesDirection(vec3(0, 0, 1), vec3(0, 0, 0)));
  }
  SECTION("dot products that round past +-1 do not produce NaN") {
    // |a| is a hair over 1 so that a . b exceeds 1 (acos would be NaN)
    const double big = 1.0 + 4e-16;
    CHECK_FALSE(fit::opposesDirection(vec3(0, 0, big), vec3(0, 0, 1)));
    CHECK(fit::opposesDirection(vec3(0, 0, big), vec3(0, 0, -1)));
    CHECK(fit::opposesDirection(vec3(0, 0, 1), vec3(0, 0, -big)));
  }
}

TEST_CASE("rectifyNormalToDirection flips only against the direction") {
  ChooseLALib::setLinearAlgebra(ChooseLALib::LinearAlgebraLibrary::Armadillo);
  Rectifier rectifier;

  SECTION("nearly parallel is kept") {
    OiVec n = vec3(0, 0, 1);
    OiVec d = vec3(1e-9, 0, 1);
    rectifier.rectify(n, d);
    CHECK(n.getAt(2) > 0.0);
  }
  SECTION("nearly antiparallel is flipped") {
    OiVec n = vec3(0, 0, 1);
    OiVec d = vec3(1e-9, 0, -1);
    rectifier.rectify(n, d);
    CHECK(n.getAt(2) < 0.0);
  }
  SECTION("exactly antiparallel is flipped") {
    OiVec n = vec3(0, 0, 1);
    OiVec d = vec3(0, 0, -1);
    rectifier.rectify(n, d);
    CHECK(n.getAt(2) == -1.0);
  }
  SECTION("unnormalised inputs whose dot product rounds past -1 are flipped") {
    OiVec n = vec3(0, 0, 1.0 + 4e-16);
    OiVec d = vec3(0, 0, -1.0);
    rectifier.rectify(n, d);
    CHECK(n.getAt(2) < 0.0);
  }
}
