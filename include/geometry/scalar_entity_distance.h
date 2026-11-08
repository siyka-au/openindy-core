#ifndef OI_CORE_GEOMETRY_SCALAR_ENTITY_DISTANCE_H_
#define OI_CORE_GEOMETRY_SCALAR_ENTITY_DISTANCE_H_
#include <map>
#include <string>

#include "core_export.h"
#include "geometry.h"

namespace oi {

/*!
 * \brief The ScalarEntityDistance class
 */
class OI_CORE_EXPORT ScalarEntityDistance : public Geometry {
  friend struct codec::Access;  // the protobuf codec (see src/codec)

 public:
  explicit ScalarEntityDistance();
  explicit ScalarEntityDistance(const double& distance);

  ScalarEntityDistance(const ScalarEntityDistance& copy);

  ScalarEntityDistance& operator=(const ScalarEntityDistance& copy);

  ~ScalarEntityDistance() override;

  // ########################################
  // order of unknown parameters (Qxx-matrix)
  // ########################################

  enum DistanceUnknowns { unknownDistance = 0 };

  // ##############################
  // get or set distance parameters
  // ##############################

  [[nodiscard]] const double& getDistance() const;

  void setDistance(const double& distance);

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

  [[nodiscard]] std::string getDisplayDistance(
      const UnitType& type, const int& digits,
      const bool& showDiff = false) const override;

 private:
  // ###################
  // distance attributes
  // ###################

  double distance{};
};

}  // namespace oi

#endif  // OI_CORE_GEOMETRY_SCALAR_ENTITY_DISTANCE_H_
