// Test that Position, Direction and Radius are real value types:
// copyable, movable, independent after copy, and carrying no Qt base class.
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <type_traits>

#include "choose_la_lib.h"
#include "direction.h"
#include "position.h"
#include "radius.h"

using namespace oi;

namespace {

static_assert(std::is_copy_constructible_v<Position>);
static_assert(std::is_move_constructible_v<Position>);
static_assert(std::is_copy_constructible_v<Direction>);
static_assert(std::is_move_constructible_v<Direction>);
static_assert(std::is_trivially_copyable_v<Radius>);
static_assert(std::is_nothrow_move_constructible_v<Radius>);

bool near(double a, double b) { return std::fabs(a - b) < 1e-12; }

TEST_CASE("positionCopyIsIndependent") {
  math::ChooseLALib::setLinearAlgebra(
      math::ChooseLALib::LinearAlgebraLibrary::Armadillo);
  Position a(1.0, 2.0, 3.0);
  Position b = a;
  b.setVector(9.0, 8.0, 7.0);
  CHECK(near(a.getVector().getAt(0), 1.0));
  CHECK(near(b.getVector().getAt(0), 9.0));
  CHECK(near(a.getVectorH().getAt(3), 1.0));
}

TEST_CASE("positionHomogeneousForms") {
  math::ChooseLALib::setLinearAlgebra(
      math::ChooseLALib::LinearAlgebraLibrary::Armadillo);
  Position p(2.0, 4.0, 6.0, 2.0);  // (x, y, z, h) -> cartesian divides by h
  CHECK(near(p.getVector().getAt(0), 1.0));
  CHECK(near(p.getVector().getAt(2), 3.0));
  CHECK(near(p.getVectorH().getAt(3), 2.0));

  OiVec v4(4);
  v4.setAt(0, 4.0);
  v4.setAt(1, 4.0);
  v4.setAt(2, 4.0);
  v4.setAt(3, 2.0);
  Position q(v4);
  CHECK(near(q.getVector().getAt(1), 2.0));
}

TEST_CASE("positionNullObject") {
  math::ChooseLALib::setLinearAlgebra(
      math::ChooseLALib::LinearAlgebraLibrary::Armadillo);
  CHECK(Position::NullObject.isNull());
  CHECK(!Position().isNull());
  CHECK(!Position(1.0, 2.0, 3.0).isNull());
  Position copy = Position::NullObject;  // null-ness is part of the value
  CHECK(copy.isNull());
}

TEST_CASE("positionMoveLeavesTargetUsable") {
  math::ChooseLALib::setLinearAlgebra(
      math::ChooseLALib::LinearAlgebraLibrary::Armadillo);
  Position a(1.0, 2.0, 3.0);
  Position b = std::move(a);
  CHECK(near(b.getVector().getAt(2), 3.0));
}

TEST_CASE("directionBehavesLikePosition") {
  math::ChooseLALib::setLinearAlgebra(
      math::ChooseLALib::LinearAlgebraLibrary::Armadillo);
  Direction d(0.0, 0.0, 1.0);
  Direction e = d;
  e.setVector(1.0, 0.0, 0.0);
  CHECK(near(d.getVector().getAt(2), 1.0));
  CHECK(near(e.getVector().getAt(0), 1.0));
  CHECK(near(Direction().getVectorH().getAt(3), 1.0));
}

TEST_CASE("radiusIsAPlainNumber") {
  math::ChooseLALib::setLinearAlgebra(
      math::ChooseLALib::LinearAlgebraLibrary::Armadillo);
  Radius r(2.5);
  Radius s = r;
  s.setRadius(7.0);
  CHECK(near(r.getRadius(), 2.5));
  CHECK(near(s.getRadius(), 7.0));
  CHECK(near(Radius().getRadius(), 0.0));
}

}  // namespace
