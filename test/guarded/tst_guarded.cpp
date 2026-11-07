#include <atomic>
#include <catch2/catch_test_macros.hpp>
#include <memory>
#include <thread>
#include <vector>

#include "guarded.h"

using oi::Guarded;
using oi::Trackable;

namespace {
struct Base : Trackable {
  int v = 1;
};
struct Derived : Base {
  int w = 2;
};
}  // namespace

TEST_CASE("Guarded is null by default and from nullptr") {
  Guarded<Base> g;
  CHECK(g.isNull());
  CHECK(g.data() == nullptr);
  CHECK_FALSE(static_cast<bool>(g));
  Guarded<Base> n(nullptr);
  CHECK(n.isNull());
}

TEST_CASE("Guarded follows the target and nulls on destruction") {
  auto* b = new Base;
  Guarded<Base> g(b);
  Guarded<Base> copy = g;
  CHECK_FALSE(g.isNull());
  CHECK(g.data() == b);
  CHECK(g->v == 1);
  CHECK((*g).v == 1);
  CHECK(static_cast<bool>(g));
  CHECK(g == b);
  CHECK(g == copy);
  delete b;
  CHECK(g.isNull());
  CHECK(copy.isNull());
  CHECK(g.data() == nullptr);
  CHECK_FALSE(static_cast<bool>(copy));
}

TEST_CASE("Guarded assignment and clear") {
  Base a, b;
  Guarded<Base> g;
  g = &a;
  CHECK(g == &a);
  g = &b;
  CHECK(g == &b);
  g = nullptr;
  CHECK(g.isNull());
  g = &a;
  g.clear();
  CHECK(g.isNull());
  CHECK(Guarded<Base>(&a) != Guarded<Base>(&b));
}

TEST_CASE("Guarded converts to base and from derived") {
  auto d = std::make_unique<Derived>();
  Guarded<Derived> gd(d.get());
  Guarded<Base> gb = gd;
  CHECK(gb.data() == static_cast<Base*>(d.get()));
  Guarded<Base> gb2(d.get());
  CHECK(gb2 == gb);
  d.reset();
  CHECK(gb.isNull());
  CHECK(gd.isNull());
  CHECK(gb2.isNull());
}

TEST_CASE("Copying a Trackable does not share identity") {
  Base a;
  Guarded<Base> g(&a);
  {
    Base copy(a);
    Guarded<Base> gc(&copy);
    CHECK(gc != g);
    copy = a;  // assignment keeps the identity
    CHECK(gc == &copy);
  }
  CHECK(g == &a);
}

TEST_CASE("Guarded outlives the target and handles many guards") {
  std::vector<Guarded<Base>> guards;
  auto* b = new Base;
  for (int i = 0; i < 100; ++i) guards.emplace_back(b);
  delete b;
  for (auto& g : guards) CHECK(g.isNull());
}

TEST_CASE("Destruction on one thread while another reads") {
  for (int round = 0; round < 50; ++round) {
    auto* b = new Base;
    Guarded<Base> g(b);
    std::atomic<bool> go{false};
    std::atomic<int> seenNull{0};
    std::thread reader([&] {
      while (!go) {
      }
      for (int i = 0; i < 2000; ++i) {
        Guarded<Base> local = g;  // concurrent copy of the control block
        if (local.isNull()) ++seenNull;
      }
    });
    go = true;
    delete b;
    reader.join();
    CHECK(g.isNull());
  }
}
