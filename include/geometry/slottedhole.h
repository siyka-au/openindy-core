#ifndef SLOTTEDHOLE_H
#define SLOTTEDHOLE_H

#include <QObject>
#include <QtMath>
#include <QtXml>

#include "direction.h"
#include "geometry.h"
#include "position.h"
#include "radius.h"

namespace oi {

/*!
 * \brief The SlottedHole class
 * Defines an elongated bore with one radius
 */
class OI_CORE_EXPORT SlottedHole : public Geometry {
  Q_OBJECT

 public:
  explicit SlottedHole(QObject* parent = nullptr);
  SlottedHole(const Position& center, const Direction& normal,
              const Radius& radius, const double& length,
              const Direction& holeAxis, QObject* parent = nullptr);
  SlottedHole(const Position& circleCenterA, const Position& circleCenterB,
              const Direction& normal, const Radius& radius,
              QObject* parent = nullptr);

  SlottedHole(const SlottedHole& copy, QObject* parent = nullptr);

  SlottedHole& operator=(const SlottedHole& copy);

  ~SlottedHole() override;

  // ########################################
  // order of unknown parameters (Qxx-matrix)
  // ########################################

  enum SlottedHoleUnknowns {
    unknownCenterX = 0,
    unknownCenterY,
    unknownCenterZ,
    unknownNormalI,
    unknownNormalJ,
    unknownNormalK,
    unknownRadius,
    unknownLength,
    unknownHoleAxisI,
    unknownHoleAxisJ,
    unknownHoleAxisK
  };

  // ###################################################################
  // check wether this geometry has a direction, a position and a radius
  // ###################################################################

  bool hasDirection() const override;
  bool hasPosition() const override;
  bool hasRadius() const override;

  // ##################################
  // get or set slotted hole parameters
  // ##################################

  const Radius& getRadius() const override;
  const Direction& getDirection() const override;
  const Position& getPosition() const override;

  const Position& getCircleCenterA() const;
  const Position& getCircleCenterB() const;
  const Direction& getHoleAxis() const;
  const double& getLength() const;

  void setSlottedHole(const Position& center, const Direction& normal,
                      const Radius& radius, const double& length,
                      const Direction& holeAxis);
  void setSlottedHole(const Position& circleCenterA,
                      const Position& circleCenterB, const Direction& normal,
                      const Radius& radius);

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

  QString getDisplayX(const UnitType& type, const int& digits,
                      const bool& showDiff = false) const override;
  QString getDisplayY(const UnitType& type, const int& digits,
                      const bool& showDiff = false) const override;
  QString getDisplayZ(const UnitType& type, const int& digits,
                      const bool& showDiff = false) const override;
  QString getDisplayPrimaryI(const int& digits,
                             const bool& showDiff = false) const override;
  QString getDisplayPrimaryJ(const int& digits,
                             const bool& showDiff = false) const override;
  QString getDisplayPrimaryK(const int& digits,
                             const bool& showDiff = false) const override;
  QString getDisplayRadiusA(const UnitType& type, const int& digits,
                            const bool& showDiff = false) const override;
  QString getDisplaySecondaryI(const int& digits,
                               const bool& showDiff = false) const override;
  QString getDisplaySecondaryJ(const int& digits,
                               const bool& showDiff = false) const override;
  QString getDisplaySecondaryK(const int& digits,
                               const bool& showDiff = false) const override;
  QString getDisplayLength(const UnitType& type, const int& digits,
                           const bool& showDiff = false) const override;

 private:
  // #######################
  // slotted hole attributes
  // #######################

  Position center;   // center of the slotted hole
  Direction normal;  // normal vector
  Radius radius;     // radius of the outer circles
  double length{};   // length of the slotted hole (= distance of the circle
                     // centers + 2 * radius)
  Direction holeAxis;  // vector pointing from one circle center to the other)

  Position circleCenterA;  // center of one circle
  Position circleCenterB;  // center of the other circle
};

}  // namespace oi

#endif  // SLOTTEDHOLE_H
