#ifndef SENSORCONFIGURATION_H
#define SENSORCONFIGURATION_H
#include <QString>
#include <QtSerialPort/QSerialPort>
#include <map>

#include "connectionkind.h"
#include "core_export.h"
#include "guarded.h"
#include "oivec.h"
#include "sensortype.h"
#include "types.h"
#include "util.h"

namespace oi {

using namespace math;

class Sensor;

// ##############
// helper classes
// ##############

/*!
 * \brief The ConnectionConfig class
 * Save all information needed to connect a sensor
 */
class OI_CORE_EXPORT ConnectionConfig {
 public:
  ConnectionConfig() {}

  // type
  ConnectionKind typeOfConnection;

  // network
  QString ip;
  QString port;

  // serial
  QString comPort;
  QSerialPort::BaudRate baudRate;
  QSerialPort::DataBits dataBits;
  QSerialPort::Parity parity;
  QSerialPort::StopBits stopBits;
  QSerialPort::FlowControl flowControl;
};

/*!
 * \brief The Accuracy class
 * Save information about the accurycy of a sensor
 */
class OI_CORE_EXPORT Accuracy {
 public:
  Accuracy() : sigmaXyz(OiVec(3)) {}

  // polar
  double sigmaAzimuth{0.0};
  double sigmaZenith{0.0};
  double sigmaDistance{0.0};

  // cartesian
  OiVec sigmaXyz;

  // temperature
  double sigmaTemp{0.0};

  // level
  double sigmaI{0.0};
  double sigmaJ{0.0};
  double sigmaK{0.0};

  // undefined
  std::map<QString, double> sigmaUndefined;
};

// ###########################
// sensor configuration itself
// ###########################

/*!
 * \brief The SensorConfiguration class
 */
class OI_CORE_EXPORT SensorConfiguration {
 public:
  SensorConfiguration();

  SensorConfiguration(const SensorConfiguration& copy);

  SensorConfiguration& operator=(const SensorConfiguration& copy);

  friend bool operator==(const SensorConfiguration& left,
                         const SensorConfiguration& right) {
    if (left.getName().compare(right.getName()) == 0) {
      return true;
    }
    return false;
  }

  // ##########################################
  // get or set sensor configuration attributes
  // ##########################################

  [[nodiscard]] const QString& getName() const;
  void setName(const QString& name);

  [[nodiscard]] const bool& getIsSaved() const;
  void setIsSaved(const bool& isSaved);

  [[nodiscard]] bool getIsValid() const;

  [[nodiscard]] const SensorType& getTypeOfSensor() const;
  void setTypeOfSensor(const SensorType& type);

  [[nodiscard]] const QString& getPluginName() const;
  void setPluginName(const QString& name);

  [[nodiscard]] const QString& getSensorName() const;
  void setSensorName(const QString& name);

  [[nodiscard]] const Accuracy& getAccuracy() const;
  void setAccuracy(const Accuracy& accuracy);

  [[nodiscard]] const ConnectionConfig& getConnectionConfig() const;
  void setConnectionConfig(const ConnectionConfig& cConfig);

  [[nodiscard]] const std::map<QString, int>& getIntegerParameter() const;
  void setIntegerParameter(const std::map<QString, int>& intParams);

  [[nodiscard]] const std::map<QString, double>& getDoubleParameter() const;
  void setDoubleParameter(const std::map<QString, double>& doubleParams);

  [[nodiscard]] const std::map<QString, QString>& getStringParameter() const;
  void setStringParameter(const std::map<QString, QString>& stringParams);

  [[nodiscard]] const std::multimap<QString, QString>&
  getAvailableStringParameter() const;
  void setAvailableStringParameter(
      const std::multimap<QString, QString>& stringParams);

 private:
  // ###############################
  // sensor configuration attributes
  // ###############################

  QString name;
  bool isSaved{false};

  QString pluginName;
  QString sensorName;
  SensorType typeOfSensor;

  Accuracy accuracy;
  ConnectionConfig cConfig;

  // parameter selection
  std::map<QString, int> integerParameter;
  std::map<QString, double> doubleParameter;
  std::map<QString, QString> stringParameter;

  // all available string parameter (specified by the sensor)
  std::multimap<QString, QString> availableStringParameter;
};

}  // namespace oi

Q_DECLARE_METATYPE(oi::SensorConfiguration)
Q_DECLARE_METATYPE(oi::SensorConfiguration*)

#endif  // SENSORCONFIGURATION_H
