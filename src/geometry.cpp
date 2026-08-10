#include "geometry.h"

#include "oijob.h"
#include "coordinatesystem.h"
#include "observation.h"
#include "station.h"
#include "function.h"
#include "fitfunction.h"

using namespace oi;
using namespace oi::math;

/*!
 * \brief Geometry::Geometry
 * \param parent
 */
Geometry::Geometry(QObject *parent) : Feature(parent), isCommon(false){

}

/*!
 * \brief Geometry::Geometry
 * \param copy
 * \param parent
 */
Geometry::Geometry(const Geometry &copy, QObject *parent) : Feature(copy, parent){

    //copy attributes
    this->isCommon = copy.isCommon;
    this->statistic = copy.statistic;
    this->simulationData = copy.simulationData;
    this->activeMeasurementConfig = copy.activeMeasurementConfig;

    this->xyz = copy.xyz;
    this->ijk = copy.ijk;
    this->radius = copy.radius;

    //deliberately not copied: realises, observations, nominalSystem -
    //duplicating a feature must not duplicate what it realises, what
    //observed it, or which system it is nominal in

}

/*!
 * \brief Geometry::operator =
 * \param copy
 * \return
 */
Geometry &Geometry::operator=(const Geometry &copy){

    //copy attributes
    this->isCommon = copy.isCommon;
    this->statistic = copy.statistic;
    this->simulationData = copy.simulationData;
    this->activeMeasurementConfig = copy.activeMeasurementConfig;

    this->xyz = copy.xyz;
    this->ijk = copy.ijk;
    this->radius = copy.radius;

    //deliberately not copied: realises, observations, nominalSystem - see
    //the copy constructor above

    return *this;

}

/*!
 * \brief Geometry::~Geometry
 */
Geometry::~Geometry(){

    //delete this geometry from the nominal list of its coordinate system,
    //if it was a nominal
    if(!this->nominalSystem.isNull()){
        this->nominalSystem->removeNominal(this->selfFeature);
    }

    //delete this geometry from the target geometries list of any
    //observations that reference it
    foreach(const QPointer<Observation> &observation, this->observations){
        if(!observation.isNull()){
            observation->removeTargetGeometry(this->id);
        }
    }

}

/*!
 * \brief Geometry::getIsCommon
 * \return
 */
const bool &Geometry::getIsCommon() const{
    return this->isCommon;
}

/*!
 * \brief Geometry::setCommonState
 * \param isCommon
 */
void Geometry::setCommonState(const bool &isCommon){
    if(isCommon != this->isCommon){
        this->isCommon = isCommon;
        emit this->geomIsCommonChanged(this->id);
    }
}

/*!
 * \brief Geometry::getOwnerDescription
 * D19: a geometry produced by a FitFunction is owned by that fit and is
 * read-only - the fit regenerates it on every recompute, so a user edit
 * would just be overwritten (or worse, silently diverge until the next
 * recalc). Anything else - no producer yet, or a scalar-parameter-only
 * producer such as NominalPoint - is user-owned and editable. This is
 * what makes a nominal editable and an actual not, without a flag: it
 * falls out of which kind of function produced it.
 * \return
 */
std::optional<QString> Geometry::getOwnerDescription() const{

    if(this->functionList.isEmpty()){
        return std::nullopt;
    }

    const QPointer<Function> producer = this->functionList.constFirst();
    if(producer.isNull() || dynamic_cast<FitFunction*>(producer.data()) == nullptr){
        return std::nullopt;
    }

    return QString("its fit function \"%1\"").arg(producer->getMetaData().name);

}

/*!
 * \brief Geometry::getRealises
 * \return
 */
const std::optional<QPointer<Geometry> > &Geometry::getRealises() const{
    return this->realises;
}

/*!
 * \brief Geometry::setRealises
 * Record which nominal this (actual) geometry realises. Not a user edit -
 * this is set by whatever created the actual to measure a specific
 * nominal (Controller's "measure this nominal" operation) - so it is not
 * gated by rejectIfNotEditable.
 * \param nominal
 * \return
 */
