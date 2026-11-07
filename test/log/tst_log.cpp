#include <catch2/catch_test_macros.hpp>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "log.h"

namespace {
struct Capture {
  std::vector<std::pair<oi::log::Level, std::string>> records;
  Capture() {
    oi::log::setLevel(oi::log::Level::kInfo);
    oi::log::setSink([this](oi::log::Level level, std::string_view text) {
      records.emplace_back(level, std::string(text));
    });
  }
  ~Capture() {
    oi::log::setSink({});
    oi::log::setLevel(oi::log::Level::kInfo);
  }
};
}  // namespace

TEST_CASE("records go to the sink, formatted") {
  Capture capture;
  oi::log::warn("plugin {} rejected: {}", "libx.so", "ABI 3");
  oi::log::error("plain");
  REQUIRE(capture.records.size() == 2);
  CHECK(capture.records[0].first == oi::log::Level::kWarn);
  CHECK(capture.records[0].second == "plugin libx.so rejected: ABI 3");
  CHECK(capture.records[1].first == oi::log::Level::kError);
}

TEST_CASE("the level filters, and kOff silences") {
  Capture capture;
  oi::log::debug("dropped");
  oi::log::info("kept");
  CHECK(capture.records.size() == 1);

  oi::log::setLevel(oi::log::Level::kDebug);
  oi::log::debug("now kept");
  CHECK(capture.records.size() == 2);
  CHECK(oi::log::level() == oi::log::Level::kDebug);

  oi::log::setLevel(oi::log::Level::kOff);
  oi::log::critical("silenced");
  CHECK(capture.records.size() == 2);
}

TEST_CASE("arguments are not formatted when the record is filtered") {
  Capture capture;
  int evaluated = 0;
  struct Counted {
    int* counter;
  };
  // std::format of a type with a formatter that counts
  oi::log::trace("{}", [&] {
    ++evaluated;
    return 1;
  }());
  // the argument expression is evaluated by the caller, but formatting is
  // skipped; nothing reaches the sink either way
  CHECK(capture.records.empty());
  CHECK(evaluated == 1);
}

TEST_CASE("logging from several threads reaches the sink completely") {
  std::mutex mutex;
  std::vector<std::string> seen;
  oi::log::setLevel(oi::log::Level::kInfo);
  oi::log::setSink([&](oi::log::Level, std::string_view text) {
    std::scoped_lock lock(mutex);
    seen.emplace_back(text);
  });
  std::vector<std::thread> threads;
  for (int t = 0; t < 4; ++t) {
    threads.emplace_back([t] {
      for (int i = 0; i < 50; ++i) oi::log::info("t{} i{}", t, i);
    });
  }
  for (auto& thread : threads) thread.join();
  oi::log::setSink({});
  CHECK(seen.size() == 200);
}

TEST_CASE("the default destination does not throw") {
  oi::log::setSink({});
  oi::log::setLevel(oi::log::Level::kOff);
  oi::log::error("not printed");
  CHECK(std::string(oi::log::levelName(oi::log::Level::kWarn)) == "warn");
}
