#ifndef FEATURE_H
#define FEATURE_H

#include <QList>
#include <QString>
#include <QtCore>
#include <optional>
#include <type_traits>

#include "core_export.h"
#include "element.h"
#include "featureobserver.h"
#include "guarded.h"

namespace oi {

class FeatureWrapper;
class Function;

/*!
 * \brief The Feature class
 */
class OI_CORE_EXPORT Feature : public Element {
  friend class OiJob;
  friend struct codec::Access;  // the protobuf codec (see src/codec)

 public:
  explicit Feature();

  Feature(const Feature& copy);

  Feature& operator=(const Feature& copy);

  ~Feature() override;

  // ############################################
  // get the feature wrapped by a feature wrapper
  // ############################################

  [[nodiscard]] const oi::Guarded<FeatureWrapper>& getFeatureWrapper() const;

  // #############################
  // get or set feature attributes
  // #############################

  [[nodiscard]] const QString& getFeatureName() const;
  void setFeatureName(const QString& name);

  [[nodiscard]] const QString& getGroupName() const;
  void setGroupName(const QString& group);

  [[nodiscard]] const QString& getComment() const;
  void setComment(const QString& comment);

  [[nodiscard]] const bool& getIsUpdated() const;
  void setIsUpdated(const bool& isUpdated);

  [[nodiscard]] const bool& getIsSolved() const;
  void setIsSolved(const bool& isSolved);

  // ###############################################################
  // Stage 7d/D21 (domain-model.md S7): producer and transform are
  // structurally distinct containers, not a validated role on one list.
  // getFunctions() is a computed view - producerFunction (if any)
  // followed by transformFunctions, in that order - kept for the many
  // existing callers that still index positionally (verified fact 1:
  // FeatureUpdater already treated position 0 as the producer by
  // convention; this makes it true by construction instead). Prefer
  // getProducerFunction()/getTransformFunctions() for new code that
  // cares about the distinction.
  // ###############################################################

  [[nodiscard]] QList<oi::Guarded<Function>> getFunctions() const;
  [[nodiscard]] const std::optional<oi::Guarded<Function>>&
  getProducerFunction() const;
  [[nodiscard]] const QList<oi::Guarded<Function>>& getTransformFunctions()
      const;
  void addFunction(const oi::Guarded<Function>& function);
  void removeFunction(const int& index);

  [[nodiscard]] const bool& getIsActiveFeature() const;
  void setActiveFeatureState(const bool& isActiveFeature);

  [[nodiscard]] const QList<oi::Guarded<FeatureWrapper>>& getUsedFor() const;
  bool addUsedFor(const oi::Guarded<FeatureWrapper>& feature);
  bool removeUsedFor(const oi::Guarded<FeatureWrapper>& feature);

  [[nodiscard]] const QList<oi::Guarded<FeatureWrapper>>& getPreviouslyNeeded()
      const;
  bool addPreviouslyNeeded(const oi::Guarded<FeatureWrapper>& feature);
  bool removePreviouslyNeeded(const oi::Guarded<FeatureWrapper>& feature);

  // ###############################################################
  // ownership determines editability and origin (D19, domain-model
  // §1). Empty means the user owns this feature and may edit it
  // freely. A non-empty result names what owns it instead - a
  // producing fit, an owning station - and is why a user edit must be
  // rejected rather than merely hidden in the UI. The base
  // implementation is always user-owned; override per owner kind
  //(Geometry does this for the nominal/actual case, D17; a
  // station-owned Observation will when it becomes a Point).
  // ###############################################################

  [[nodiscard]] virtual std::optional<QString> getOwnerDescription() const;

  // ###########################
  // reexecute the function list
  // ###########################

  virtual void recalc();

  // ###############
  // display methods
  // ###############

  // general attributes
  [[nodiscard]] virtual QString getDisplayType() const;
  [[nodiscard]] virtual QString getDisplayIsSolved() const;
  [[nodiscard]] virtual QString getDisplayIsUpdated() const;
  [[nodiscard]] virtual QString getDisplayFunctions() const;
  [[nodiscard]] virtual QString getDisplayUsedFor() const;
  [[nodiscard]] virtual QString getDisplayPreviouslyNeeded() const;
  [[nodiscard]] virtual QString getDisplayStDev(const UnitType& type,
                                                const int& digits) const;

  // geometry specific attributes
  [[nodiscard]] virtual QString getDisplayMeasurementConfig() const;
  [[nodiscard]] virtual QString getDisplayObservations() const;
  [[nodiscard]] virtual QString getDisplayIsCommon() const;
  [[nodiscard]] virtual QString getDisplayIsActual() const;

