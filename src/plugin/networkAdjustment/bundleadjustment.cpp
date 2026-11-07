#include "bundleadjustment.h"

#include <utility>

#include "snapshot.h"

using namespace oi;

/*!
 * \brief BundleAdjustment::BundleAdjustment
 * \param parent
 */
BundleAdjustment::BundleAdjustment(QObject* parent) : QObject(parent) {}

/*!
 * \brief BundleAdjustment::~BundleAdjustment
 */
BundleAdjustment::~BundleAdjustment() {}

/*!
 * \brief BundleAdjustment::init
 */
void BundleAdjustment::init() {}

/*!
 * \brief BundleAdjustment::runBundle
 * \return
 */
bool BundleAdjustment::runBundle() {
  emit this->sendMessage(
      "Bundle \"" + this->getMetaData().name + "\" not implemented",
      MessageTypes::eCriticalMessage, MessageDestinations::eMessageBoxMessage);
  return false;
}

/*!
 * \brief BundleAdjustment::getMetaData
 * \return
 */
const PluginMetaData& BundleAdjustment::getMetaData() const {
  return this->metaData;
}

/*!
 * \brief BundleAdjustment::getIntegerParameter
 * \return
 */
const QMap<QString, int>& BundleAdjustment::getIntegerParameter() const {
  return this->integerParameters;
}

/*!
 * \brief BundleAdjustment::getDoubleParameter
 * \return
 */
const QMap<QString, double>& BundleAdjustment::getDoubleParameter() const {
  return this->doubleParameters;
}

/*!
 * \brief BundleAdjustment::getStringParameter
 * \return
 */
const QMultiMap<QString, QString>& BundleAdjustment::getStringParameter()
    const {
  return this->stringParameters;
}

/*!
 * \brief BundleAdjustment::getScalarInputParams
 * \return
 */
const ScalarInputParams& BundleAdjustment::getScalarInputParams() {
  // use default values if no scalar input params have been specified
  if (!this->scalarInputParams.isValid) {
    this->scalarInputParams.doubleParameter = this->doubleParameters;
    this->scalarInputParams.intParameter = this->integerParameters;
    QStringList keys = this->stringParameters.keys();
    for (const QString& key : std::as_const(keys)) {
      this->scalarInputParams.stringParameter.insert(
          key, this->stringParameters.value(key));
    }
  }

  return this->scalarInputParams;
}

/*!
 * \brief BundleAdjustment::setScalarInputParams
 * \param params
 */
void BundleAdjustment::setScalarInputParams(const ScalarInputParams& params) {
  this->scalarInputParams = params;
  this->scalarInputParams.isValid = true;
}

/*!
 * \brief BundleAdjustment::getInputStations
 * \return
 */
const QList<BundleStation>& BundleAdjustment::getInputStations() const {
  return this->stations;
}

/*!
 * \brief BundleAdjustment::setInputStations
 * \param stations
 */
void BundleAdjustment::setInputStations(const QList<BundleStation>& stations) {
  this->stations = stations;
}

/*!
 * \brief BundleAdjustment::getBaseStation
 * \return
 */
const BundleStation& BundleAdjustment::getBaseStation() const {
  return this->baseSystem;
}

/*!
 * \brief BundleAdjustment::setBaseStation
 * \param station
 */
void BundleAdjustment::setBaseStation(const BundleStation& station) {
  this->baseSystem = station;
}

/*!
 * \brief BundleAdjustment::getBundleSystem
 * \return
 */
const QPointer<CoordinateSystem>& BundleAdjustment::getBundleSystem() const {
  return this->bundleSystem;
}

/*!
 * \brief BundleAdjustment::setBundleSystem
 * \param system
 */
void BundleAdjustment::setBundleSystem(
    const QPointer<CoordinateSystem>& system) {
  this->bundleSystem = system;
}

/*!
 * \brief BundleAdjustment::getCurrentJob
 * \return
 */
const QPointer<OiJob>& BundleAdjustment::getCurrentJob() const {
  return this->currentJob;
}

/*!
 * \brief BundleAdjustment::setCurrentJob
 * \param job
 */
void BundleAdjustment::setCurrentJob(const QPointer<OiJob>& job) {
  // disconnect old job
  this->disconnectJob();

  // set and connect new job
  this->currentJob = job;
  this->connectJob();
}

/*!
 * \brief BundleAdjustment::getId
 * \return
 */
const int& BundleAdjustment::getId() const { return this->id; }

/*!
 * \brief BundleAdjustment::getOutputGeometries
 * \return
 */
const QList<BundleGeometry>& BundleAdjustment::getOutputGeometries() const {
  return this->geometries;
}

/*!
 * \brief BundleAdjustment::getOutputTransformations
 * \return
 */
const QList<BundleTransformation>& BundleAdjustment::getOutputTransformations()
    const {
  return this->transformations;
}

/*!
 * \brief BundleAdjustment::clear
 */
void BundleAdjustment::clear() {
  this->scalarInputParams.isValid = false;
  this->stations.clear();
  this->baseSystem = BundleStation();
  this->clearResults();
}

/*!
 * \brief BundleAdjustment::clearResults
 */
void BundleAdjustment::clearResults() {
  this->geometries.clear();
  this->transformations.clear();
}

/*!
 * \brief BundleAdjustment::connectJob
 */
void BundleAdjustment::connectJob() {
  // check job
  if (this->currentJob.isNull()) {
    return;
  }

  this->messageSink = this->currentJob.data();
}

/*!
 * \brief BundleAdjustment::disconnectJob
 */
void BundleAdjustment::disconnectJob() {
  // check job
  if (this->currentJob.isNull()) {
    return;
  }

  if (this->messageSink == this->currentJob.data()) {
    this->messageSink = nullptr;
  }
}
