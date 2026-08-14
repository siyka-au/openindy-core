#ifndef FEATURE_H
#define FEATURE_H

#include <QObject>
#include <QPointer>
#include <QList>
#include <QString>
#include <QtXml>
#include <optional>

#include "element.h"

namespace oi{

class FeatureWrapper;
class Function;

/*!
 * \brief The Feature class
 */
class OI_CORE_EXPORT Feature : public Element
{
    friend class OiJob;
    Q_OBJECT

public:
    explicit Feature(QObject *parent = 0);

    Feature(const Feature &copy, QObject *parent = 0);

    Feature &operator=(const Feature &copy);

    virtual ~Feature();

    //############################################
    //get the feature wrapped by a feature wrapper
    //############################################

    const QPointer<FeatureWrapper> &getFeatureWrapper() const;

    //#############################
    //get or set feature attributes
    //#############################

    const QString &getFeatureName() const;
    void setFeatureName(const QString &name);

    const QString &getGroupName() const;
    void setGroupName(const QString &group);

    const QString &getComment() const;
    void setComment(const QString &comment);

    const bool &getIsUpdated() const;
    void setIsUpdated(const bool &isUpdated);

    const bool &getIsSolved() const;
    void setIsSolved(const bool &isSolved);

    //###############################################################
    //Stage 7d/D21 (domain-model.md S7): producer and transform are
    //structurally distinct containers, not a validated role on one list.
    //getFunctions() is a computed view - producerFunction (if any)
    //followed by transformFunctions, in that order - kept for the many
    //existing callers that still index positionally (verified fact 1:
    //FeatureUpdater already treated position 0 as the producer by
    //convention; this makes it true by construction instead). Prefer
    //getProducerFunction()/getTransformFunctions() for new code that
    //cares about the distinction.
    //###############################################################

    QList<QPointer<Function> > getFunctions() const;
    const std::optional<QPointer<Function> > &getProducerFunction() const;
    const QList<QPointer<Function> > &getTransformFunctions() const;
    void addFunction(const QPointer<Function> &function);
    void removeFunction(const int &index);

    const bool &getIsActiveFeature() const;
    void setActiveFeatureState(const bool &isActiveFeature);

    const QList<QPointer<FeatureWrapper> > &getUsedFor() const;
    bool addUsedFor(const QPointer<FeatureWrapper> &feature);
    bool removeUsedFor(const QPointer<FeatureWrapper> &feature);

    const QList<QPointer<FeatureWrapper> > &getPreviouslyNeeded() const;
    bool addPreviouslyNeeded(const QPointer<FeatureWrapper> &feature);
    bool removePreviouslyNeeded(const QPointer<FeatureWrapper> &feature);

    //###############################################################
    //ownership determines editability and origin (D19, domain-model
    //§1). Empty means the user owns this feature and may edit it
    //freely. A non-empty result names what owns it instead - a
    //producing fit, an owning station - and is why a user edit must be
    //rejected rather than merely hidden in the UI. The base
    //implementation is always user-owned; override per owner kind
    //(Geometry does this for the nominal/actual case, D17; a
    //station-owned Observation will when it becomes a Point).
    //###############################################################

    virtual std::optional<QString> getOwnerDescription() const;

    //###########################
    //reexecute the function list
    //###########################

    virtual void recalc();

    //#################
    //save and load XML
    //#################

    virtual QDomElement toOpenIndyXML(QDomDocument &xmlDoc) const;
    virtual bool fromOpenIndyXML(QDomElement &xmlElem);

    //###############
    //display methods
    //###############

    //general attributes
    virtual QString getDisplayType() const;
    virtual QString getDisplayIsSolved() const;
    virtual QString getDisplayIsUpdated() const;
    virtual QString getDisplayFunctions() const;
    virtual QString getDisplayUsedFor() const;
    virtual QString getDisplayPreviouslyNeeded() const;
    virtual QString getDisplayStDev(const UnitType &type, const int &digits) const;

    //geometry specific attributes
    virtual QString getDisplayMeasurementConfig() const;
    virtual QString getDisplayObservations() const;
    virtual QString getDisplayIsCommon() const;
    virtual QString getDisplayIsActual() const;

    //unknown geometry parameters
    virtual QString getDisplayX(const UnitType &type, const int &digits, const bool &showDiff = false) const;
    virtual QString getDisplayY(const UnitType &type, const int &digits, const bool &showDiff = false) const;
    virtual QString getDisplayZ(const UnitType &type, const int &digits, const bool &showDiff = false) const;
    virtual QString getDisplayPrimaryI(const int &digits, const bool &showDiff = false) const;
    virtual QString getDisplayPrimaryJ(const int &digits, const bool &showDiff = false) const;
    virtual QString getDisplayPrimaryK(const int &digits, const bool &showDiff = false) const;
    virtual QString getDisplayRadiusA(const UnitType &type, const int &digits, const bool &showDiff = false) const;
    virtual QString getDisplayRadiusB(const UnitType &type, const int &digits, const bool &showDiff = false) const;
    virtual QString getDisplaySecondaryI(const int &digits, const bool &showDiff = false) const;
    virtual QString getDisplaySecondaryJ(const int &digits, const bool &showDiff = false) const;
    virtual QString getDisplaySecondaryK(const int &digits, const bool &showDiff = false) const;
    virtual QString getDisplayAperture(const UnitType &type, const int &digits, const bool &showDiff = false) const;
    virtual QString getDisplayA(const UnitType &type, const int &digits, const bool &showDiff = false) const;
    virtual QString getDisplayB(const UnitType &type, const int &digits, const bool &showDiff = false) const;
    virtual QString getDisplayC(const UnitType &type, const int &digits, const bool &showDiff = false) const;
    virtual QString getDisplayAngle(const UnitType &type, const int &digits, const bool &showDiff = false) const;
    virtual QString getDisplayDistance(const UnitType &type, const int &digits, const bool &showDiff = false) const;
    virtual QString getDisplayMeasurementSeries(const UnitType &type, const int &digits, const bool &showDiff = false) const;
    virtual QString getDisplayTemperature(const UnitType &type, const int &digits, const bool &showDiff = false) const;
    virtual QString getDisplayLength(const UnitType &type, const int &digits, const bool &showDiff = false) const;
    virtual QString getDisplayFormError(const UnitType &type, const int &digits) const;