  // unknown geometry parameters
  [[nodiscard]] virtual QString getDisplayX(const UnitType& type,
                                            const int& digits,
                                            const bool& showDiff = false) const;
  [[nodiscard]] virtual QString getDisplayY(const UnitType& type,
                                            const int& digits,
                                            const bool& showDiff = false) const;
  [[nodiscard]] virtual QString getDisplayZ(const UnitType& type,
                                            const int& digits,
                                            const bool& showDiff = false) const;
  [[nodiscard]] virtual QString getDisplayPrimaryI(
      const int& digits, const bool& showDiff = false) const;
  [[nodiscard]] virtual QString getDisplayPrimaryJ(
      const int& digits, const bool& showDiff = false) const;
  [[nodiscard]] virtual QString getDisplayPrimaryK(
      const int& digits, const bool& showDiff = false) const;
  [[nodiscard]] virtual QString getDisplayRadiusA(
      const UnitType& type, const int& digits,
      const bool& showDiff = false) const;
  [[nodiscard]] virtual QString getDisplayRadiusB(
      const UnitType& type, const int& digits,
      const bool& showDiff = false) const;
  [[nodiscard]] virtual QString getDisplaySecondaryI(
      const int& digits, const bool& showDiff = false) const;
  [[nodiscard]] virtual QString getDisplaySecondaryJ(
      const int& digits, const bool& showDiff = false) const;
  [[nodiscard]] virtual QString getDisplaySecondaryK(
      const int& digits, const bool& showDiff = false) const;
  [[nodiscard]] virtual QString getDisplayAperture(
      const UnitType& type, const int& digits,
      const bool& showDiff = false) const;
  [[nodiscard]] virtual QString getDisplayA(const UnitType& type,
                                            const int& digits,
                                            const bool& showDiff = false) const;
  [[nodiscard]] virtual QString getDisplayB(const UnitType& type,
                                            const int& digits,
                                            const bool& showDiff = false) const;
  [[nodiscard]] virtual QString getDisplayC(const UnitType& type,
                                            const int& digits,
                                            const bool& showDiff = false) const;
  [[nodiscard]] virtual QString getDisplayAngle(
      const UnitType& type, const int& digits,
      const bool& showDiff = false) const;
  [[nodiscard]] virtual QString getDisplayDistance(
      const UnitType& type, const int& digits,
      const bool& showDiff = false) const;
  [[nodiscard]] virtual QString getDisplayMeasurementSeries(
      const UnitType& type, const int& digits,
      const bool& showDiff = false) const;
  [[nodiscard]] virtual QString getDisplayTemperature(
      const UnitType& type, const int& digits,
      const bool& showDiff = false) const;
  [[nodiscard]] virtual QString getDisplayLength(
      const UnitType& type, const int& digits,
      const bool& showDiff = false) const;
  [[nodiscard]] virtual QString getDisplayFormError(const UnitType& type,
                                                    const int& digits) const;

  // coordinate system specific
  [[nodiscard]] virtual QString getDisplayExpansionOriginX(
      const UnitType& type, const int& digits) const;
  [[nodiscard]] virtual QString getDisplayExpansionOriginY(
      const UnitType& type, const int& digits) const;
  [[nodiscard]] virtual QString getDisplayExpansionOriginZ(
      const UnitType& type, const int& digits) const;
  [[nodiscard]] virtual QString getDisplayXAxisI(const int& digits) const;
  [[nodiscard]] virtual QString getDisplayXAxisJ(const int& digits) const;
  [[nodiscard]] virtual QString getDisplayXAxisK(const int& digits) const;
  [[nodiscard]] virtual QString getDisplayYAxisI(const int& digits) const;
  [[nodiscard]] virtual QString getDisplayYAxisJ(const int& digits) const;
  [[nodiscard]] virtual QString getDisplayYAxisK(const int& digits) const;
  [[nodiscard]] virtual QString getDisplayZAxisI(const int& digits) const;
  [[nodiscard]] virtual QString getDisplayZAxisJ(const int& digits) const;
  [[nodiscard]] virtual QString getDisplayZAxisK(const int& digits) const;

