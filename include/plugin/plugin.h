#ifndef OI_CORE_PLUGIN_PLUGIN_H_
#define OI_CORE_PLUGIN_PLUGIN_H_
#include <memory>
#include <string>
#include <vector>

#include "bundle_adjustment.h"
#include "exchange_defined_format.h"
#include "exchange_simple_ascii.h"
#include "function.h"
#include "guarded.h"
#include "sensor.h"
#include "simulation_model.h"

namespace oi {

/*!
 * \brief The Plugin class
 */
class Plugin {
 public:
  virtual ~Plugin() {}

  // ##########################################################
  // create instances of all available plugin types in a plugin
  // ##########################################################

  virtual std::vector<std::unique_ptr<Sensor>> createSensors() = 0;
  virtual std::vector<std::unique_ptr<Function>> createFunctions() = 0;
  virtual std::vector<std::unique_ptr<BundleAdjustment>>
  createBundleAdjustments() = 0;
  virtual std::vector<std::unique_ptr<SimulationModel>> createSimulations() = 0;
  virtual std::vector<std::unique_ptr<ExchangeSimpleAscii>>
  createSimpleAsciiExchanges() = 0;
  virtual std::vector<std::unique_ptr<ExchangeDefinedFormat>>
  createDefinedFormatExchanges() = 0;

  // ###################################################
  // create instances of a plugin type with a given name
  // ###################################################

  virtual std::unique_ptr<Sensor> createSensor(const std::string& name) = 0;
  virtual std::unique_ptr<Function> createFunction(const std::string& name) = 0;
  virtual std::unique_ptr<BundleAdjustment> createBundleAdjustment(
      const std::string& name) = 0;
  virtual std::unique_ptr<SimulationModel> createSimulation(
      const std::string& name) = 0;
  virtual std::unique_ptr<ExchangeSimpleAscii> createSimpleAsciiExchange(
      const std::string& name) = 0;
  virtual std::unique_ptr<ExchangeDefinedFormat> createDefinedFormatExchange(
      const std::string& name) = 0;
};

}  // namespace oi

#include "plugin_abi.h"
#include "toolchain.h"

// Identifier of the plugin interface revision, keyed off the ABI version. It
// is no longer used for loading (the entry point checks the version number),
// but the app's plugin database records it.
inline const std::string Plugin_iidd = oi::pluginIid("de.openIndy.plugin.v");

// Plugin sources and the application headers have always found oi:: names
// through this header.
using namespace oi;

// Plugin entry points
// -------------------
// A plugin library exports these `extern "C"` functions (use
// OI_DECLARE_PLUGIN; do not write them by hand):
//
//   int          oi_plugin_abi_version();    // kPluginAbiVersion it was
//                                            // built with
//   const char*  oi_plugin_toolchain();      // oi::kToolchain, see toolchain.h
//   oi::Plugin*  oi_plugin_create();         // new factory, owned by the
//   caller void         oi_plugin_destroy(oi::Plugin*);  // frees it inside the
//   plugin const char*  oi_plugin_metadata_json();  // optional: static JSON
//   text
//
// The loader (oi::PluginLoader) checks the ABI version, then the toolchain
// string, before it calls oi_plugin_create, so a plugin built against another
// interface revision or another C++ ABI / standard library / OS / architecture
// is rejected instead of being called through a mismatching vtable.
#if defined(_WIN32) || defined(__CYGWIN__)
#define OI_PLUGIN_EXPORT __declspec(dllexport)
#else
#define OI_PLUGIN_EXPORT __attribute__((visibility("default")))
#endif

inline constexpr const char* kPluginSymbolAbiVersion = "oi_plugin_abi_version";
inline constexpr const char* kPluginSymbolToolchain = "oi_plugin_toolchain";
inline constexpr const char* kPluginSymbolCreate = "oi_plugin_create";
inline constexpr const char* kPluginSymbolDestroy = "oi_plugin_destroy";
inline constexpr const char* kPluginSymbolMetadata = "oi_plugin_metadata_json";

/*!
 * Declares the entry points of a plugin library. `ClassName` must derive from
 * oi::Plugin and be default constructible. Use once, in one .cpp of the
 * plugin. `metadataJson` is a string literal (or any `const char*` with static
 * storage) returned by oi_plugin_metadata_json(); the host reads the plugin's
 * name, version, compiler and so on from it.
 */
// The entry points must be extern "C" definitions with these exact names in the
// plugin's own translation unit, so a plugin author needs a macro (a template
// or function cannot define an exported extern "C" symbol).
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define OI_DECLARE_PLUGIN_WITH_METADATA(ClassName, metadataJson)             \
  extern "C" {                                                               \
  OI_PLUGIN_EXPORT int oi_plugin_abi_version() { return kPluginAbiVersion; } \
  OI_PLUGIN_EXPORT const char* oi_plugin_toolchain() {                       \
    return oi::kToolchain;                                                   \
  }                                                                          \
  /* the C ABI cannot carry a unique_ptr: ownership passes to the loader, */ \
  /* which hands the factory back to oi_plugin_destroy */                    \
  OI_PLUGIN_EXPORT oi::Plugin* oi_plugin_create() {                          \
    return std::make_unique<ClassName>().release();                          \
  }                                                                          \
  OI_PLUGIN_EXPORT void oi_plugin_destroy(oi::Plugin* plugin) {              \
    const std::unique_ptr<oi::Plugin> owner(plugin);                         \
  }                                                                          \
  OI_PLUGIN_EXPORT const char* oi_plugin_metadata_json() {                   \
    return metadataJson;                                                     \
  }                                                                          \
  }

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage): see the macro above
#define OI_DECLARE_PLUGIN(ClassName) \
  OI_DECLARE_PLUGIN_WITH_METADATA(ClassName, "")

#endif  // OI_CORE_PLUGIN_PLUGIN_H_
