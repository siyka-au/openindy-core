#include "pluginloader.h"

#include <exception>
#include <utility>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <dlfcn.h>
#endif

namespace oi {

namespace {

void* openLibrary(const std::string& path, std::string& error) {
#if defined(_WIN32)
  HMODULE h = LoadLibraryA(path.c_str());
  if (!h) {
    error = "LoadLibrary failed with error " + std::to_string(GetLastError());
  }
  return reinterpret_cast<void*>(h);
#else
  void* h = dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
  if (!h) {
    const char* msg = dlerror();
    error = msg ? msg : "dlopen failed";
  }
  return h;
#endif
}

void* findSymbol(void* handle, const char* name) {
#if defined(_WIN32)
  return reinterpret_cast<void*>(
      GetProcAddress(reinterpret_cast<HMODULE>(handle), name));
#else
  return dlsym(handle, name);
#endif
}

void closeLibrary(void* handle) {
  if (!handle) {
    return;
  }
#if defined(_WIN32)
  FreeLibrary(reinterpret_cast<HMODULE>(handle));
#else
  dlclose(handle);
#endif
}

template <typename Fn>
Fn symbolAs(void* handle, const char* name) {
  return reinterpret_cast<Fn>(findSymbol(handle, name));
}

}  // namespace

LoadedPlugin::LoadedPlugin(LoadedPlugin&& other) noexcept
    : handle_(std::exchange(other.handle_, nullptr)),
      plugin_(std::exchange(other.plugin_, nullptr)),
      destroy_(std::exchange(other.destroy_, nullptr)),
      abiVersion_(std::exchange(other.abiVersion_, 0)),
      path_(std::move(other.path_)),
      metadataJson_(std::move(other.metadataJson_)) {}

LoadedPlugin& LoadedPlugin::operator=(LoadedPlugin&& other) noexcept {
  if (this != &other) {
    reset();
    handle_ = std::exchange(other.handle_, nullptr);
    plugin_ = std::exchange(other.plugin_, nullptr);
    destroy_ = std::exchange(other.destroy_, nullptr);
    abiVersion_ = std::exchange(other.abiVersion_, 0);
    path_ = std::move(other.path_);
    metadataJson_ = std::move(other.metadataJson_);
  }
  return *this;
}

LoadedPlugin::~LoadedPlugin() { reset(); }

void LoadedPlugin::reset() noexcept {
  if (plugin_ && destroy_) {
    destroy_(plugin_);
  }
  plugin_ = nullptr;
  destroy_ = nullptr;
  closeLibrary(handle_);
  handle_ = nullptr;
}

PluginLoadResult PluginLoader::load(const std::string& path) {
  PluginLoadResult result;
  const std::string where = path + ": ";

  std::string osError;
  void* handle = openLibrary(path, osError);
  if (!handle) {
    result.status = PluginLoadStatus::kCannotLoadLibrary;
    result.error = where + "cannot load library: " + osError;
    return result;
  }

  using AbiFn = int (*)();
  using CreateFn = Plugin* (*)();
  using DestroyFn = void (*)(Plugin*);
  using MetaFn = const char* (*)();

  const auto abi = symbolAs<AbiFn>(handle, OI_PLUGIN_SYMBOL_ABI_VERSION);
  const auto create = symbolAs<CreateFn>(handle, OI_PLUGIN_SYMBOL_CREATE);
  const auto destroy = symbolAs<DestroyFn>(handle, OI_PLUGIN_SYMBOL_DESTROY);
  if (!abi || !create || !destroy) {
    closeLibrary(handle);
    result.status = PluginLoadStatus::kMissingEntryPoint;
    result.error = where + "not an OpenIndy plugin (missing entry point " +
                   (!abi      ? OI_PLUGIN_SYMBOL_ABI_VERSION
                    : !create ? OI_PLUGIN_SYMBOL_CREATE
                              : OI_PLUGIN_SYMBOL_DESTROY) +
                   ")";
    return result;
  }

  const int pluginAbi = abi();
  if (pluginAbi != OI_PLUGIN_ABI_VERSION) {
    closeLibrary(handle);
    result.status = PluginLoadStatus::kAbiMismatch;
    result.error = where + "built against plugin ABI " +
                   std::to_string(pluginAbi) + ", this build requires " +
                   std::to_string(OI_PLUGIN_ABI_VERSION) +
                   ". Rebuild the plugin against the current core.";
    return result;
  }

  Plugin* plugin = nullptr;
  try {
    plugin = create();
  } catch (const std::exception& e) {
    closeLibrary(handle);
    result.status = PluginLoadStatus::kFactoryFailed;
    result.error = where + "plugin factory threw: " + e.what();
    return result;
  } catch (...) {
    closeLibrary(handle);
    result.status = PluginLoadStatus::kFactoryFailed;
    result.error = where + "plugin factory threw an unknown exception";
    return result;
  }
  if (!plugin) {
    closeLibrary(handle);
    result.status = PluginLoadStatus::kFactoryFailed;
    result.error = where + "plugin factory returned null";
    return result;
  }

  result.status = PluginLoadStatus::kOk;
  LoadedPlugin& loaded = result.plugin;
  loaded.handle_ = handle;
  loaded.plugin_ = plugin;
  loaded.destroy_ = destroy;
  loaded.abiVersion_ = pluginAbi;
  loaded.path_ = path;
  if (const auto meta = symbolAs<MetaFn>(handle, OI_PLUGIN_SYMBOL_METADATA)) {
    if (const char* json = meta()) {
      loaded.metadataJson_ = json;
    }
  }
  return result;
}

}  // namespace oi
