#ifndef PARABOLOID_H
#define PARABOLOID_H

#include <QObject>
#include <QtXml>

#include "direction.h"
#include "geometry.h"
#include "position.h"

namespace oi {

/*!
 * \brief The EllipticParaboloid class
 * Defines an elliptic paraboloid of rotation of the form z = x^2/a^2 + y^2/a^2,
 * a > 0
 */
class OI_CORE_EXPORT Paraboloid : public Geometry {
  Q_OBJECT

 public:
  explicit Paraboloid(QObject* parent = nullptr);
  Paraboloid(const Position& apex, const Direction& axis, const double& a,
             QObject* parent = nullptr);

  Paraboloid(const Paraboloid& copy, QObject* parent = nullptr);

  Paraboloid& operator=(const Paraboloid& copy);

  ~Paraboloid() override;

  // ########################################
  // order of unknown parameters (Qxx-matrix)
  // ########################################

  enum ParaboloidUnknowns {
    unknownApexX = 0,
    unknownApexY,
    unknownApexZ,
    unknownAxisI,
    unknownAxisJ,
    unknownAxisK,
    unknownA
  };

  // ###################################################################
  // check wether this geometry has a direction, a position and a radius
  // ###################################################################

  bool hasDirection() const override;
  bool hasPosition() const override;

  // ################################
  // get or set paraboloid parameters
  // ################################

  const double& getA() const;

  void setParaboloid(const Position& apex, const Direction& axis,
                     const double& a);

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

  QString getDisplayA(const UnitType& type, const int& digits,
                      const bool& showDiff = false) const override;

 private:
  // #####################
  // paraboloid attributes
  // #####################

  double a{};  // compression or streatch parameter
};

}  // namespace oi

#endif  // PARABOLOID_H
