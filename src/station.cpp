#include "station.h"

#include <utility>

#include "coordinatesystem.h"
#include "featurewrapper.h"
#include "oijob.h"
#include "reading.h"
#include "sensor.h"
#include "sensorcontrol.h"
#include "snapshot.h"

using namespace oi;

/*!
 * \brief Station::Station
 */
Station::Station() : Feature(), isActiveStation(false) {
  // set up feature wrapper
  if (!this->selfFeature.isNull()) {
    this->selfFeature->setStation(this);
  }

  // create a point object as the station's position
  this->position = new Point();

  // create a coordinate system object as the station's coordinate system
  this->stationSystem = new CoordinateSystem(oi::Guarded<Station>(this));

  // create the sensor control object
  this->createSensorControl();
}

/*!
 * \brief Station::Station
 * \param name
 */
Station::Station(const QString& name) : Feature(), isActiveStation(false) {
  // set up feature wrapper
  if (!this->selfFeature.isNull()) {
    this->selfFeature->setStation(this);
  }

  this->name = name;

  // create a point object as the station's position
  this->position = new Point();
  this->position->setFeatureName(name);

  // create a coordinate system object as the station's coordinate system
  this->stationSystem = new CoordinateSystem(oi::Guarded<Station>(this));
  this->stationSystem->setFeatureName(this->name);

  // create the sensor control object
  this->createSensorControl();
}

/*!
 * \brief Station::Station
 * \param copy
 */
Station::Station(const Station& copy) : Feature(copy) {
  // set up feature wrapper
  if (!this->selfFeature.isNull()) {
    this->selfFeature->setStation(this);
  }

  // copy general station attributes
  this->isActiveStation = copy.isActiveStation;
  if (!copy.position.isNull()) {
    this->position = new Point(*copy.position.data());
  }
}

/*!
 * \brief Station::operator =
 * \param copy
 * \return
 */
Station& Station::operator=(const Station& copy) {
  // set up feature wrapper
  if (!this->selfFeature.isNull()) {
    this->selfFeature->setStation(this);
  }

  // copy general station attributes
  this->isActiveStation = copy.isActiveStation;
  if (!copy.position.isNull()) {
    this->position = new Point(*copy.position.data());
  }

  return *this;
}

/*!
 * \brief Station::~Station
 */
Station::~Station() {
  // stop the sensor first: afterwards no result is delivered to anybody
  this->sensorControl.reset();

  // tell the observer, then forget it
  if (StationObserver* observer = this->stationObserver.exchange(nullptr);
      observer != nullptr) {
    observer->onStationDestroyed(this);
  }
  this->sensorResults.reset();

  // delete corresponding coordinate system with all observations made from this
  // station
  if (!this->stationSystem.isNull()) {
    delete this->stationSystem.data();
  }

  // delete position of this station
  if (!this->position.isNull()) {
    delete this->position.data();
  }
}

/*!
 * \brief Station::getIsActiveStation
 * \return
 */
const bool& Station::getIsActiveStation() const {
  return this->isActiveStation;
}

/*!
 * \brief Station::setActiveStationState
 * \param isActiveStation
 */
void Station::setActiveStationState(const bool& isActiveStation) {
  if (this->isActiveStation != isActiveStation) {
    this->isActiveStation = isActiveStation;
    this->notify(&FeatureObserver::onActiveStationChanged, this->id);
  }
}

/*!
 * \brief Station::getPosition
 * \return
 */
const oi::Guarded<Point>& Station::getPosition() const {
  return this->position;
}

/*!
 * \brief Station::getXAxis
 * \return
 */
const Direction& Station::getXAxis() const { return this->xAxis; }

/*!
 * \brief Station::getYAxis
 * \return
 */
const Direction& Station::getYAxis() const { return this->yAxis; }

/*!
 * \brief Station::getZAxis
 * \return
 */
const Direction& Station::getZAxis() const { return this->zAxis; }

/*!
 * \brief Station::setCoordinateSystem
 * \param origin
 * \param xAxis
 * \param yAxis
 * \param zAxis
 */
void Station::setCoordinateSystem(const Position& origin,
                                  const Direction& xAxis,
                                  const Direction& yAxis,
                                  const Direction& zAxis) {
  this->position->setPoint(origin);
  this->xAxis = xAxis;
  this->yAxis = yAxis;
  this->zAxis = zAxis;
}

