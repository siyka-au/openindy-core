#ifndef OI_CORE_PLUGIN_SENSOR_TOTAL_STATION_H_
#define OI_CORE_PLUGIN_SENSOR_TOTAL_STATION_H_

#include "core_export.h"
#include "sensor.h"

namespace oi {

/*!
 * \brief The TotalStation class
 */
class OI_CORE_EXPORT TotalStation : public Sensor {
 public:
  explicit TotalStation() : Sensor() {}

  ~TotalStation() override {}

  // ##########################################
  // calls the appropriate laser tracker method
  // ##########################################

  bool accept(const SensorFunctions& method,
              const SensorAttributes& sAttr) override {
    switch (method) {
      case (SensorFunctions::eMoveAngle):
        return this->move(sAttr.moveAzimuth, sAttr.moveZenith,
                          sAttr.moveDistance, sAttr.moveIsRelative);
      case (SensorFunctions::eMoveXYZ):
        return this->move(sAttr.moveX, sAttr.moveY, sAttr.moveZ);
      case (SensorFunctions::eToggleSight):
        return this->toggleSightOrientation();
      default:
        break;
    }
    return false;
  }

 protected:
  // #####################
  // total station methods
  // #####################

  /*!
   * \brief move
   * \param azimuth
   * \param zenith
   * \param distance
   * \param isrelativ
   * \return
   */
  virtual bool move(const double& /*azimuth*/, const double& /*zenith*/,
                    const double& /*distance*/, const bool& /*isRelative*/) {
    this->sensorMessage(
        std::string(
            "Total Station not implemented correctly: move method missing"),
        MessageTypes::eCriticalMessage,
        MessageDestinations::eMessageBoxMessage);
    return false;
  }

  /*!
   * \brief move
   * \param x
   * \param y
   * \param z
   * \return
   */
  virtual bool move(const double& /*x*/, const double& /*y*/,
                    const double& /*z*/) {
    this->sensorMessage(
        std::string(
            "Total Station not implemented correctly: move method missing"),
        MessageTypes::eCriticalMessage,
        MessageDestinations::eMessageBoxMessage);
    return false;
  }

  /*!
   * \brief toggleSightOrientation
   * \return
   */
  virtual bool toggleSightOrientation() {
    this->sensorMessage(std::string("Total Station not implemented correctly: "
                                    "toggle sight method missing"),
                        MessageTypes::eCriticalMessage,
                        MessageDestinations::eMessageBoxMessage);
    return false;
  }
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

#define TOTALSTATION_PREFIX de.openIndy.plugin.sensor.totalStation.v
#define TotalStation_iidd \
  STR2(CONCAT2(TOTALSTATION_PREFIX, OI_PLUGIN_ABI_VERSION))

#endif  // OI_CORE_PLUGIN_SENSOR_TOTAL_STATION_H_
