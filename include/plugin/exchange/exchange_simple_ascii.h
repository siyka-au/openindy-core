#ifndef EXCHANGESIMPLEASCII_H
#define EXCHANGESIMPLEASCII_H
#include <map>
#include <vector>

#include "core_export.h"
#include "exchange_interface.h"
#include "oi_job.h"

namespace oi {

/*!
 * \brief The ExchangeSimpleAscii class
 * Interface that shall be used for simple not standardized ascii files.
 * The user of OpenIndy can select the meaning of each column of the file on his
 * own.
 */
class OI_CORE_EXPORT ExchangeSimpleAscii : public ExchangeInterface {
 public:
  explicit ExchangeSimpleAscii();

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

  [[nodiscard]] virtual std::vector<ExchangeSimpleAscii::ColumnType>
  getDefaultColumnOrder(const GeometryTypes& typeOfGeometry) const;
  [[nodiscard]]
  virtual std::map<ExchangeSimpleAscii::ColumnType, std::vector<oi::Value>>
  getFilePreview(const GeometryTypes& typeOfGeometry) const;
  [[nodiscard]] virtual std::vector<ExchangeSimpleAscii::ColumnType>
  getPossibleColumns(const GeometryTypes& typeOfGeometry) const;

  [[nodiscard]] const std::vector<std::string>& getSupportedDelimiters() const;

  [[nodiscard]] const std::vector<ExchangeSimpleAscii::ColumnType>&
  getUserDefinedColumns() const;
  void setUserDefinedColumns(
      const std::vector<ExchangeSimpleAscii::ColumnType>& userDefinedColumns);

  [[nodiscard]] const GeometryTypes& getTypeOfGeometry() const;
  void setGeometryType(const GeometryTypes& typeOfGeometry);

  [[nodiscard]] const bool& getSkipFirstLine() const;
  void setSkipFirstLine(const bool& skipFirstLine);

  [[nodiscard]] const std::string& getUsedDelimiter() const;
  void setDelimiter(const std::string& usedDelimiter);

  void setCurrentJob(oi::Guarded<OiJob> job);
  oi::Guarded<OiJob> getCurrentJob();

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

  std::vector<ExchangeSimpleAscii::ColumnType>
      userDefinedColumns;  // an ordered list of enum values that specify what
                           // to do with each column
  GeometryTypes typeOfGeometry{
      GeometryTypes::eUndefinedGeometry};  // specifies which type of geometry
                                           // shall be im- or exported
  bool skipFirstLine{};  // Skip the first line in a file during import / do not
                         // write headers to file during export
  std::string
      usedDelimiter;  // the delimiter that shall be used when parsing the file

  std::vector<std::string> supportedDelimiters;

  oi::Guarded<OiJob> currentJob;

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

#include "plugin_abi.h"
#include "value.h"

#define EXCHANGE_SIMPLEASCII_PREFIX \
  de.openIndy.plugin.exchange.exchangeSimpleAscii.v
#define ExchangeSimpleAscii_iidd \
  STR2(CONCAT2(EXCHANGE_SIMPLEASCII_PREFIX, OI_PLUGIN_ABI_VERSION))

#endif  // EXCHANGESIMPLEASCII_H