bool Geometry::setRealises(const QPointer<Geometry> &nominal){

    if(nominal.isNull()){
        return false;
    }

    //the nominal side of the relationship must actually be user-owned -
    //an actual realising another actual makes no sense
    if(nominal->getOwnerDescription().has_value()){
        return false;
    }

    //compare feature types
    if(this->selfFeature.isNull() || nominal->selfFeature.isNull()
            || this->selfFeature->getFeatureTypeEnum() != nominal->selfFeature->getFeatureTypeEnum()){
        return false;
    }

    //check if the nominal is in the same job
    if(!this->job.isNull()){
        QPointer<FeatureWrapper> jobNominal = this->job->getFeatureById(nominal->getId());
        if(jobNominal.isNull() || jobNominal->getGeometry().isNull() || jobNominal->getGeometry() != nominal){
            return false;
        }
    }

    this->realises = nominal;
    emit this->geomRealisesChanged(this->id);

    return true;

}

/*!
 * \brief Geometry::clearRealises
 */
void Geometry::clearRealises(){
    if(this->realises.has_value()){
        this->realises = std::nullopt;
        emit this->geomRealisesChanged(this->id);
    }
}

/*!
 * \brief Geometry::getObservations
 * \return
 */
const QList<QPointer<Observation> > &Geometry::getObservations() const{
    return this->observations;
}

/*!
 * \brief Geometry::addObservation
 * \param obs
 */
void Geometry::addObservation(const QPointer<Observation> &obs){

    //this is the acquisition path writing to whatever it is measuring
    //into (an actual), not a user edit, so it is not gated by D19
    if(!obs.isNull()){

        //check if obs is already included
        if(this->observations.contains(obs)){
            return;
        }

        this->observations.append(obs);

        //update target geometries
        obs->addTargetGeometry(this);

        //update used reading types
        if(!obs->getReading().isNull() && !this->usedReadingTypes.contains(obs->getReading()->getTypeOfReading())){
            this->usedReadingTypes.append(obs->getReading()->getTypeOfReading());
        }

        this->isUpdated = false;

        emit this->geomObservationsChanged(this->id);

    }

}

/*!
 * \brief Geometry::removeObservation
 * \param obs
 */
void Geometry::removeObservation(const QPointer<Observation> &obs){

    //check if obs is valid
    if(obs.isNull()){
        return;
    }

    //remove observation
    if(this->observations.removeOne(obs)){

        //update target geometries
        obs->removeTargetGeometry(this);

        //update used reading types
        if(!obs->getReading().isNull() && this->usedReadingTypes.contains(obs->getReading()->getTypeOfReading())){
            this->usedReadingTypes.removeOne(obs->getReading()->getTypeOfReading());
        }

        this->isUpdated = false;

        emit this->geomObservationsChanged(this->id);

    }

}

/*!
 * \brief Geometry::getNominalSystem
 * \return
 */
const QPointer<CoordinateSystem> &Geometry::getNominalSystem() const{
    return this->nominalSystem;
}

/*!
 * \brief Geometry::setNominalSystem
 * \param nomSys
 * \return
 */
bool Geometry::setNominalSystem(const QPointer<CoordinateSystem> &nomSys){

    //only a user-owned geometry (a nominal) is defined in a nominal
    //system; a fit-owned actual is not (D19)
    if(!nomSys.isNull() && !this->getOwnerDescription().has_value() && this->nominalSystem != nomSys){

        //check if nomSys is in the same job
        if(!this->job.isNull()){
            QPointer<FeatureWrapper> jobSystem = this->job->getFeatureById(nomSys->getId());
            if(jobSystem.isNull() || jobSystem->getCoordinateSystem().isNull() || jobSystem->getCoordinateSystem() != nomSys){
                return false;
            }
        }

        this->nominalSystem = nomSys;

        nomSys->addNominal(this->selfFeature);

        emit this->geomNominalSystemChanged(this->id);

        return true;

    }

    return false;

}

