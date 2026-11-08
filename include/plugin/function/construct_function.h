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

#define CONSTRUCTFUNCTION_PREFIX de.openIndy.plugin.function.constructFunction.v
#define ConstructFunction_iidd \
  STR2(CONCAT2(CONSTRUCTFUNCTION_PREFIX, OI_PLUGIN_ABI_VERSION))

#endif  // OI_CORE_PLUGIN_FUNCTION_CONSTRUCT_FUNCTION_H_
