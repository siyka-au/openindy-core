#include "reading.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <format>
#include <sstream>

#include "observation.h"
#include "sensor.h"
#include "snapshot.h"
#include "string_util.h"

using namespace oi;
using namespace oi::math;

/*!
 * \brief Reading::Reading
 */
Reading::Reading()
    : Element(),
      face(SensorFaces::eUndefinedSide),
      imported(false),
      hasBackup(false) {
  // set default attributes
  this->measuredAt = std::chrono::system_clock::now();
}

/*!
 * \brief Reading::Reading
 * \param reading
 */
Reading::Reading(const ReadingPolar& reading)
    : Element(),
      face(SensorFaces::eUndefinedSide),
      imported(false),
      rPolar(reading),
      typeOfReading(ReadingTypes::ePolarReading),
      hasBackup(false) {
  // set the reading and transform into cartesian

  this->toCartesian();
  this->rCartesian.sigmaXyz = this->errorPropagationPolarToCartesian();

  // set default attributes
  this->measuredAt = std::chrono::system_clock::now();
}

/*!
 * \brief Reading::Reading
 * \param reading
 */
Reading::Reading(const ReadingCartesian& reading)
    : Element(), hasBackup(false) {
  if (reading.xyz.getSize() != 3 || reading.sigmaXyz.getSize() != 3) {
    this->typeOfReading = ReadingTypes::eCartesianReading;
    return;
  }

  // set the reading and transform into polar
  this->typeOfReading = ReadingTypes::eCartesianReading;
  this->rCartesian = reading;
  this->toPolar();

  // set default attributes
  this->measuredAt = std::chrono::system_clock::now();
  this->face = SensorFaces::eUndefinedSide;
  this->imported = false;
}
Reading::Reading(const ReadingCartesian6D& reading)
    : Element(), hasBackup(false) {
  if (reading.xyz.getSize() != 3 || reading.ijk.getSize() != 3 ||
      reading.sigmaXyz.getSize() != 3) {
    this->typeOfReading = ReadingTypes::eCartesianReading6D;
    return;
  }

  // set the reading and transform into polar
  this->typeOfReading = ReadingTypes::eCartesianReading6D;
  this->rCartesian6D = reading;
  this->toPolar();

  // set default attributes
  this->measuredAt = std::chrono::system_clock::now();
  this->face = SensorFaces::eUndefinedSide;
  this->imported = false;
}
/*!
 * \brief Reading::Reading
 * \param reading
 */
Reading::Reading(const ReadingDirection& reading)
    : Element(),
      face(SensorFaces::eUndefinedSide),
      imported(false),
      rDirection(reading),
      typeOfReading(ReadingTypes::eDirectionReading),
      hasBackup(false) {
  // set the reading

  // set default attributes
  this->measuredAt = std::chrono::system_clock::now();
}

/*!
 * \brief Reading::Reading
 * \param reading
 */
Reading::Reading(const ReadingDistance& reading)
    : Element(),
      face(SensorFaces::eUndefinedSide),
      imported(false),
      rDistance(reading),
      typeOfReading(ReadingTypes::eDistanceReading),
      hasBackup(false) {
  // set the reading and

  // set default attributes
  this->measuredAt = std::chrono::system_clock::now();
}

/*!
 * \brief Reading::Reading
 * \param reading
 */
Reading::Reading(const ReadingTemperature& reading)
    : Element(),
      face(SensorFaces::eUndefinedSide),
      imported(false),
      rTemperature(reading),
      typeOfReading(ReadingTypes::eTemperatureReading),
      hasBackup(false) {
  // set the reading

  // set default attributes
  this->measuredAt = std::chrono::system_clock::now();
}

/*!
 * \brief Reading::Reading
 * \param reading
 */
Reading::Reading(const ReadingLevel& reading)
    : Element(),
      face(SensorFaces::eUndefinedSide),
      imported(false),
      rLevel(reading),
      typeOfReading(ReadingTypes::eLevelReading),
      hasBackup(false) {
  // set the reading

  // set default attributes
  this->measuredAt = std::chrono::system_clock::now();
}

