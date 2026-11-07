#ifndef MEASUREMENTCONFIG_H
#define MEASUREMENTCONFIG_H
#include <map>
#include <string>
#include <vector>

#include "core_export.h"
#include "sensortype.h"
#include "types.h"
#include "util.h"
#include "value.h"

namespace oi {

enum class MeasurementTypes {
  eUnknown_MeasurementType = -1,
  eSinglePoint_MeasurementType = 0,
  eScanTimeDependent_MeasurementType,
  eScanDistanceDependent_MeasurementType,
  eLevel_MeasurementType,
  eDirection_MeasurementType,
  eDistance_MeasurementType,
  eTemperature_MeasurementType
};

enum class MeasurementModes {
  eFast_MeasurementMode = 0,
  eStandard_MeasurementMode,
  ePrecise_MeasurementMode
};

enum class ConfigTypes {
  eUndefinded = -1,
  eUserConfig = 0,
  eProjectConfig,
  eStandardConfig
};

class OI_CORE_EXPORT MeasurementConfigKey {
 public:
  MeasurementConfigKey() : configType(ConfigTypes::eUndefinded) {}
  MeasurementConfigKey(std::string name, ConfigTypes configType,
                       SensorType sensorType = SensorType())
      : name(name), configType(configType), sensorType(sensorType) {}
  MeasurementConfigKey(const MeasurementConfigKey& copy)
      : name(copy.name),
        configType(copy.configType),
        sensorType(copy.sensorType) {}
  MeasurementConfigKey& operator=(const MeasurementConfigKey& copy) {
    this->name = copy.name;
    this->configType = copy.configType;
    this->sensorType = copy.sensorType;
    return *this;
  }

  // name+configType alone collided across driver-published configs the
  // moment two different sensor types published the same standard-config
  // name (every driver's "FastPoint") - one silently overwrote the other
  // in a std::map keyed on this. sensorType is part of the identity for the
  // same reason SensorConfiguration's type isn't optional.
  friend bool operator==(const MeasurementConfigKey& left,
                         const MeasurementConfigKey& right) {
    return left.getName() == right.getName() &&
           left.configType == right.configType &&
           left.sensorType == right.sensorType;
  }

  friend bool operator<(const MeasurementConfigKey& left,
                        const MeasurementConfigKey& right) {
    if (left.name != right.name) {
      return left.name < right.name;
    } else if (left.configType != right.configType) {
      return left.configType < right.configType;
    } else {
      return left.sensorType.id() < right.sensorType.id();
    }
  }

  [[nodiscard]] const std::string getName() const { return this->name; }

  [[nodiscard]] ConfigTypes getConfigType() const { return this->configType; }

  [[nodiscard]] const SensorType& getSensorType() const {
    return this->sensorType;
  }

  [[nodiscard]] bool isUserConfig() const {
    return ConfigTypes::eUserConfig == this->configType;
  }
  [[nodiscard]] bool isProjectConfig() const {
    return ConfigTypes::eProjectConfig == this->configType ||
           ConfigTypes::eStandardConfig == this->configType;
  }
  [[nodiscard]] bool isStandardConfig() const {
    return ConfigTypes::eStandardConfig == this->configType;
  }
  [[nodiscard]] bool isValid() const {
    return !this->name.empty() && this->configType != ConfigTypes::eUndefinded;
  }

 private:
  std::string name;
  ConfigTypes configType;
  SensorType sensorType;
};

/*!
 * \brief The MeasurementConfig class
 * Contains all the configuration parameters needed to start a measurement.
 */
class OI_CORE_EXPORT MeasurementConfig {
 public:
  MeasurementConfig();

  MeasurementConfig(const MeasurementConfig& copy);

  MeasurementConfig& operator=(const MeasurementConfig& copy);

  friend bool operator==(const MeasurementConfig& left,
                         const MeasurementConfig& right) {
    return left.getName() == right.getName()       // combined key
           && left.configType == right.configType  // combined key
           && left.editable == right.editable &&
           left.getMeasurementType() == right.getMeasurementType() &&
           left.getMeasurementMode() == right.getMeasurementMode() &&
           left.getMeasureTwoSides() == right.getMeasureTwoSides() &&
           left.getMaxObservations() == right.getMaxObservations() &&
           left.getTimeInterval() == right.getTimeInterval() &&
           almostEqual(left.getDistanceInterval(), right.getDistanceInterval(),
                       8) &&
           left.getSensorType() == right.getSensorType();
  }

  // ########################################
  // get or set measurement config attributes
  // ########################################

  [[nodiscard]] const std::string& getName() const;
  void setName(const std::string& name);
  [[nodiscard]] MeasurementConfigKey getKey() const;

  [[nodiscard]] bool isUserConfig() const;
  [[nodiscard]] bool isProjectConfig() const;
  [[nodiscard]] bool isEditable() const;
  [[nodiscard]] bool isStandardConfig() const;
  [[nodiscard]] bool isValid() const;

  [[nodiscard]] const bool& getMeasureTwoSides() const;
  void setMeasureTwoSides(const bool& measureTwoSides);

  [[nodiscard]] const double& getTimeInterval() const;
  void setTimeInterval(const double& interval);

  [[nodiscard]] const double& getDistanceInterval() const;
  void setDistanceInterval(const double& interval);

  [[nodiscard]] const oi::Value getTransientData(const std::string key) const;
  void setTransientData(const std::string key, const oi::Value value);

  void setMeasurementMode(const MeasurementModes mode);
  [[nodiscard]] MeasurementModes getMeasurementMode() const;

  void setMeasurementType(const MeasurementTypes type);
  [[nodiscard]] MeasurementTypes getMeasurementType() const;

  [[nodiscard]] const int& getMaxObservations() const;
  void setMaxObservations(const int& maxObservations);

  //! which sensor type published this config (set once, at creation, by
  //! whichever driver's init() built it) - the compatibility check between
  //! a selected config and the connected sensor is keyed off this, not
  //! anything the user can edit after the fact. An invalid SensorType
  //! (isValid() == false) means this config predates the tag or was
  //! authored generically and isn't gated to any one type.
  [[nodiscard]] const SensorType& getSensorType() const;
  void setSensorType(const SensorType& type);

  void makeUserConfig();
  void makeStandardConfig();
  void makeProjectConfig();

  bool applicableFor(const ElementTypes elementType,
                     std::vector<FeatureTypes> functionIsApplicableFor);

 private:
  // #############################
  // measurement config attributes
  // #############################
  std::string name;
  ConfigTypes configType{ConfigTypes::eUndefinded};

  bool measureTwoSides;     // true if the sensor shall measure in both faces
                            // (false if not)
  double timeInterval;      // time interval in which the sensor shall measure
  double distanceInterval;  // distance interval in which the sensor shall
                            // measure [mm]

  oi::ValueMap transientData;

  MeasurementTypes measurementType;
  MeasurementModes measurementMode;
  int maxObservations;

  SensorType sensorType;

  bool editable{true};
};

}  // namespace oi

#endif  // MEASUREMENTCONFIG_H
