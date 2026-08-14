#include "function.h"
#include "specialfunction.h"

using namespace oi;

namespace {

/*!
 * Stage 7d/D21 trap, found via tst_function/tst_calibratedconstellation
 * regressions while migrating: Geometry subtypes' and TrafoParam's
 * hand-written operator= (Plane::operator=, TrafoParam::operator=, ...)
 * only ever copy that *subtype's own* added fields (xyz/ijk/d for Plane,
 * rotation/translation/scale for TrafoParam, ...), never Geometry's or
 * TrafoParam's own "statistic" member - because before Stage 7d, exec()
 * mutated the live target object directly and operator= was never in the
 * loop at all. Now that a Producer/Transform builds a fresh local T and
 * `*target = result.value()` uses that type's operator= to write it back
 * (see execAs() below), the same operator= gap silently drops whatever
 * statistic the function's own setUpResult()-style helper set on its
 * local result - visible as getStatistic().getStdev() reading back 0.
 * Recorded in domain-model.md S10 as trap #12. Fixed generically here,
 * not by editing every affected operator=, so it also covers any future T
 * with the same shape without another silent gap.
 */
template<typename T>
concept HasStatisticAccessors = requires(T &mutableTarget, const T &constSource){
    { constSource.getStatistic() } -> std::convertible_to<const Statistic&>;
    mutableTarget.setStatistic(constSource.getStatistic());
};

template<typename T>
void propagateStatisticIfPresent(T &target, const T &source){
    if constexpr (HasStatisticAccessors<T>){
        target.setStatistic(source.getStatistic());
    }
}

/*!
 * Stage 7d/D21 (domain-model.md S7): the one place a Function is actually
 * dispatched to its Producer<T>/Transform<T> mixin and the result written
 * back into the feature's own geometry object via that type's ordinary
 * operator= (which - like Plane::operator=, Point::operator=, ... -
 * copies only the geometric fields, never id/parent/functionList, exactly
 * matching what the old in-place exec(Type&) family did) plus statistic,
 * explicitly, per the trap above. A cast that fails on both mixins is the
 * "this function doesn't belong in this slot" rejection.
 */
template<typename T>
std::expected<void, QString> execAs(oi::Function *function, T *target, const QString &typeName){

    if(oi::Producer<T> *producer = dynamic_cast<oi::Producer<T>*>(function)){
        std::expected<T, QString> result = producer->exec();
        if(!result.has_value()){
            return std::unexpected(result.error());
        }
        *target = result.value();
        propagateStatisticIfPresent(*target, result.value());
        return {};
    }

    if(oi::Transform<T> *transform = dynamic_cast<oi::Transform<T>*>(function)){
        std::expected<T, QString> result = transform->exec(*target);
        if(!result.has_value()){
            return std::unexpected(result.error());
        }
        *target = result.value();
        propagateStatisticIfPresent(*target, result.value());
        return {};
    }

    return std::unexpected(QString("Function \"%1\" not implemented for type %2")
                                .arg(function->getMetaData().name, typeName));

}

/*!
 * Stage 7d/D21: does `function` implement Producer<T> (isProducer=true) or
 * Transform<T> (isProducer=false) for this concrete type? Used by both
 * Function::canActAsProducer/canActAsTransform (attach-time check, no
 * feature instance needed) - the type T is supplied by the caller's
 * switch case, exactly mirroring execByFeatureType's dispatch switch.
 */
template<typename T>
bool implementsProducer(oi::Function *function){
    return dynamic_cast<oi::Producer<T>*>(function) != nullptr;
}

template<typename T>
bool implementsTransform(oi::Function *function){
    return dynamic_cast<oi::Transform<T>*>(function) != nullptr;
}

}

/*!
 * \brief Function::Function
 * \param parent
 */
Function::Function(QObject *parent) : QObject(parent){

    this->supportsWeights = false;
}

/*!
 * \brief Function::~Function
 */
Function::~Function(){

}

/*!
 * \brief Function::init
 */
void Function::init(){

}

/*!
 * \brief Function::execChecked
 * D16/domain-model.md S5: the pipeline choke point. Every Function
 * subclass funnels through here (Feature::recalc() calls exactly this
 * method), so this is where a NeededElement's declared pipeline is
 * applied - generically, with zero changes needed in any fit function's
 * own Producer<T>/Transform<T>::exec(), because they already read
 * InputElement::shouldBeUsed (Stage 7c-ii/7f groundwork).
 *
 * Two passes, not a single Stream walk, because one stage type
 * (eResidualOutlierRejectionStage) needs a completed fit's residuals as
 * its own input - it cannot be evaluated before a fit exists. Pass 1
 * applies the deterministic stages (ExplicitList, Exclude) and runs the
 * real fit; pass 2, only if the residual stage actually excluded
 * something, reruns the fit once more with the reduced set. This is
 * exactly two passes, not iterative reweighting to convergence - see
 * domain-model.md S9 for why that is an accepted, documented limitation
 * rather than an oversight.
 *
 * Stage 7d/D21: this is the std::expected boundary Feature::recalc() uses
 * directly - see feature.h/feature.cpp. exec() (below) is a thin
 * bool-returning wrapper kept for every pre-7d caller.
 * \param feature
 * \return
 */
