// Reports an ABI version other than the host's; the loader must reject it
// without calling oi_plugin_create (which here would abort).
#include <cstdlib>

#include "plugin.h"

extern "C" {
OI_PLUGIN_EXPORT int oi_plugin_abi_version() {
  return OI_PLUGIN_ABI_VERSION + 1;
}
OI_PLUGIN_EXPORT oi::Plugin* oi_plugin_create() { std::abort(); }
OI_PLUGIN_EXPORT void oi_plugin_destroy(oi::Plugin*) {}
}
