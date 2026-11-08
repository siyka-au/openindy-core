#ifndef OI_CORE_GEOMETRY_CIRCLE_H_
#define OI_CORE_GEOMETRY_CIRCLE_H_
#include <map>
#include <string>

#include "core_export.h"
#include "direction.h"
#include "geometry.h"
#include "position.h"
#include "radius.h"

namespace oi {

/*!
 * \brief The Circle class
 */
class OI_CORE_EXPORT Circle : public Geometry {
 public:
  explicit Circle();
  Circle(const Position& center, const Direction& normal, const Radius& radius);

  Circle(const Circle& copy);

  Circle& operator=(const Circle& copy);

  ~Circle() override;

  // ########################################
  // order of unknown parameters (Qxx-matrix)
  // ########################################

  enum CircleUnknowns {
    unknownCenterX = 0,
    unknownCenterY,
    unknownCenterZ,
    unknownNormalI,
    unknownNormalJ,
    unknownNormalK,
    unknownRadius
  };

  // ###################################################################
  // check wether this geometry has a direction, a position and a radius
  // ###################################################################

  [[nodiscard]] bool hasDirection() const override;
  [[nodiscard]] bool hasPosition() const override;
  [[nodiscard]] bool hasRadius() const override;

  void setCircle(const Position& center, const Direction& normal,
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

#endif  // OI_CORE_GEOMETRY_CIRCLE_H_
