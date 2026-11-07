#ifndef SYSTEMTRANSFORMATION_H
#define SYSTEMTRANSFORMATION_H

#include "core_export.h"
#include "function.h"

class FeatureUpdater;

namespace oi {

/*!
 * \brief The SystemTransformation class
 * Functions that solves transformation parameters between two coordinate
 * systems (station-station, part-part, alignments)
 */
class OI_CORE_EXPORT SystemTransformation : public Function {
  friend class ::FeatureUpdater;

 public:
  explicit SystemTransformation() : Function() {
    // only applicable for trafo params
    this->applicableFor.clear();
    this->applicableFor.append(FeatureTypes::eTrafoParamFeature);
  }

  ~SystemTransformation() override {}

  void setInputPoint(const QList<Point>& startSystem,
                     const QList<Point>& destinationSystem) {
    this->inputPointsStartSystem = startSystem;
    this->inputPointsDestinationSystem = destinationSystem;
  }

  void setInputElements(
      const QMap<int, QList<InputElement> >& startSystem,
      const QMap<int, QList<InputElement> >& destinationSystem) {
    this->inputElementsStartSystem = startSystem;
    this->inputElementsDestinationSystem = destinationSystem;
  }

 protected:
  // special attributes for system transformations (normal transformations)
  QList<Point> inputPointsStartSystem;  // input elements solved in start system
  QList<Point> inputPointsDestinationSystem;  // input elements solved in
                                              // destination system

  // special attributes for system transformations (alignments)
  QMap<int, QList<InputElement> >
      inputElementsStartSystem;  // input elements solved in start system
  QMap<int, QList<InputElement> >
      inputElementsDestinationSystem;  // input elements solved in destination
                                       // system
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

#define SYSTEMTRANSFORMATIONFUNCTION_PREFIX \
  de.openIndy.plugin.function.systemTransformation.v
#define SystemTransformation_iidd \
  STR2(CONCAT2(SYSTEMTRANSFORMATIONFUNCTION_PREFIX, OI_PLUGIN_ABI_VERSION))

#endif  // SYSTEMTRANSFORMATION_H