std::expected<void, QString> Function::execChecked(const QPointer<FeatureWrapper> &feature){

    this->applyDeterministicPipelineStages();

    std::expected<void, QString> result = this->execByFeatureType(feature);

    if(result.has_value() && this->applyResidualPipelineStages()){
        result = this->execByFeatureType(feature);
    }

    return result;

}

/*!
 * \brief Function::exec
 * Thin bool-returning wrapper over execChecked(), kept unchanged in
 * signature for every caller that predates Stage 7d (tests calling a
 * function directly, cfvisitor.cpp, OiJob) - see execChecked() above.
 * \param feature
 * \return
 */
bool Function::exec(const QPointer<FeatureWrapper> &feature){
    return this->execChecked(feature).has_value();
}

/*!
 * \brief Function::execByFeatureType
 * Stage 7d/D21 (domain-model.md S7): the producer/transform dispatch
 * boundary. A caller only ever holds a type-erased FeatureWrapper, so the
 * runtime switch on feature type is unavoidable - what changed is what
 * happens inside each case: dynamic_cast to Producer<ConcreteType>-star or
 * Transform<ConcreteType>-star (execAs<T>(), top of this file) instead of
 * calling a retired per-type virtual. A failed cast on both mixins *is*
 * the "this function doesn't belong in this slot" rejection.
 * \param feature
 * \return
 */
std::expected<void, QString> Function::execByFeatureType(const QPointer<FeatureWrapper> &feature){

    if(feature.isNull()){
        return std::unexpected(QString("no feature to execute against"));
    }

    switch(feature->getFeatureTypeEnum()){
    case eStationFeature:
        if(!feature->getStation().isNull()){ return execAs<Station>(this, feature->getStation().data(), "station"); }
        break;
    case eTrafoParamFeature:
        if(!feature->getTrafoParam().isNull()){ return execAs<TrafoParam>(this, feature->getTrafoParam().data(), "transformation parameter"); }
        break;
    case eCoordinateSystemFeature:
        if(!feature->getCoordinateSystem().isNull()){ return execAs<CoordinateSystem>(this, feature->getCoordinateSystem().data(), "coordinate system"); }
        break;
    case eCircleFeature:
        if(!feature->getCircle().isNull()){ return execAs<Circle>(this, feature->getCircle().data(), "circle"); }
        break;
    case eConeFeature:
        if(!feature->getCone().isNull()){ return execAs<Cone>(this, feature->getCone().data(), "cone"); }
        break;
    case eCylinderFeature:
        if(!feature->getCylinder().isNull()){ return execAs<Cylinder>(this, feature->getCylinder().data(), "cylinder"); }
        break;
    case eEllipseFeature:
        if(!feature->getEllipse().isNull()){ return execAs<Ellipse>(this, feature->getEllipse().data(), "ellipse"); }
        break;
    case eEllipsoidFeature:
        if(!feature->getEllipsoid().isNull()){ return execAs<Ellipsoid>(this, feature->getEllipsoid().data(), "ellipsoid"); }
        break;
    case eHyperboloidFeature:
        if(!feature->getHyperboloid().isNull()){ return execAs<Hyperboloid>(this, feature->getHyperboloid().data(), "hyperboloid"); }
        break;
    case eLineFeature:
        if(!feature->getLine().isNull()){ return execAs<Line>(this, feature->getLine().data(), "line"); }
        break;
    case eNurbsFeature:
        if(!feature->getNurbs().isNull()){ return execAs<Nurbs>(this, feature->getNurbs().data(), "nurbs"); }
        break;
    case eParaboloidFeature:
        if(!feature->getParaboloid().isNull()){ return execAs<Paraboloid>(this, feature->getParaboloid().data(), "paraboloid"); }
        break;
    case ePlaneFeature:
        if(!feature->getPlane().isNull()){ return execAs<Plane>(this, feature->getPlane().data(), "plane"); }
        break;
    case ePointFeature:
        if(!feature->getPoint().isNull()){ return execAs<Point>(this, feature->getPoint().data(), "point"); }
        break;
    case ePointCloudFeature:
        if(!feature->getPointCloud().isNull()){ return execAs<PointCloud>(this, feature->getPointCloud().data(), "point cloud"); }
        break;
    case eScalarEntityAngleFeature:
        if(!feature->getScalarEntityAngle().isNull()){ return execAs<ScalarEntityAngle>(this, feature->getScalarEntityAngle().data(), "angle"); }
        break;
    case eScalarEntityDistanceFeature:
        if(!feature->getScalarEntityDistance().isNull()){ return execAs<ScalarEntityDistance>(this, feature->getScalarEntityDistance().data(), "distance"); }
        break;
    case eScalarEntityMeasurementSeriesFeature:
        if(!feature->getScalarEntityMeasurementSeries().isNull()){ return execAs<ScalarEntityMeasurementSeries>(this, feature->getScalarEntityMeasurementSeries().data(), "measurement series"); }
        break;
    case eScalarEntityTemperatureFeature:
        if(!feature->getScalarEntityTemperature().isNull()){ return execAs<ScalarEntityTemperature>(this, feature->getScalarEntityTemperature().data(), "temperature"); }
        break;
    case eSlottedHoleFeature:
        if(!feature->getSlottedHole().isNull()){ return execAs<SlottedHole>(this, feature->getSlottedHole().data(), "slotted hole"); }
        break;
    case eSphereFeature:
        if(!feature->getSphere().isNull()){ return execAs<Sphere>(this, feature->getSphere().data(), "sphere"); }
        break;
    case eTorusFeature:
        if(!feature->getTorus().isNull()){ return execAs<Torus>(this, feature->getTorus().data(), "torus"); }
        break;
    default:
        break;
    }

    return std::unexpected(QString("Function \"%1\" not implemented for this feature type").arg(this->getMetaData().name));

}