void Station::setEstimatedCoordinateSystem(oi::Guarded<CoordinateSystem> cs) {
  this->estimatedStationSystem = cs;
}
const oi::Guarded<CoordinateSystem>& Station::getEstimatedCoordinateSystem()
    const {
  return this->estimatedStationSystem;
}

/*!
 * \brief Station::getCoordinateSystem
 * \return
 */
const oi::Guarded<CoordinateSystem>& Station::getCoordinateSystem() const {
  return this->stationSystem;
}

/*!
 * \brief Station::getSensorConfiguration
 * \return
 */
SensorConfiguration Station::getSensorConfiguration() {
  // check sensor control
  if (!this->sensorControl) {
    return SensorConfiguration();
  }

  return this->sensorControl->getSensorConfiguration();
}

/*!
 * \brief Station::setSensorConfiguration
 * \param sConfig
 */
void Station::setSensorConfiguration(const SensorConfiguration& sConfig) {
  // check sensor control
  if (!this->sensorControl) {
    return;
  }

  this->sensorControl->setSensorConfiguration(sConfig);
}

/*!
 * \brief Station::getSensor
 * \return
 */
Sensor Station::getSensor() const {
  // check sensor control
  if (!this->sensorControl) {
    return Sensor();
  }

  return this->sensorControl->getSensor();
}

/*!
 * \brief Station::setSensor
 * \param sensor
 */
void Station::setSensor(const oi::Guarded<Sensor>& sensor) {
  // check active station state
  if (!this->isActiveStation) {
    return;
  }

  // check sensor
  if (sensor.isNull()) {
    return;
  }

  // check sensor control
  if (!this->sensorControl) {
    return;
  }

  this->sensorControl->setSensor(sensor);
  this->notify(&FeatureObserver::onSensorChanged, this->id);
}

/*!
 * \brief Station::takeSensor
 * \return
 */
oi::Guarded<Sensor> Station::takeSensor() {
  // check sensor control
  if (!this->sensorControl) {
    return oi::Guarded<Sensor>(nullptr);
  }

  oi::Guarded<Sensor> sensor = this->sensorControl->takeSensor();

  this->notify(&FeatureObserver::onSensorChanged, this->id);

  return sensor;
}

/*!
 * \brief Station::resetSensor
 */
void Station::resetSensor() {
  // check sensor control
  if (!this->sensorControl) {
    return;
  }

  this->sensorControl->resetSensor();
  this->notify(&FeatureObserver::onSensorChanged, this->id);
}

/*!
 * \brief Station::getUsedSensors
 * \return
 */
const QList<Sensor> Station::getUsedSensors() const {
  // check sensor control
  if (!this->sensorControl) {
    return QList<Sensor>();
  }

  return this->sensorControl->getUsedSensors();
}

/*!
 * \brief Station::getTargetGeometries
 * \return
 */
QList<oi::Guarded<Geometry>> Station::getTargetGeometries() const {
  QList<oi::Guarded<Geometry>> geometries;

  // get a list of all readings
  QList<oi::Guarded<Reading>> readings;
  for (const oi::Guarded<Reading>& reading :
       oi::snapshot(this->cartesianReadings)) {
    readings.append(reading);
  }
  for (const oi::Guarded<Reading>& reading :
       oi::snapshot(this->directionReadings)) {
    readings.append(reading);
  }
  for (const oi::Guarded<Reading>& reading :
       oi::snapshot(this->distanceReadings)) {
    readings.append(reading);
  }
  for (const oi::Guarded<Reading>& reading :
       oi::snapshot(this->polarReadings)) {
    readings.append(reading);
  }
  for (const oi::Guarded<Reading>& reading :
       oi::snapshot(this->levelReadings)) {
    readings.append(reading);
  }
  for (const oi::Guarded<Reading>& reading :
       oi::snapshot(this->temperatureRadings)) {
    readings.append(reading);
  }
  for (const oi::Guarded<Reading>& reading :
       oi::snapshot(this->undefinedReadings)) {
    readings.append(reading);
  }

  // get target geometries
  for (const oi::Guarded<Reading>& reading : std::as_const(readings)) {
    // check reading
    if (reading.isNull() || reading->getObservation().isNull()) {
      continue;
    }

    // add target geometries of the observation
    const QList<oi::Guarded<Geometry>>& targetGeometries =
        reading->getObservation()->getTargetGeometries();
    for (const oi::Guarded<Geometry>& geom : std::as_const(targetGeometries)) {
      if (!geom.isNull() && !geometries.contains(geom)) {
        geometries.append(geom);
      }
    }
  }

  return geometries;
}