/*!
 * \brief Reading::Reading
 * \param reading
 */
Reading::Reading(const ReadingUndefined& reading)
    : Element(),
      face(SensorFaces::eUndefinedSide),
      imported(false),
      rUndefined(reading),
      typeOfReading(ReadingTypes::eUndefinedReading),
      hasBackup(false) {
  // set the reading

  // set default attributes
  this->measuredAt = std::chrono::system_clock::now();
}

/*!
 * \brief Reading::Reading
 * \param copy
 */
Reading::Reading(const Reading& copy)
    : Element(copy),
      backupCartesian(copy.backupCartesian),
      backupDirection(copy.backupDirection),
      backupDistance(copy.backupDistance),
      backupLevel(copy.backupLevel),
      backupPolar(copy.backupPolar),
      backupTemperature(copy.backupTemperature),
      backupUndefined(copy.backupUndefined),
      face(copy.face),
      hasBackup(copy.hasBackup),
      imported(copy.imported),
      mConfig(copy.mConfig),
      measuredAt(copy.measuredAt),
      observation(copy.observation),
      rCartesian(copy.rCartesian),
      rDirection(copy.rDirection),
      rDistance(copy.rDistance),
      rLevel(copy.rLevel),
      rPolar(copy.rPolar),
      rTemperature(copy.rTemperature),
      rUndefined(copy.rUndefined),
      sConfig(copy.sConfig),
      typeOfReading(copy.typeOfReading) {
  // copy reading attributes

  // copy readings
}

/*!
 * \brief Reading::operator =
 * \param copy
 * \return
 */
Reading& Reading::operator=(const Reading& copy) {
  // copy reading attributes
  this->measuredAt = copy.measuredAt;
  this->mConfig = copy.mConfig;
  this->face = copy.face;
  this->sConfig = copy.sConfig;
  this->observation = copy.observation;
  this->hasBackup = copy.hasBackup;
  this->imported = copy.imported;

  // copy readings
  this->typeOfReading = copy.typeOfReading;
  this->rPolar = copy.rPolar;
  this->backupPolar = copy.backupPolar;
  this->rCartesian = copy.rCartesian;
  this->backupCartesian = copy.backupCartesian;
  this->rDirection = copy.rDirection;
  this->backupDirection = copy.backupDirection;
  this->rDistance = copy.rDistance;
  this->backupDistance = copy.backupDistance;
  this->rTemperature = copy.rTemperature;
  this->backupTemperature = copy.backupTemperature;
  this->rLevel = copy.rLevel;
  this->backupLevel = copy.backupLevel;
  this->rUndefined = copy.rUndefined;
  this->backupUndefined = copy.backupUndefined;

  return *this;
}

/*!
 * \brief Reading::~Reading
 */
Reading::~Reading() {}

/*!
 * \brief Reading::getTypeOfReading
 * \return
 */
const ReadingTypes& Reading::getTypeOfReading() const {
  return this->typeOfReading;
}

/*!
 * \brief Reading::getPolarReading
 * \return
 */
const ReadingPolar& Reading::getPolarReading() const { return this->rPolar; }

/*!
 * \brief Reading::getCartesianReading
 * \return
 */
const ReadingCartesian& Reading::getCartesianReading() const {
  return this->rCartesian;
}

const ReadingCartesian6D& Reading::getCartesianReading6D() const {
  return this->rCartesian6D;
}

/*!
 * \brief Reading::getDirectionReading
 * \return
 */
const ReadingDirection& Reading::getDirectionReading() const {
  return this->rDirection;
}

/*!
 * \brief Reading::getDistanceReading
 * \return
 */
const ReadingDistance& Reading::getDistanceReading() const {
  return this->rDistance;
}

/*!
 * \brief Reading::getTemperatureReading
 * \return
 */
