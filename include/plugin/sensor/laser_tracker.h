#ifndef OI_CORE_PLUGIN_SENSOR_LASER_TRACKER_H_
#define OI_CORE_PLUGIN_SENSOR_LASER_TRACKER_H_

#include "core_export.h"
#include "sensor.h"

namespace oi {

/*!
 * \brief The LaserTracker class
 */
class OI_CORE_EXPORT LaserTracker : public Sensor {
 public:
  explicit LaserTracker() : Sensor() {}

  ~LaserTracker() override {}

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
      case (SensorFunctions::eInitialize):
        return this->initialize();
      case (SensorFunctions::eHome):
        return this->home();
      case (SensorFunctions::eToggleSight):
        return this->toggleSightOrientation();
      case (SensorFunctions::eMotorState):
        return this->changeMotorState();
      case (SensorFunctions::eCompensation):
        return this->compensation();
      case (SensorFunctions::eSearch):
        return this->search();
      default:
        break;
    }
    return false;
  }

 protected:
  // #####################
  // laser tracker methods
  // #####################

  /*!
   * \brief initialize
   * \return
   */
  virtual bool initialize() {
    this->sensorMessage(std::string("Laser Tracker not implemented correctly: "
                                    "initialization method missing"),
                        MessageTypes::eCriticalMessage,
                        MessageDestinations::eMessageBoxMessage);
    return false;
  }

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
            "Laser Tracker not implemented correctly: move method missing"),
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
            "Laser Tracker not implemented correctly: move method missing"),
        MessageTypes::eCriticalMessage,
        MessageDestinations::eMessageBoxMessage);
    return false;
  }

  /*!
   * \brief home
   * \return
   */
  virtual bool home() {
    this->sensorMessage(
        std::string(
            "Laser Tracker not implemented correctly: home method missing"),
        MessageTypes::eCriticalMessage,
        MessageDestinations::eMessageBoxMessage);
    return false;
  }

  /*!
   * \brief changeMotorState
   * \return
   */
  virtual bool changeMotorState() {
    this->sensorMessage(std::string("Laser Tracker not implemented correctly: "
                                    "motor state method missing"),
                        MessageTypes::eCriticalMessage,
                        MessageDestinations::eMessageBoxMessage);
    return false;
  }

  /*!
   * \brief toggleSightOrientation
   * \return
   */
  virtual bool toggleSightOrientation() {
    this->sensorMessage(std::string("Laser Tracker not implemented correctly: "
                                    "toggle sight method missing"),
                        MessageTypes::eCriticalMessage,
                        MessageDestinations::eMessageBoxMessage);
    return false;
  }

  /*!
   * \brief compensation
   * \return
   */
  virtual bool compensation() {
    this->sensorMessage(std::string("Laser Tracker not implemented correctly: "
                                    "compensation method missing"),
                        MessageTypes::eCriticalMessage,
                        MessageDestinations::eMessageBoxMessage);
    return false;
  }

  bool search() override {
    this->sensorMessage(
        std::string(
            "Laser Tracker not implemented correctly: search method missing"),
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

#define LASERTRACKER_PREFIX de.openIndy.plugin.sensor.laserTracker.v
#define LaserTracker_iidd \
  STR2(CONCAT2(LASERTRACKER_PREFIX, OI_PLUGIN_ABI_VERSION))

#endif  // OI_CORE_PLUGIN_SENSOR_LASER_TRACKER_H_
