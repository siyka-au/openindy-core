#ifndef PLUGINMETADATA_H
#define PLUGINMETADATA_H

#include <string>
#include <vector>

#include "core_export.h"
#include "types.h"

namespace oi {

//! A file the plugin needs next to it: its name and where it is installed
//! ("app" for the application's own folder).
struct PluginDependency {
  std::string name;
  std::string path;
};

class OI_CORE_EXPORT PluginMetaData {
 public:
  std::string iid;
  std::string name;
  std::string pluginName;
  std::string pluginVersion;
  std::string author;
  std::string compiler;
  std::string operatingSystem;
  bool dependencies{};
  std::vector<PluginDependency> dependenciesPath;
  std::string description;
  std::string path;
  bool isValid{};
  std::string pluginType;
};

}  // namespace oi

#endif  // PLUGINMETADATA_H