const ReadingTemperature& Reading::getTemperatureReading() const {
  return this->rTemperature;
}

/*!
 * \brief Reading::getLevelReading
 * \return
 */
const ReadingLevel& Reading::getLevelReading() const { return this->rLevel; }

/*!
 * \brief Reading::getUndefinedReading
 * \return
 */
const ReadingUndefined& Reading::getUndefinedReading() const {
  return this->rUndefined;
}

/*!
 * \brief Reading::setPolarReading
 * \param rPolar
 */
void Reading::setPolarReading(const ReadingPolar& rPolar) {
  if (this->rPolar.isValid && rPolar.isValid) {
    this->rPolar = rPolar;
  }
}

/*!
 * \brief Reading::makeBackup
 */
void Reading::makeBackup() {
  this->backupPolar = this->rPolar;
  this->backupCartesian = this->rCartesian;
  this->backupDirection = this->rDirection;
  this->backupDistance = this->rDistance;
  this->backupTemperature = this->rTemperature;
  this->backupUndefined = this->rUndefined;
  this->backupLevel = this->rLevel;

  this->hasBackup = true;
}

/*!
 * \brief Reading::restoreBackup
 */
void Reading::restoreBackup() {
  // do not restore backup if no backup was made
  if (!this->hasBackup) {
    return;
  }

  this->rPolar = this->backupPolar;
  this->rCartesian = this->backupCartesian;
  this->rDirection = this->backupDirection;
  this->rDistance = this->backupDistance;
  this->rTemperature = this->backupTemperature;
  this->rUndefined = this->backupUndefined;
  this->rLevel = this->backupLevel;
}

/*!
 * \brief Reading::toCartesian
 * Returns a vector containing the cartesian coordinates corresponding to the
 * given polar elements
 * \param azimuth
 * \param zenith
 * \param distance
 * \return
 */
OiVec Reading::toCartesian(const double& azimuth, const double& zenith,
                           const double& distance) {
  OiVec xyz(3);

  xyz.setAt(0, distance * std::sin(zenith) * std::cos(azimuth));
  xyz.setAt(1, distance * std::sin(zenith) * std::sin(azimuth));
  xyz.setAt(2, distance * std::cos(zenith));

  return xyz;
}

/*!
 * \brief Reading::toPolar
 * Returns a vector containing the polar elements (azimuth, zenith, distance)
 * corresponding to the given cartesian coordinates
 * \param x
 * \param y
 * \param z
 * \return
 */
OiVec Reading::toPolar(const double& x, const double& y, const double& z) {
  OiVec polar(3);

  double azimuth = std::atan2(y, x);
  double s = std::sqrt(x * x + y * y + z * z);
  double zenith = acos(z / s);

  polar.setAt(0, azimuth);
  polar.setAt(1, zenith);
  polar.setAt(2, s);

  return polar;
}

/*!
 * \brief Reading::getMeasurementConfig
 * \return
 */
const MeasurementConfigKey& Reading::getMeasurementConfig() {
  return this->mConfig;
}

/*!
 * \brief Reading::setMeasurementConfig
 * \param mConfig
 */
void Reading::setMeasurementConfig(const MeasurementConfigKey& mConfig) {
  this->mConfig = mConfig;
}

/*!
 * \brief Reading::getMeasuredAt
 * \return
 */
std::chrono::system_clock::time_point Reading::getMeasuredAt() const {
  return this->measuredAt;
}

/*!
 * \brief Reading::setMeasuredAt
 * \param measuredAt
 */
void Reading::setMeasuredAt(std::chrono::system_clock::time_point measuredAt) {
  this->measuredAt = measuredAt;
}

/*!
 * \brief Reading::getFace
 * \return
 */
const SensorFaces& Reading::getFace() const { return this->face; }

/*!
 * \brief Reading::setSensorFace
 * \param face
 */
void Reading::setSensorFace(const SensorFaces& face) { this->face = face; }

/*!
 * \brief indicate that "reading" was not measured but imported
 * \return
 */
