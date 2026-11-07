#ifndef LINE_H
#define LINE_H
#include <map>
#include <string>

#include "core_export.h"
#include "direction.h"
#include "geometry.h"
#include "position.h"

namespace oi {

/*!
 * \brief The Line class
 */
class OI_CORE_EXPORT Line : public Geometry {
 public:
  explicit Line();
  Line(const Position& xyz, const Direction& axis);

  Line(const Line& copy);

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

#endif  // LINE_H