/*!
 * \brief Station::setUsedSensors
 * \param sensors
 */
void Station::setUsedSensors(const QList<Sensor>& sensors) {
  // check sensor control
  if (!this->sensorControl) {
    return;
  }

  return this->sensorControl->setUsedSensors(sensors);
}

/*!
 * \brief Station::getIsSensorSet
 * \return
 */
bool Station::getIsSensorSet() {
  // check sensor control
  if (!this->sensorControl) {
    return false;
  }

  return this->sensorControl->getIsSensorSet();
}

/*!
 * \brief Station::getIsSensorConnected
 * \return
 */
bool Station::getIsSensorConnected() {
  // check sensor control
  if (!this->sensorControl) {
    return false;
  }

  return this->sensorControl->getIsSensorConnected();
}

/*!
 * \brief Station::getIsReadyForMeasurement
 * \return
 */
bool Station::getIsReadyForMeasurement() {
  // check sensor control
  if (!this->sensorControl) {
    return false;
  }

  return this->sensorControl->getIsReadyForMeasurement();
}

/*!
 * \brief Station::getIsBusy
 * \return
 */
bool Station::getIsBusy() {
  // check sensor control
  if (!this->sensorControl) {
    return false;
  }

  return this->sensorControl->getIsBusy();
}

/*!
 * \brief Station::getSensorStatus
 * \return
 */
QMap<QString, QString> Station::getSensorStatus() {
  // check sensor control
  if (!this->sensorControl) {
    return QMap<QString, QString>();
  }

  return this->sensorControl->getSensorStatus();
}

/*!
 * \brief Station::getActiveSensorType
 * \return
 */
SensorType Station::getActiveSensorType() const {
  // check sensor control
  if (!this->sensorControl) {
    return SensorType();
  }

  return this->sensorControl->getActiveSensorType();
}

/*!
 * \brief Station::getSupportedReadingTypes
 * \return
 */
QList<ReadingTypes> Station::getSupportedReadingTypes() const {
  // check sensor control
  if (!this->sensorControl) {
    return QList<ReadingTypes>();
  }

  return this->sensorControl->getSupportedReadingTypes();
}

/*!
 * \brief Station::getSupportedConnectionTypes
 * \return
 */
QList<ConnectionKind> Station::getSupportedConnectionTypes() const {
  // check sensor control
  if (!this->sensorControl) {
    return QList<ConnectionKind>();
  }

  return this->sensorControl->getSupportedConnectionTypes();
}

/*!
 * \brief Station::getSupportedSensorActions
 * \return
 */
QList<SensorFunctions> Station::getSupportedSensorActions() const {
  // check sensor control
  if (!this->sensorControl) {
    return QList<SensorFunctions>();
  }

  return this->sensorControl->getSupportedSensorActions();
}

/*!
 * \brief Station::getSelfDefinedActions
 * \return
 */
QStringList Station::getSelfDefinedActions() const {
  // check sensor control
  if (!this->sensorControl) {
    return QStringList();
  }

  return this->sensorControl->getSelfDefinedActions();
}

/*!
 * \brief Station::getStreamFormat
 * \return
 */
ReadingTypes Station::getStreamFormat() {
  // check sensor control
  if (!this->sensorControl) {
    return ReadingTypes::eUndefinedReading;
  }

  return this->sensorControl->getStreamFormat();
}

/*!
 * \brief Station::setStreamFormat
 * \param streamFormat
 */
void Station::setStreamFormat(ReadingTypes streamFormat) {
  // check sensor control
  if (!this->sensorControl) {
    return;
  }

  this->sensorControl->setStreamFormat(streamFormat);
}

/*!
 * \brief Station::recalc
 */
void Station::recalc() { Feature::recalc(); }

/*!
 * \brief Station::getDisplayX
 * \param type
 * \param digits
 * \param showDiff
 * \return
 */
