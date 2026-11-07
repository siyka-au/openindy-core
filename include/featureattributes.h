#ifndef FEATUREATTRIBUTES_H
#define FEATUREATTRIBUTES_H

#include <string>
#include <utility>

#include "core_export.h"
#include "measurementconfig.h"
#include "types.h"

namespace oi {

/*!
 * \brief The FeatureAttributes class
 * Helper class that is used when creating new features to specify their
 * attributes
 */
class OI_CORE_EXPORT FeatureAttributes {
 public:
  FeatureAttributes() {}

  // general attributes
  FeatureTypes typeOfFeature;
  std::string name;
  std::string group;
  int count{0};

  // only when typeOfFeature specifies a geometry. D17: there is no
  // isNominal/isActual flag any more - whether the created geometry ends
  // up nominal or actual falls out of which producer function is
  // attached (functionPlugin, below): a scalar-only function makes it a
  // user-editable nominal, a fit makes it a read-only actual (D19).
  // nominalSystem is only meaningful - and only required - when the
  // chosen function is a nominal producer; leave it empty otherwise.
  bool isCommon{false};
  std::string nominalSystem;
  std::string measurementConfigName;
  MeasurementConfig measurementConfig;

  // function
  std::pair<std::string, std::string>
      functionPlugin;  // T1 = function, T2 = file path

  // only when typeOfFeature specifies a trafo param
  std::string startSystem;
  std::string destinationSystem;

  // only when typeOfFeature specifies a coordinate system
  bool isBundleSystem{false};
};

}  // namespace oi

#endif  // FEATUREATTRIBUTES_H