/*!
 * \brief Function::canActAsProducer
 * Stage 7d/D21: attach-time capability check - see function.h.
 * \param type
 * \param function
 * \return
 */
bool Function::canActAsProducer(const FeatureTypes &type, Function *function){

    if(function == nullptr){
        return false;
    }

    //SpecialFunction (e.g. ConfiguredFunction, openindy-default-plugin's
    //cf/) manages its own execution by overriding the top-level
    //Function::exec()/execChecked() directly (specialfunction.h: "handles
    //special function e.g. configured functions") - it never goes through
    //execByFeatureType's Producer<T>/Transform<T> dispatch, so the mixin
    //check does not apply. Exempt here rather than in execByFeatureType,
    //since a SpecialFunction's own exec() override means it never reaches
    //execAs<T>() in the first place regardless of this check.
    if(dynamic_cast<SpecialFunction*>(function) != nullptr){
        return true;
    }

    switch(type){
    case eStationFeature: return implementsProducer<Station>(function);
    case eTrafoParamFeature: return implementsProducer<TrafoParam>(function);
    case eCoordinateSystemFeature: return implementsProducer<CoordinateSystem>(function);
    case eCircleFeature: return implementsProducer<Circle>(function);
    case eConeFeature: return implementsProducer<Cone>(function);
    case eCylinderFeature: return implementsProducer<Cylinder>(function);
    case eEllipseFeature: return implementsProducer<Ellipse>(function);
    case eEllipsoidFeature: return implementsProducer<Ellipsoid>(function);
    case eHyperboloidFeature: return implementsProducer<Hyperboloid>(function);
    case eLineFeature: return implementsProducer<Line>(function);
    case eNurbsFeature: return implementsProducer<Nurbs>(function);
    case eParaboloidFeature: return implementsProducer<Paraboloid>(function);
    case ePlaneFeature: return implementsProducer<Plane>(function);
    case ePointFeature: return implementsProducer<Point>(function);
    case ePointCloudFeature: return implementsProducer<PointCloud>(function);
    case eScalarEntityAngleFeature: return implementsProducer<ScalarEntityAngle>(function);
    case eScalarEntityDistanceFeature: return implementsProducer<ScalarEntityDistance>(function);
    case eScalarEntityMeasurementSeriesFeature: return implementsProducer<ScalarEntityMeasurementSeries>(function);
    case eScalarEntityTemperatureFeature: return implementsProducer<ScalarEntityTemperature>(function);
    case eSlottedHoleFeature: return implementsProducer<SlottedHole>(function);
    case eSphereFeature: return implementsProducer<Sphere>(function);
    case eTorusFeature: return implementsProducer<Torus>(function);
    default: return false;
    }

}

/*!
 * \brief Function::canActAsTransform
 * Stage 7d/D21: attach-time capability check - see function.h.
 * \param type
 * \param function
 * \return
 */
bool Function::canActAsTransform(const FeatureTypes &type, Function *function){

    if(function == nullptr){
        return false;
    }

    //see canActAsProducer() above for why SpecialFunction is exempt.
    if(dynamic_cast<SpecialFunction*>(function) != nullptr){
        return true;
    }

    switch(type){
    case eStationFeature: return implementsTransform<Station>(function);
    case eTrafoParamFeature: return implementsTransform<TrafoParam>(function);
    case eCoordinateSystemFeature: return implementsTransform<CoordinateSystem>(function);
    case eCircleFeature: return implementsTransform<Circle>(function);
    case eConeFeature: return implementsTransform<Cone>(function);
    case eCylinderFeature: return implementsTransform<Cylinder>(function);
    case eEllipseFeature: return implementsTransform<Ellipse>(function);
    case eEllipsoidFeature: return implementsTransform<Ellipsoid>(function);
    case eHyperboloidFeature: return implementsTransform<Hyperboloid>(function);
    case eLineFeature: return implementsTransform<Line>(function);
    case eNurbsFeature: return implementsTransform<Nurbs>(function);
    case eParaboloidFeature: return implementsTransform<Paraboloid>(function);
    case ePlaneFeature: return implementsTransform<Plane>(function);
    case ePointFeature: return implementsTransform<Point>(function);
    case ePointCloudFeature: return implementsTransform<PointCloud>(function);
    case eScalarEntityAngleFeature: return implementsTransform<ScalarEntityAngle>(function);
    case eScalarEntityDistanceFeature: return implementsTransform<ScalarEntityDistance>(function);
    case eScalarEntityMeasurementSeriesFeature: return implementsTransform<ScalarEntityMeasurementSeries>(function);
    case eScalarEntityTemperatureFeature: return implementsTransform<ScalarEntityTemperature>(function);
    case eSlottedHoleFeature: return implementsTransform<SlottedHole>(function);
    case eSphereFeature: return implementsTransform<Sphere>(function);
    case eTorusFeature: return implementsTransform<Torus>(function);
    default: return false;
    }

}

/*!
 * \brief Function::getNeededElements
 * \return
 */
