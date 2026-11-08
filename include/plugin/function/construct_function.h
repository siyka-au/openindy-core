#ifndef OI_CORE_PLUGIN_FUNCTION_CONSTRUCT_FUNCTION_H_
#define OI_CORE_PLUGIN_FUNCTION_CONSTRUCT_FUNCTION_H_

#include "core_export.h"
#include "function.h"

namespace oi {

/*!
 * \brief The ConstructFunction class
 * Function that solves geometries by constructing them from other elements
 */
class OI_CORE_EXPORT ConstructFunction : public Function {
 public:
  explicit ConstructFunction() : Function() {}

  ~ConstructFunction() override {}
};

}  // namespace oi

#include "plugin_abi.h"

inline const std::string ConstructFunction_iidd =
    oi::pluginIid("de.openIndy.plugin.function.constructFunction.v");

#endif  // OI_CORE_PLUGIN_FUNCTION_CONSTRUCT_FUNCTION_H_
