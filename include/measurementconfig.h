#ifndef MEASUREMENTCONFIG_H
#define MEASUREMENTCONFIG_H

#include <QString>
#include <QtXml>
#include <QHash>

#include "types.h"
#include "util.h"

namespace oi{

enum MeasurementTypes{
    eUnknown_MeasurementType = -1,
    eSinglePoint_MeasurementType = 0,
    eScanTimeDependent_MeasurementType,
    eScanDistanceDependent_MeasurementType,
    eLevel_MeasurementType,
    eDirection_MeasurementType,
    eDistance_MeasurementType,
    eTemperature_MeasurementType
};

enum MeasurementModes {
    eFast_MeasurementMode = 0,
    eStandard_MeasurementMode,
    ePrecise_MeasurementMode
};

enum ConfigTypes {
    eUndefinded = -1,
    eUserConfig = 0,
    eProjectConfig,
    eStandardConfig
};

class OI_CORE_EXPORT MeasurementConfigKey {
public:
    MeasurementConfigKey(): configType(eUndefinded) {}
    MeasurementConfigKey(QString name, ConfigTypes configType) : name(name), configType(configType){}
    MeasurementConfigKey(const MeasurementConfigKey &copy) {
        this->name = copy.name;
        this->configType = copy.configType;
    }
    MeasurementConfigKey &operator=(const MeasurementConfigKey &copy) {
        this->name = copy.name;
        this->configType = copy.configType;
        return *this;
    }

    friend bool operator==(const MeasurementConfigKey &left, const MeasurementConfigKey &right){

        return left.getName() == right.getName()        // combined key
                && left.configType == right.configType; // combined key
    }

    friend uint qHash(const MeasurementConfigKey key, uint seed) {
        QtPrivate::QHashCombine hash(seed);
        seed = hash(seed, key.name);
        seed = hash(seed, key.configType);
        return seed;
    }

    friend bool operator<(const MeasurementConfigKey &left, const MeasurementConfigKey &right) {
        if(left.name != right.name) {
            return left.name < right.name;
        } else {
            return left.configType < right.configType;
        }
    }

    const QString getName() const {
        return this->name;
    }

    ConfigTypes getConfigType() const {
        return this->configType;
    }

    bool isUserConfig() const{
        return eUserConfig == this->configType;
    }
    bool isProjectConfig() const{
        return eProjectConfig == this->configType || eStandardConfig == this->configType;
    }
    bool isStandardConfig() const {
        return eStandardConfig == this->configType;
    }
    bool isValid() const{
        return !this->name.isEmpty() && this->configType != eUndefinded;
    }

private:
    QString name;
    ConfigTypes configType;
};

/*!
 * \brief The MeasurementConfig class
 * Contains all the configuration parameters needed to start a measurement.
 */
class OI_CORE_EXPORT MeasurementConfig
{

public:
    MeasurementConfig();

    MeasurementConfig(const MeasurementConfig &copy);

    MeasurementConfig &operator=(const MeasurementConfig &copy);

    friend bool operator==(const MeasurementConfig &left, const MeasurementConfig &right){

        return left.getName() == right.getName()        // combined key
                && left.configType == right.configType  // combined key
                && left.editable == right.editable
                && left.getMeasurementType() == right.getMeasurementType()
                && left.getMeasurementMode() == right.getMeasurementMode()
                && left.getMeasureTwoSides() == right.getMeasureTwoSides()
                && left.getMaxObservations() == right.getMaxObservations()
                && left.getTimeInterval() == right.getTimeInterval()
                && almostEqual(left.getDistanceInterval(), right.getDistanceInterval(), 8);
    }

    //########################################
    //get or set measurement config attributes
    //########################################

    const QString &getName() const;
    void setName(const QString &name);
    MeasurementConfigKey getKey() const;

    bool isUserConfig() const;
    bool isProjectConfig() const;
    bool isEditable() const;
    bool isStandardConfig() const;
    bool isValid() const;

    const bool &getMeasureTwoSides() const;
    void setMeasureTwoSides(const bool &measureTwoSides);

    const double &getTimeInterval() const;
    void setTimeInterval(const double &interval);

    const double &getDistanceInterval() const;
    void setDistanceInterval(const double &interval);

    const QVariant getTransientData(const QString key) const;
    void setTransientData(const QString key, const QVariant value);

    void setMeasurementMode(const MeasurementModes mode);
    MeasurementModes getMeasurementMode() const;

    void setMeasurementType(const MeasurementTypes type);
    MeasurementTypes getMeasurementType() const;

    const int &getMaxObservations() const;
    void setMaxObservations(const int &maxObservations);

    void makeUserConfig();
    void makeStandardConfig();
    void makeProjectConfig();


    //#################
    //save and load XML
    //#################

    QDomElement toOpenIndyXML(QDomDocument &xmlDoc) const;
    bool fromOpenIndyXML(QDomElement &xmlElem);

    bool applicableFor(const ElementTypes elementType, QList<FeatureTypes> functionIsApplicableFor);

private:

    //#############################
    //measurement config attributes
    //#############################
    QString name;
    ConfigTypes configType;

    bool measureTwoSides; //true if the sensor shall measure in both faces (false if not)
    double timeInterval; //time interval in which the sensor shall measure
    double distanceInterval; //distance interval in which the sensor shall measure [mm]

    QMap<QString, QVariant> transientData;

    MeasurementTypes measurementType;
    MeasurementModes measurementMode;
    int maxObservations;

    bool editable;
};

}

Q_DECLARE_METATYPE( oi::MeasurementConfig )
Q_DECLARE_METATYPE( oi::MeasurementConfig* )

#endif // MEASUREMENTCONFIG_H