const QList<NeededElement> &Function::getNeededElements() const{
    return this->neededElements;
}

/*!
 * \brief Function::getApplicableFor
 * \return
 */
const QList<FeatureTypes> &Function::getApplicableFor() const{
    return this->applicableFor;
}

/*!
 * \brief Function::getMetaData
 * \return
 */
const PluginMetaData &Function::getMetaData() const{
    return this->metaData;
}

/*!
 * \brief Function::getIntegerParameter
 * \return
 */
const QMap<QString, int> &Function::getIntegerParameter() const{
    return this->integerParameters;
}

/*!
 * \brief Function::getDoubleParameter
 * \return
 */
const QMap<QString, double> &Function::getDoubleParameter() const{
    return this->doubleParameters;
}

/*!
 * \brief Function::getStringParameter
 * \return
 */
const QMultiMap<QString, QString> &Function::getStringParameter() const{
    return this->stringParameters;
}

/*!
 * \brief Function::setScalarInputParams
 * \param params
 */
void Function::setScalarInputParams(const ScalarInputParams &params){
    this->scalarInputParams = params;
    this->scalarInputParams.isValid = true;
    emit this->scalarInputParametersChanged();
}

/*!
 * \brief Function::getScalarInputParams
 * \return
 */
const ScalarInputParams &Function::getScalarInputParams(){

    //use default values if no scalar input params have been specified
    if(!this->scalarInputParams.isValid){
        this->scalarInputParams.doubleParameter = this->doubleParameters;
        this->scalarInputParams.intParameter = this->integerParameters;
        QStringList keys = this->stringParameters.keys();
        foreach(const QString &key, keys){
            this->scalarInputParams.stringParameter.insert(key, this->stringParameters.value(key));
        }
    }

    return this->scalarInputParams;

}

/*!
 * \brief Function::getFixedParameters
 * \return
 */
const QList<FixedParameter> &Function::getFixedParameters() const{
    return this->fixedParameters;
}

/*!
 * \brief Function::fixParameter
 * \param parameter
 */
void Function::fixParameter(const FixedParameter &parameter){
    if(!this->fixedParameters.contains(parameter)){
        this->fixedParameters.append(parameter);
    }
}

/*!
 * \brief Function::unfixParameter
 * \param parameter
 */
void Function::unfixParameter(const GeometryParameters &parameter){
    this->fixedParameters.removeOne(FixedParameter(parameter));
}

/*!
 * \brief Function::unfixAllParameters
 */
void Function::unfixAllParameters(){
    this->fixedParameters.clear();
}

/*!
 * \brief Function::getResultProtocol
 * \return
 */
const QStringList &Function::getResultProtocol() const{
    return this->resultProtocol;
}

/*!
 * \brief Function::getStatistic
 * \return
 */
const Statistic &Function::getStatistic() const{
    return this->statistic;
}

/*!
 * \brief Function::getId
 * \return
 */
const int &Function::getId() const{
    return this->id;
}

/*!
 * \brief Function::getSupportWeights
 * \return
 */
const bool &Function::getSupportWeights()
{
    return this->supportsWeights;
}

/*!
 * \brief Function::setSupportWeights
 * \return
 */
void Function::setSupportWeights(bool supports)
{
    this->supportsWeights = supports;
}

/*!
 * \brief Function::getInputElements
 * \return
 */
const QMap<int, QList<InputElement> > &Function::getInputElements() const{
    return this->inputElements;
}

/*!
 * \brief Function::getInputElement
 * Returns the InputElement with the specified id or an empty InputElement
 * \param id
 * \return
 */
InputElement Function::getInputElement(const int &id) const{

    QList<int> keys = this->inputElements.keys();
    foreach(const int &key, keys){
        foreach(const InputElement &element, this->inputElements[key]){
            if(element.id == id){
                return element;
            }
        }
    }
    return InputElement();

}

/*!
 * \brief Function::getInputElement
 * Returns the InputElement with the specified id at the specified position
 * \param id
 * \param position
 * \return
 */
InputElement Function::getInputElement(const int &id, const int &position) const{

    if(this->inputElements.contains(position)){
        foreach(const InputElement &element, this->inputElements[position]){
            if(element.id == id){
                return element;
            }
        }
    }
    return InputElement();

}

/*!
 * \brief Function::hasInputElement
 * Check wether the function contains the specified InputElement
 * \param id
 * \return
 */
bool Function::hasInputElement(const int &id) const{

    QList<int> keys = this->inputElements.keys();
    foreach(const int &key, keys){
        foreach(const InputElement &element, this->inputElements[key]){
            if(element.id == id){
                return true;
            }
        }
    }
    return false;

}

/*!
 * \brief Function::addInputElement
 * \param element
 * \param position
 */
void Function::addInputElement(const InputElement &element, const int &position){

    //add the input element
    if(this->inputElements.contains(position)){
        this->inputElements[position].append(element);
    }else{
        QList<InputElement> elements;
        elements.append(element);
        this->inputElements.insert(position, elements);
    }

    emit this->inputElementsChanged();

}

/*!
 * \brief Function::removeInputElement
 * \param id
 * \param position
 */
void Function::removeInputElement(const int &id, const int &position){
    if(this->inputElements.contains(position)){
        this->inputElements[position].removeOne(InputElement(id));
        emit this->inputElementsChanged();
    }
}

/*!
 * \brief Function::removeInputElement
 * \param id
 */
