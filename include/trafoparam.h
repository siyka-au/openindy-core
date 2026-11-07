#ifndef TRAFOPARAM_H
#define TRAFOPARAM_H

#include <QtCore/qmath.h>

#include "core_export.h"
#include "feature.h"
#include "guarded.h"
#include "oimat.h"
#include "oivec.h"
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
  [[nodiscard]] virtual QMap<TrafoParamParameters, QString>
  getUnknownParameters(const QMap<DimensionType, UnitType>& displayUnits,
                       const QMap<DimensionType, int>& displayDigits) const;
  virtual void setUnknownParameters(
      const QMap<TrafoParamParameters, double>& parameters);

  // ###########################
  // reexecute the function list
  // ###########################

  void recalc() override;

  // ###############
  // display methods
  // ###############

  [[nodiscard]] QString getDisplayStDev(const UnitType& type,
                                        const int& digits) const override;

  [[nodiscard]] QString getDisplayStartSystem() const override;
  [[nodiscard]] QString getDisplayDestinationSystem() const override;
  [[nodiscard]] QString getDisplayTranslationX(
      const UnitType& type, const int& digits) const override;
  [[nodiscard]] QString getDisplayTranslationY(
      const UnitType& type, const int& digits) const override;
  [[nodiscard]] QString getDisplayTranslationZ(
      const UnitType& type, const int& digits) const override;
  [[nodiscard]] QString getDisplayRotationX(const UnitType& type,
                                            const int& digits) const override;
  [[nodiscard]] QString getDisplayRotationY(const UnitType& type,
                                            const int& digits) const override;
  [[nodiscard]] QString getDisplayRotationZ(const UnitType& type,
                                            const int& digits) const override;
  [[nodiscard]] QString getDisplayScaleX(const int& digits) const override;
  [[nodiscard]] QString getDisplayScaleY(const int& digits) const override;
  [[nodiscard]] QString getDisplayScaleZ(const int& digits) const override;
  [[nodiscard]] QString getDisplayIsUsed() const override;
  [[nodiscard]] QString getDisplayIsDatumTransformation() const override;

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

#endif  // TRAFOPARAM_H
