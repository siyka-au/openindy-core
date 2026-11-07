#ifndef PLUGIN_H
#define PLUGIN_H
#include <string>
#include <vector>

#include "bundleadjustment.h"
#include "exchangedefinedformat.h"
#include "exchangesimpleascii.h"
#include "function.h"
#include "guarded.h"
#include "sensor.h"
#include "simulationmodel.h"

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

  virtual std::vector<oi::Guarded<Sensor> > createSensors() = 0;
  virtual std::vector<oi::Guarded<Function> > createFunctions() = 0;
  virtual std::vector<oi::Guarded<BundleAdjustment> >
  createBundleAdjustments() = 0;
  virtual std::vector<oi::Guarded<SimulationModel> > createSimulations() = 0;
  virtual std::vector<oi::Guarded<ExchangeSimpleAscii> >
  createSimpleAsciiExchanges() = 0;
  virtual std::vector<oi::Guarded<ExchangeDefinedFormat> >
  createDefinedFormatExchanges() = 0;

  // ###################################################
  // create instances of a plugin type with a given name
  // ###################################################

  virtual oi::Guarded<Sensor> createSensor(const std::string& name) = 0;
  virtual oi::Guarded<Function> createFunction(const std::string& name) = 0;
  virtual oi::Guarded<BundleAdjustment> createBundleAdjustment(
      const std::string& name) = 0;
  virtual oi::Guarded<SimulationModel> createSimulation(
      const std::string& name) = 0;
  virtual oi::Guarded<ExchangeSimpleAscii> createSimpleAsciiExchange(
      const std::string& name) = 0;
  virtual oi::Guarded<ExchangeDefinedFormat> createDefinedFormatExchange(
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

#include "pluginabi.h"

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
//   built with oi::Plugin*  oi_plugin_create();         // new factory, owned
//   by the caller void         oi_plugin_destroy(oi::Plugin*);  // frees it
//   inside the plugin const char*  oi_plugin_metadata_json();  // optional:
//   static JSON text
//
// The loader (oi::PluginLoader) checks the ABI version before it calls
// oi_plugin_create, so a plugin built against another interface revision is
// rejected instead of being called through a mismatching vtable.
#if defined(_WIN32) || defined(__CYGWIN__)
#define OI_PLUGIN_EXPORT __declspec(dllexport)
#else
#define OI_PLUGIN_EXPORT __attribute__((visibility("default")))
#endif

#define OI_PLUGIN_SYMBOL_ABI_VERSION "oi_plugin_abi_version"
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
  OI_PLUGIN_EXPORT oi::Plugin* oi_plugin_create() { return new ClassName(); } \
  OI_PLUGIN_EXPORT void oi_plugin_destroy(oi::Plugin* plugin) {               \
    delete plugin;                                                            \
  }                                                                           \
  OI_PLUGIN_EXPORT const char* oi_plugin_metadata_json() {                    \
    return metadataJson;                                                      \
  }                                                                           \
  }

#define OI_DECLARE_PLUGIN(ClassName) \
  OI_DECLARE_PLUGIN_WITH_METADATA(ClassName, "")

#endif  // PLUGIN_H