void Function::removeInputElement(const int &id){
    for(int i = 0; i < this->inputElements.size(); i++){
        this->inputElements[i].removeOne(InputElement(id));
        emit this->inputElementsChanged();
    }
}

/*!
 * \brief Function::replaceInputElement
 * \param element
 * \param position
 */
void Function::replaceInputElement(const InputElement &element, const int &position){
    if(this->inputElements.contains(position)){
        int index = this->inputElements[position].indexOf(element);
        if(index > -1){
            this->inputElements[position].replace(index, element);
            emit this->inputElementsChanged();
        }
    }
}

/*!
 * \brief Function::getIsUsed
 * \param position
 * \param id
 * \return
 */
bool Function::getIsUsed(const int &position, const int &id){

    if(this->inputElements.contains(position)){
        int index = this->inputElements[position].indexOf(id);
        if(index > -1){
            return this->inputElements[position][index].isUsed;
        }
    }

    return false;

}

/*!
 * \brief Function::getShouldBeUsed
 * \param position
 * \param id
 * \return
 */
bool Function::getShouldBeUsed(const int &position, const int &id){

    if(this->inputElements.contains(position)){
        int index = this->inputElements[position].indexOf(id);
        if(index > -1){
            return this->inputElements[position][index].shouldBeUsed;
        }
    }

    return false;

}

/*!
 * \brief Function::setShouldBeUsed
 * \param position
 * \param id
 * \param state
 */
void Function::setShouldBeUsed(const int &position, const int &id, const bool &state){

    //get the element at position with id and set shouldBeUsed to state
    if(this->inputElements.contains(position)){
        int index = this->inputElements[position].indexOf(id);
        if(index > -1){
            this->inputElements[position][index].shouldBeUsed = state;
            emit this->inputElementsChanged();
        }
    }

}

/*!
 * \brief Function::getPipeline
 * \param position
 * \return
 */
const QList<PipelineStage> &Function::getPipeline(const int &position) const{
    static const QList<PipelineStage> empty;
    if(position < 0 || position >= this->neededElements.size()){
        return empty;
    }
    return this->neededElements.at(position).pipeline;
}

/*!
 * \brief Function::setPipeline
 * \param position
 * \param stages
 */
void Function::setPipeline(const int &position, const QList<PipelineStage> &stages){
    if(position < 0 || position >= this->neededElements.size()){
        return;
    }
    this->neededElements[position].pipeline = stages;
    emit this->inputElementsChanged();
}

/*!
 * \brief Function::addPipelineStage
 * \param position
 * \param stage
 */
void Function::addPipelineStage(const int &position, const PipelineStage &stage){
    if(position < 0 || position >= this->neededElements.size()){
        return;
    }
    this->neededElements[position].pipeline.append(stage);
    emit this->inputElementsChanged();
}

/*!
 * \brief Function::clearPipeline
 * \param position
 */
void Function::clearPipeline(const int &position){
    if(position < 0 || position >= this->neededElements.size()){
        return;
    }
    this->neededElements[position].pipeline.clear();
    emit this->inputElementsChanged();
}

/*!
 * \brief Function::getPipelineExclusions
 * \param position
 * \return
 */
const QList<PipelineExclusion> &Function::getPipelineExclusions(const int &position) const{
    static const QList<PipelineExclusion> empty;
    QMap<int, QList<PipelineExclusion> >::const_iterator it = this->pipelineExclusions.constFind(position);
    if(it == this->pipelineExclusions.constEnd()){
        return empty;
    }
    return it.value();
}

/*!
 * \brief Function::applyDeterministicPipelineStages
 * Walks every NeededElement position that has a non-empty declared
 * pipeline and applies its ExplicitList/Exclude stages, in declared
 * order, to the tag-resolved candidate set already sitting in
 * inputElements[position]. A position with an empty pipeline is left
 * completely untouched - a pipeline is opt-in per role
 * (function.h:NeededElement::pipeline).
 *
 * eResidualOutlierRejectionStage is a deliberate no-op here (the "yields
 * incoming unchanged" Stream case): it needs a completed fit, which does
 * not exist yet on the first pass. applyResidualPipelineStages() handles
 * it after execByFeatureType() runs once.
 * \return whether any position had a pipeline declared at all
 */
bool Function::applyDeterministicPipelineStages(){

    bool anyPipelineDeclared = false;

    for(int position = 0; position < this->neededElements.size(); position++){

        const QList<PipelineStage> &stages = this->neededElements.at(position).pipeline;
        if(stages.isEmpty()){
            continue;
        }
        anyPipelineDeclared = true;

        if(!this->inputElements.contains(position)){
            continue;
        }

        QList<int> survivors;
        foreach(const InputElement &element, this->inputElements.value(position)){
            survivors.append(element.id);
        }

        QList<PipelineExclusion> exclusions;

        foreach(const PipelineStage &stage, stages){

            switch(stage.type){

            case eExplicitListStage:{
                QList<int> next;
                foreach(int id, survivors){
                    if(stage.explicitIds.contains(id)){
                        next.append(id);
                    }else{
                        exclusions.append(PipelineExclusion(id, "not in the declared explicit list"));
                    }
                }
                survivors = next;
                break;
            }

            case eExcludeStage:{
                foreach(const PipelineExclusion &excl, stage.explicitExclusions){
                    if(survivors.removeOne(excl.elementId)){
                        exclusions.append(excl);
                    }
                }
                break;
            }

            case eResidualOutlierRejectionStage:
                //handled after a fit exists - see applyResidualPipelineStages()
                break;

            }

        }

        QList<InputElement> updated = this->inputElements.value(position);
        for(int i = 0; i < updated.size(); i++){
            updated[i].shouldBeUsed = survivors.contains(updated[i].id);
        }
        this->inputElements[position] = updated;
        this->pipelineExclusions[position] = exclusions;

    }

    return anyPipelineDeclared;

}

