#ifndef OI_CORE_PLUGIN_FUNCTION_GENERATE_FEATURE_FUNCTION_H_
#define OI_CORE_PLUGIN_FUNCTION_GENERATE_FEATURE_FUNCTION_H_

#include "core_export.h"
#include "function.h"

namespace oi {

/*!
 * \brief The GenerateFeatureFunction class
 * Function that uses its input elements to generate one or more new feature
 */
class OI_CORE_EXPORT GenerateFeatureFunction : public Function {
 public:
  explicit GenerateFeatureFunction() : Function() {}

  ~GenerateFeatureFunction() override {}
};

}  // namespace oi

#include "plugin_abi.h"

inline const std::string GenerateFeatureFunction_iidd =
    oi::pluginIid("de.openIndy.plugin.function.generateFeatureFunction.v");

#endif  // OI_CORE_PLUGIN_FUNCTION_GENERATE_FEATURE_FUNCTION_H_
