#ifndef OI_CORE_PLUGIN_FUNCTION_OBJECT_TRANSFORMATION_H_
#define OI_CORE_PLUGIN_FUNCTION_OBJECT_TRANSFORMATION_H_

#include "core_export.h"
#include "function.h"

namespace oi {

/*!
 * \brief The ObjectTransformation class
 * Function that changes a previously solved feature by using other elements
 */
class OI_CORE_EXPORT ObjectTransformation : public Function {
 public:
  explicit ObjectTransformation() : Function() {}

  ~ObjectTransformation() override {}
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

#define OBJECTTRANSFORMATIONFUNCTION_PREFIX \
  de.openIndy.plugin.function.objectTransformation.v
#define ObjectTransformation_iidd \
  STR2(CONCAT2(OBJECTTRANSFORMATIONFUNCTION_PREFIX, OI_PLUGIN_ABI_VERSION))

#endif  // OI_CORE_PLUGIN_FUNCTION_OBJECT_TRANSFORMATION_H_
