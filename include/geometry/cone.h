#ifndef CONE_H
#define CONE_H
#include <QtCore>
#include <map>

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
  friend struct codec::Access;  // the protobuf codec (see src/codec)

 public:
  explicit Cone();
  Cone(const Position& apex, const Direction& axis, const double& aperture);

  Cone(const Cone& copy);

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

  [[nodiscard]] std::map<GeometryParameters, QString> getUnknownParameters(
      const std::map<DimensionType, UnitType>& displayUnits,
      const std::map<DimensionType, int>& displayDigits) const override;
  void applyUnknownParameters(
      const std::map<GeometryParameters, double>& parameters) override;

  // ###########################
  // reexecute the function list
  // ###########################

  void recalc() override;

 private:
  // ###############
  // cone attributes
  // ###############

  double aperture{};  // opening angle = 2 * angle between surface line and r
};

}  // namespace oi

#endif  // CONE_H
