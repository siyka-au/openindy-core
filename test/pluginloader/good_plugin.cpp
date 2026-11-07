// A minimal, valid plugin used by tst_pluginloader.
#include "plugin.h"

namespace {
struct TestPlugin : oi::Plugin {
  std::vector<oi::Guarded<oi::Sensor> > createSensors() override { return {}; }
  std::vector<oi::Guarded<oi::Function> > createFunctions() override {
    return {};
  }
  std::vector<oi::Guarded<oi::BundleAdjustment> > createBundleAdjustments()
      override {
    return {};
  }
  std::vector<oi::Guarded<oi::SimulationModel> > createSimulations() override {
    return {};
  }
  std::vector<oi::Guarded<oi::ExchangeSimpleAscii> >
  createSimpleAsciiExchanges() override {
    return {};
  }
  std::vector<oi::Guarded<oi::ExchangeDefinedFormat> >
  createDefinedFormatExchanges() override {
    return {};
  }
  oi::Guarded<oi::Sensor> createSensor(const std::string&) override {
    return {};
  }
  oi::Guarded<oi::Function> createFunction(const std::string&) override {
    return {};
  }
  oi::Guarded<oi::BundleAdjustment> createBundleAdjustment(
      const std::string&) override {
    return {};
  }
  oi::Guarded<oi::SimulationModel> createSimulation(
      const std::string&) override {
    return {};
  }
  oi::Guarded<oi::ExchangeSimpleAscii> createSimpleAsciiExchange(
      const std::string&) override {
    return {};
  }
  oi::Guarded<oi::ExchangeDefinedFormat> createDefinedFormatExchange(
      const std::string&) override {
    return {};
  }
};
}  // namespace

OI_DECLARE_PLUGIN_WITH_METADATA(TestPlugin, R"({"name": "test plugin"})")