/*!
 * \brief Function::applyResidualPipelineStages
 * For every NeededElement position whose pipeline declares
 * eResidualOutlierRejectionStage, reads the *just-completed* fit's
 * per-element residual (via residualMagnitude(), sourced from the
 * addDisplayResidual() calls the fit itself already made) and excludes
 * any surviving candidate whose residual magnitude exceeds
 * sigmaMultiplier * this->statistic.getStdev(). Deterministic exclusions
 * from applyDeterministicPipelineStages() are kept, not recomputed.
 *
 * Degrades to a no-op (not a crash) whenever there is nothing usable to
 * compare against - no stdev yet, or a fit type that never reported a
 * residual under a recognised key (domain-model.md S9).
 * \return whether this changed any element's shouldBeUsed - the signal
 * that a second execByFeatureType() pass is warranted
 */
bool Function::applyResidualPipelineStages(){

    bool changed = false;

    for(int position = 0; position < this->neededElements.size(); position++){

        const QList<PipelineStage> &stages = this->neededElements.at(position).pipeline;

        bool hasResidualStage = false;
        double sigmaMultiplier = 3.0;
        foreach(const PipelineStage &stage, stages){
            if(stage.type == eResidualOutlierRejectionStage){
                hasResidualStage = true;
                sigmaMultiplier = stage.sigmaMultiplier;
                break;
            }
        }

        if(!hasResidualStage || !this->inputElements.contains(position)){
            continue;
        }

        const double stdev = this->statistic.getStdev();
        if(stdev <= 0.0){
            continue;
        }
        const double threshold = sigmaMultiplier * stdev;

        QList<InputElement> current = this->inputElements.value(position);
        QList<PipelineExclusion> exclusions = this->pipelineExclusions.value(position);
        bool changedHere = false;

        for(int i = 0; i < current.size(); i++){

            if(!current[i].shouldBeUsed){
                continue; //already excluded by a deterministic stage
            }

            const std::optional<double> residual = this->residualMagnitude(current[i].id);
            if(!residual.has_value()){
                continue; //this fit type does not report a comparable residual
            }

            if(qAbs(residual.value()) > threshold){
                current[i].shouldBeUsed = false;
                exclusions.append(PipelineExclusion(current[i].id,
                    QString("residual %1 exceeds %2 x sigma (stdev %3, threshold %4)")
                        .arg(residual.value()).arg(sigmaMultiplier).arg(stdev).arg(threshold)));
                changedHere = true;
            }

        }

        if(changedHere){
            this->inputElements[position] = current;
            this->pipelineExclusions[position] = exclusions;
            changed = true;
        }

    }

    return changed;

}

/*!
 * \brief Function::residualMagnitude
 * \param elementId
 * \return
 */
std::optional<double> Function::residualMagnitude(int elementId) const{

    const Residual residual = this->statistic.getDisplayResidual(elementId);
    if(residual.elementId != elementId){
        return std::nullopt;
    }

    const QString vKey = getObservationDisplayAttributesName(eObservationDisplayV);
    if(residual.corrections.contains(vKey)){
        return residual.corrections.value(vKey);
    }

    const QString vrKey = getObservationDisplayAttributesName(eObservationDisplayVR);
    if(residual.corrections.contains(vrKey)){
        return residual.corrections.value(vrKey);
    }

    return std::nullopt;

}

/*!
 * \brief Function::clear
 */
void Function::clear(){
    this->inputElements.clear();
    this->pipelineExclusions.clear();
    this->fixedParameters.clear();
    this->scalarInputParams.isValid = false;
    this->resultProtocol.clear();
    this->statistic.setIsValid(false);

    emit this->inputElementsChanged();
}

/*!
 * \brief Function::clearResults
 */
void Function::clearResults(){

    //clear statistic and protocol
    this->resultProtocol.clear();
    this->statistic.setIsValid(false);

    //set all elements to not have been used
    QList<int> keys = this->inputElements.keys();
    foreach(const int &key, keys){
        for(int i = 0; i < this->inputElements[key].size(); ++i){
            this->inputElements[key][i].isUsed = false;
        }
    }

}

/*!
 * \brief Function::toOpenIndyXML
 * \param xmlDoc
 * \return
 */
