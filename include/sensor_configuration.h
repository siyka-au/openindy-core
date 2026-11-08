#ifndef OI_CORE_SENSOR_CONFIGURATION_H_
#define OI_CORE_SENSOR_CONFIGURATION_H_
#include <map>
#include <string>

#include "connection_kind.h"
#include "core_export.h"
#include "guarded.h"
#include "oi_vec.h"
#include "sensor_type.h"
#include "types.h"
#include "util.h"

namespace oi {

using namespace math;

class Sensor;

// Serial line settings. The numeric values are the ones Qt's QSerialPort
// enums use, so a host can static_cast between the two.
enum class BaudRate : int {
  Baud1200 = 1200,
  Baud2400 = 2400,
  Baud4800 = 4800,
  Baud9600 = 9600,
  Baud19200 = 19200,
  Baud38400 = 38400,
  Baud57600 = 57600,
  Baud115200 = 115200
};
enum class DataBits : int { Data5 = 5, Data6 = 6, Data7 = 7, Data8 = 8 };
enum class Parity : int { None = 0, Even = 2, Odd = 3, Space = 4, Mark = 5 };
enum class StopBits : int { One = 1, Two = 2, OneAndHalf = 3 };
enum class FlowControl : int { None = 0, Hardware = 1, Software = 2 };

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
  std::string ip;
  std::string port;

  // serial
  std::string comPort;
  oi::BaudRate baudRate{oi::BaudRate::Baud9600};
  oi::DataBits dataBits{oi::DataBits::Data8};
  oi::Parity parity{oi::Parity::None};
  oi::StopBits stopBits{oi::StopBits::One};
  oi::FlowControl flowControl{oi::FlowControl::None};
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
  std::map<std::string, double> sigmaUndefined;
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

  [[nodiscard]] const std::string& getName() const;
  void setName(const std::string& name);

  [[nodiscard]] const bool& getIsSaved() const;
  void setIsSaved(const bool& isSaved);

  [[nodiscard]] bool getIsValid() const;

  [[nodiscard]] const SensorType& getTypeOfSensor() const;
  void setTypeOfSensor(const SensorType& type);

  [[nodiscard]] const std::string& getPluginName() const;
  void setPluginName(const std::string& name);

  [[nodiscard]] const std::string& getSensorName() const;
  void setSensorName(const std::string& name);

  [[nodiscard]] const Accuracy& getAccuracy() const;
  void setAccuracy(const Accuracy& accuracy);

  [[nodiscard]] const ConnectionConfig& getConnectionConfig() const;
  void setConnectionConfig(const ConnectionConfig& cConfig);

  [[nodiscard]] const std::map<std::string, int>& getIntegerParameter() const;
  void setIntegerParameter(const std::map<std::string, int>& intParams);

  [[nodiscard]] const std::map<std::string, double>& getDoubleParameter() const;
  void setDoubleParameter(const std::map<std::string, double>& doubleParams);

  [[nodiscard]] const std::map<std::string, std::string>& getStringParameter()
      const;
  void setStringParameter(
      const std::map<std::string, std::string>& stringParams);

  [[nodiscard]] const std::multimap<std::string, std::string>&
  getAvailableStringParameter() const;
  void setAvailableStringParameter(
      const std::multimap<std::string, std::string>& stringParams);

 private:
  // ###############################
  // sensor configuration attributes
  // ###############################

  std::string name;
  bool isSaved{false};

  std::string pluginName;
  std::string sensorName;
  SensorType typeOfSensor;

  Accuracy accuracy;
  ConnectionConfig cConfig;

  // parameter selection
  std::map<std::string, int> integerParameter;
  std::map<std::string, double> doubleParameter;
  std::map<std::string, std::string> stringParameter;

  // all available string parameter (specified by the sensor)
  std::multimap<std::string, std::string> availableStringParameter;
};

}  // namespace oi

#endif  // OI_CORE_SENSOR_CONFIGURATION_H_