/*!
 * \brief Geometry::getMeasurementConfig
 * \return
 */
const MeasurementConfigKey &Geometry::getMeasurementConfig() const{
    return this->activeMeasurementConfig;
}

/*!
 * \brief Geometry::setMeasurementConfig
 * \param myConfig
 */
void Geometry::setMeasurementConfig(const MeasurementConfigKey &myConfig){
    QString oldName = this->activeMeasurementConfig.getName();
    MeasurementConfigKey oldKey = this->activeMeasurementConfig;
    this->activeMeasurementConfig = myConfig;
    emit this->geomMeasurementConfigChanged(this->id, oldName, oldKey);
}

/*!
 * \brief Geometry::getStatistic
 * \return
 */
const Statistic &Geometry::getStatistic() const{
    return this->statistic;
}

/*!
 * \brief Geometry::setStatistic
 * \param myStatistic
 */
void Geometry::setStatistic(const Statistic &myStatistic){
    this->statistic = myStatistic;
}

/*!
 * \brief Geometry::getSimulationData
 * \return
 */
const SimulationData &Geometry::getSimulationData(){
    return this->simulationData;
}

/*!
 * \brief Geometry::setSimulationData
 * \param s
 */
void Geometry::setSimulationData(const SimulationData &s){
    this->simulationData = s;
}

/*!
 * \brief Geometry::getUsedReadingTypes
 * \return
 */
const QList<ReadingTypes> &Geometry::getUsedReadingTypes() const{
    return this->usedReadingTypes;
}

/*!
 * \brief Geometry::hasDirection
 * \return
 */
bool Geometry::hasDirection() const{
    return false;
}

/*!
 * \brief Geometry::hasPosition
 * \return
 */
bool Geometry::hasPosition() const{
    return false;
}

/*!
 * \brief Geometry::hasRadius
 * \return
 */
bool Geometry::hasRadius() const{
    return false;
}

/*!
 * \brief Geometry::getRadius
 * \return
 */
const Radius &Geometry::getRadius() const{
    return this->radius;
}

/*!
 * \brief Geometry::getDirection
 * \return
 */
const Direction &Geometry::getDirection() const{
    return this->ijk;
}

void Geometry::setDirection(Direction &ijk) {
    this->ijk = ijk;
    emit this->geomParametersChanged(this->id);
}


/*!
 * \brief Geometry::getPosition
 * \return
 */
const Position &Geometry::getPosition() const{
    return this->xyz;
}

void Geometry::setPosition(Position &xyz) {
    this->xyz = xyz;
    emit this->geomParametersChanged(this->id);
}
/*!
 * \brief Geometry::getUnknownParameters
 * \param displayUnits
 * \param displayDigits
 * \return
 */
QMap<GeometryParameters, QString> Geometry::getUnknownParameters(const QMap<DimensionType, UnitType> &/*displayUnits*/, const QMap<DimensionType, int> &/*displayDigits*/) const{
    return QMap<GeometryParameters, QString>();
}

/*!
 * \brief Geometry::setUnknownParameters
 * D19 enforcement point for direct user edits of a geometry's own
 * parameters (reached from the nominal properties dialog). See the
 * comment on the declaration for why this - rather than
 * applyUnknownParameters - is where the check lives.
 * \param parameters
 */
void Geometry::setUnknownParameters(const QMap<GeometryParameters, double> &parameters){
    if(this->rejectIfNotEditable("parameters")){
        return;
    }
    this->applyUnknownParameters(parameters);
}

/*!
 * \brief Geometry::applyUnknownParameters
 * Base case: no unknown parameters. Subclasses that have any override
 * this instead of setUnknownParameters.
 * \param parameters
 */
