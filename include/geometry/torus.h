#ifndef TORUS_H
#define TORUS_H

#include <QObject>
#include <QtXml>

#include "direction.h"
#include "geometry.h"
#include "position.h"
#include "radius.h"

namespace oi {

/*!
 * \brief The Torus class
 * Defines a torus of rotation
 */
class OI_CORE_EXPORT Torus : public Geometry {
  Q_OBJECT

 public:
  explicit Torus(QObject* parent = nullptr);
  Torus(const Position& center, const Direction& normal, const Radius& radiusA,
        const Radius& radiusB, QObject* parent = nullptr);

  Torus(const Torus& copy, QObject* parent = nullptr);

  Torus& operator=(const Torus& copy);

  ~Torus() override;

  // ########################################
  // order of unknown parameters (Qxx-matrix)
  // ########################################

  enum TorusUnknowns {
    unknownCenterX = 0,
    unknownCenterY,
    unknownCenterZ,
    unknownNormalI,
    unknownNormalJ,
    unknownNormalK,
    unknownRadiusA,
    unknownRadiusB
  };

  // ###################################################################
  // check wether this geometry has a direction, a position and a radius
  // ###################################################################

  [[nodiscard]] bool hasDirection() const override;
  [[nodiscard]] bool hasPosition() const override;
  [[nodiscard]] bool hasRadius() const override;

  // ###########################
  // get or set torus parameters
  // ###########################

  [[nodiscard]] const Radius& getSmallRadius() const;

  void setTorus(const Position& center, const Direction& normal,
                const Radius& radiusA, const Radius& radiusB);

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

  [[nodiscard]] QString getDisplayRadiusB(
      const UnitType& type, const int& digits,
      const bool& showDiff = false) const override;

 private:
  // ################
  // torus attributes
  // ################

  Radius radiusB;  // distance of the center curve to the torus surface (radiusA
                   // > radiusB)
};

}  // namespace oi

#endif  // TORUS_H
