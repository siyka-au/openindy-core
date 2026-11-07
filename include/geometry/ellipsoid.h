#ifndef ELLIPSOID_H
#define ELLIPSOID_H
#include <QtCore>
#include <map>

#include "core_export.h"
#include "direction.h"
#include "geometry.h"
#include "position.h"

namespace oi {

/*!
 * \brief The Ellipsoid class
 * Defines an ellipsoid of rotation
 */
class OI_CORE_EXPORT Ellipsoid : public Geometry {
  friend struct codec::Access;  // the protobuf codec (see src/codec)

 public:
  explicit Ellipsoid();
  Ellipsoid(const Position& center, const Direction& majorAxis, const double& a,
            const double& b);

  Ellipsoid(const Ellipsoid& copy);

  Ellipsoid& operator=(const Ellipsoid& copy);

  ~Ellipsoid() override;

  // ########################################
  // order of unknown parameters (Qxx-matrix)
  // ########################################

  enum EllipsoidUnknowns {
    unknownCenterX = 0,
    unknownCenterY,
    unknownCenterZ,
    unknownMajorAxisI,
    unknownMajorAxisJ,
    unknownMajorAxisK,
    unknownA,
    unknownB
  };

  // ###################################################################
  // check wether this geometry has a direction, a position and a radius
  // ###################################################################

  [[nodiscard]] bool hasDirection() const override;
  [[nodiscard]] bool hasPosition() const override;

  // ###############################
  // get or set ellipsoid parameters
  // ###############################

  [[nodiscard]] const double& getA() const;
  [[nodiscard]] const double& getB() const;

  void setEllipsoid(const Position& center, const Direction& majorAxis,
                    const double& a, const double& b);

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

  // ###############
  // display methods
  // ###############

  [[nodiscard]] QString getDisplayA(
      const UnitType& type, const int& digits,
      const bool& showDiff = false) const override;
  [[nodiscard]] QString getDisplayB(
      const UnitType& type, const int& digits,
      const bool& showDiff = false) const override;

 private:
  // ####################
  // ellipsoid attributes
  // ####################

  double a{};  // semi-major axis
  double b{};  // semi-minor axis
};

}  // namespace oi

#endif  // ELLIPSOID_H
