#ifndef ELLIPSE_H
#define ELLIPSE_H

#include <QObject>
#include <QtXml>

#include "direction.h"
#include "geometry.h"
#include "position.h"
#include "radius.h"

namespace oi {

/*!
 * \brief The Ellipse class
 */
class OI_CORE_EXPORT Ellipse : public Geometry {
  Q_OBJECT

 public:
  Ellipse(QObject* parent = nullptr);
  Ellipse(const Position& center, const Direction& normal, const double& a,
          const double& b, const Direction& semiMajorAxis,
          QObject* parent = nullptr);

  Ellipse(const Ellipse& copy, QObject* parent = nullptr);

  Ellipse& operator=(const Ellipse& copy);

  ~Ellipse() override;

  // ########################################
  // order of unknown parameters (Qxx-matrix)
  // ########################################

  enum EllipseUnknowns {
    unknownCenterX = 0,
    unknownCenterY,
    unknownCenterZ,
    unknownNormalI,
    unknownNormalJ,
    unknownNormalK,
    unknownA,
    unknownB,
    unknownSemiMajorAxisI,
    unknownSemiMajorAxisJ,
    unknownSemiMajorAxisK
  };

  // ###################################################################
  // check wether this geometry has a direction, a position and a radius
  // ###################################################################

  bool hasDirection() const override;
  bool hasPosition() const override;

  // #############################
  // get or set ellipse parameters
  // #############################

  const double& getA() const;
  const double& getB() const;
  const Direction& getSemiMajorAxisDirection() const;

  void setEllipse(const Position& center, const Direction& normal,
                  const double& a, const double& b,
                  const Direction& semiMajorAxis);

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

  QString getDisplaySecondaryI(const int& digits,
                               const bool& showDiff = false) const override;
  QString getDisplaySecondaryJ(const int& digits,
                               const bool& showDiff = false) const override;
  QString getDisplaySecondaryK(const int& digits,
                               const bool& showDiff = false) const override;
  QString getDisplayA(const UnitType& type, const int& digits,
                      const bool& showDiff = false) const override;
  QString getDisplayB(const UnitType& type, const int& digits,
                      const bool& showDiff = false) const override;

 private:
  // ##################
  // ellipse attributes
  // ##################

  double a;                 // semi-major axis length
  double b;                 // semi-minor axis length
  Direction semiMajorAxis;  // semi-major axis direction
};

}  // namespace oi

#endif  // ELLIPSE_H