  // trafo param specific
  [[nodiscard]] virtual QString getDisplayStartSystem() const;
  [[nodiscard]] virtual QString getDisplayDestinationSystem() const;
  [[nodiscard]] virtual QString getDisplayTranslationX(const UnitType& type,
                                                       const int& digits) const;
  [[nodiscard]] virtual QString getDisplayTranslationY(const UnitType& type,
                                                       const int& digits) const;
  [[nodiscard]] virtual QString getDisplayTranslationZ(const UnitType& type,
                                                       const int& digits) const;
  [[nodiscard]] virtual QString getDisplayRotationX(const UnitType& type,
                                                    const int& digits) const;
  [[nodiscard]] virtual QString getDisplayRotationY(const UnitType& type,
                                                    const int& digits) const;
  [[nodiscard]] virtual QString getDisplayRotationZ(const UnitType& type,
                                                    const int& digits) const;
  [[nodiscard]] virtual QString getDisplayScaleX(const int& digits) const;
  [[nodiscard]] virtual QString getDisplayScaleY(const int& digits) const;
  [[nodiscard]] virtual QString getDisplayScaleZ(const int& digits) const;
  [[nodiscard]] virtual QString getDisplayIsUsed() const;
  [[nodiscard]] virtual QString getDisplayValidTime() const;
  [[nodiscard]] virtual QString getDisplayIsMovement() const;
  [[nodiscard]] virtual QString getDisplayIsDatumTransformation() const;

 public:
  // ###################################################################
  // change notifications: a plain, non-owning observer (see
  // featureobserver.h); null (nobody is told) until a job attaches itself
  // ###################################################################

  void setObserver(FeatureObserver* observer) { this->observer = observer; }
  [[nodiscard]] FeatureObserver* getObserver() const { return this->observer; }

  // suppress the notifications to the observer while true (returns the
  // previous state)
  bool blockSignals(bool block) {
    const bool previous = this->notificationsBlocked;
    this->notificationsBlocked = block;
    return previous;
  }
  [[nodiscard]] bool signalsBlocked() const {
    return this->notificationsBlocked;
  }

 protected:
  // called (unless signals are blocked) after the name changed, before the
  // observer is told - lets a subclass react to its own rename
  virtual void nameWasChanged(const QString& /*oldName*/) {}

  // tell the observer, if there is one (and notifications are not
  // blocked, see blockSignals)
  template <typename... Params>
  void notify(void (FeatureObserver::*method)(Params...),
              std::type_identity_t<Params>... args) {
    if (this->observer != nullptr && !this->signalsBlocked()) {
      (this->observer->*method)(args...);
    }
  }

  // ###############################################################
  // enforcement helper for D19 - call at the top of any setter that
  // represents a direct user edit (not a function/exec write, which
  // owns the right to write regardless). Returns true - and reports
  // the reason through OiJob::sendMessage, naming the owner - if the
  // edit must be rejected. This is the "reject at the API" mechanism;
  // without it ownership would be convention only.
  // ###############################################################

  [[nodiscard]] bool rejectIfNotEditable(const QString& attemptedEdit) const;

  // ######################################
  // update feature id when the job was set
  // ######################################

  virtual void setUpFeatureId();

  // ##################
  // feature attributes
  // ##################

  FeatureObserver* observer{nullptr};  // not owned, never copied
  bool notificationsBlocked{false};    // see blockSignals

  QString name;
  QString group;
  QString comment;

  bool isSolved{
      false};  // true of the feature is solved in the current disply system
  bool isUpdated{false};  // helper attribute to indicate if a feature was
                          // already recalced during recalculation process

  bool isActiveFeature{false};

  // Stage 7d/D21 (domain-model.md S7): at most one producer - the
  // function that defines this feature's value from external inputs (a
  // fit, a construct function, a nominal producer) - followed by any
  // number of transforms that refine that value in place, in order. Two
  // producers, or a transform where a producer belongs, is not a
  // validated-and-rejected input: it is a shape addFunction() will not
  // construct (see Function::canActAsProducer/canActAsTransform, the
  // runtime dynamic_cast check at the attach call site).
  std::optional<oi::Guarded<Function>> producerFunction;
  QList<oi::Guarded<Function>> transformFunctions;

  QList<oi::Guarded<FeatureWrapper>>
      usedForList;  // features which need this feature to recalc
  QMap<int, oi::Guarded<FeatureWrapper>> usedForMap;
  QList<oi::Guarded<FeatureWrapper>>
      previouslyNeededList;  // features which are needed to recalc this feature
  QMap<int, oi::Guarded<FeatureWrapper>> previouslyNeededMap;

  // ######################################
  // this pointer wrapped by FeatureWrapper
  // ######################################

  oi::Guarded<FeatureWrapper> selfFeature;

  // #############################################
  // pointer to OiJob that this feature belongs to
  // #############################################

  virtual void setJob(const oi::Guarded<OiJob>& job);

  oi::Guarded<OiJob> job;
};

}  // namespace oi

Q_DECLARE_METATYPE(oi::Feature*)

#endif  // FEATURE_H
