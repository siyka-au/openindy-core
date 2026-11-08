#ifndef OI_CORE_PLUGIN_FUNCTION_GEODETIC_FUNCTION_H_
#define OI_CORE_PLUGIN_FUNCTION_GEODETIC_FUNCTION_H_

#include "core_export.h"
#include "function.h"

namespace oi {

/*!
 * \brief The GeodeticFunction class
 * for future use only
 */
class OI_CORE_EXPORT GeodeticFunction : public Function {
 public:
  explicit GeodeticFunction() : Function() {}

  ~GeodeticFunction() override {}
};

}  // namespace oi

#include "plugin_abi.h"

inline const std::string GeodeticFunction_iidd =
    oi::pluginIid("de.openIndy.plugin.function.geodeticFunction.v");

#endif  // OI_CORE_PLUGIN_FUNCTION_GEODETIC_FUNCTION_H_
