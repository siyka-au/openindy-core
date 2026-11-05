#ifndef CYLINDER_H
#define CYLINDER_H

#include <QObject>
#include <QtXml>

#include "direction.h"
#include "geometry.h"
#include "position.h"
#include "radius.h"

namespace oi {

/*!
 * \brief The Cylinder class
 * Defines a right circular cylinder
 */
class OI_CORE_EXPORT Cylinder : public Geometry {
  Q_OBJECT

 public:
  explicit Cylinder(QObject* parent = nullptr);
  Cylinder(const Position& axisPoint, const Direction& axis,
           const Radius& radius, QObject* parent = nullptr);

  Cylinder(const Cylinder& copy, QObject* parent = nullptr);

  Cylinder& operator=(const Cylinder& copy);

  ~Cylinder() override;

  // ########################################
  // order of unknown parameters (Qxx-matrix)
  // ########################################

  enum CylinderUnknowns {
    unknownAxisPointX = 0,
    unknownAxisPointY,
    unknownAxisPointZ,
    unknownAxisI,
    unknownAxisJ,
    unknownAxisK,
    unknownRadius
  };

  // ###################################################################
  // check wether this geometry has a direction, a position and a radius
  // ###################################################################

  [[nodiscard]] bool hasDirection() const override;
  [[nodiscard]] bool hasPosition() const override;
  [[nodiscard]] bool hasRadius() const override;

  // ##############################
  // get or set cylinder parameters
  // ##############################

  void setCylinder(const Position& axisPoint, const Direction& axis,
                   const Radius& radius);

  // #############################
  // get or set unknown parameters
  // #############################

  [[nodiscard]] QMap<GeometryParameters, QString> getUnknownParameters(
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
};

}  // namespace oi

#endif  // CYLINDER_H