const bool& Reading::isImported() const { return this->imported; }

/*!
 * \brief indicate that "reading" was not measured but imported
 * \param imported
 */
void Reading::setImported(const bool& imported) { this->imported = imported; }

/*!
 * \brief Reading::getSensorConfiguration
 * \return
 */
const SensorConfiguration& Reading::getSensorConfiguration() const {
  return this->sConfig;
}

/*!
 * \brief Reading::setSensorConfiguration
 * \param sensor
 */
void Reading::setSensorConfiguration(const SensorConfiguration& sConfig) {
  this->sConfig = sConfig;
  this->sensorConfigName = sConfig.getName();
}

/*!
 * \brief Reading::getObservation
 * \return
 */
const oi::Guarded<Observation>& Reading::getObservation() const {
  return this->observation;
}

/*!
 * \brief Reading::setObservation
 * \param observation
 */
void Reading::setObservation(const oi::Guarded<Observation>& observation) {
  // check observation
  if (observation.isNull()) {
    return;
  }

  // set observation position and standard deviation
  if (this->rCartesian.isValid) {
    // set position
    observation->originalXyz.setAt(0, this->rCartesian.xyz.getAt(0));
    observation->originalXyz.setAt(1, this->rCartesian.xyz.getAt(1));
    observation->originalXyz.setAt(2, this->rCartesian.xyz.getAt(2));
    observation->originalXyz.setAt(3, 1.0);

    // set standard deviation
    observation->originalSigmaXyz.setAt(0, this->rCartesian.sigmaXyz.getAt(0));
    observation->originalSigmaXyz.setAt(1, this->rCartesian.sigmaXyz.getAt(1));
    observation->originalSigmaXyz.setAt(2, this->rCartesian.sigmaXyz.getAt(2));
    observation->originalSigmaXyz.setAt(3, 1.0);

    // set ijk
    observation->originalIjk.setAt(0, 0.0);
    observation->originalIjk.setAt(1, 0.0);
    observation->originalIjk.setAt(2, 0.0);
    observation->originalIjk.setAt(3, 1.0);

    if (this->getTransientAttribute("isDummyPoint").isValid()) {
      // set only if the attribute is available
      observation->isDummyPoint =
          this->getTransientAttribute("isDummyPoint").toBool();
    }

    // set observation to valid
    observation->isValid = true;

  } else if (this->rCartesian6D.isValid) {
    // set position
    observation->originalXyz.setAt(0, this->rCartesian6D.xyz.getAt(0));
    observation->originalXyz.setAt(1, this->rCartesian6D.xyz.getAt(1));
    observation->originalXyz.setAt(2, this->rCartesian6D.xyz.getAt(2));
    observation->originalXyz.setAt(3, 1.0);

    // set standard deviation
    observation->originalSigmaXyz.setAt(0,
                                        this->rCartesian6D.sigmaXyz.getAt(0));
    observation->originalSigmaXyz.setAt(1,
                                        this->rCartesian6D.sigmaXyz.getAt(1));
    observation->originalSigmaXyz.setAt(2,
                                        this->rCartesian6D.sigmaXyz.getAt(2));
    observation->originalSigmaXyz.setAt(3, 1.0);

    // set ijk
    observation->originalIjk.setAt(0, this->rCartesian6D.ijk.getAt(0));
    observation->originalIjk.setAt(1, this->rCartesian6D.ijk.getAt(1));
    observation->originalIjk.setAt(2, this->rCartesian6D.ijk.getAt(2));
    observation->originalIjk.setAt(3, 1.0);

    if (this->getTransientAttribute("isDummyPoint").isValid()) {
      // set only if the attribute is available
      observation->isDummyPoint =
          this->getTransientAttribute("isDummyPoint").toBool();
    }

    // set observation to valid
    observation->isValid = true;

  } else if (this->rLevel.isValid) {
    // set ijk
    observation->originalIjk.setAt(0, this->rLevel.i);
    observation->originalIjk.setAt(1, this->rLevel.j);
    observation->originalIjk.setAt(2, this->rLevel.k);
    observation->originalIjk.setAt(3, 1.0);

    // set standard deviation
    observation->originalSigmaIjk.setAt(0, this->rLevel.sigmaI);
    observation->originalSigmaIjk.setAt(1, this->rLevel.sigmaJ);
    observation->originalSigmaIjk.setAt(2, this->rLevel.sigmaK);
    observation->originalSigmaIjk.setAt(3, 1.0);

    // set position
    observation->originalXyz.setAt(0, 0.0);
    observation->originalXyz.setAt(1, 0.0);
    observation->originalXyz.setAt(2, 0.0);
    observation->originalXyz.setAt(3, 1.0);

    // set observation to valid
    observation->isValid = true;

    // set observation direction valid
    observation->hasDirection = true;
  }

  // set up dependencies
  observation->reading = this;
  this->observation = observation;
}

