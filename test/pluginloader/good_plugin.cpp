// A minimal, valid plugin used by tst_pluginloader.
#include "plugin.h"

namespace {
struct TestPlugin : oi::Plugin {
  std::vector<std::unique_ptr<oi::Sensor>> createSensors() override {
    return {};
  }
  std::vector<std::unique_ptr<oi::Function>> createFunctions() override {
    return {};
  }
  std::vector<std::unique_ptr<oi::BundleAdjustment>> createBundleAdjustments()
      override {
    return {};
  }
  std::vector<std::unique_ptr<oi::SimulationModel>> createSimulations()
      override {
    return {};
  }
  std::vector<std::unique_ptr<oi::ExchangeSimpleAscii>>
  createSimpleAsciiExchanges() override {
    return {};
  }
  std::vector<std::unique_ptr<oi::ExchangeDefinedFormat>>
  createDefinedFormatExchanges() override {
    return {};
  }
  std::unique_ptr<oi::Sensor> createSensor(const std::string&) override {
    return {};
  }
  std::unique_ptr<oi::Function> createFunction(const std::string&) override {
    return {};
  }
  std::unique_ptr<oi::BundleAdjustment> createBundleAdjustment(
      const std::string&) override {
    return {};
  }
  std::unique_ptr<oi::SimulationModel> createSimulation(
      const std::string&) override {
    return {};
  }
  std::unique_ptr<oi::ExchangeSimpleAscii> createSimpleAsciiExchange(
      const std::string&) override {
    return {};
  }
  std::unique_ptr<oi::ExchangeDefinedFormat> createDefinedFormatExchange(
      const std::string&) override {
    return {};
  }
};
}  // namespace

OI_DECLARE_PLUGIN_WITH_METADATA(TestPlugin, R"({"name": "test plugin"})")
