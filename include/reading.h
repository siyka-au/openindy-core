#ifndef READING_H
#define READING_H
#include <chrono>
#include <map>
#include <string>

#include "core_export.h"
#include "element.h"
#include "guarded.h"
#include "measurementconfig.h"
#include "oimat.h"
#include "oivec.h"
#include "sensorconfiguration.h"
#include "types.h"
#include "util.h"

class ProjectExchanger;

namespace oi {

using namespace math;

class Observation;
class CoordinateSystem;
class Station;

// ###########################
// definition of reading types
// ###########################

/*!
 * \brief The ReadingPolar class
 */
class OI_CORE_EXPORT ReadingPolar {
 public:
  ReadingPolar()

  {}

  double azimuth{0.0};
  double zenith{0.0};
  double distance{0.0};
  double sigmaAzimuth{0.0};
  double sigmaZenith{0.0};
  double sigmaDistance{0.0};
  bool isValid{false};
};

/*!
 * \brief The ReadingDirection class
 */
class OI_CORE_EXPORT ReadingDirection {
 public:
  ReadingDirection()

  {}

  double azimuth{0.0};
  double zenith{0.0};
  double sigmaAzimuth{0.0};
  double sigmaZenith{0.0};
  bool isValid{false};
};

/*!
 * \brief The ReadingDistance class
 */
class OI_CORE_EXPORT ReadingDistance {
 public:
  ReadingDistance() {}

  double distance{0.0};
  double sigmaDistance{0.0};
  bool isValid{false};
};

/*!
 * \brief The ReadingCartesian class
 */
class OI_CORE_EXPORT ReadingCartesian {
 public:
  ReadingCartesian() : xyz(3), sigmaXyz(3) {}

  OiVec xyz;
  OiVec sigmaXyz;
  bool isValid{false};
};

/*!
 * \brief The ReadingCartesian6D class
 */
class OI_CORE_EXPORT ReadingCartesian6D {
 public:
  ReadingCartesian6D() : xyz(3), ijk(3), sigmaXyz(3) {}

  OiVec xyz;
  OiVec ijk;
  OiVec sigmaXyz;
  bool isValid{false};
};

/*!
 * \brief The ReadingTemperature class
 */
class OI_CORE_EXPORT ReadingTemperature {
 public:
  ReadingTemperature() {}

  double temperature{0.0};
  double sigmaTemperature{0.0};
  bool isValid{false};
};

/*!
 * \brief The ReadingUndefined class
 */
class OI_CORE_EXPORT ReadingUndefined {
 public:
  ReadingUndefined() {}

  std::map<std::string, double> values;
  std::map<std::string, double> sigmaValues;
  bool isValid{false};
};

/*!
 * \brief The ReadingLevel class
 */
class OI_CORE_EXPORT ReadingLevel {
 public:
  ReadingLevel() {}
  double i{};
  double j{};
  double k{};
  double sigmaI{};
  double sigmaJ{};
  double sigmaK{};
  bool isValid{false};
};

// ########################
// the reading class itself
// ########################

/*!
 * \brief The Reading class
 */
class OI_CORE_EXPORT Reading : public Element {
  friend class Station;
  friend class CoordinateSystem;
  friend class Observation;
  friend class ProjectExchanger;
  friend struct codec::Access;  // the protobuf codec (see src/codec)

 private:
  explicit Reading();

 public:
  explicit Reading(const ReadingPolar& reading);
  explicit Reading(const ReadingCartesian& reading);
  explicit Reading(const ReadingCartesian6D& reading);
  explicit Reading(const ReadingDirection& reading);
  explicit Reading(const ReadingDistance& reading);
  explicit Reading(const ReadingTemperature& reading);
  explicit Reading(const ReadingLevel& reading);
  explicit Reading(const ReadingUndefined& reading);

  Reading(const Reading& copy);

  Reading& operator=(const Reading& copy);

  ~Reading() override;

  // ###########################
  // get reading type and values
  // ###########################

  [[nodiscard]] const ReadingTypes& getTypeOfReading() const;

  [[nodiscard]] const ReadingPolar& getPolarReading() const;
  [[nodiscard]] const ReadingCartesian& getCartesianReading() const;
  [[nodiscard]] const ReadingCartesian6D& getCartesianReading6D() const;
  [[nodiscard]] const ReadingDirection& getDirectionReading() const;
  [[nodiscard]] const ReadingDistance& getDistanceReading() const;
  [[nodiscard]] const ReadingTemperature& getTemperatureReading() const;
  [[nodiscard]] const ReadingLevel& getLevelReading() const;
  [[nodiscard]] const ReadingUndefined& getUndefinedReading() const;

  // ##############################
  // set polar reading (simulation)
  // ##############################

  void setPolarReading(const ReadingPolar& rPolar);

  // ################################
  // make and restore reading backups
  // ################################

  void makeBackup();
  void restoreBackup();

  // ############################################
  // convert between polar and cartesian readings
  // ############################################

  static OiVec toCartesian(const double& azimuth, const double& zenith,
                           const double& distance);
  static OiVec toPolar(const double& x, const double& y, const double& z);