/*!
 * \brief Reading::getDisplayId
 * \return
 */
std::string Reading::getDisplayId() const { return oi::number(this->id); }

/*!
 * \brief Reading::getDisplayType
 * \return
 */
std::string Reading::getDisplayType() const {
  return getReadingTypeName(this->typeOfReading);
}

/*!
 * \brief Reading::getDisplayTime
 * \return
 */
std::string Reading::getDisplayTime() const {
  const auto seconds =
      std::chrono::floor<std::chrono::seconds>(this->measuredAt);
  try {
    return (std::format("{:%F %T}", std::chrono::zoned_time{
                                        std::chrono::current_zone(), seconds}));
  } catch (const std::exception&) {
    // no time zone database available: show UTC rather than nothing
    return (std::format("{:%F %T} UTC", seconds));
  }
}

/*!
 * \brief Reading::getDisplaySensorConfigName
 * \return
 */
std::string Reading::getDisplaySensorConfigName() const {
  return this->sensorConfigName;
}

/*!
 * \brief Reading::getDisplayMeasurementConfigName
 * \return
 */
std::string Reading::getDisplayMeasurementConfigName() const {
  return this->mConfig.getName();
}

/*!
 * \brief Reading::getDisplaySide
 * \return
 */
std::string Reading::getDisplaySide() const {
  switch (this->face) {
    case SensorFaces::eFrontSide:
      return "FS";
      break;
    case SensorFaces::eBackSide:
      return "BS";
      break;
    default:
      return "";
  }
}

/*!
 * \brief Reading::getDisplayAzimuth
 * \param type
 * \param digits
 * \return
 */
std::string Reading::getDisplayAzimuth(const UnitType& type,
                                       const int& digits) const {
  if (this->rPolar.isValid) {
    return oi::number(convertFromDefault(this->rPolar.azimuth, type), 'f',
                      digits);
  } else if (this->rDirection.isValid) {
    return oi::number(convertFromDefault(this->rDirection.azimuth, type), 'f',
                      digits);
  }
  return std::string("");
}

/*!
 * \brief Reading::getDisplayZenith
 * \param type
 * \param digits
 * \return
 */
std::string Reading::getDisplayZenith(const UnitType& type,
                                      const int& digits) const {
  if (this->rPolar.isValid) {
    return oi::number(convertFromDefault(this->rPolar.zenith, type), 'f',
                      digits);
  } else if (this->rDirection.isValid) {
    return oi::number(convertFromDefault(this->rDirection.zenith, type), 'f',
                      digits);
  }
  return std::string("");
}

/*!
 * \brief Reading::getDisplayDistance
 * \param type
 * \param digits
 * \return
 */
std::string Reading::getDisplayDistance(const UnitType& type,
                                        const int& digits) const {
  if (this->rPolar.isValid) {
    return oi::number(convertFromDefault(this->rPolar.distance, type), 'f',
                      digits);
  } else if (this->rDistance.isValid) {
    return oi::number(convertFromDefault(this->rDistance.distance, type), 'f',
                      digits);
  }
  return std::string("");
}

