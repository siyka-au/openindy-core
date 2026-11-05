#ifndef ELLIPSOID_H
#define ELLIPSOID_H

#include <QObject>
#include <QtXml>

#include "direction.h"
#include "geometry.h"
#include "position.h"

namespace oi {

/*!
 * \brief The Ellipsoid class
 * Defines an ellipsoid of rotation
 */
class OI_CORE_EXPORT Ellipsoid : public Geometry {
  Q_OBJECT

 public:
  explicit Ellipsoid(QObject* parent = nullptr);
  Ellipsoid(const Position& center, const Direction& majorAxis, const double& a,
            const double& b, QObject* parent = nullptr);

  Ellipsoid(const Ellipsoid& copy, QObject* parent = nullptr);

  Ellipsoid& operator=(const Ellipsoid& copy);

  ~Ellipsoid() override;

  // ########################################
  // order of unknown parameters (Qxx-matrix)
  // ########################################

  enum EllipsoidUnknowns {
    unknownCenterX = 0,
    unknownCenterY,
    unknownCenterZ,
    unknownMajorAxisI,
    unknownMajorAxisJ,
    unknownMajorAxisK,
    unknownA,
    unknownB
  };

  // ###################################################################
  // check wether this geometry has a direction, a position and a radius
  // ###################################################################

  bool hasDirection() const override;
  bool hasPosition() const override;

  // ###############################
  // get or set ellipsoid parameters
  // ###############################

  const double& getA() const;
  const double& getB() const;

  void setEllipsoid(const Position& center, const Direction& majorAxis,
                    const double& a, const double& b);

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
  QString getDisplayB(const UnitType& type, const int& digits,
                      const bool& showDiff = false) const override;

 private:
  // ####################
  // ellipsoid attributes
  // ####################

  double a{};  // semi-major axis
  double b{};  // semi-minor axis
};

}  // namespace oi

#endif  // ELLIPSOID_H
