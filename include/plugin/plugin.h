#ifndef PLUGIN_H
#define PLUGIN_H
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

  virtual oi::Guarded<Sensor> createSensor(const QString& name) = 0;
  virtual oi::Guarded<Function> createFunction(const QString& name) = 0;
  virtual oi::Guarded<BundleAdjustment> createBundleAdjustment(
      const QString& name) = 0;
  virtual oi::Guarded<SimulationModel> createSimulation(
      const QString& name) = 0;
  virtual oi::Guarded<ExchangeSimpleAscii> createSimpleAsciiExchange(
      const QString& name) = 0;
  virtual oi::Guarded<ExchangeDefinedFormat> createDefinedFormatExchange(
      const QString& name) = 0;
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

#define PLUGIN_PREFIX de.openIndy.plugin.v
#define Plugin_iidd STR2(CONCAT2(PLUGIN_PREFIX, OI_PLUGIN_ABI_VERSION))

using namespace oi;
Q_DECLARE_INTERFACE(Plugin, Plugin_iidd)

#endif  // PLUGIN_H
