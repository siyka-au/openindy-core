#ifndef STATION_H
#define STATION_H

#include <QList>
#include <atomic>
#include <memory>

#include "core_export.h"
#include "executor.h"
#include "feature.h"
#include "guarded.h"
#include "point.h"
#include "sensorcontrol.h"
#include "stationobserver.h"

class ProjectExchanger;

namespace oi {

class Reading;
class CoordinateSystem;
class Sensor;

/*!
 * \brief The Station class
 */
class OI_CORE_EXPORT Station : public Feature {
  friend class OiJob;
  friend class ::ProjectExchanger;
  friend class Observation;
  friend struct codec::Access;  // the protobuf codec (see src/codec)

 public:
  explicit Station();
  explicit Station(const QString& name);

  Station(const Station& copy);

  Station& operator=(const Station& copy);

  ~Station() override;

  // #############################
  // get or set station attributes
  // #############################

  // active state
  [[nodiscard]] const bool& getIsActiveStation() const;
  void setActiveStationState(const bool& isActiveStation);

  // position
  [[nodiscard]] const oi::Guarded<Point>& getPosition() const;
  [[nodiscard]] const Direction& getXAxis() const;
  [[nodiscard]] const Direction& getYAxis() const;
  [[nodiscard]] const Direction& getZAxis() const;
  void setCoordinateSystem(const Position& origin, const Direction& xAxis,
                           const Direction& yAxis, const Direction& zAxis);

  // coordinate system
  [[nodiscard]] const oi::Guarded<CoordinateSystem>& getCoordinateSystem()
      const;
  void setEstimatedCoordinateSystem(oi::Guarded<CoordinateSystem> cs);
  [[nodiscard]] const oi::Guarded<CoordinateSystem>&
  getEstimatedCoordinateSystem() const;

  // get or set sensor configuration
  SensorConfiguration getSensorConfiguration();
  void setSensorConfiguration(const SensorConfiguration& sConfig);

  // get or set sensor
  [[nodiscard]] Sensor getSensor() const;
  void setSensor(const oi::Guarded<Sensor>& sensor);
  oi::Guarded<Sensor> takeSensor();
  void resetSensor();

  // previously used sensors
  [[nodiscard]] const QList<Sensor> getUsedSensors() const;

  // get geometries measured from this station
  [[nodiscard]] QList<oi::Guarded<Geometry>> getTargetGeometries() const;

  // ####################################################
  // get information about the currently connected sensor
  // ####################################################

  // status information
  bool getIsSensorSet();
  bool getIsSensorConnected();
  bool getIsReadyForMeasurement();
  bool getIsBusy();
  QMap<QString, QString> getSensorStatus();

  // sensor type
  [[nodiscard]] SensorType getActiveSensorType() const;

  // reading and connection types
  [[nodiscard]] QList<ReadingTypes> getSupportedReadingTypes() const;
  [[nodiscard]] QList<ConnectionKind> getSupportedConnectionTypes() const;

  // sensor actions
  [[nodiscard]] QList<SensorFunctions> getSupportedSensorActions() const;
  [[nodiscard]] QStringList getSelfDefinedActions() const;

  // stream format
  ReadingTypes getStreamFormat();
  void setStreamFormat(ReadingTypes streamFormat);

  // ###########################
  // reexecute the function list
  // ###########################

  void recalc() override;

  // ###############
  // display methods
  // ###############

  [[nodiscard]] QString getDisplayX(
      const UnitType& type, const int& digits,
      const bool& showDiff = false) const override;
  [[nodiscard]] QString getDisplayY(
      const UnitType& type, const int& digits,
      const bool& showDiff = false) const override;
  [[nodiscard]] QString getDisplayZ(
      const UnitType& type, const int& digits,
      const bool& showDiff = false) const override;
  [[nodiscard]] QString getDisplayPrimaryI(
      const int& digits, const bool& showDiff = false) const override;
  [[nodiscard]] QString getDisplayPrimaryJ(
      const int& digits, const bool& showDiff = false) const override;
  [[nodiscard]] QString getDisplayPrimaryK(
      const int& digits, const bool& showDiff = false) const override;