/*!
 * \brief Reading::getDisplayX
 * \param type
 * \param digits
 * \return
 */
std::string Reading::getDisplayX(const UnitType& type,
                                 const int& digits) const {
  if (this->rCartesian.isValid) {
    return oi::number(convertFromDefault(this->rCartesian.xyz.getAt(0), type),
                      'f', digits);
  } else if (this->rCartesian6D.isValid) {
    return oi::number(convertFromDefault(this->rCartesian6D.xyz.getAt(0), type),
                      'f', digits);
  }
  return std::string("");
}

/*!
 * \brief Reading::getDisplayY
 * \param type
 * \param digits
 * \return
 */
std::string Reading::getDisplayY(const UnitType& type,
                                 const int& digits) const {
  if (this->rCartesian.isValid) {
    return oi::number(convertFromDefault(this->rCartesian.xyz.getAt(1), type),
                      'f', digits);
  } else if (this->rCartesian6D.isValid) {
    return oi::number(convertFromDefault(this->rCartesian6D.xyz.getAt(1), type),
                      'f', digits);
  }
  return std::string("");
}

/*!
 * \brief Reading::getDisplayZ
 * \param type
 * \param digits
 * \return
 */
std::string Reading::getDisplayZ(const UnitType& type,
                                 const int& digits) const {
  if (this->rCartesian.isValid) {
    return oi::number(convertFromDefault(this->rCartesian.xyz.getAt(2), type),
                      'f', digits);
  } else if (this->rCartesian6D.isValid) {
    return oi::number(convertFromDefault(this->rCartesian6D.xyz.getAt(2), type),
                      'f', digits);
  }
  return std::string("");
}

/*!
 * \brief Reading::getDisplayI
 * \param type
 * \param digits
 * \return
 */
std::string Reading::getDisplayI(const int& digits) const {
  if (this->rLevel.isValid) {
    return oi::number(rLevel.i, 'f', digits);
  } else if (this->rCartesian6D.isValid) {
    return oi::number(this->rCartesian6D.ijk.getAt(0), 'f', digits);
  }
  return std::string("");
}

/*!
 * \brief Reading::getDisplayJ
 * \param type
 * \param digits
 * \return
 */
std::string Reading::getDisplayJ(const int& digits) const {
  if (this->rLevel.isValid) {
    return oi::number(this->rLevel.j, 'f', digits);
  } else if (this->rCartesian6D.isValid) {
    return oi::number(this->rCartesian6D.ijk.getAt(1), 'f', digits);
  }
  return std::string("");
}

/*!
 * \brief Reading::getDisplayK
 * \param type
 * \param digits
 * \return
 */
std::string Reading::getDisplayK(const int& digits) const {
  if (this->rLevel.isValid) {
    return oi::number(this->rLevel.k, 'f', digits);
  } else if (this->rCartesian6D.isValid) {
    return oi::number(this->rCartesian6D.ijk.getAt(2), 'f', digits);
  }
  return std::string("");
}

/*!
 * \brief Reading::getDisplayTemperature
 * \param type
 * \param digits
 * \return
 */
std::string Reading::getDisplayTemperature(const UnitType& type,
                                           const int& digits) const {
  if (this->rTemperature.isValid) {
    return oi::number(convertFromDefault(this->rTemperature.temperature, type),
                      'f', digits);
  }
  return std::string("");
}

/*!
 * \brief Reading::getDisplaySigmaAzimuth
 * \param type
 * \param digits
 * \return
 */
std::string Reading::getDisplaySigmaAzimuth(const UnitType& type,
                                            const int& digits) const {
  if (this->rPolar.isValid) {
    return oi::number(convertFromDefault(this->rPolar.sigmaAzimuth, type), 'f',
                      digits);
  } else if (this->rDirection.isValid) {
    return oi::number(convertFromDefault(this->rDirection.sigmaAzimuth, type),
                      'f', digits);
  }
  return std::string("");
}

