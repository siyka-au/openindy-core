#ifndef OBJECTTRANSFORMATION_H
#define OBJECTTRANSFORMATION_H

#include "function.h"

namespace oi {

/*!
 * \brief The ObjectTransformation class
 * Function that changes a previously solved feature by using other elements
 */
class OI_CORE_EXPORT ObjectTransformation : public Function {
  Q_OBJECT
  Q_DISABLE_COPY_MOVE(ObjectTransformation)

 public:
  explicit ObjectTransformation(QObject* parent = nullptr) : Function(parent) {}

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

#include "pluginabi.h"

#define OBJECTTRANSFORMATIONFUNCTION_PREFIX \
  de.openIndy.plugin.function.objectTransformation.v
#define ObjectTransformation_iidd \
  STR2(CONCAT2(OBJECTTRANSFORMATIONFUNCTION_PREFIX, OI_PLUGIN_ABI_VERSION))

#endif  // OBJECTTRANSFORMATION_H
