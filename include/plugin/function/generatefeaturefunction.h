#ifndef GENERATEFEATUREFUNCTION_H
#define GENERATEFEATUREFUNCTION_H

#include "core_export.h"
#include "function.h"

namespace oi {

/*!
 * \brief The GenerateFeatureFunction class
 * Function that uses its input elements to generate one or more new feature
 */
class OI_CORE_EXPORT GenerateFeatureFunction : public Function {
  Q_OBJECT
  Q_DISABLE_COPY_MOVE(GenerateFeatureFunction)

 public:
  explicit GenerateFeatureFunction(QObject* parent = nullptr)
      : Function(parent) {}

  ~GenerateFeatureFunction() override {}

 signals:
  void addFeature(const QPointer<FeatureWrapper>& feature);
  void addFeatures(const QList<QPointer<FeatureWrapper> >& features);
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

#define GENERATEFEATUREFUNCTION_PREFIX \
  de.openIndy.plugin.function.generateFeatureFunction.v
#define GenerateFeatureFunction_iidd \
  STR2(CONCAT2(GENERATEFEATUREFUNCTION_PREFIX, OI_PLUGIN_ABI_VERSION))

#endif  // GENERATEFEATUREFUNCTION_H
