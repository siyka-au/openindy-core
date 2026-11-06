#ifndef TRAFOPARAM_H
#define TRAFOPARAM_H

#include <QtCore/qmath.h>

#include <QDateTime>
#include <QObject>
#include <QPointer>

#include "core_export.h"
#include "feature.h"
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
  Q_OBJECT

 public:
  explicit TrafoParam(QObject* parent = nullptr);

  TrafoParam(const TrafoParam& copy, QObject* parent = nullptr);

  TrafoParam& operator=(const TrafoParam& copy);

  ~TrafoParam() override;

  // #################################
  // get or set trafo param attributes
  // #################################

  // used state
  [[nodiscard]] const bool& getIsUsed() const;
  void setIsUsed(const bool& isUsed);

  // coordinate systems
  [[nodiscard]] const QPointer<CoordinateSystem>& getStartSystem() const;
  [[nodiscard]] const QPointer<CoordinateSystem>& getDestinationSystem() const;
  bool setCoordinateSystems(const QPointer<CoordinateSystem>& from,
                            const QPointer<CoordinateSystem>& to);

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

  // #################
  // save and load XML
  // #################

  QDomElement toOpenIndyXML(QDomDocument& xmlDoc) const override;
  bool fromOpenIndyXML(QDomElement& xmlElem) override;

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

 signals:

  // ###########################################
  // signals to inform about trafo param changes
  // ###########################################

  void transformationParameterChanged(const int& tpId);
  void coordinateSystemsChanged(const int& tpId);
  void isUsedChanged(const int& tpId);
  // void validTimeChanged(const int &tpId);
  // void isMovementChanged(const int &tpId);
  void isBundleChanged(const int& tpId);
  void isDatumTrafoChanged(const int& tpId);

 private:
  // ##############################
  // general trafo param attributes
  // ##############################

  // coordinate systems
  QPointer<CoordinateSystem> from;
  QPointer<CoordinateSystem> to;

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
