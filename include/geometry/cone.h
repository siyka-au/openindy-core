#ifndef CONE_H
#define CONE_H

#include <QObject>
#include <QtXml>

#include "core_export.h"
#include "direction.h"
#include "geometry.h"
#include "position.h"

namespace oi {

/*!
 * \brief The Cone class
 * Defines a right circular cone
 */
class OI_CORE_EXPORT Cone : public Geometry {
  Q_OBJECT

 public:
  explicit Cone(QObject* parent = nullptr);
  Cone(const Position& apex, const Direction& axis, const double& aperture,
       QObject* parent = nullptr);

  Cone(const Cone& copy, QObject* parent = nullptr);

  Cone& operator=(const Cone& copy);

  ~Cone() override;

  // ########################################
  // order of unknown parameters (Qxx-matrix)
  // ########################################

  enum ConeUnknowns {
    unknownApexX = 0,
    unknownApexY,
    unknownApexZ,
    unknownAxisI,
    unknownAxisJ,
    unknownAxisK,
    unknownAperture
  };

  // ###################################################################
  // check wether this geometry has a direction, a position and a radius
  // ###################################################################

  [[nodiscard]] bool hasDirection() const override;
  [[nodiscard]] bool hasPosition() const override;

  // ##########################
  // get or set cone parameters
  // ##########################

  [[nodiscard]] const double& getAperture() const;

  void setCone(const Position& apex, const Direction& axis,
               const double& aperture);

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

 private:
  // ###############
  // cone attributes
  // ###############

  double aperture{};  // opening angle = 2 * angle between surface line and r
};

}  // namespace oi

#endif  // CONE_H
