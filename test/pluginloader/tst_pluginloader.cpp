#include <catch2/catch_test_macros.hpp>
#include <string>
#include <utility>

#include "pluginloader.h"

TEST_CASE("PluginLoader loads a valid plugin") {
  auto result = oi::PluginLoader::load(OI_TEST_GOOD_PLUGIN);
  REQUIRE(result);
  CHECK(result.status == oi::PluginLoadStatus::kOk);
  CHECK(result.error.empty());
  CHECK(result.plugin.abiVersion() == OI_PLUGIN_ABI_VERSION);
  CHECK(result.plugin.path() == OI_TEST_GOOD_PLUGIN);
  CHECK(result.plugin.metadataJson() == R"({"name": "test plugin"})");
  CHECK(result.plugin->createSensors().empty());
  CHECK(result.plugin->createFunction("x") == nullptr);
}

TEST_CASE("LoadedPlugin is movable and the moved-from one is empty") {
  auto result = oi::PluginLoader::load(OI_TEST_GOOD_PLUGIN);
  REQUIRE(result);
  oi::LoadedPlugin a = std::move(result.plugin);
  CHECK(a);
  CHECK_FALSE(result.plugin);
  oi::LoadedPlugin b;
  b = std::move(a);
  CHECK(b);
  CHECK_FALSE(a);
  CHECK(b->createFunctions().empty());
}

TEST_CASE("the same plugin can be loaded twice") {
  auto first = oi::PluginLoader::load(OI_TEST_GOOD_PLUGIN);
  auto second = oi::PluginLoader::load(OI_TEST_GOOD_PLUGIN);
  REQUIRE(first);
  REQUIRE(second);
  CHECK(first.plugin.get() != second.plugin.get());
}

TEST_CASE("PluginLoader rejects an ABI mismatch before creating anything") {
  auto result = oi::PluginLoader::load(OI_TEST_WRONG_ABI_PLUGIN);
  CHECK_FALSE(result);
  CHECK(result.status == oi::PluginLoadStatus::kAbiMismatch);
  CHECK(result.error.find("ABI " + std::to_string(OI_PLUGIN_ABI_VERSION + 1)) !=
        std::string::npos);
  CHECK(
      result.error.find("requires " + std::to_string(OI_PLUGIN_ABI_VERSION)) !=
      std::string::npos);
}

TEST_CASE("PluginLoader reports a library without entry points") {
  auto result = oi::PluginLoader::load(OI_TEST_NO_ENTRY_PLUGIN);
  CHECK_FALSE(result);
  CHECK(result.status == oi::PluginLoadStatus::kMissingEntryPoint);
  CHECK(result.error.find("missing entry point") != std::string::npos);
  CHECK(result.error.find(OI_PLUGIN_SYMBOL_ABI_VERSION) != std::string::npos);
}

TEST_CASE("PluginLoader reports a missing file") {
  auto result = oi::PluginLoader::load("/nonexistent/dir/libnothing.so");
  CHECK_FALSE(result);
  CHECK(result.status == oi::PluginLoadStatus::kCannotLoadLibrary);
  CHECK(result.error.find("/nonexistent/dir/libnothing.so") !=
        std::string::npos);
  CHECK(result.error.find("cannot load library") != std::string::npos);
}

TEST_CASE("PluginLoader reports a file that is not a library") {
  auto result = oi::PluginLoader::load(OI_TEST_NOT_A_LIBRARY);
  CHECK_FALSE(result);
  CHECK_FALSE(result.error.empty());
}