QString Station::getDisplayX(const UnitType& type, const int& digits,
                             const bool& showDiff) const {
  if (!this->position.isNull()) {
    return this->position->getDisplayX(type, digits, showDiff);
  }
  return QString("-/-");
}

/*!
 * \brief Station::getDisplayY
 * \param type
 * \param digits
 * \param showDiff
 * \return
 */
QString Station::getDisplayY(const UnitType& type, const int& digits,
                             const bool& showDiff) const {
  if (!this->position.isNull()) {
    return this->position->getDisplayY(type, digits, showDiff);
  }
  return QString("-/-");
}

/*!
 * \brief Station::getDisplayZ
 * \param type
 * \param digits
 * \param showDiff
 * \return
 */
QString Station::getDisplayZ(const UnitType& type, const int& digits,
                             const bool& showDiff) const {
  if (!this->position.isNull()) {
    return this->position->getDisplayZ(type, digits, showDiff);
  }
  return QString("-/-");
}

/*!
 * \brief Plane::getDisplayPrimaryI
 * \param digits
 * \param showDiff
 * \return
 */
QString Station::getDisplayPrimaryI(const int& digits,
                                    const bool& /*showDiff*/) const {
  return QString::number(this->zAxis.getVector().getAt(0), 'f', digits);
}

/*!
 * \brief Plane::getDisplayPrimaryJ
 * \param digits
 * \param showDiff
 * \return
 */
QString Station::getDisplayPrimaryJ(const int& digits,
                                    const bool& /*showDiff*/) const {
  return QString::number(this->zAxis.getVector().getAt(1), 'f', digits);
}

/*!
 * \brief Plane::getDisplayPrimaryK
 * \param digits
 * \param showDiff
 * \return
 */
QString Station::getDisplayPrimaryK(const int& digits,
                                    const bool& /*showDiff*/) const {
  return QString::number(this->zAxis.getVector().getAt(2), 'f', digits);
}

/*!
 * \brief Station::setUpFeatureId
 * Generate a new unique id when the current job was set
 */
void Station::setUpFeatureId() {
  // check job
  if (this->job.isNull()) {
    return;
  }

  // generate unique id for station's position
  if (!this->position.isNull()) {
    this->position->id = this->job->generateUniqueId();
  }

  // generate unique id for station's coordinate system
  if (!this->stationSystem.isNull()) {
    this->stationSystem->id = this->job->generateUniqueId();
  }

  // generate unique ids for station's readings
  for (const oi::Guarded<Reading>& reading :
       oi::snapshot(this->cartesianReadings)) {
    if (!reading.isNull()) {
      reading->id = this->job->generateUniqueId();
    }
  }
  for (const oi::Guarded<Reading>& reading :
       oi::snapshot(this->polarReadings)) {
    if (!reading.isNull()) {
      reading->id = this->job->generateUniqueId();
    }
  }
  for (const oi::Guarded<Reading>& reading :
       oi::snapshot(this->directionReadings)) {
    if (!reading.isNull()) {
      reading->id = this->job->generateUniqueId();
    }
  }
  for (const oi::Guarded<Reading>& reading :
       oi::snapshot(this->distanceReadings)) {
    if (!reading.isNull()) {
      reading->id = this->job->generateUniqueId();
    }
  }
  for (const oi::Guarded<Reading>& reading :
       oi::snapshot(this->temperatureRadings)) {
    if (!reading.isNull()) {
      reading->id = this->job->generateUniqueId();
    }
  }
  for (const oi::Guarded<Reading>& reading :
       oi::snapshot(this->levelReadings)) {
    if (!reading.isNull()) {
      reading->id = this->job->generateUniqueId();
    }
  }
  for (const oi::Guarded<Reading>& reading :
       oi::snapshot(this->undefinedReadings)) {
    if (!reading.isNull()) {
      reading->id = this->job->generateUniqueId();
    }
  }

  // feature specific
  Feature::setUpFeatureId();
}

/*!
 * \brief Station::stationNameChanged
 * Update the station system name whenever the station's name has changed
 * \param featureId
 * \param oldName
 */
void Station::stationNameChanged(const int& /*featureId*/,
                                 const QString& /*oldName*/) {
  if (!this->stationSystem.isNull()) {
    this->stationSystem->setFeatureName(this->name);
  }
}

