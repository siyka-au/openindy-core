#ifndef OI_CORE_GEOMETRY_SLOTTED_HOLE_H_
#define OI_CORE_GEOMETRY_SLOTTED_HOLE_H_
#include <map>
#include <string>

#include "core_export.h"
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
  friend struct codec::Access;  // the protobuf codec (see src/codec)

 public:
  explicit SlottedHole();
  SlottedHole(const Position& center, const Direction& normal,
              const Radius& radius, const double& length,
              const Direction& holeAxis);
  SlottedHole(const Position& circleCenterA, const Position& circleCenterB,
              const Direction& normal, const Radius& radius);

  SlottedHole(const SlottedHole& copy);

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

  [[nodiscard]] bool hasDirection() const override;
  [[nodiscard]] bool hasPosition() const override;
  [[nodiscard]] bool hasRadius() const override;

  // ##################################
  // get or set slotted hole parameters
  // ##################################

  [[nodiscard]] const Radius& getRadius() const override;
  [[nodiscard]] const Direction& getDirection() const override;
  [[nodiscard]] const Position& getPosition() const override;

  [[nodiscard]] const Position& getCircleCenterA() const;
  [[nodiscard]] const Position& getCircleCenterB() const;
  [[nodiscard]] const Direction& getHoleAxis() const;
  [[nodiscard]] const double& getLength() const;

  void setSlottedHole(const Position& center, const Direction& normal,
                      const Radius& radius, const double& length,
                      const Direction& holeAxis);
  void setSlottedHole(const Position& circleCenterA,
                      const Position& circleCenterB, const Direction& normal,
                      const Radius& radius);

  // #############################
  // get or set unknown parameters
  // #############################

  [[nodiscard]] std::map<GeometryParameters, std::string> getUnknownParameters(
      const std::map<DimensionType, UnitType>& displayUnits,
      const std::map<DimensionType, int>& displayDigits) const override;
  void applyUnknownParameters(
      const std::map<GeometryParameters, double>& parameters) override;

  // ###########################
  // reexecute the function list
  // ###########################

  void recalc() override;

  // ###############
  // display methods
  // ###############

  [[nodiscard]] std::string getDisplayX(
      const UnitType& type, const int& digits,
      const bool& showDiff = false) const override;
  [[nodiscard]] std::string getDisplayY(
      const UnitType& type, const int& digits,
      const bool& showDiff = false) const override;
  [[nodiscard]] std::string getDisplayZ(
      const UnitType& type, const int& digits,
      const bool& showDiff = false) const override;
  [[nodiscard]] std::string getDisplayPrimaryI(
      const int& digits, const bool& showDiff = false) const override;
  [[nodiscard]] std::string getDisplayPrimaryJ(
      const int& digits, const bool& showDiff = false) const override;
  [[nodiscard]] std::string getDisplayPrimaryK(
      const int& digits, const bool& showDiff = false) const override;
  [[nodiscard]] std::string getDisplayRadiusA(
      const UnitType& type, const int& digits,
      const bool& showDiff = false) const override;
  [[nodiscard]] std::string getDisplaySecondaryI(
      const int& digits, const bool& showDiff = false) const override;
  [[nodiscard]] std::string getDisplaySecondaryJ(
      const int& digits, const bool& showDiff = false) const override;
  [[nodiscard]] std::string getDisplaySecondaryK(
      const int& digits, const bool& showDiff = false) const override;
  [[nodiscard]] std::string getDisplayLength(
      const UnitType& type, const int& digits,
      const bool& showDiff = false) const override;

 private:
  // #######################
  // slotted hole attributes
  // #######################

  Position center;     // center of the slotted hole
  Direction normal;    // normal vector
  Radius radius;       // radius of the outer circles
  double length{};     // length of the slotted hole (= distance of the circle
                       // centers + 2 * radius)
  Direction holeAxis;  // vector pointing from one circle center to the other)

  Position circleCenterA;  // center of one circle
  Position circleCenterB;  // center of the other circle
};

}  // namespace oi

#endif  // OI_CORE_GEOMETRY_SLOTTED_HOLE_H_
