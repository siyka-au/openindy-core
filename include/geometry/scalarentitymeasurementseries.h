#ifndef SCALARENTITYMEASUREMENTSERIES_H
#define SCALARENTITYMEASUREMENTSERIES_H

#include <QObject>
#include <QtXml>

#include "core_export.h"
#include "geometry.h"

namespace oi {

/*!
 * \brief The ScalarEntityMeasurementSeries class
 */
class OI_CORE_EXPORT ScalarEntityMeasurementSeries : public Geometry {
  friend struct codec::Access;  // the protobuf codec (see src/codec)
  Q_OBJECT

 public:
  explicit ScalarEntityMeasurementSeries(QObject* parent = nullptr);
  explicit ScalarEntityMeasurementSeries(const double& seriesValue,
                                         QObject* parent = nullptr);

  ScalarEntityMeasurementSeries(const ScalarEntityMeasurementSeries& copy,
                                QObject* parent = nullptr);

  ScalarEntityMeasurementSeries& operator=(
      const ScalarEntityMeasurementSeries& copy);

  ~ScalarEntityMeasurementSeries() override;

  // ########################################
  // order of unknown parameters (Qxx-matrix)
  // ########################################

  enum MeasurementSeriesUnknowns { unknownMeasurementSeries = 0 };

  // ########################################
  // get or set measurement series parameters
  // ########################################

  [[nodiscard]] const double& getSeriesValue() const;

  void setSeriesValue(const double& seriesValue);

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

  [[nodiscard]] QString getDisplayMeasurementSeries(
      const UnitType& type, const int& digits,
      const bool& showDiff = false) const override;

 private:
  // #############################
  // measurement series attributes
  // #############################

  double seriesValue{};
};

}  // namespace oi

#endif  // SCALARENTITYMEASUREMENTSERIES_H
