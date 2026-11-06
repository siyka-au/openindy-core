#ifndef SCALARENTITYTEMPERATURE_H
#define SCALARENTITYTEMPERATURE_H

#include <QObject>
#include <QtXml>

#include "core_export.h"
#include "geometry.h"

namespace oi {

/*!
 * \brief The ScalarEntityTemperature class
 */
class OI_CORE_EXPORT ScalarEntityTemperature : public Geometry {
  Q_OBJECT

 public:
  explicit ScalarEntityTemperature(QObject* parent = nullptr);
  explicit ScalarEntityTemperature(const double& temperature,
                                   QObject* parent = nullptr);

  ScalarEntityTemperature(const ScalarEntityTemperature& copy,
                          QObject* parent = nullptr);

  ScalarEntityTemperature& operator=(const ScalarEntityTemperature& copy);

  ~ScalarEntityTemperature() override;

  // ########################################
  // order of unknown parameters (Qxx-matrix)
  // ########################################

  enum TemperatureUnknowns { unknownTemperature = 0 };

  // #################################
  // get or set temperature parameters
  // #################################

  [[nodiscard]] const double& getTemperature() const;

  void setTemperature(const double& temperature);

  // #############################
  // get or set unknown parameters
  // #############################

  [[nodiscard]] QMap<GeometryParameters, QString> getUnknownParameters(
      const QMap<DimensionType, UnitType>& displayUnits,
      const QMap<DimensionType, int>& displayDigits) const override;
  void applyUnknownParameters(
      const QMap<GeometryParameters, double>& parameters) override;

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

  [[nodiscard]] QString getDisplayTemperature(
      const UnitType& type, const int& digits,
      const bool& showDiff = false) const override;

 private:
  // ######################
  // temperature attributes
  // ######################

  double temperature{};
};

}  // namespace oi

#endif  // SCALARENTITYTEMPERATURE_H
