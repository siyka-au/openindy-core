#ifndef OI_CORE_PLUGIN_FUNCTION_SPECIAL_FUNCTION_H_
#define OI_CORE_PLUGIN_FUNCTION_SPECIAL_FUNCTION_H_

#include "core_export.h"
#include "function.h"

namespace oi {

/*!
 * \brief The SpecialFunction class
 * handles special function e. g. configured functions
 */
class OI_CORE_EXPORT SpecialFunction : public Function {
 public:
  explicit SpecialFunction() : Function() {}

  ~SpecialFunction() override {}
};

}  // namespace oi

#include "plugin_abi.h"

inline const std::string SpecialFunction_iidd =
    oi::pluginIid("de.openIndy.plugin.function.specialFunction.v");

#endif  // OI_CORE_PLUGIN_FUNCTION_SPECIAL_FUNCTION_H_