void Geometry::applyUnknownParameters(const QMap<GeometryParameters, double> &/*parameters*/){
    return;
}

/*!
 * \brief Geometry::recalc
 */
void Geometry::recalc(){

    Feature::recalc();

    //a geometry with a nominal system but no producer function yet (a
    //freshly created nominal before its first parameter entry) is
    //considered solved as soon as that system is the active one
    if(this->functionList.size() == 0 && !this->nominalSystem.isNull() && this->nominalSystem->getIsActiveCoordinateSystem()){
        this->isSolved = true;
    }

    //reset statistic if not solved
    if(!this->isSolved){
        this->statistic.reset();
    }

}

/*!
 * \brief Geometry::toOpenIndyXML
 * \param xmlDoc
 * \return
 */
QDomElement Geometry::toOpenIndyXML(QDomDocument &xmlDoc) const{

    QDomElement geometry = Feature::toOpenIndyXML(xmlDoc);

    if(geometry.isNull()){
        return geometry;
    }

    geometry.setTagName("geometry");

    //D17/D19: no "nominal" flag is persisted any more. Whether this
    //geometry is a nominal or an actual is not stored state - it is
    //re-derived from its producer function once functions are attached on
    //load, same as at runtime (Geometry::getOwnerDescription).
    const bool isEditable = !this->getOwnerDescription().has_value();
    geometry.setAttribute("common", this->getIsCommon());

    //add position
    if(this->hasPosition()){
        QDomElement coordinates = xmlDoc.createElement("coordinates");
        if(this->isSolved || isEditable){
            coordinates.setAttribute("x", this->getPosition().getVector().getAt(0));
            coordinates.setAttribute("y", this->getPosition().getVector().getAt(1));
            coordinates.setAttribute("z", this->getPosition().getVector().getAt(2));
        }else{
            coordinates.setAttribute("x", 0.0);
            coordinates.setAttribute("y", 0.0);
            coordinates.setAttribute("z", 0.0);
        }
        geometry.appendChild(coordinates);
    }

    //add direction
    if(this->hasDirection()){
        QDomElement coordinates = xmlDoc.createElement("spatialDirection");
        if(this->isSolved || isEditable){
            coordinates.setAttribute("i", this->getDirection().getVector().getAt(0));
            coordinates.setAttribute("j", this->getDirection().getVector().getAt(1));
            coordinates.setAttribute("k", this->getDirection().getVector().getAt(2));
        }else{
            coordinates.setAttribute("i", 0.0);
            coordinates.setAttribute("j", 0.0);
            coordinates.setAttribute("k", 0.0);
        }
        geometry.appendChild(coordinates);
    }

    //add radius
    if(this->hasRadius()){
        QDomElement coordinates = xmlDoc.createElement("radius");
        if(this->isSolved || isEditable){
            coordinates.setAttribute("value", this->getRadius().getRadius());
        }else{
            coordinates.setAttribute("value", 0.0);
        }
        geometry.appendChild(coordinates);
    }

    //add standard deviation
    QDomElement stdv = xmlDoc.createElement("standardDeviation");
    if(this->getIsSolved()){
        stdv.setAttribute("value", this->statistic.getStdev());
    }else{
        stdv.setAttribute("value", 0.0);
    }
    geometry.appendChild(stdv);

    //add observations (only ever non-empty for an actual, structurally -
    //nothing attaches observations to a nominal)
    if(this->observations.size() > 0){
        QDomElement observations = xmlDoc.createElement("observations");
        foreach(const QPointer<Observation> &obs, this->observations){
            QDomElement observation = xmlDoc.createElement("observation");
            observation.setAttribute("ref", obs->getId());
            observations.appendChild(observation);
        }
        geometry.appendChild(observations);
    }

    //add nominal system
    if(!this->nominalSystem.isNull()){
        QDomElement nominalSystem = xmlDoc.createElement("nominalCoordinateSystem");
        nominalSystem.setAttribute("ref", this->nominalSystem->getId());
        geometry.appendChild(nominalSystem);
    }

    //add the nominal this geometry realises, if any (D17)
    if(this->realises.has_value() && !this->realises->isNull()){
        QDomElement realises = xmlDoc.createElement("realises");
        realises.setAttribute("ref", this->realises.value()->getId());
        geometry.appendChild(realises);
    }

    //add measurement config
    if(this->activeMeasurementConfig.isValid()){
        QDomElement mConfig = xmlDoc.createElement("measurementConfig");
        mConfig.setAttribute("name", this->activeMeasurementConfig.getName());
        //sensorType disambiguates configs that share a name across driver
        //types (e.g. two drivers both publishing "FastPoint") - name alone
        //was ambiguous and made this round-trip lossy
        if(this->activeMeasurementConfig.getSensorType().isValid()){
            mConfig.setAttribute("sensorType", this->activeMeasurementConfig.getSensorType().id());
        }
        geometry.appendChild(mConfig);
    }

    return geometry;

}