  // #########################################
  // get general information about the reading
  // #########################################

  const MeasurementConfigKey& getMeasurementConfig();
  void setMeasurementConfig(const MeasurementConfigKey& mConfig);

  [[nodiscard]] std::chrono::system_clock::time_point getMeasuredAt() const;
  void setMeasuredAt(std::chrono::system_clock::time_point measuredAt);

  [[nodiscard]] const SensorFaces& getFace() const;
  void setSensorFace(const SensorFaces& face);

  [[nodiscard]] const bool& isImported() const;
  void setImported(const bool& imported);

  // #################################################
  // get and set sensor configuration and observations
  // #################################################

  [[nodiscard]] const SensorConfiguration& getSensorConfiguration() const;
  void setSensorConfiguration(const SensorConfiguration& sConfig);

  [[nodiscard]] const oi::Guarded<Observation>& getObservation() const;
  void setObservation(const oi::Guarded<Observation>& observation);

  // ###############
  // display methods
  // ###############

  [[nodiscard]] std::string getDisplayId() const;
  [[nodiscard]] std::string getDisplayType() const;
  [[nodiscard]] std::string getDisplayTime() const;
  [[nodiscard]] std::string getDisplaySensorConfigName() const;
  [[nodiscard]] std::string getDisplayMeasurementConfigName() const;
  [[nodiscard]] std::string getDisplaySide() const;
  [[nodiscard]] std::string getDisplayAzimuth(const UnitType& type,
                                              const int& digits) const;
  [[nodiscard]] std::string getDisplayZenith(const UnitType& type,
                                             const int& digits) const;
  [[nodiscard]] std::string getDisplayDistance(const UnitType& type,
                                               const int& digits) const;
  [[nodiscard]] std::string getDisplayX(const UnitType& type,
                                        const int& digits) const;
  [[nodiscard]] std::string getDisplayY(const UnitType& type,
                                        const int& digits) const;
  [[nodiscard]] std::string getDisplayZ(const UnitType& type,
                                        const int& digits) const;
  [[nodiscard]] std::string getDisplayI(const int& digits) const;
  [[nodiscard]] std::string getDisplayJ(const int& digits) const;
  [[nodiscard]] std::string getDisplayK(const int& digits) const;
  [[nodiscard]] std::string getDisplayTemperature(const UnitType& type,
                                                  const int& digits) const;
  [[nodiscard]] std::string getDisplaySigmaAzimuth(const UnitType& type,
                                                   const int& digits) const;
  [[nodiscard]] std::string getDisplaySigmaZenith(const UnitType& type,
                                                  const int& digits) const;
  [[nodiscard]] std::string getDisplaySigmaDistance(const UnitType& type,
                                                    const int& digits) const;
  [[nodiscard]] std::string getDisplaySigmaX(const UnitType& type,
                                             const int& digits) const;
  [[nodiscard]] std::string getDisplaySigmaY(const UnitType& type,
                                             const int& digits) const;
  [[nodiscard]] std::string getDisplaySigmaZ(const UnitType& type,
                                             const int& digits) const;
  [[nodiscard]] std::string getDisplaySigmaI(const int& digits) const;
  [[nodiscard]] std::string getDisplaySigmaJ(const int& digits) const;
  [[nodiscard]] std::string getDisplaySigmaK(const int& digits) const;
  [[nodiscard]] std::string getDisplaySigmaTemperature(const UnitType& type,
                                                       const int& digits) const;

 private:
  // #######################
  // reading type and values
  // #######################

  ReadingTypes typeOfReading;

  ReadingPolar rPolar;
  ReadingCartesian rCartesian;
  ReadingCartesian6D rCartesian6D;
  ReadingDirection rDirection;
  ReadingDistance rDistance;
  ReadingTemperature rTemperature;
  ReadingLevel rLevel;
  ReadingUndefined rUndefined;

  // ###################################
  // backup readings used for simulation
  // ###################################

  ReadingPolar backupPolar;
  ReadingCartesian backupCartesian;
  ReadingDirection backupDirection;
  ReadingDistance backupDistance;
  ReadingTemperature backupTemperature;
  ReadingLevel backupLevel;
  ReadingUndefined backupUndefined;

  // ##########################
  // general reading attributes
  // ##########################

  std::chrono::system_clock::time_point measuredAt;
  MeasurementConfigKey mConfig;  // the configuration that the sensor used to
                                 // produce this reading
  SensorFaces face;
  bool imported;  // indicate that "reading" was not measured but imported

  // ######################
  // sensor and observation
  // ######################

  SensorConfiguration sConfig;
  std::string sensorConfigName;
  oi::Guarded<Observation> observation;

  // ################
  // helper variables
  // ################

  bool hasBackup;  // true if a backup was made using makeBackup

 private:
  // ##############
  // helper methods
  // ##############

  void toCartesian();
  void toPolar();

  OiVec errorPropagationPolarToCartesian();
};

}  // namespace oi

#endif  // READING_H
