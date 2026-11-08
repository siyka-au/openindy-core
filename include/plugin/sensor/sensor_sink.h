#ifndef OI_CORE_PLUGIN_SENSOR_SENSOR_SINK_H_
#define OI_CORE_PLUGIN_SENSOR_SENSOR_SINK_H_
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "core_export.h"
#include "guarded.h"
#include "types.h"
#include "value.h"

namespace oi {

class Reading;

/*!
 * \brief Receiver of what a Sensor driver reports on its own initiative.
 *
 * Replaces the sensorStatus, sensorMessage, asyncSensorResponse,
 * asyncMeasurementResult, asyncStreamResult and asyncSensorNotification Qt
 * signals of the Sensor. A sensor holds a plain, non-owning pointer to at most
 * one sink (Sensor::setSensorSink); without one everything it reports is
 * dropped. The Station's SensorWorker is the sink of the sensor it runs.
 *
 * THREADING: a driver may report from any thread it owns (a socket thread, a
 * timer, the worker thread while it executes a command), so every method may
 * be called from any thread, concurrently. Implementations must be thread-safe
 * and should return quickly. The sensor keeps its sink alive for the duration
 * of a call: Sensor::setSensorSink(nullptr) does not return while a callback
 * is still running on another thread, so a sink only has to detach itself
 * before it is destroyed.
 */
class OI_CORE_EXPORT SensorSink {
 public:
  SensorSink() = default;
  virtual ~SensorSink() = default;

  SensorSink(const SensorSink&) = delete;
  SensorSink& operator=(const SensorSink&) = delete;
  SensorSink(SensorSink&&) = delete;
  SensorSink& operator=(SensorSink&&) = delete;

  virtual void onSensorStatus(const SensorStatus& status,
                              const std::string& msg) = 0;
  virtual void onSensorMessage(const std::string& msg,
                               const MessageTypes& msgType,
                               const MessageDestinations& msgDest) = 0;
  virtual void onAsyncSensorResponse(const oi::ValueMap& response,
                                     SensorAction sensorAction) = 0;
  virtual void onAsyncMeasurementResult(
      const int& geomId, std::vector<std::unique_ptr<Reading>> measurements,
      SensorAction sensorAction) = 0;
  virtual void onAsyncStreamResult(const oi::ValueMap& reading) = 0;
  virtual void onAsyncSensorNotification(const oi::ValueMap& response) = 0;
};

}  // namespace oi

#endif  // OI_CORE_PLUGIN_SENSOR_SENSOR_SINK_H_