/*!
 * \brief Station::nameWasChanged
 * Keeps the station system's name in step (used to be a self-connection of
 * Feature::featureNameChanged)
 */
void Station::nameWasChanged(const QString& oldName) {
  this->stationNameChanged(this->id, oldName);
}

/*!
 * \brief Station::addReadings
 * \param geomId
 * \param readings
 */
void Station::addReadings(const int& geomId,
                          const QList<oi::Guarded<Reading>>& readings) {
  MeasurementConfigKey measurementConfig;
  if (!this->job.isNull()) {
    const oi::Guarded<FeatureWrapper> feature =
        this->job->getFeatureById(geomId);
    if (!feature.isNull() && !feature->getGeometry().isNull()) {
      measurementConfig = feature->getGeometry()->getMeasurementConfig();
    }
  }

  for (const oi::Guarded<Reading>& reading : oi::snapshot(readings)) {
    // check reading
    if (reading.isNull()) {
      continue;
    }

    // set sensor configuration of reading
    reading->setSensorConfiguration(this->getSensorConfiguration());
    reading->setMeasurementConfig(measurementConfig);

    switch (reading->getTypeOfReading()) {
      case ReadingTypes::eCartesianReading:
        this->cartesianReadings.append(reading);
        break;
      case ReadingTypes::eDirectionReading:
        this->directionReadings.append(reading);
        break;
      case ReadingTypes::eDistanceReading:
        this->distanceReadings.append(reading);
        break;
      case ReadingTypes::ePolarReading:
        this->polarReadings.append(reading);
        break;
      case ReadingTypes::eLevelReading:
        this->levelReadings.append(reading);
        break;
      case ReadingTypes::eTemperatureReading:
        this->temperatureRadings.append(reading);
        break;
      default:
        break;
    }
  }
}

/*!
 * \brief Station::removeReading
 * \param reading
 */
void Station::removeReading(const oi::Guarded<Reading>& reading) {
  this->cartesianReadings.removeOne(reading);
  this->directionReadings.removeOne(reading);
  this->distanceReadings.removeOne(reading);
  this->polarReadings.removeOne(reading);
  this->levelReadings.removeOne(reading);
  this->temperatureRadings.removeOne(reading);
  this->undefinedReadings.removeOne(reading);
}

/*!
 * \brief Station::setJob
 * \param job
 */
void Station::setJob(const oi::Guarded<OiJob>& job) {
  // check and set job
  if (job.isNull()) {
    return;
  }
  this->job = job;

  // sensor results are delivered the way the job wants them
  if (this->sensorControl) {
    this->sensorControl->setExecutor(job->getExecutor());
  }

  // pass job to the station system
  if (!this->stationSystem.isNull()) {
    this->stationSystem->setJob(job);
  }

  // pass the job to the station point
  if (!this->position.isNull()) {
    this->position->setJob(job);
  }

  // generate feature id
  this->setUpFeatureId();
}

// ###############################################################
// sensor communication
// ###############################################################

/*!
 * The observer of the sensor control. Runs wherever the executor runs the
 * results (never after the station's destructor started): takes the readings
 * of a measurement into the station and hands everything on to the station's
 * own observer.
 */
class Station::SensorResults final : public StationObserver {
 public:
  explicit SensorResults(Station& station) : station(station) {}

  void onRealTimeReading(const QVariantMap& reading) override {
    if (auto* o = this->observer()) {
      o->onRealTimeReading(reading);
    }
  }
  void onRealTimeStatus(const QMap<QString, QString>& status) override {
    if (auto* o = this->observer()) {
      o->onRealTimeStatus(status);
    }
  }
  void onConnectionLost() override {
    if (auto* o = this->observer()) {
      o->onConnectionLost();
    }
  }
  void onConnectionReceived() override {
    if (auto* o = this->observer()) {
      o->onConnectionReceived();
    }
  }
  void onIsReadyForMeasurement(bool isReady) override {
    if (auto* o = this->observer()) {
      o->onIsReadyForMeasurement(isReady);
    }
  }
  void onCommandFinished(bool success, const QString& msg,
                         SensorAction action) override {
    if (auto* o = this->observer()) {
      o->onCommandFinished(success, msg, action);
    }
  }
  void onMeasurementFinished(
      int geomId, const QList<oi::Guarded<Reading>>& readings) override {
    // the readings belong to the station before anybody hears about them
    this->station.addReadings(geomId, readings);
    if (auto* o = this->observer()) {
      o->onMeasurementFinished(geomId, readings);
    }
  }
  void onMeasurementDone(bool success) override {
    if (auto* o = this->observer()) {
      o->onMeasurementDone(success);
    }
  }
  void onSensorMessage(const QString& msg, MessageTypes msgType,
                       MessageDestinations msgDest) override {
    if (auto* o = this->observer()) {
      o->onSensorMessage(msg, msgType, msgDest);
    }
  }
  void onSensorStatus(SensorStatus status, const QString& msg) override {
    if (auto* o = this->observer()) {
      o->onSensorStatus(status, msg);
    }
  }

