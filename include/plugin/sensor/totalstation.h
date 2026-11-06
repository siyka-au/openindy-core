#ifndef TOTALSTATION_H
#define TOTALSTATION_H

#include "sensor.h"

namespace oi {

/*!
 * \brief The TotalStation class
 */
class OI_CORE_EXPORT TotalStation : public Sensor {
  Q_OBJECT
  Q_DISABLE_COPY_MOVE(TotalStation)

 public:
  explicit TotalStation(QObject* parent = nullptr) : Sensor(parent) {}

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
    emit this->sensorMessage(
        QString("Total Station not implemented correctly: move method missing"),
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
    emit this->sensorMessage(
        QString("Total Station not implemented correctly: move method missing"),
        MessageTypes::eCriticalMessage,
        MessageDestinations::eMessageBoxMessage);
    return false;
  }

  /*!
   * \brief toggleSightOrientation
   * \return
   */
  virtual bool toggleSightOrientation() {
    emit this->sensorMessage(QString("Total Station not implemented correctly: "
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

#include "pluginabi.h"

#define TOTALSTATION_PREFIX de.openIndy.plugin.sensor.totalStation.v
#define TotalStation_iidd \
  STR2(CONCAT2(TOTALSTATION_PREFIX, OI_PLUGIN_ABI_VERSION))

#endif  // TOTALSTATION_H
