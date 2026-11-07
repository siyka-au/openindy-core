#ifndef CYLINDER_H
#define CYLINDER_H
#include <map>
#include <string>

#include "core_export.h"
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
 public:
  explicit Cylinder();
  Cylinder(const Position& axisPoint, const Direction& axis,
           const Radius& radius);

  Cylinder(const Cylinder& copy);

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

  [[nodiscard]] std::map<GeometryParameters, std::string> getUnknownParameters(
      const std::map<DimensionType, UnitType>& displayUnits,
      const std::map<DimensionType, int>& displayDigits) const override;
  void applyUnknownParameters(
      const std::map<GeometryParameters, double>& parameters) override;

  // ###########################
  // reexecute the function list
  // ###########################

  void recalc() override;
};

}  // namespace oi

#endif  // CYLINDER_H
