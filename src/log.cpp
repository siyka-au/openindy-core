#include "log.h"

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <atomic>
#include <memory>
#include <mutex>

namespace oi::log {

namespace {

std::mutex& sinkMutex() {
  static std::mutex mutex;
  return mutex;
}

Sink& customSink() {
  static Sink sink;
  return sink;
}

std::atomic<Level>& currentLevel() {
  static std::atomic<Level> level{Level::kInfo};
  return level;
}

// the default destination: a spdlog logger on stderr
spdlog::logger& stderrLogger() {
  static auto logger = [] {
    auto created = std::make_shared<spdlog::logger>(
        "openindy", std::make_shared<spdlog::sinks::stderr_color_sink_mt>());
    created->set_level(spdlog::level::trace);  // filtering is done above
    created->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] %v");
    return created;
  }();
  return *logger;
}

spdlog::level::level_enum toSpdlog(Level level) {
  switch (level) {
    case Level::kTrace:
      return spdlog::level::trace;
    case Level::kDebug:
      return spdlog::level::debug;
    case Level::kInfo:
      return spdlog::level::info;
    case Level::kWarn:
      return spdlog::level::warn;
    case Level::kError:
      return spdlog::level::err;
    case Level::kCritical:
      return spdlog::level::critical;
    case Level::kOff:
      return spdlog::level::off;
  }
  return spdlog::level::info;
}

}  // namespace

void setSink(Sink sink) {
  const std::scoped_lock lock(sinkMutex());
  customSink() = std::move(sink);
}

void setLevel(Level level) { currentLevel().store(level); }

Level level() { return currentLevel().load(); }

const char* levelName(Level level) {
  switch (level) {
    case Level::kTrace:
      return "trace";
    case Level::kDebug:
      return "debug";
    case Level::kInfo:
      return "info";
    case Level::kWarn:
      return "warn";
    case Level::kError:
      return "error";
    case Level::kCritical:
      return "critical";
    case Level::kOff:
      return "off";
  }
  return "?";
}

void write(Level level, std::string_view message) {
  if (level < currentLevel().load() || level == Level::kOff) {
    return;
  }
  Sink sink;
  {
    const std::scoped_lock lock(sinkMutex());
    sink = customSink();
  }
  if (sink) {
    sink(level, message);
    return;
  }
  stderrLogger().log(toSpdlog(level), "{}", message);
}

}  // namespace oi::log
