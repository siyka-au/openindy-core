#ifndef EXCHANGESIMPLEASCII_H
#define EXCHANGESIMPLEASCII_H

#include <QVariantList>

#include "exchangeinterface.h"
#include "oijob.h"

namespace oi {

/*!
 * \brief The ExchangeSimpleAscii class
 * Interface that shall be used for simple not standardized ascii files.
 * The user of OpenIndy can select the meaning of each column of the file on his
 * own.
 */
class OI_CORE_EXPORT ExchangeSimpleAscii : public ExchangeInterface {
  Q_OBJECT

 public:
  explicit ExchangeSimpleAscii(QObject* parent = nullptr);

  ~ExchangeSimpleAscii() override;

  // ###############################################
  // all possible column types (geometry attributes)
  // ###############################################

  enum ColumnType {
    eColumnIgnore,
    eColumnFeatureName,
    eColumnGroupName,
    eColumnComment,
    eColumnX,
    eColumnY,
    eColumnZ,
    eColumnPrimaryI,
    eColumnPrimaryJ,
    eColumnPrimaryK,
    eColumnSecondaryI,
    eColumnSecondaryJ,
    eColumnSecondaryK,
    eColumnRadiusA,
    eColumnRadiusB,
    eColumnAperture,
    eColumnA,
    eColumnB,
    eColumnC,
    eColumnAngle,
    eColumnDistance,
    eColumnMeasurementSeries,
    eColumnTemperature,
    eColumnLength,
    eColumnCommonState
  };

  // #########################################
  // methods to get or set further information
  // #########################################

  virtual QList<ExchangeSimpleAscii::ColumnType> getDefaultColumnOrder(
      const GeometryTypes& typeOfGeometry) const;
  virtual QMap<ExchangeSimpleAscii::ColumnType, QVariantList> getFilePreview(
      const GeometryTypes& typeOfGeometry) const;
  virtual QList<ExchangeSimpleAscii::ColumnType> getPossibleColumns(
      const GeometryTypes& typeOfGeometry) const;

  const QStringList& getSupportedDelimiters() const;

  const QList<ExchangeSimpleAscii::ColumnType>& getUserDefinedColumns() const;
  void setUserDefinedColumns(
      const QList<ExchangeSimpleAscii::ColumnType>& userDefinedColumns);

  const GeometryTypes& getTypeOfGeometry() const;
  void setGeometryType(const GeometryTypes& typeOfGeometry);

  const bool& getSkipFirstLine() const;
  void setSkipFirstLine(const bool& skipFirstLine);

  const QString& getUsedDelimiter() const;
  void setDelimiter(const QString& usedDelimiter);

  void setCurrentJob(QPointer<OiJob> job);
  QPointer<OiJob> getCurrentJob();

  void setDistanceDigits(int value);
  int getDistanceDigits();

  void setAngleDigits(int value);
  int getAngleDigits();

  void setTemperatureDigits(int value);
  int getTemperatureDigits();

 protected:
  // ###########################
  // input and output parameters
  // ###########################

  QList<ExchangeSimpleAscii::ColumnType>
      userDefinedColumns;  // an ordered list of enum values that specify what
                           // to do with each column
  GeometryTypes typeOfGeometry{
      GeometryTypes::eUndefinedGeometry};  // specifies which type of geometry
                                           // shall be im- or exported
  bool skipFirstLine{};  // Skip the first line in a file during import / do not
                         // write headers to file during export
  QString
      usedDelimiter;  // the delimiter that shall be used when parsing the file

  QStringList supportedDelimiters;

  QPointer<OiJob> currentJob;

  int distanceDigits;
  int angleDigits;
  int temperatureDigits;
};

}  // namespace oi

#ifndef STR
#define STR(x) #x
#endif
#ifndef STR2
#define STR2(x) STR(x)
#endif

#ifndef CONCAT
#define CONCAT(a, b) a##b
#endif
#ifndef CONCAT2
#define CONCAT2(a, b) CONCAT(a, b)
#endif

#include "pluginabi.h"

#define EXCHANGE_SIMPLEASCII_PREFIX \
  de.openIndy.plugin.exchange.exchangeSimpleAscii.v
#define ExchangeSimpleAscii_iidd \
  STR2(CONCAT2(EXCHANGE_SIMPLEASCII_PREFIX, OI_PLUGIN_ABI_VERSION))

#endif  // EXCHANGESIMPLEASCII_H
