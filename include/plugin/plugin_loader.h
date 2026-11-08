#ifndef OI_CORE_PLUGIN_PLUGIN_LOADER_H_
#define OI_CORE_PLUGIN_PLUGIN_LOADER_H_

#include <string>

#include "core_export.h"
#include "plugin.h"

namespace oi {

/*!
 * \brief A plugin library that is loaded and has produced its Plugin factory.
 *
 * Owns both the factory and the library handle. Destroying it destroys the
 * factory inside the plugin (oi_plugin_destroy) and then unloads the library.
 * Everything the factory created (sensors, functions, ...) has code and vtables
 * in the library, so those objects must be destroyed BEFORE the LoadedPlugin.
 * A host that cannot guarantee that simply keeps the LoadedPlugin for the whole
 * process (the Qt application does).
 *
 * Move-only.
 */
class OI_CORE_EXPORT LoadedPlugin {
 public:
  LoadedPlugin() = default;
  LoadedPlugin(const LoadedPlugin&) = delete;
  LoadedPlugin& operator=(const LoadedPlugin&) = delete;
  LoadedPlugin(LoadedPlugin&& other) noexcept;
  LoadedPlugin& operator=(LoadedPlugin&& other) noexcept;
  ~LoadedPlugin();

  [[nodiscard]] Plugin* get() const noexcept { return plugin_; }
  Plugin* operator->() const noexcept { return plugin_; }
  [[nodiscard]] explicit operator bool() const noexcept {
    return plugin_ != nullptr;
  }

  [[nodiscard]] const std::string& path() const noexcept { return path_; }
  /// The ABI version the library reports (equals kPluginAbiVersion).
  [[nodiscard]] int abiVersion() const noexcept { return abiVersion_; }
  /// Text returned by the library's oi_plugin_metadata_json(); may be empty.
  [[nodiscard]] const std::string& metadataJson() const noexcept {
    return metadataJson_;
  }

 private:
  friend class PluginLoader;
  void reset() noexcept;

  void* handle_ = nullptr;
  Plugin* plugin_ = nullptr;
  void (*destroy_)(Plugin*) = nullptr;
  int abiVersion_ = 0;
  std::string path_;
  std::string metadataJson_;
};

/// Why PluginLoader::load did or did not produce a plugin.
enum class PluginLoadStatus {
  kOk,
  kCannotLoadLibrary,  ///< missing file, not a library, unresolved dependency
  kMissingEntryPoint,  ///< a library, but not an OpenIndy plugin
  kAbiMismatch,        ///< built against another plugin ABI version
  kToolchainMismatch,  ///< other C++ ABI / standard library / OS / architecture
  kFactoryFailed       ///< oi_plugin_create threw or returned null
};

/// Outcome of PluginLoader::load: a plugin, or the reason there is none.
struct PluginLoadResult {
  LoadedPlugin plugin;
  PluginLoadStatus status = PluginLoadStatus::kCannotLoadLibrary;
  std::string error;  ///< empty on success

  [[nodiscard]] explicit operator bool() const noexcept {
    return static_cast<bool>(plugin);
  }
};

/*!
 * \brief Loads a compute plugin (a shared library with the entry points of
 * OI_DECLARE_PLUGIN) with dlopen/LoadLibrary. Std-only; knows nothing of Qt.
 *
 * The library's ABI version is read and compared with kPluginAbiVersion
 * before oi_plugin_create is ever called, then its toolchain string (see
 * toolchain.h) is compared with the host's. Failures (missing file, not a
 * library, a missing entry point, an ABI or toolchain mismatch, a factory that
 * throws or returns null) come back as text in PluginLoadResult::error; load()
 * does not throw. A library that fails to load is unloaded again.
 */
class OI_CORE_EXPORT PluginLoader {
 public:
  [[nodiscard]] static PluginLoadResult load(const std::string& path);

 private:
  static PluginLoadResult loadImpl(const std::string& path);
};

}  // namespace oi

#endif  // OI_CORE_PLUGIN_PLUGIN_LOADER_H_
