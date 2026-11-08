#include "exchange_interface.h"

#include <map>
#include <utility>
#include <vector>

using namespace oi;

/*!
 * \brief ExchangeInterface::ExchangeInterface
 */
ExchangeInterface::ExchangeInterface() {
  // init units
  this->units.insert_or_assign(DimensionType::eMetric, UnitType::eUnitMeter);
  this->units.insert_or_assign(DimensionType::eAngular,
                               UnitType::eUnitDecimalDegree);
  this->units.insert_or_assign(DimensionType::eTemperature,
                               UnitType::eUnitGrad);
}

/*!
 * \brief ExchangeInterface::~ExchangeInterface
 */
ExchangeInterface::~ExchangeInterface() = default;

/*!
 * \brief ExchangeInterface::getMetaData
 * \return
 */
const PluginMetaData& ExchangeInterface::getMetaData() const {
  return this->metaData;
}

/*!
 * \brief ExchangeInterface::getSupportedGeometries
 * \return
 */
const std::vector<GeometryTypes>& ExchangeInterface::getSupportedGeometries() {
  return this->supportedGeometries;
}

/*!
 * \brief ExchangeInterface::getStream
 * \return
 */
std::iostream* ExchangeInterface::getStream() const { return this->stream; }

/*!
 * \brief ExchangeInterface::setStream
 * \param stream
 */
void ExchangeInterface::setStream(std::iostream* stream) {
  this->stream = stream;
}

/*!
 * \brief ExchangeInterface::getFeatures
 * \return
 */
const std::vector<oi::Guarded<FeatureWrapper> >&
ExchangeInterface::getFeatures() const {
  return this->features;
}

/*!
 * \brief ExchangeInterface::setFeatures
 * \param features
 */
void ExchangeInterface::setFeatures(
    const std::vector<oi::Guarded<FeatureWrapper> >& features) {
  this->features = features;
}

std::vector<std::unique_ptr<Feature> >
ExchangeInterface::takeImportedFeatures() {
  return std::exchange(this->importedFeatures, {});
}

void ExchangeInterface::addImportedFeature(std::unique_ptr<Feature> feature) {
  if (!feature || feature->getFeatureWrapper().isNull()) {
    return;
  }
  this->features.push_back(feature->getFeatureWrapper());
  this->importedFeatures.push_back(std::move(feature));
}

/*!
 * \brief ExchangeInterface::getExportObservations
 * \return
 */
const bool& ExchangeInterface::getExportObservations() const {
  return this->exportObservations;
}

/*!
 * \brief ExchangeInterface::setExportObservations
 * \param exportObservations
 */
void ExchangeInterface::setExportObservations(const bool& exportObservations) {
  this->exportObservations = exportObservations;
}

/*!
 * \brief ExchangeInterface::getGroupName
 * \return
 */
const std::string& ExchangeInterface::getGroupName() const {
  return this->groupName;
}

/*!
 * \brief ExchangeInterface::setGroupName
 * \param groupName
 */
void ExchangeInterface::setGroupName(const std::string& groupName) {
  this->groupName = groupName;
}

/*!
 * \brief ExchangeInterface::getNominalSystem
 * \return
 */
const oi::Guarded<CoordinateSystem>& ExchangeInterface::getNominalSystem()
    const {
  return this->nominalSystem;
}

/*!
 * \brief ExchangeInterface::setNominalSystem
 * \param nominalSystem
 */
void ExchangeInterface::setNominalSystem(
    const oi::Guarded<CoordinateSystem>& nominalSystem) {
  this->nominalSystem = nominalSystem;
}

/*!
 * \brief ExchangeInterface::getUnits
 * \return
 */
const std::map<DimensionType, UnitType>& ExchangeInterface::getUnits() const {
  return this->units;
}

/*!
 * \brief ExchangeInterface::setUnit
 * \param dimension
 * \param unit
 */
void ExchangeInterface::setUnit(const DimensionType& dimension,
                                const UnitType& unit) {
  this->units.insert_or_assign(dimension, unit);
}

/*!
 * \brief ExchangeInterface::importOiData
 */
void ExchangeInterface::importOiData(const OperationContext& /*ctx*/) {
  this->sendMessage(
      std::string(
          "Exchange Plugin not implemented correctly: import method missing"),
      MessageTypes::eCriticalMessage, MessageDestinations::eMessageBoxMessage);
}

/*!
 * \brief ExchangeInterface::exportOiData
 */
void ExchangeInterface::exportOiData(const OperationContext& /*ctx*/) {
  this->sendMessage(
      std::string(
          "Exchange Plugin not implemented correctly: export method missing"),
      MessageTypes::eCriticalMessage, MessageDestinations::eMessageBoxMessage);
}

/*!
 * \brief ExchangeInterface::init
 */
void ExchangeInterface::init() {}