/*!
 * \brief Reading::getDisplaySigmaZenith
 * \param type
 * \param digits
 * \return
 */
std::string Reading::getDisplaySigmaZenith(const UnitType& type,
                                           const int& digits) const {
  if (this->rPolar.isValid) {
    return oi::number(convertFromDefault(this->rPolar.sigmaZenith, type), 'f',
                      digits);
  } else if (this->rDirection.isValid) {
    return oi::number(convertFromDefault(this->rDirection.sigmaZenith, type),
                      'f', digits);
  }
  return std::string("");
}

/*!
 * \brief Reading::getDisplaySigmaDistance
 * \param type
 * \param digits
 * \return
 */
std::string Reading::getDisplaySigmaDistance(const UnitType& type,
                                             const int& digits) const {
  if (this->rPolar.isValid) {
    return oi::number(convertFromDefault(this->rPolar.sigmaDistance, type), 'f',
                      digits);
  } else if (this->rDistance.isValid) {
    return oi::number(convertFromDefault(this->rDistance.sigmaDistance, type),
                      'f', digits);
  }
  return std::string("");
}

/*!
 * \brief Reading::getDisplaySigmaX
 * \param type
 * \param digits
 * \return
 */
std::string Reading::getDisplaySigmaX(const UnitType& type,
                                      const int& digits) const {
  if (this->rCartesian.isValid) {
    return oi::number(
        convertFromDefault(this->rCartesian.sigmaXyz.getAt(0), type), 'f',
        digits);
  } else if (this->rCartesian6D.isValid) {
    return oi::number(
        convertFromDefault(this->rCartesian6D.sigmaXyz.getAt(0), type), 'f',
        digits);
  }
  return std::string("");
}

/*!
 * \brief Reading::getDisplaySigmaY
 * \param type
 * \param digits
 * \return
 */
std::string Reading::getDisplaySigmaY(const UnitType& type,
                                      const int& digits) const {
  if (this->rCartesian.isValid) {
    return oi::number(
        convertFromDefault(this->rCartesian.sigmaXyz.getAt(1), type), 'f',
        digits);
  }
  return std::string("");
}

/*!
 * \brief Reading::getDisplaySigmaZ
 * \param type
 * \param digits
 * \return
 */
std::string Reading::getDisplaySigmaZ(const UnitType& type,
                                      const int& digits) const {
  if (this->rCartesian.isValid) {
    return oi::number(
        convertFromDefault(this->rCartesian.sigmaXyz.getAt(2), type), 'f',
        digits);
  }
  return std::string("");
}

/*!
 * \brief Reading::getDisplaySigmaRX
 * \param type
 * \param digits
 * \return
 */
std::string Reading::getDisplaySigmaI(const int& digits) const {
  if (this->rLevel.isValid) {
    return oi::number(this->rLevel.sigmaI, 'f', digits);
  }
  return std::string("");
}

/*!
 * \brief Reading::getDisplaySigmaRY
 * \param type
 * \param digits
 * \return
 */
std::string Reading::getDisplaySigmaJ(const int& digits) const {
  if (this->rLevel.isValid) {
    return oi::number(this->rLevel.sigmaJ, 'f', digits);
  }
  return std::string("");
}

/*!
 * \brief Reading::getDisplaySigmaRZ
 * \param type
 * \param digits
 * \return
 */
std::string Reading::getDisplaySigmaK(const int& digits) const {
  if (this->rLevel.isValid) {
    return oi::number(this->rLevel.sigmaK, 'f', digits);
  }
  return std::string("");
}

/*!
 * \brief Reading::getDisplaySigmaTemperature
 * \param type
 * \param digits
 * \return
 */
std::string Reading::getDisplaySigmaTemperature(const UnitType& type,
                                                const int& digits) const {
  if (this->rTemperature.isValid) {
    return oi::number(convertFromDefault(this->rTemperature.temperature, type),
                      'f', digits);
  }
  return std::string("");
}

