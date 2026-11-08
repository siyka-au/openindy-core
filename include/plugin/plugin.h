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

#ifndef STR
#define STR(x) #x
#endif
#ifndef STR2
#define STR2(x) STR(x)
#endif
#ifndef CONCAT
#define CONCAT(a, b) a##b
#endif
#ifndef CONCAT2
#define CONCAT2(a, b) CONCAT(a, b)
#endif

#include "plugin_abi.h"
#include "toolchain.h"

// Identifier of the plugin interface revision, keyed off the ABI version. It
// is no longer used for loading (the entry point checks the version number),
// but the app's plugin database records it.
#define PLUGIN_PREFIX de.openIndy.plugin.v
#define Plugin_iidd STR2(CONCAT2(PLUGIN_PREFIX, OI_PLUGIN_ABI_VERSION))

// Plugin sources and the application headers have always found oi:: names
// through this header.
using namespace oi;

// Plugin entry points
// -------------------
// A plugin library exports these `extern "C"` functions (use
// OI_DECLARE_PLUGIN; do not write them by hand):
//
//   int          oi_plugin_abi_version();    // OI_PLUGIN_ABI_VERSION it was
//                                            // built with
//   const char*  oi_plugin_toolchain();      // OI_TOOLCHAIN, see toolchain.h
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

#define OI_PLUGIN_SYMBOL_ABI_VERSION "oi_plugin_abi_version"
#define OI_PLUGIN_SYMBOL_TOOLCHAIN "oi_plugin_toolchain"
#define OI_PLUGIN_SYMBOL_CREATE "oi_plugin_create"
#define OI_PLUGIN_SYMBOL_DESTROY "oi_plugin_destroy"
#define OI_PLUGIN_SYMBOL_METADATA "oi_plugin_metadata_json"

/*!
 * Declares the entry points of a plugin library. `ClassName` must derive from
 * oi::Plugin and be default constructible. Use once, in one .cpp of the
 * plugin. `metadataJson` is a string literal (or any `const char*` with static
 * storage) returned by oi_plugin_metadata_json(); the host reads the plugin's
 * name, version, compiler and so on from it.
 */
#define OI_DECLARE_PLUGIN_WITH_METADATA(ClassName, metadataJson)              \
  extern "C" {                                                                \
  OI_PLUGIN_EXPORT int oi_plugin_abi_version() {                              \
    return OI_PLUGIN_ABI_VERSION;                                             \
  }                                                                           \
  OI_PLUGIN_EXPORT const char* oi_plugin_toolchain() { return OI_TOOLCHAIN; } \
  /* the C ABI cannot carry a unique_ptr: ownership passes to the loader, */  \
  /* which hands the factory back to oi_plugin_destroy */                     \
  OI_PLUGIN_EXPORT oi::Plugin* oi_plugin_create() {                           \
    return std::make_unique<ClassName>().release();                           \
  }                                                                           \
  OI_PLUGIN_EXPORT void oi_plugin_destroy(oi::Plugin* plugin) {               \
    const std::unique_ptr<oi::Plugin> owner(plugin);                          \
  }                                                                           \
  OI_PLUGIN_EXPORT const char* oi_plugin_metadata_json() {                    \
    return metadataJson;                                                      \
  }                                                                           \
  }

#define OI_DECLARE_PLUGIN(ClassName) \
  OI_DECLARE_PLUGIN_WITH_METADATA(ClassName, "")

#endif  // OI_CORE_PLUGIN_PLUGIN_H_
