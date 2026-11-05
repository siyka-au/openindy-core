#ifndef LINE_H
#define LINE_H

#include <QObject>
#include <QtXml>

#include "direction.h"
#include "geometry.h"
#include "position.h"

namespace oi {

/*!
 * \brief The Line class
 */
class OI_CORE_EXPORT Line : public Geometry {
  Q_OBJECT
 public:
  explicit Line(QObject* parent = nullptr);
  Line(const Position& xyz, const Direction& axis, QObject* parent = nullptr);

  Line(const Line& copy, QObject* parent = nullptr);

  Line& operator=(const Line& copy);

  ~Line() override;

  // ########################################
  // order of unknown parameters (Qxx-matrix)
  // ########################################

  enum LineUnknowns {
    unknownPositionX,
    unknownPositionY,
    unknownPositionZ,
    unknownAxisI,
    unknownAxisJ,
    unknownAxisK
  };

  // ###################################################################
  // check wether this geometry has a direction, a position and a radius
  // ###################################################################

  [[nodiscard]] bool hasDirection() const override;
  [[nodiscard]] bool hasPosition() const override;

  // ##########################
  // get or set line parameters
  // ##########################

  void setLine(const Position& xyz, const Direction& axis);

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

#endif  // LINE_H
