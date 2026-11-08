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

#ifndef STR
#define STR(x) #x
#endif
#ifndef STR2
#define STR2(x) STR(x)
#endif

#ifndef CONCAT
#define CONCAT(a, b) a##b
#endif
#ifndef CONCAT2
#define CONCAT2(a, b) CONCAT(a, b)
#endif

#include "plugin_abi.h"

#define SPECIALFUNCTION_PREFIX de.openIndy.plugin.function.specialFunction.v
#define SpecialFunction_iidd \
  STR2(CONCAT2(SPECIALFUNCTION_PREFIX, OI_PLUGIN_ABI_VERSION))

#endif  // OI_CORE_PLUGIN_FUNCTION_SPECIAL_FUNCTION_H_
