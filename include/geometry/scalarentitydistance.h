#ifndef SCALARENTITYDISTANCE_H
#define SCALARENTITYDISTANCE_H

#include <QObject>
#include <QtXml>

#include "geometry.h"

namespace oi {

/*!
 * \brief The ScalarEntityDistance class
 */
class OI_CORE_EXPORT ScalarEntityDistance : public Geometry {
  Q_OBJECT

 public:
  explicit ScalarEntityDistance(QObject* parent = nullptr);
  explicit ScalarEntityDistance(const double& distance,
                                QObject* parent = nullptr);

  ScalarEntityDistance(const ScalarEntityDistance& copy,
                       QObject* parent = nullptr);

  ScalarEntityDistance& operator=(const ScalarEntityDistance& copy);

  ~ScalarEntityDistance() override;

  // ########################################
  // order of unknown parameters (Qxx-matrix)
  // ########################################

  enum DistanceUnknowns { unknownDistance = 0 };

  // ##############################
  // get or set distance parameters
  // ##############################

  const double& getDistance() const;

  void setDistance(const double& distance);

  // #############################
  // get or set unknown parameters
  // #############################

  QMap<GeometryParameters, QString> getUnknownParameters(
      const QMap<DimensionType, UnitType>& displayUnits,
      const QMap<DimensionType, int>& displayDigits) const override;
  void applyUnknownParameters(
      const QMap<GeometryParameters, double>& parameters) override;

  // ###########################
  // reexecute the function list
  // ###########################

  void recalc() override;

  // #################
  // save and load XML
  // #################

  QDomElement toOpenIndyXML(QDomDocument& xmlDoc) const override;
  bool fromOpenIndyXML(QDomElement& xmlElem) override;

  // ###############
  // display methods
  // ###############

  QString getDisplayDistance(const UnitType& type, const int& digits,
                             const bool& showDiff = false) const override;

 private:
  // ###################
  // distance attributes
  // ###################

  double distance{};
};

}  // namespace oi

#endif  // SCALARENTITYDISTANCE_H