/*!
 * \brief Reading::toCartesian
 */
void Reading::toCartesian() {
  if (!this->rPolar.isValid           // check if the polar reading is not valid
      || this->rCartesian.isValid) {  // cartesian reading available
    return;
  }

  // transform polar to cartesian
  this->rCartesian.xyz.setAt(0, this->rPolar.distance *
                                    std::sin(this->rPolar.zenith) *
                                    std::cos(this->rPolar.azimuth));
  this->rCartesian.xyz.setAt(1, this->rPolar.distance *
                                    std::sin(this->rPolar.zenith) *
                                    std::sin(this->rPolar.azimuth));
  this->rCartesian.xyz.setAt(
      2, this->rPolar.distance * std::cos(this->rPolar.zenith));
  this->rCartesian.isValid = true;
}

/*!
 * \brief Reading::toPolar
 */
void Reading::toPolar() {
  if (!this->rCartesian.isValid   // nothing to convert from
      || this->rPolar.isValid) {  // polar reading already available
    return;
  }

  // transform cartesian to polar
  double x = this->rCartesian.xyz.getAt(0);
  double y = this->rCartesian.xyz.getAt(1);
  double z = this->rCartesian.xyz.getAt(2);
  this->rPolar.azimuth = std::atan2(y, x);
  this->rPolar.distance = std::sqrt(x * x + y * y + z * z);
  // a zero distance has no direction: zenith is arbitrary there (pi/2, as
  // the pseudo tracker does); the clamp guards against a ratio rounding
  // past +-1
  this->rPolar.zenith =
      this->rPolar.distance > 0.0
          ? std::acos(std::clamp(z / this->rPolar.distance, -1.0, 1.0))
          : kPi / 2;
  this->rPolar.isValid = true;
}

/*!
 * \brief Reading::errorPropagationPolarToCartesian
 * \return
 */
OiVec Reading::errorPropagationPolarToCartesian() {
  // check if polar reading is valid
  if (!rPolar.isValid) {
    return OiVec(3);
  }

  OiVec sigmaXyz;

  OiMat F(3, 3);
  F.setAt(0, 0, std::sin(this->rPolar.zenith) * std::cos(this->rPolar.azimuth));
  F.setAt(0, 1,
          this->rPolar.distance * std::sin(this->rPolar.zenith) *
              -std::sin(this->rPolar.azimuth));
  F.setAt(0, 2,
          this->rPolar.distance * std::cos(this->rPolar.zenith) *
              std::cos(this->rPolar.azimuth));
  F.setAt(1, 0, std::sin(this->rPolar.zenith) * std::sin(this->rPolar.azimuth));
  F.setAt(1, 1,
          this->rPolar.distance * std::sin(this->rPolar.zenith) *
              std::cos(this->rPolar.azimuth));
  F.setAt(1, 2,
          this->rPolar.distance * std::cos(this->rPolar.zenith) *
              std::sin(this->rPolar.azimuth));
  F.setAt(2, 0, std::cos(this->rPolar.zenith));
  F.setAt(2, 1, 0.0);
  F.setAt(2, 2, this->rPolar.distance * -std::sin(this->rPolar.zenith));

  OiMat Sll(3, 3);
  Sll.setAt(0, 0, this->rPolar.sigmaDistance * this->rPolar.sigmaDistance);
  Sll.setAt(1, 1, this->rPolar.sigmaAzimuth * this->rPolar.sigmaAzimuth);
  Sll.setAt(2, 2, this->rPolar.sigmaZenith * this->rPolar.sigmaZenith);

  OiMat Qxx = F * Sll * F.t();

  sigmaXyz.add(std::sqrt(Qxx.getAt(0, 0)));
  sigmaXyz.add(std::sqrt(Qxx.getAt(1, 1)));
  sigmaXyz.add(std::sqrt(Qxx.getAt(2, 2)));

  return sigmaXyz;
}
