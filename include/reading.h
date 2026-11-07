#ifndef READING_H
#define READING_H

#include <QtCore/qmath.h>

#include <QObject>
#include <QPointer>
#include <QtCore>
#include <chrono>

#include "core_export.h"
#include "element.h"
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

  QMap<QString, double> values;
  QMap<QString, double> sigmaValues;
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
  Q_OBJECT

 private:
  explicit Reading(QObject* parent = nullptr);

 public:
  explicit Reading(const ReadingPolar& reading, QObject* parent = nullptr);
  explicit Reading(const ReadingCartesian& reading, QObject* parent = nullptr);
  explicit Reading(const ReadingCartesian6D& reading,
                   QObject* parent = nullptr);
  explicit Reading(const ReadingDirection& reading, QObject* parent = nullptr);
  explicit Reading(const ReadingDistance& reading, QObject* parent = nullptr);
  explicit Reading(const ReadingTemperature& reading,
                   QObject* parent = nullptr);
  explicit Reading(const ReadingLevel& reading, QObject* parent = nullptr);
  explicit Reading(const ReadingUndefined& reading, QObject* parent = nullptr);

  Reading(const Reading& copy, QObject* parent = nullptr);

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

  [[nodiscard]] const QPointer<Observation>& getObservation() const;
  void setObservation(const QPointer<Observation>& observation);

  // ###############
  // display methods
  // ###############

  [[nodiscard]] QString getDisplayId() const;
  [[nodiscard]] QString getDisplayType() const;
  [[nodiscard]] QString getDisplayTime() const;
  [[nodiscard]] QString getDisplaySensorConfigName() const;
  [[nodiscard]] QString getDisplayMeasurementConfigName() const;
  [[nodiscard]] QString getDisplaySide() const;
  [[nodiscard]] QString getDisplayAzimuth(const UnitType& type,
                                          const int& digits) const;
  [[nodiscard]] QString getDisplayZenith(const UnitType& type,
                                         const int& digits) const;
  [[nodiscard]] QString getDisplayDistance(const UnitType& type,
                                           const int& digits) const;
  [[nodiscard]] QString getDisplayX(const UnitType& type,
                                    const int& digits) const;
  [[nodiscard]] QString getDisplayY(const UnitType& type,
                                    const int& digits) const;
  [[nodiscard]] QString getDisplayZ(const UnitType& type,
                                    const int& digits) const;
  [[nodiscard]] QString getDisplayI(const int& digits) const;
  [[nodiscard]] QString getDisplayJ(const int& digits) const;
  [[nodiscard]] QString getDisplayK(const int& digits) const;
  [[nodiscard]] QString getDisplayTemperature(const UnitType& type,
                                              const int& digits) const;
  [[nodiscard]] QString getDisplaySigmaAzimuth(const UnitType& type,
                                               const int& digits) const;
  [[nodiscard]] QString getDisplaySigmaZenith(const UnitType& type,
                                              const int& digits) const;
  [[nodiscard]] QString getDisplaySigmaDistance(const UnitType& type,
                                                const int& digits) const;
  [[nodiscard]] QString getDisplaySigmaX(const UnitType& type,
                                         const int& digits) const;
  [[nodiscard]] QString getDisplaySigmaY(const UnitType& type,
                                         const int& digits) const;
  [[nodiscard]] QString getDisplaySigmaZ(const UnitType& type,
                                         const int& digits) const;
  [[nodiscard]] QString getDisplaySigmaI(const int& digits) const;
  [[nodiscard]] QString getDisplaySigmaJ(const int& digits) const;
  [[nodiscard]] QString getDisplaySigmaK(const int& digits) const;
  [[nodiscard]] QString getDisplaySigmaTemperature(const UnitType& type,
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
  QString sensorConfigName;
  QPointer<Observation> observation;

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

Q_DECLARE_METATYPE(oi::Reading*)

#endif  // READING_H