 private:
  [[nodiscard]] StationObserver* observer() const {
    return this->station.stationObserver.load();
  }

  Station& station;
};

void Station::createSensorControl() {
  this->sensorResults = std::make_unique<SensorResults>(*this);
  this->sensorControl = std::make_unique<SensorControl>(
      this->sensorResults.get(), &InlineExecutor::instance());
}

void Station::setStationObserver(StationObserver* observer) {
  this->stationObserver = observer;
}

StationObserver* Station::getStationObserver() const {
  return this->stationObserver;
}

void Station::setExecutor(Executor* executor) {
  if (this->sensorControl) {
    this->sensorControl->setExecutor(executor);
  }
}

SensorControl* Station::getSensorControl() const {
  return this->sensorControl.get();
}

void Station::connectSensor() {
  if (this->sensorControl) {
    this->sensorControl->connectSensor();
  }
}

void Station::disconnectSensor() {
  if (this->sensorControl) {
    this->sensorControl->disconnectSensor();
  }
}

void Station::measure(const int& geomId, const MeasurementConfig& mConfig) {
  if (this->sensorControl) {
    this->sensorControl->measure(geomId, mConfig);
  }
}

void Station::move(const double& azimuth, const double& zenith,
                   const double& distance, const bool& isRelative,
                   const bool& measure, const int& geomId,
                   const MeasurementConfig& mConfig) {
  if (this->sensorControl) {
    this->sensorControl->move(azimuth, zenith, distance, isRelative, measure,
                              geomId, mConfig);
  }
}

void Station::move(const double& x, const double& y, const double& z,
                   const bool& measure, const int& geomId,
                   const MeasurementConfig& mConfig) {
  if (this->sensorControl) {
    this->sensorControl->move(x, y, z, measure, geomId, mConfig);
  }
}

void Station::initialize() {
  if (this->sensorControl) {
    this->sensorControl->initialize();
  }
}

void Station::motorState() {
  if (this->sensorControl) {
    this->sensorControl->motorState();
  }
}

void Station::home() {
  if (this->sensorControl) {
    this->sensorControl->home();
  }
}

void Station::toggleSight() {
  if (this->sensorControl) {
    this->sensorControl->toggleSight();
  }
}

void Station::compensation() {
  if (this->sensorControl) {
    this->sensorControl->compensation();
  }
}

void Station::selfDefinedAction(const QString& action) {
  if (this->sensorControl) {
    this->sensorControl->selfDefinedAction(action);
  }
}

void Station::search() {
  if (this->sensorControl) {
    this->sensorControl->search();
  }
}

void Station::startReadingStream() {
  if (this->sensorControl) {
    this->sensorControl->startReadingStream();
  }
}

void Station::stopReadingStream() {
  if (this->sensorControl) {
    this->sensorControl->stopReadingStream();
  }
}

void Station::startConnectionMonitoringStream() {
  if (this->sensorControl) {
    this->sensorControl->startConnectionMonitoringStream();
  }
}

void Station::stopConnectionMonitoringStream() {
  if (this->sensorControl) {
    this->sensorControl->stopConnectionMonitoringStream();
  }
}

void Station::startStatusMonitoringStream() {
  if (this->sensorControl) {
    this->sensorControl->startStatusMonitoringStream();
  }
}

void Station::stopStatusMonitoringStream() {
  if (this->sensorControl) {
    this->sensorControl->stopStatusMonitoringStream();
  }
}

void Station::finishMeasurement() {
  if (this->sensorControl) {
    this->sensorControl->finishMeasurement();
  }
}
