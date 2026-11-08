#ifndef OI_CORE_PLUGIN_FUNCTION_SYSTEM_TRANSFORMATION_H_
#define OI_CORE_PLUGIN_FUNCTION_SYSTEM_TRANSFORMATION_H_
#include <map>
#include <vector>

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
    this->applicableFor.push_back(FeatureTypes::eTrafoParamFeature);
  }

  ~SystemTransformation() override {}

  void setInputPoint(const std::vector<Point>& startSystem,
                     const std::vector<Point>& destinationSystem) {
    this->inputPointsStartSystem = startSystem;
    this->inputPointsDestinationSystem = destinationSystem;
  }

  void setInputElements(
      const std::map<int, std::vector<InputElement> >& startSystem,
      const std::map<int, std::vector<InputElement> >& destinationSystem) {
    this->inputElementsStartSystem = startSystem;
    this->inputElementsDestinationSystem = destinationSystem;
  }

 protected:
  // special attributes for system transformations (normal transformations)
  std::vector<Point>
      inputPointsStartSystem;  // input elements solved in start system
  std::vector<Point> inputPointsDestinationSystem;  // input elements solved in
                                                    // destination system

  // special attributes for system transformations (alignments)
  std::map<int, std::vector<InputElement> >
      inputElementsStartSystem;  // input elements solved in start system
  std::map<int, std::vector<InputElement> >
      inputElementsDestinationSystem;  // input elements solved in destination
                                       // system
};

}  // namespace oi

#include "plugin_abi.h"

inline const std::string SystemTransformation_iidd =
    oi::pluginIid("de.openIndy.plugin.function.systemTransformation.v");

#endif  // OI_CORE_PLUGIN_FUNCTION_SYSTEM_TRANSFORMATION_H_
