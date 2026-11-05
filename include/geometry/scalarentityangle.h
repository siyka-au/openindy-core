#ifndef SCALARENTITYANGLE_H
#define SCALARENTITYANGLE_H

#include <QObject>
#include <QtXml>

#include "geometry.h"

namespace oi {

/*!
 * \brief The ScalarEntityAngle class
 */
class OI_CORE_EXPORT ScalarEntityAngle : public Geometry {
  Q_OBJECT

 public:
  explicit ScalarEntityAngle(QObject* parent = nullptr);
  explicit ScalarEntityAngle(const double& angle, QObject* parent = nullptr);

  ScalarEntityAngle(const ScalarEntityAngle& copy, QObject* parent = nullptr);

  ScalarEntityAngle& operator=(const ScalarEntityAngle& copy);

  ~ScalarEntityAngle() override;

  // ########################################
  // order of unknown parameters (Qxx-matrix)
  // ########################################

  enum AngleUnknowns { unknownAngle = 0 };

  // ###########################
  // get or set angle parameters
  // ###########################

  const double& getAngle() const;

  void setAngle(const double& angle);

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

  QString getDisplayAngle(const UnitType& type, const int& digits,
                          const bool& showDiff = false) const override;

 private:
  // ################
  // angle attributes
  // ################

  double angle;
};

}  // namespace oi

#endif  // SCALARENTITYANGLE_H
