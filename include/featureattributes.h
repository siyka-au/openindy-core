#ifndef FEATUREATTRIBUTES_H
#define FEATUREATTRIBUTES_H

#include <QPair>
#include <QString>

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
  FeatureAttributes() {
    this->count = 0;
    this->isCommon = false;
    this->isBundleSystem = false;
  }

  // general attributes
  FeatureTypes typeOfFeature;
  QString name;
  QString group;
  int count;

  // only when typeOfFeature specifies a geometry. D17: there is no
  // isNominal/isActual flag any more - whether the created geometry ends
  // up nominal or actual falls out of which producer function is
  // attached (functionPlugin, below): a scalar-only function makes it a
  // user-editable nominal, a fit makes it a read-only actual (D19).
  // nominalSystem is only meaningful - and only required - when the
  // chosen function is a nominal producer; leave it empty otherwise.
  bool isCommon;
  QString nominalSystem;
  QString measurementConfigName;
  MeasurementConfig measurementConfig;

  // function
  QPair<QString, QString> functionPlugin;  // T1 = function, T2 = file path

  // only when typeOfFeature specifies a trafo param
  QString startSystem;
  QString destinationSystem;

  // only when typeOfFeature specifies a coordinate system
  bool isBundleSystem;
};

}  // namespace oi

#endif  // FEATUREATTRIBUTES_H
