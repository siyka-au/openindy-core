#ifndef STATIONOBSERVER_H
#define STATIONOBSERVER_H
#include <map>
#include <string>
#include <vector>

#include "core_export.h"
#include "guarded.h"
#include "types.h"
#include "value.h"

namespace oi {

class Reading;
class Station;

/*!
 * \brief Receiver of everything a station's sensor reports back: the results
 * of sensor commands, the streams and sensor messages.
 *
 * Replaces the result signals of Station (and of SensorControl/SensorWorker).
 * A Station holds a plain, non-owning pointer to at most one observer
 * (Station::setStationObserver); every method does nothing by default.
 *
 * Where the callbacks run is decided by the Executor the station was given
 * (see executor.h): with the default InlineExecutor on the sensor worker
 * thread, with a UI executor on the UI thread. Callbacks of one station arrive
 * in the order the worker produced them.
 *
 * ~Station calls onStationDestroyed first; the observer must not use the
 * station afterwards and should drop its reference to it. No callback is
 * delivered once the station's destructor has started.
 */
class OI_CORE_EXPORT StationObserver {
 public:
  StationObserver() = default;
  virtual ~StationObserver() = default;

  StationObserver(const StationObserver&) = delete;
  StationObserver& operator=(const StationObserver&) = delete;
  StationObserver(StationObserver&&) = delete;
  StationObserver& operator=(StationObserver&&) = delete;

  virtual void onStationDestroyed(Station* /*station*/) {}

  // streaming results
  virtual void onRealTimeReading(const oi::ValueMap& /*reading*/) {}
  virtual void onRealTimeStatus(
      const std::map<std::string, std::string>& /*status*/) {}
  virtual void onConnectionLost() {}
  virtual void onConnectionReceived() {}
  virtual void onIsReadyForMeasurement(bool /*isReady*/) {}

  // results of sensor commands
  virtual void onCommandFinished(bool /*success*/, const std::string& /*msg*/,
                                 SensorAction /*sensorAction*/) {}
  virtual void onMeasurementFinished(
      int /*geomId*/, const std::vector<oi::Guarded<Reading>>& /*readings*/) {}
  virtual void onMeasurementDone(bool /*success*/) {}

  // what the driver reports on its own
  virtual void onSensorMessage(const std::string& /*msg*/,
                               MessageTypes /*msgType*/,
                               MessageDestinations /*msgDest*/) {}
  virtual void onSensorStatus(SensorStatus /*status*/,
                              const std::string& /*msg*/) {}
};

}  // namespace oi

#endif  // STATIONOBSERVER_H
