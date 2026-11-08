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

#include "plugin_abi.h"

inline const std::string ObjectTransformation_iidd =
    oi::pluginIid("de.openIndy.plugin.function.objectTransformation.v");

#endif  // OI_CORE_PLUGIN_FUNCTION_OBJECT_TRANSFORMATION_H_
