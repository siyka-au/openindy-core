#ifndef OI_CORE_TRAFO_PARAM_H_
#define OI_CORE_TRAFO_PARAM_H_
#include <map>
#include <string>

#include "core_export.h"
#include "feature.h"
#include "guarded.h"
#include "oi_mat.h"
#include "oi_vec.h"
#include "statistic.h"

namespace oi {

using namespace math;

class CoordinateSystem;

/*!
 * \brief The TrafoParam class
 */
class OI_CORE_EXPORT TrafoParam : public Feature {
  friend class OiJob;
  friend struct codec::Access;  // the protobuf codec (see src/codec)

 public:
  explicit TrafoParam();

  TrafoParam(const TrafoParam& copy);

  TrafoParam& operator=(const TrafoParam& copy);

  ~TrafoParam() override;

  // #################################
  // get or set trafo param attributes
  // #################################

  // used state
  [[nodiscard]] const bool& getIsUsed() const;
  void setIsUsed(const bool& isUsed);

  // coordinate systems
  [[nodiscard]] const oi::Guarded<CoordinateSystem>& getStartSystem() const;
  [[nodiscard]] const oi::Guarded<CoordinateSystem>& getDestinationSystem()
      const;
  bool setCoordinateSystems(const oi::Guarded<CoordinateSystem>& from,
                            const oi::Guarded<CoordinateSystem>& to);

  // bundle
  [[nodiscard]] bool getIsBundle() const;

  // statistic
  [[nodiscard]] const Statistic& getStatistic() const;
  void setStatistic(const Statistic& statistic);

  // datum transformation
  [[nodiscard]] const bool& getIsDatumTrafo() const;
  void setIsDatumTrafo(const bool& isDatumTrafo);

  // ####################################
  // get or set transformation parameters
  // ####################################

  // getter
  [[nodiscard]] const OiMat& getHomogenMatrix() const;
  [[nodiscard]] const OiVec& getTranslation() const;
  [[nodiscard]] const OiVec& getRotation() const;
  [[nodiscard]] const OiVec& getScale() const;

  // setter
  bool setTransformationParameters(const OiVec& rotation,
                                   const OiVec& translation,
                                   const OiVec& scale);
  bool setTransformationParameters(const OiMat& rotation,
                                   const OiMat& translation,
                                   const OiMat& scale);
  bool setTransformationParameters(const OiMat& homogenMatrix);

  // #############################
  // get or set unknown parameters
  // #############################

  // unknown parameters
  [[nodiscard]] virtual std::map<TrafoParamParameters, std::string>
  getUnknownParameters(const std::map<DimensionType, UnitType>& displayUnits,
                       const std::map<DimensionType, int>& displayDigits) const;
  virtual void setUnknownParameters(
      const std::map<TrafoParamParameters, double>& parameters);

  // ###########################
  // reexecute the function list
  // ###########################

  void recalc() override;

  // ###############
  // display methods
  // ###############

  [[nodiscard]] std::string getDisplayStDev(const UnitType& type,
                                            const int& digits) const override;

  [[nodiscard]] std::string getDisplayStartSystem() const override;
  [[nodiscard]] std::string getDisplayDestinationSystem() const override;
  [[nodiscard]] std::string getDisplayTranslationX(
      const UnitType& type, const int& digits) const override;
  [[nodiscard]] std::string getDisplayTranslationY(
      const UnitType& type, const int& digits) const override;
  [[nodiscard]] std::string getDisplayTranslationZ(
      const UnitType& type, const int& digits) const override;
  [[nodiscard]] std::string getDisplayRotationX(
      const UnitType& type, const int& digits) const override;
  [[nodiscard]] std::string getDisplayRotationY(
      const UnitType& type, const int& digits) const override;
  [[nodiscard]] std::string getDisplayRotationZ(
      const UnitType& type, const int& digits) const override;
  [[nodiscard]] std::string getDisplayScaleX(const int& digits) const override;
  [[nodiscard]] std::string getDisplayScaleY(const int& digits) const override;
  [[nodiscard]] std::string getDisplayScaleZ(const int& digits) const override;
  [[nodiscard]] std::string getDisplayIsUsed() const override;
  [[nodiscard]] std::string getDisplayIsDatumTransformation() const override;

 private:
  // ##############################
  // general trafo param attributes
  // ##############################

  // coordinate systems
  oi::Guarded<CoordinateSystem> from;
  oi::Guarded<CoordinateSystem> to;

  // statistic
  Statistic statistic;

  bool isDatumTrafo;
  bool isUsed;  // true if this trafo param object is used for transformation
                // (false if not)

  // #########################
  // transformation parameters
  // #########################

  OiVec translation;
  OiVec rotation;
  OiVec scale;

  OiMat homogenMatrix;
};

}  // namespace oi

#endif  // OI_CORE_TRAFO_PARAM_H_