/*!
 * \brief Geometry::fromOpenIndyXML
 * \param xmlElem
 * \return
 */
bool Geometry::fromOpenIndyXML(QDomElement &xmlElem){

    bool result = Feature::fromOpenIndyXML(xmlElem);

    if(result){

        //set geometry attributes. No "nominal" flag to read any more - see
        //toOpenIndyXML.
        if(!xmlElem.hasAttribute("common")){
            return false;
        }
        this->isCommon = xmlElem.attribute("common").toInt();

        //set standard deviation
        QDomElement stdv = xmlElem.firstChildElement("standardDeviation");
        if(stdv.isNull() || !stdv.hasAttribute("value")){
            return false;
        }
        this->statistic.setStdev(stdv.attribute("value").toDouble());

    }

    return result;

}

/*!
 * \brief Geometry::getDisplayStDev
 * \param type
 * \param digits
 * \return
 */
QString Geometry::getDisplayStDev(const UnitType &type, const int &digits) const{
    if(this->statistic.getIsValid()){
        return QString::number(convertFromDefault(this->statistic.getStdev(), type), 'f', digits);
    }
    return QString("");
}

/*!
 * \brief Geometry::getDisplayFormError
 * \param type
 * \param digits
 * \return
 */
QString Geometry::getDisplayFormError(const UnitType &type, const int &digits) const{
    if(this->statistic.getIsValid()){
        return QString::number(convertFromDefault(this->statistic.getFormError(), type), 'f', digits);
    }
    return QString("");
}
/*!
 * \brief Geometry::getDisplayMeasurementConfig
 * \return
 */
QString Geometry::getDisplayMeasurementConfig() const{
    if(this->activeMeasurementConfig.isValid()){
        if(this->activeMeasurementConfig.isProjectConfig()){
            return this->activeMeasurementConfig.getName();
        }else{
            return QString("%1 [user]").arg(this->activeMeasurementConfig.getName());
        }
    }
    return QString("");
}

/*!
 * \brief Geometry::getDisplayObservations
 * \return
 */
QString Geometry::getDisplayObservations() const{

    //a user-owned (nominal-like) geometry structurally never has
    //observations - nothing acquires into it
    if(!this->getOwnerDescription().has_value()){
        return "";
    }

    int validObs = 0;
    int totalObs = this->observations.size();

    //get the number of used observations in the first function of this geometry
    if(totalObs > 0 && this->functionList.size() > 0){

        //get and check function pointer
        QPointer<Function> firstFunction = this->functionList.at(0);
        if(firstFunction.isNull()){
            return QString("0/%1").arg(totalObs);
        }

        //get the number of used observations
        QMap<int, QList<InputElement> > inputElements = firstFunction->getInputElements();
        QList<int> keys = inputElements.keys();
        foreach(const int &key, keys){

            //check if input alements at key are observations
            if(inputElements[key].size() > 0 && inputElements[key][0].typeOfElement == eObservationElement){

                //run through all observations and check their used state
                foreach(const InputElement &element, inputElements[key]){
                    if(element.isUsed){
                        validObs++;
                    }
                }
            }
        }

        return QString("%1/%2").arg(validObs).arg(totalObs);
    }

    return QString("0/%1").arg(totalObs);
}

