#include "exchange_simple_ascii.h"

#include <map>
#include <vector>

#include "value.h"

using namespace oi;

/*!
 * \brief ExchangeSimpleAscii::ExchangeSimpleAscii
 */
ExchangeSimpleAscii::ExchangeSimpleAscii() : ExchangeInterface() {
  this->distanceDigits = 2;
  this->angleDigits = 6;
  this->temperatureDigits = 1;
}

/*!
 * \brief ExchangeSimpleAscii::~ExchangeSimpleAscii
 */
ExchangeSimpleAscii::~ExchangeSimpleAscii() {}

/*!
 * \brief ExchangeSimpleAscii::getDefaultColumnOrder
 * Returns an ordered list of columns that a file possibly contains (for a given
 * geometry type)
 * \param typeOfGeometry
 * \return
 */
std::vector<ExchangeSimpleAscii::ColumnType>
ExchangeSimpleAscii::getDefaultColumnOrder(
    const GeometryTypes& /*typeOfGeometry*/) const {
  return std::vector<ExchangeSimpleAscii::ColumnType>();
}

/*!
 * \brief ExchangeSimpleAscii::getFilePreview
 * Returns a list of sample file entries for each column specified in
 * getDefaultColumnOrder()
 * \param typeOfGeometry
 * \return
 */
std::map<ExchangeSimpleAscii::ColumnType, std::vector<oi::Value>>
ExchangeSimpleAscii::getFilePreview(
    const GeometryTypes& /*typeOfGeometry*/) const {
  return std::map<ExchangeSimpleAscii::ColumnType, std::vector<oi::Value>>();
}

/*!
 * \brief ExchangeSimpleAscii::getPossibleColumns
 * Returns a list of possible columns for a given geometry type
 * \param typeOfGeometry
 * \return
 */
std::vector<ExchangeSimpleAscii::ColumnType>
ExchangeSimpleAscii::getPossibleColumns(
    const GeometryTypes& /*typeOfGeometry*/) const {
  return std::vector<ExchangeSimpleAscii::ColumnType>();
}

/*!
 * \brief ExchangeSimpleAscii::getSupportedDelimiters
 * \return
 */
const std::vector<std::string>& ExchangeSimpleAscii::getSupportedDelimiters()
    const {
  return this->supportedDelimiters;
}

/*!
 * \brief ExchangeSimpleAscii::getUserDefinedColumns
 * Get the meaning of each column
 * \return
 */
const std::vector<ExchangeSimpleAscii::ColumnType>&
ExchangeSimpleAscii::getUserDefinedColumns() const {
  return this->userDefinedColumns;
}

/*!
 * \brief ExchangeSimpleAscii::setUserDefinedColumns
 * Specify the meaning of each column
 * \param userDefinedColumns
 */
void ExchangeSimpleAscii::setUserDefinedColumns(
    const std::vector<ExchangeSimpleAscii::ColumnType>& userDefinedColumns) {
  this->userDefinedColumns = userDefinedColumns;
}

/*!
 * \brief ExchangeSimpleAscii::getTypeOfGeometry
 * Returns the type of geometry that shall be im- or exported
 * \return
 */
const GeometryTypes& ExchangeSimpleAscii::getTypeOfGeometry() const {
  return this->typeOfGeometry;
}

/*!
 * \brief ExchangeSimpleAscii::setGeometryType
 * Specify which type of geometry shall be im- or exported
 * \param typeOfGeometry
 */
void ExchangeSimpleAscii::setGeometryType(const GeometryTypes& typeOfGeometry) {
  if (this->typeOfGeometry != typeOfGeometry) {
    this->typeOfGeometry = typeOfGeometry;
    this->userDefinedColumns.clear();
  }
}

/*!
 * \brief ExchangeSimpleAscii::getSkipFirstLine
 * \return
 */
const bool& ExchangeSimpleAscii::getSkipFirstLine() const {
  return this->skipFirstLine;
}

/*!
 * \brief ExchangeSimpleAscii::setSkipFirstLine
 * Skip the first line in a file during import / do not write headers to file
 * during export
 * \param skipFirstLine
 */
void ExchangeSimpleAscii::setSkipFirstLine(const bool& skipFirstLine) {
  this->skipFirstLine = skipFirstLine;
}

/*!
 * \brief ExchangeSimpleAscii::getUsedDelimiter
 * \return
 */
const std::string& ExchangeSimpleAscii::getUsedDelimiter() const {
  return this->usedDelimiter;
}

/*!
 * \brief ExchangeSimpleAscii::setDelimiter
 * Set the delimiter that shall be used when parsing the file
 * \param usedDelimiter
 */
void ExchangeSimpleAscii::setDelimiter(const std::string& usedDelimiter) {
  this->usedDelimiter = usedDelimiter;
}

/*!
 * \brief ExchangeSimpleAscii::setCurrentJob
 * \param job
 */
void ExchangeSimpleAscii::setCurrentJob(oi::Guarded<OiJob> job) {
  this->currentJob = job;
}

/*!
 * \brief ExchangeSimpleAscii::getCurrentJob
 * \return
 */
oi::Guarded<OiJob> ExchangeSimpleAscii::getCurrentJob() {
  return this->currentJob;
}

/*!
 * \brief ExchangeSimpleAscii::setDistanceDigits
 * \param value
 */
void ExchangeSimpleAscii::setDistanceDigits(int value) {
  this->distanceDigits = value;
}

/*!
 * \brief ExchangeSimpleAscii::getDistanceDigits
 * \return
 */
int ExchangeSimpleAscii::getDistanceDigits() { return this->distanceDigits; }

/*!
 * \brief ExchangeSimpleAscii::setAngleDigits
 * \param value
 */
void ExchangeSimpleAscii::setAngleDigits(int value) {
  this->angleDigits = value;
}

/*!
 * \brief ExchangeSimpleAscii::getAngleDigits
 * \return
 */
int ExchangeSimpleAscii::getAngleDigits() { return this->angleDigits; }

/*!
 * \brief ExchangeSimpleAscii::setTemperatureDigits
 * \param value
 */
void ExchangeSimpleAscii::setTemperatureDigits(int value) {
  this->temperatureDigits = value;
}

/*!
 * \brief ExchangeSimpleAscii::getTemperatureDigits
 * \return
 */
int ExchangeSimpleAscii::getTemperatureDigits() {
  return this->temperatureDigits;
}