  // ###############################
  // start sensor actions: plain calls that enqueue the action for the
  // station's sensor worker thread and return at once (they used to be
  // signals connected to the SensorControl). The outcome arrives through the
  // StationObserver.
  // ###############################

  // connect or disconnect
  void connectSensor();
  void disconnectSensor();

  // measure
  void measure(const int& geomId, const MeasurementConfig& mConfig);

  // general sensor actions
  void move(const double& azimuth, const double& zenith, const double& distance,
            const bool& isRelative, const bool& measure, const int& geomId = -1,
            const MeasurementConfig& mConfig = MeasurementConfig());
  void move(const double& x, const double& y, const double& z,
            const bool& measure, const int& geomId = -1,
            const MeasurementConfig& mConfig = MeasurementConfig());
  void initialize();
  void motorState();
  void home();
  void toggleSight();
  void compensation();
  void selfDefinedAction(const QString& action);
  void search();

  // ################
  // sensor streaming
  // ################

  // reading stream
  void startReadingStream();
  void stopReadingStream();

  // connection monitoring stream
  void startConnectionMonitoringStream();
  void stopConnectionMonitoringStream();

  // status monitoring stream
  void startStatusMonitoringStream();
  void stopStatusMonitoringStream();

  void finishMeasurement();

  // ##############################################################
  // what the sensor reports back (command results, streams, messages)
  // goes to this observer (see stationobserver.h); at most one, not
  // owned, null until someone sets it
  // ##############################################################

  void setStationObserver(StationObserver* observer);
  [[nodiscard]] StationObserver* getStationObserver() const;

  // on which thread those reports are delivered (see executor.h). OiJob hands
  // its executor to every station it owns; a station without a job delivers
  // inline, on the sensor worker thread.
  void setExecutor(Executor* executor);

  // the sensor control (for tests and hosts that need the worker)
  [[nodiscard]] SensorControl* getSensorControl() const;

 protected:
  // ######################################
  // update feature id when the job was set
  // ######################################

  void setUpFeatureId() override;

 private:
  // ##########################
  // general station attributes
  // ##########################

  bool isActiveStation;

  // the position of the station
  oi::Guarded<Point> position;
  Direction xAxis;  // x axis of the coordinate system in the current display
                    // coordinate system
  Direction yAxis;  // y axis of the coordinate system in the current display
                    // coordinate system
  Direction zAxis;  // z (ijk / normal) axis of the coordinate system in the
                    // current display coordinate system

  // sensor communication
  class SensorResults;  // the observer of the sensor control
  std::unique_ptr<SensorResults> sensorResults;
  std::unique_ptr<SensorControl> sensorControl;
  std::atomic<StationObserver*> stationObserver{nullptr};

  // the corresponding coordinate system
  oi::Guarded<CoordinateSystem> stationSystem;
  oi::Guarded<CoordinateSystem> estimatedStationSystem;

  // #############################
  // readings made by this station
  // #############################

  QList<oi::Guarded<Reading>> cartesianReadings;
  QList<oi::Guarded<Reading>> directionReadings;
  QList<oi::Guarded<Reading>> distanceReadings;
  QList<oi::Guarded<Reading>> polarReadings;
  QList<oi::Guarded<Reading>> levelReadings;
  QList<oi::Guarded<Reading>> temperatureRadings;
  QList<oi::Guarded<Reading>> undefinedReadings;

 protected:
  // keep the station system's name in step with the station's
  void nameWasChanged(const QString& oldName) override;

 private:
  // ##############
  // helper methods
  // ##############

  // station attribute changes
  void stationNameChanged(const int& featureId, const QString& oldName);

  // the sensor control with the observer that feeds the station's own
  // observer
  void createSensorControl();

  // add or remove readings
  void addReadings(const int& geomId,
                   const QList<oi::Guarded<Reading>>& readings);
  void removeReading(const oi::Guarded<Reading>& reading);

  // set previously used sensors
  void setUsedSensors(const QList<Sensor>& sensors);

 protected:
  // #############################################
  // pointer to OiJob that this feature belongs to
  // #############################################

  void setJob(const oi::Guarded<OiJob>& job) override;
};

}  // namespace oi

#endif  // STATION_H