/*!
 * \brief Geometry::getDisplayIsCommon
 * \return
 */
QString Geometry::getDisplayIsCommon() const{
    return this->isCommon?"true":"false";
}

/*!
 * \brief Geometry::getDisplayIsActual
 * \return
 */
QString Geometry::getDisplayIsActual() const
{
    if(!this->getOwnerDescription().has_value()){
        QString output = "nominal";
        if(!this->getNominalSystem().isNull()){
            output.append(" ").append(this->getNominalSystem()->getFeatureName());
        }
        return output;
    }else{
        return "actual";
    }
}


/*!
 * \brief Plane::getDisplayX
 * \param type
 * \param digits
 * \param showDiff
 * \return
 */
QString Geometry::getDisplayX(const UnitType &type, const int &digits, const bool &/*showDiff*/) const{
    if(this->isSolved){
        return QString::number(convertFromDefault(this->xyz.getVector().getAt(0), type), 'f', digits);
    }
    return QString("");
}

/*!
 * \brief Plane::getDisplayY
 * \param type
 * \param digits
 * \param showDiff
 * \return
 */
QString Geometry::getDisplayY(const UnitType &type, const int &digits, const bool &/*showDiff*/) const{
    if(this->isSolved){
        return QString::number(convertFromDefault(this->xyz.getVector().getAt(1), type), 'f', digits);
    }
    return QString("");
}

/*!
 * \brief Plane::getDisplayZ
 * \param type
 * \param digits
 * \param showDiff
 * \return
 */
QString Geometry::getDisplayZ(const UnitType &type, const int &digits, const bool &/*showDiff*/) const{
    if(this->isSolved){
        return QString::number(convertFromDefault(this->xyz.getVector().getAt(2), type), 'f', digits);
    }
    return QString("");
}

/*!
 * \brief Plane::getDisplayPrimaryI
 * \param digits
 * \param showDiff
 * \return
 */
QString Geometry::getDisplayPrimaryI(const int &digits, const bool &showDiff) const{
    if(!this->hasDirection()) {
        return Feature::getDisplayPrimaryI(digits, showDiff);
    }
    if(this->isSolved){
        return QString::number(this->ijk.getVector().getAt(0), 'f', digits);
    }
    return QString("");
}

/*!
 * \brief Plane::getDisplayPrimaryJ
 * \param digits
 * \param showDiff
 * \return
 */
QString Geometry::getDisplayPrimaryJ(const int &digits, const bool &showDiff) const{
    if(!this->hasDirection()) {
        return Feature::getDisplayPrimaryJ(digits, showDiff);
    }
    if(this->isSolved){
        return QString::number(this->ijk.getVector().getAt(1), 'f', digits);
    }
    return QString("");
}

/*!
 * \brief Plane::getDisplayPrimaryK
 * \param digits
 * \param showDiff
 * \return
 */
QString Geometry::getDisplayPrimaryK(const int &digits, const bool &showDiff) const{
    if(!this->hasDirection()) {
        return Feature::getDisplayPrimaryK(digits, showDiff);
    }
    if(this->isSolved){
        return QString::number(this->ijk.getVector().getAt(2), 'f', digits);
    }
    return QString("");
}

/*!
 * \brief Cylinder::getDisplayRadiusA
 * \param type
 * \param digits
 * \param showDiff
 * \return
 */
QString Geometry::getDisplayRadiusA(const UnitType &type, const int &digits, const bool &/*showDiff*/) const{
    if(this->isSolved){
        return QString::number(convertFromDefault(this->radius.getRadius(), type), 'f', digits);
    }
    return QString("");
}