    //coordinate system specific
    virtual QString getDisplayExpansionOriginX(const UnitType &type, const int &digits) const;
    virtual QString getDisplayExpansionOriginY(const UnitType &type, const int &digits) const;
    virtual QString getDisplayExpansionOriginZ(const UnitType &type, const int &digits) const;
    virtual QString getDisplayXAxisI(const int &digits) const;
    virtual QString getDisplayXAxisJ(const int &digits) const;
    virtual QString getDisplayXAxisK(const int &digits) const;
    virtual QString getDisplayYAxisI(const int &digits) const;
    virtual QString getDisplayYAxisJ(const int &digits) const;
    virtual QString getDisplayYAxisK(const int &digits) const;
    virtual QString getDisplayZAxisI(const int &digits) const;
    virtual QString getDisplayZAxisJ(const int &digits) const;
    virtual QString getDisplayZAxisK(const int &digits) const;

    //trafo param specific
    virtual QString getDisplayStartSystem() const;
    virtual QString getDisplayDestinationSystem() const;
    virtual QString getDisplayTranslationX(const UnitType &type, const int &digits) const;
    virtual QString getDisplayTranslationY(const UnitType &type, const int &digits) const;
    virtual QString getDisplayTranslationZ(const UnitType &type, const int &digits) const;
    virtual QString getDisplayRotationX(const UnitType &type, const int &digits) const;
    virtual QString getDisplayRotationY(const UnitType &type, const int &digits) const;
    virtual QString getDisplayRotationZ(const UnitType &type, const int &digits) const;
    virtual QString getDisplayScaleX(const int &digits) const;
    virtual QString getDisplayScaleY(const int &digits) const;
    virtual QString getDisplayScaleZ(const int &digits) const;
    virtual QString getDisplayIsUsed() const;
    virtual QString getDisplayValidTime() const;
    virtual QString getDisplayIsMovement() const;
    virtual QString getDisplayIsDatumTransformation() const;

signals:

    //#######################################
    //signals to inform about feature changes
    //#######################################

    void featureNameChanged(const int &featureId, const QString &oldName);
    void featureGroupChanged(const int &featureId, const QString &oldGroup);
    void featureCommentChanged(const int &featureId);
    void featureIsUpdatedChanged(const int &featureId);
    void featureIsSolvedChanged(const int &featureId);
    void featureFunctionListChanged(const int &featureId);
    void featureUsedForChanged(const int &featureId);
    void featurePreviouslyNeededChanged(const int &featureId);
    void featureIsActiveChanged(const int &featureId);

protected:

    //###############################################################
    //enforcement helper for D19 - call at the top of any setter that
    //represents a direct user edit (not a function/exec write, which
    //owns the right to write regardless). Returns true - and reports
    //the reason through OiJob::sendMessage, naming the owner - if the
    //edit must be rejected. This is the "reject at the API" mechanism;
    //without it ownership would be convention only.
    //###############################################################

    bool rejectIfNotEditable(const QString &attemptedEdit) const;

    //######################################
    //update feature id when the job was set
    //######################################

    virtual void setUpFeatureId();

    //##################
    //feature attributes
    //##################

    QString name;
    QString group;
    QString comment;

    bool isSolved; //true of the feature is solved in the current disply system
    bool isUpdated; //helper attribute to indicate if a feature was already recalced during recalculation process

    bool isActiveFeature;

    //Stage 7d/D21 (domain-model.md S7): at most one producer - the
    //function that defines this feature's value from external inputs (a
    //fit, a construct function, a nominal producer) - followed by any
    //number of transforms that refine that value in place, in order. Two
    //producers, or a transform where a producer belongs, is not a
    //validated-and-rejected input: it is a shape addFunction() will not
    //construct (see Function::canActAsProducer/canActAsTransform, the
    //runtime dynamic_cast check at the attach call site).
    std::optional<QPointer<Function> > producerFunction;
    QList<QPointer<Function> > transformFunctions;

    QList< QPointer<FeatureWrapper> > usedForList; //features which need this feature to recalc
    QMap<int, QPointer<FeatureWrapper> > usedForMap;
    QList< QPointer<FeatureWrapper> > previouslyNeededList; //features which are needed to recalc this feature
    QMap<int, QPointer<FeatureWrapper> > previouslyNeededMap;

    //######################################
    //this pointer wrapped by FeatureWrapper
    //######################################

    QPointer<FeatureWrapper> selfFeature;

    //#############################################
    //pointer to OiJob that this feature belongs to
    //#############################################

    virtual void setJob(const QPointer<OiJob> &job);

    QPointer<OiJob> job;

};

}

Q_DECLARE_METATYPE( oi::Feature* )

#endif // FEATURE_H
