#include "exchangeinterface.h"

#include <map>
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
ExchangeInterface::~ExchangeInterface() {
  // delete device if not deleted yet
  if (!this->device.isNull()) {
    delete this->device.data();
  }
}

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
 * \brief ExchangeInterface::getDevice
 * \return
 */
const QPointer<QIODevice>& ExchangeInterface::getDevice() const {
  return this->device;
}

/*!
 * \brief ExchangeInterface::setDevice
 * \param device
 */
void ExchangeInterface::setDevice(const QPointer<QIODevice>& device) {
  this->device = device;
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
const QString& ExchangeInterface::getGroupName() const {
  return this->groupName;
}

/*!
 * \brief ExchangeInterface::setGroupName
 * \param groupName
 */
void ExchangeInterface::setGroupName(const QString& groupName) {
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
void ExchangeInterface::importOiData() {
  this->sendMessage(
      QString(
          "Exchange Plugin not implemented correctly: import method missing"),
      MessageTypes::eCriticalMessage, MessageDestinations::eMessageBoxMessage);
}

/*!
 * \brief ExchangeInterface::exportOiData
 */
void ExchangeInterface::exportOiData() {
  this->sendMessage(
      QString(
          "Exchange Plugin not implemented correctly: export method missing"),
      MessageTypes::eCriticalMessage, MessageDestinations::eMessageBoxMessage);
}

/*!
 * \brief ExchangeInterface::init
 */
void ExchangeInterface::init() {}
