#ifndef SPHERE_H
#define SPHERE_H
#include <map>
#include <string>

#include "core_export.h"
#include "direction.h"
#include "geometry.h"
#include "position.h"
#include "radius.h"

namespace oi {

/*!
 * \brief The Sphere class
 */
class OI_CORE_EXPORT Sphere : public Geometry {
 public:
  explicit Sphere();
  Sphere(const Position& center, const Radius& radius);

  Sphere(const Sphere& copy);

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
};

}  // namespace oi

#endif  // SPHERE_H