QDomElement Function::toOpenIndyXML(QDomDocument &xmlDoc) const{

    if(xmlDoc.isNull()){
        return QDomElement();
    }

    //add function attributes
    QDomElement function = xmlDoc.createElement("function");
    function.setAttribute("name", this->getMetaData().name);
    function.setAttribute("type", this->getMetaData().iid);
    function.setAttribute("plugin", this->getMetaData().pluginName);

    //add used elements
    QDomElement inputElements = xmlDoc.createElement("inputElements");
    QMapIterator<int, QList<InputElement> > inputIterator(this->getInputElements());
    while (inputIterator.hasNext()) {
        inputIterator.next();
        foreach(const InputElement &input, inputIterator.value()){
            QDomElement inputElement = xmlDoc.createElement("element");
            inputElement.setAttribute("index", inputIterator.key());
            inputElement.setAttribute("type", input.typeOfElement);
            inputElement.setAttribute("ref", input.id);
            inputElement.setAttribute("isUsed", input.isUsed);
            inputElement.setAttribute("shouldBeUsed", input.shouldBeUsed);

            //add ignored destination parameters of the input element
            QDomElement ignoredDestinationParams = xmlDoc.createElement("ignoredDestinationParams");

            for(int i=0; i< input.ignoredDestinationParams.size();i++){
                //add all ignored parameters to the element
                QDomElement ignoredParam = xmlDoc.createElement("ignoredParam");
                ignoredParam.setAttribute("parameter", getGeometryParameterName(input.ignoredDestinationParams.at(i)));
                ignoredDestinationParams.appendChild(ignoredParam);
            }
            inputElement.appendChild(ignoredDestinationParams);

            inputElements.appendChild(inputElement);
        }
    }
    function.appendChild(inputElements);

    //D16/domain-model.md S5: persist each NeededElement position's
    //declared pipeline - the source of truth, not pipelineExclusions
    //(which is re-derived from this plus current data on every exec(),
    //same as isUsed). Only ids and scalars, no object references, so
    //this round-trips through Function::fromOpenIndyXML alone - no
    //ProjectExchanger ID-resolution pass needed the way inputElements'
    //object pointers require.
    QDomElement pipelines = xmlDoc.createElement("pipelines");
    for(int position = 0; position < this->neededElements.size(); position++){
        const QList<PipelineStage> &stages = this->neededElements.at(position).pipeline;
        if(stages.isEmpty()){
            continue;
        }
        QDomElement pipelineElem = xmlDoc.createElement("pipeline");
        pipelineElem.setAttribute("position", position);
        foreach(const PipelineStage &stage, stages){
            QDomElement stageElem = stage.toOpenIndyXML(xmlDoc);
            if(!stageElem.isNull()){
                pipelineElem.appendChild(stageElem);
            }
        }
        pipelines.appendChild(pipelineElem);
    }
    function.appendChild(pipelines);

    ScalarInputParams params;
    if(!this->scalarInputParams.isValid){
        params.doubleParameter = this->doubleParameters;
        params.intParameter = this->integerParameters;
        QStringList keys = this->stringParameters.keys();
        foreach(const QString &key, keys){
            params.stringParameter.insert(key, this->stringParameters.value(key));
        }
    }else{
        params = this->scalarInputParams;
    }

    //add integer parameters
    QDomElement integerParams = xmlDoc.createElement("integerParameters");
    QMapIterator<QString, int> intIterator(params.intParameter);
    while (intIterator.hasNext()) {
        intIterator.next();
        QDomElement intParam = xmlDoc.createElement("parameter");
        intParam.setAttribute("name", intIterator.key());
        intParam.setAttribute("value", intIterator.value());
        integerParams.appendChild(intParam);
    }
    function.appendChild(integerParams);

    //add double parameters
    QDomElement doubleParams = xmlDoc.createElement("doubleParameters");
    QMapIterator<QString, double> doubleIterator(params.doubleParameter);
    while (doubleIterator.hasNext()) {
        doubleIterator.next();
        QDomElement doubleParam = xmlDoc.createElement("parameter");
        doubleParam.setAttribute("name", doubleIterator.key());
        doubleParam.setAttribute("value", doubleIterator.value());
        doubleParams.appendChild(doubleParam);
    }
    function.appendChild(doubleParams);

    //add string parameters
    QDomElement stringParams = xmlDoc.createElement("stringParameters");
    QMapIterator<QString, QString> stringIterator(params.stringParameter);
    while (stringIterator.hasNext()) {
        stringIterator.next();
        QDomElement stringParam = xmlDoc.createElement("parameter");
        stringParam.setAttribute("name", stringIterator.key());
        stringParam.setAttribute("value", stringIterator.value());
        stringParams.appendChild(stringParam);
    }
    function.appendChild(stringParams);

    return function;

}

/*!
 * \brief Function::fromOpenIndyXML
 * \param xmlElem
 * \return
 */
