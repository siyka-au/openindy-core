#ifndef OI_LOG_H_
#define OI_LOG_H_

#include <format>
#include <functional>
#include <string>
#include <string_view>
#include <utility>

#include "core_export.h"

/*!
 * \file log.h
 * \brief Diagnostic logging of the core, for developers and support; NOT the
 * user-visible job messages (those go through MessageSink).
 *
 * The facade is std-only: callers never see the backend (spdlog, inside the
 * core library). By default records go to stderr. A host redirects them with
 * oi::log::setSink() - the Qt application forwards them to its own log, the
 * command-line tool leaves the default - and filters with setLevel().
 *
 *     oi::log::warn("plugin {} rejected: {}", path, reason);
 *
 * Thread-safe. The sink may be called from any thread and must not log
 * itself.
 */
namespace oi::log {

enum class Level { kTrace, kDebug, kInfo, kWarn, kError, kCritical, kOff };

/// Receives every record that passes the level filter.
using Sink = std::function<void(Level, std::string_view)>;

/// Replaces the destination of all records; an empty function restores the
/// default (stderr).
OI_CORE_EXPORT void setSink(Sink sink);

/// Records below `level` are dropped. Default: kInfo.
OI_CORE_EXPORT void setLevel(Level level);
OI_CORE_EXPORT Level level();

OI_CORE_EXPORT void write(Level level, std::string_view message);

[[nodiscard]] OI_CORE_EXPORT const char* levelName(Level level);

template <typename... Args>
void log(Level level, std::format_string<Args...> fmt, Args&&... args) {
  if (level >= oi::log::level()) {
    write(level, std::format(fmt, std::forward<Args>(args)...));
  }
}

template <typename... Args>
void trace(std::format_string<Args...> fmt, Args&&... args) {
  log(Level::kTrace, fmt, std::forward<Args>(args)...);
}
template <typename... Args>
void debug(std::format_string<Args...> fmt, Args&&... args) {
  log(Level::kDebug, fmt, std::forward<Args>(args)...);
}
template <typename... Args>
void info(std::format_string<Args...> fmt, Args&&... args) {
  log(Level::kInfo, fmt, std::forward<Args>(args)...);
}
template <typename... Args>
void warn(std::format_string<Args...> fmt, Args&&... args) {
  log(Level::kWarn, fmt, std::forward<Args>(args)...);
}
template <typename... Args>
void error(std::format_string<Args...> fmt, Args&&... args) {
  log(Level::kError, fmt, std::forward<Args>(args)...);
}
template <typename... Args>
void critical(std::format_string<Args...> fmt, Args&&... args) {
  log(Level::kCritical, fmt, std::forward<Args>(args)...);
}

}  // namespace oi::log

#endif  // OI_LOG_H_
