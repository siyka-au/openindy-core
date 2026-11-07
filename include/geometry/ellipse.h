#ifndef ELLIPSE_H
#define ELLIPSE_H
#include <map>
#include <string>

#include "core_export.h"
#include "direction.h"
#include "geometry.h"
#include "position.h"
#include "radius.h"

namespace oi {

/*!
 * \brief The Ellipse class
 */
class OI_CORE_EXPORT Ellipse : public Geometry {
  friend struct codec::Access;  // the protobuf codec (see src/codec)

 public:
  explicit Ellipse();
  Ellipse(const Position& center, const Direction& normal, const double& a,
          const double& b, const Direction& semiMajorAxis);

  Ellipse(const Ellipse& copy);

  Ellipse& operator=(const Ellipse& copy);

  ~Ellipse() override;

  // ########################################
  // order of unknown parameters (Qxx-matrix)
  // ########################################

  enum EllipseUnknowns {
    unknownCenterX = 0,
    unknownCenterY,
    unknownCenterZ,
    unknownNormalI,
    unknownNormalJ,
    unknownNormalK,
    unknownA,
    unknownB,
    unknownSemiMajorAxisI,
    unknownSemiMajorAxisJ,
    unknownSemiMajorAxisK
  };

  // ###################################################################
  // check wether this geometry has a direction, a position and a radius
  // ###################################################################

  [[nodiscard]] bool hasDirection() const override;
  [[nodiscard]] bool hasPosition() const override;

  // #############################
  // get or set ellipse parameters
  // #############################

  [[nodiscard]] const double& getA() const;
  [[nodiscard]] const double& getB() const;
  [[nodiscard]] const Direction& getSemiMajorAxisDirection() const;

  void setEllipse(const Position& center, const Direction& normal,
                  const double& a, const double& b,
                  const Direction& semiMajorAxis);

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

  [[nodiscard]] std::string getDisplaySecondaryI(
      const int& digits, const bool& showDiff = false) const override;
  [[nodiscard]] std::string getDisplaySecondaryJ(
      const int& digits, const bool& showDiff = false) const override;
  [[nodiscard]] std::string getDisplaySecondaryK(
      const int& digits, const bool& showDiff = false) const override;
  [[nodiscard]] std::string getDisplayA(
      const UnitType& type, const int& digits,
      const bool& showDiff = false) const override;
  [[nodiscard]] std::string getDisplayB(
      const UnitType& type, const int& digits,
      const bool& showDiff = false) const override;

 private:
  // ##################
  // ellipse attributes
  // ##################

  double a{};               // semi-major axis length
  double b{};               // semi-minor axis length
  Direction semiMajorAxis;  // semi-major axis direction
};

}  // namespace oi

#endif  // ELLIPSE_H