bool Function::fromOpenIndyXML(QDomElement &xmlElem){

    if(xmlElem.isNull()){
        return false;
    }

    if(!xmlElem.hasAttribute("name") || !xmlElem.hasAttribute("plugin") || !xmlElem.hasAttribute("executionIndex")
            || !xmlElem.hasAttribute("type")){
        return false;
    }

    //set integer parameter
    QDomElement integerParameters = xmlElem.firstChildElement("integerParameters");
    if(!integerParameters.isNull()){
        QDomNodeList paramList = integerParameters.childNodes();
        for(int i = 0; i < paramList.size(); i++){
            QDomElement integerParameter = paramList.at(i).toElement();
            if(integerParameter.tagName().compare("parameter") == 0 && integerParameter.hasAttribute("value")
                    && integerParameter.hasAttribute("name")){
                this->scalarInputParams.intParameter.insert(integerParameter.attribute("name"), integerParameter.attribute("value").toInt());
            }
        }
    }

    //set double parameter
    QDomElement doubleParameters = xmlElem.firstChildElement("doubleParameters");
    if(!doubleParameters.isNull()){
        QDomNodeList paramList = doubleParameters.childNodes();
        for(int i = 0; i < paramList.size(); i++){
            QDomElement doubleParameter = paramList.at(i).toElement();
            if(doubleParameter.tagName().compare("parameter") == 0 && doubleParameter.hasAttribute("value")
                    && doubleParameter.hasAttribute("name")){
                this->scalarInputParams.doubleParameter.insert(doubleParameter.attribute("name"), doubleParameter.attribute("value").toDouble());
            }
        }
    }

    //set string parameter
    QDomElement stringParameters = xmlElem.firstChildElement("stringParameters");
    if(!stringParameters.isNull()){
        QDomNodeList paramList = stringParameters.childNodes();
        for(int i = 0; i < paramList.size(); i++){
            QDomElement stringParameter = paramList.at(i).toElement();
            if(stringParameter.tagName().compare("parameter") == 0 && stringParameter.hasAttribute("value")
                    && stringParameter.hasAttribute("name")){
                this->scalarInputParams.stringParameter.insert(stringParameter.attribute("name"), stringParameter.attribute("value"));
            }
        }
    }

    this->scalarInputParams.isValid = true;

    //D16/domain-model.md S5: restore each position's declared pipeline.
    //Must run after init() has already populated this->neededElements
    //(PluginLoader::loadFunctionPlugin constructs and inits the function
    //before ProjectExchanger calls fromOpenIndyXML on it) - this mutates
    //existing entries by position rather than rebuilding the list, so it
    //cannot race init()'s neededElements.clear().
    QDomElement pipelines = xmlElem.firstChildElement("pipelines");
    if(!pipelines.isNull()){
        QDomNodeList pipelineList = pipelines.elementsByTagName("pipeline");
        for(int i = 0; i < pipelineList.size(); i++){
            QDomElement pipelineElem = pipelineList.at(i).toElement();
            if(!pipelineElem.hasAttribute("position")){
                continue;
            }
            const int position = pipelineElem.attribute("position").toInt();
            if(position < 0 || position >= this->neededElements.size()){
                continue;
            }
            QList<PipelineStage> stages;
            QDomNodeList stageList = pipelineElem.elementsByTagName("stage");
            for(int j = 0; j < stageList.size(); j++){
                PipelineStage stage;
                if(stage.fromOpenIndyXML(stageList.at(j).toElement())){
                    stages.append(stage);
                }
            }
            this->neededElements[position].pipeline = stages;
        }
    }

    return true;

}

/*!
 * \brief Function::setIsUsed
 * \param position
 * \param id
 * \param state
 */
void Function::setIsUsed(const int &position, const int &id, const bool &state){

    //get the element at position with id and set isUsed to state
    if(this->inputElements.contains(position)){
        int index = this->inputElements[position].indexOf(id);
        if(index > -1){
            this->inputElements[position][index].isUsed = state;
        }
    }

}

void Function::filterObservations(QList<QPointer<Observation> > &allUsableObservations, QList<QPointer<Observation> > &inputObservations) {
    foreach(const InputElement &element, this->getInputElements()[0]){
        if(!element.observation.isNull()
                && element.observation->getIsSolved()
                && element.observation->getIsValid()) {
            allUsableObservations.append(element.observation);
            this->setIsUsed(0, element.id, element.shouldBeUsed);
            if(element.shouldBeUsed){
                inputObservations.append(element.observation);
            }
            continue;
        }
        this->setIsUsed(0, element.id, false);
    }
}

void Function::addDisplayResidual(int elementId, double vr) {
    Residual residual;
    residual.elementId = elementId;
    residual.dimension = eMetric;
    residual.corrections.insert(getObservationDisplayAttributesName(eObservationDisplayVR), vr);
    this->statistic.addDisplayResidual(residual);
}

void Function::addDisplayResidual(int elementId, double vx, double vy, double vz, double v) {
    Residual residual;
    residual.elementId = elementId;
    residual.dimension = eMetric;
    residual.corrections.insert(getObservationDisplayAttributesName(eObservationDisplayVX), vx);
    residual.corrections.insert(getObservationDisplayAttributesName(eObservationDisplayVY), vy);
    residual.corrections.insert(getObservationDisplayAttributesName(eObservationDisplayVZ), vz);
    residual.corrections.insert(getObservationDisplayAttributesName(eObservationDisplayV), v);
    this->statistic.addDisplayResidual(residual);
}

void Function::addDisplayResidual(int elementId, double vx, double vy, double vz, double v, double vi, double vj, double vk) {
    Residual residual;
    residual.elementId = elementId;
    residual.dimension = eMetric;
    residual.corrections.insert(getObservationDisplayAttributesName(eObservationDisplayVX), vx);
    residual.corrections.insert(getObservationDisplayAttributesName(eObservationDisplayVY), vy);
    residual.corrections.insert(getObservationDisplayAttributesName(eObservationDisplayVZ), vz);
    residual.corrections.insert(getObservationDisplayAttributesName(eObservationDisplayV), v);
    residual.corrections.insert(getObservationDisplayAttributesName(eObservationDisplayVI), vi);
    residual.corrections.insert(getObservationDisplayAttributesName(eObservationDisplayVJ), vj);
    residual.corrections.insert(getObservationDisplayAttributesName(eObservationDisplayVK), vk);
    this->statistic.addDisplayResidual(residual);
}
