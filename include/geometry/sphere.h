#ifndef SPHERE_H
#define SPHERE_H

#include <QObject>
#include <QtXml>

#include "direction.h"
#include "geometry.h"
#include "position.h"
#include "radius.h"

namespace oi {

/*!
 * \brief The Sphere class
 */
class OI_CORE_EXPORT Sphere : public Geometry {
  Q_OBJECT

 public:
  explicit Sphere(QObject* parent = nullptr);
  Sphere(const Position& center, const Radius& radius,
         QObject* parent = nullptr);

  Sphere(const Sphere& copy, QObject* parent = nullptr);

  Sphere& operator=(const Sphere& copy);

  ~Sphere() override;

  // ########################################
  // order of unknown parameters (Qxx-matrix)
  // ########################################

  enum SphereUnknowns {
    unknownCenterX = 0,
    unknownCenterY,
    unknownCenterZ,
    unknownRadius
  };

  // ###################################################################
  // check wether this geometry has a direction, a position and a radius
  // ###################################################################

  [[nodiscard]] bool hasPosition() const override;
  [[nodiscard]] bool hasRadius() const override;

  // ############################
  // get or set sphere parameters
  // ############################

  void setSphere(const Position& center, const Radius& radius);

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

  // ###############
  // display methods
  // ###############
};

}  // namespace oi

#endif  // SPHERE_H
