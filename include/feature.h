#ifndef FEATURE_H
#define FEATURE_H
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <vector>

#include "core_export.h"
#include "element.h"
#include "feature_observer.h"
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

  [[nodiscard]] const std::string& getFeatureName() const;
  void setFeatureName(const std::string& name);

  [[nodiscard]] const std::string& getGroupName() const;
  void setGroupName(const std::string& group);

  [[nodiscard]] const std::string& getComment() const;
  void setComment(const std::string& comment);

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

  [[nodiscard]] std::vector<oi::Guarded<Function>> getFunctions() const;
  [[nodiscard]] const std::optional<oi::Guarded<Function>>&
  getProducerFunction() const;
  [[nodiscard]] const std::vector<oi::Guarded<Function>>&
  getTransformFunctions() const;
  // Takes the function; returns false (and frees it) when it fits neither slot.
  bool addFunction(std::unique_ptr<Function> function);
  // Removes and destroys the function at the getFunctions() index.
  void removeFunction(const int& index);

  [[nodiscard]] const bool& getIsActiveFeature() const;
  void setActiveFeatureState(const bool& isActiveFeature);

  [[nodiscard]] const std::vector<oi::Guarded<FeatureWrapper>>& getUsedFor()
      const;
  bool addUsedFor(const oi::Guarded<FeatureWrapper>& feature);
  bool removeUsedFor(const oi::Guarded<FeatureWrapper>& feature);

  [[nodiscard]] const std::vector<oi::Guarded<FeatureWrapper>>&
  getPreviouslyNeeded() const;
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

  [[nodiscard]] virtual std::optional<std::string> getOwnerDescription() const;

  // ###########################
  // reexecute the function list
  // ###########################

  virtual void recalc();

  // ###############
  // display methods
  // ###############

  // general attributes
  [[nodiscard]] virtual std::string getDisplayType() const;
  [[nodiscard]] virtual std::string getDisplayIsSolved() const;
  [[nodiscard]] virtual std::string getDisplayIsUpdated() const;
  [[nodiscard]] virtual std::string getDisplayFunctions() const;
  [[nodiscard]] virtual std::string getDisplayUsedFor() const;
  [[nodiscard]] virtual std::string getDisplayPreviouslyNeeded() const;
  [[nodiscard]] virtual std::string getDisplayStDev(const UnitType& type,
                                                    const int& digits) const;

  // geometry specific attributes
  [[nodiscard]] virtual std::string getDisplayMeasurementConfig() const;
  [[nodiscard]] virtual std::string getDisplayObservations() const;
  [[nodiscard]] virtual std::string getDisplayIsCommon() const;
  [[nodiscard]] virtual std::string getDisplayIsActual() const;

  // unknown geometry parameters
  [[nodiscard]] virtual std::string getDisplayX(
      const UnitType& type, const int& digits,
      const bool& showDiff = false) const;
  [[nodiscard]] virtual std::string getDisplayY(
      const UnitType& type, const int& digits,
      const bool& showDiff = false) const;
  [[nodiscard]] virtual std::string getDisplayZ(
      const UnitType& type, const int& digits,
      const bool& showDiff = false) const;
  [[nodiscard]] virtual std::string getDisplayPrimaryI(
      const int& digits, const bool& showDiff = false) const;
  [[nodiscard]] virtual std::string getDisplayPrimaryJ(
      const int& digits, const bool& showDiff = false) const;
  [[nodiscard]] virtual std::string getDisplayPrimaryK(
      const int& digits, const bool& showDiff = false) const;
  [[nodiscard]] virtual std::string getDisplayRadiusA(
      const UnitType& type, const int& digits,
      const bool& showDiff = false) const;
  [[nodiscard]] virtual std::string getDisplayRadiusB(
      const UnitType& type, const int& digits,
      const bool& showDiff = false) const;
  [[nodiscard]] virtual std::string getDisplaySecondaryI(
      const int& digits, const bool& showDiff = false) const;
  [[nodiscard]] virtual std::string getDisplaySecondaryJ(
      const int& digits, const bool& showDiff = false) const;
  [[nodiscard]] virtual std::string getDisplaySecondaryK(
      const int& digits, const bool& showDiff = false) const;
  [[nodiscard]] virtual std::string getDisplayAperture(
      const UnitType& type, const int& digits,
      const bool& showDiff = false) const;
  [[nodiscard]] virtual std::string getDisplayA(
      const UnitType& type, const int& digits,
      const bool& showDiff = false) const;
  [[nodiscard]] virtual std::string getDisplayB(
      const UnitType& type, const int& digits,
      const bool& showDiff = false) const;
  [[nodiscard]] virtual std::string getDisplayC(
      const UnitType& type, const int& digits,
      const bool& showDiff = false) const;
  [[nodiscard]] virtual std::string getDisplayAngle(
      const UnitType& type, const int& digits,
      const bool& showDiff = false) const;
  [[nodiscard]] virtual std::string getDisplayDistance(
      const UnitType& type, const int& digits,
      const bool& showDiff = false) const;
  [[nodiscard]] virtual std::string getDisplayMeasurementSeries(
      const UnitType& type, const int& digits,
      const bool& showDiff = false) const;
  [[nodiscard]] virtual std::string getDisplayTemperature(
      const UnitType& type, const int& digits,
      const bool& showDiff = false) const;
  [[nodiscard]] virtual std::string getDisplayLength(
      const UnitType& type, const int& digits,
      const bool& showDiff = false) const;
  [[nodiscard]] virtual std::string getDisplayFormError(
      const UnitType& type, const int& digits) const;

  // coordinate system specific
  [[nodiscard]] virtual std::string getDisplayExpansionOriginX(
      const UnitType& type, const int& digits) const;
  [[nodiscard]] virtual std::string getDisplayExpansionOriginY(
      const UnitType& type, const int& digits) const;
  [[nodiscard]] virtual std::string getDisplayExpansionOriginZ(
      const UnitType& type, const int& digits) const;
  [[nodiscard]] virtual std::string getDisplayXAxisI(const int& digits) const;
  [[nodiscard]] virtual std::string getDisplayXAxisJ(const int& digits) const;
  [[nodiscard]] virtual std::string getDisplayXAxisK(const int& digits) const;
  [[nodiscard]] virtual std::string getDisplayYAxisI(const int& digits) const;
  [[nodiscard]] virtual std::string getDisplayYAxisJ(const int& digits) const;
  [[nodiscard]] virtual std::string getDisplayYAxisK(const int& digits) const;
  [[nodiscard]] virtual std::string getDisplayZAxisI(const int& digits) const;
  [[nodiscard]] virtual std::string getDisplayZAxisJ(const int& digits) const;
  [[nodiscard]] virtual std::string getDisplayZAxisK(const int& digits) const;

  // trafo param specific
  [[nodiscard]] virtual std::string getDisplayStartSystem() const;
  [[nodiscard]] virtual std::string getDisplayDestinationSystem() const;
  [[nodiscard]] virtual std::string getDisplayTranslationX(
      const UnitType& type, const int& digits) const;
  [[nodiscard]] virtual std::string getDisplayTranslationY(
      const UnitType& type, const int& digits) const;
  [[nodiscard]] virtual std::string getDisplayTranslationZ(
      const UnitType& type, const int& digits) const;
  [[nodiscard]] virtual std::string getDisplayRotationX(
      const UnitType& type, const int& digits) const;
  [[nodiscard]] virtual std::string getDisplayRotationY(
      const UnitType& type, const int& digits) const;
  [[nodiscard]] virtual std::string getDisplayRotationZ(
      const UnitType& type, const int& digits) const;
  [[nodiscard]] virtual std::string getDisplayScaleX(const int& digits) const;
  [[nodiscard]] virtual std::string getDisplayScaleY(const int& digits) const;
  [[nodiscard]] virtual std::string getDisplayScaleZ(const int& digits) const;
  [[nodiscard]] virtual std::string getDisplayIsUsed() const;
  [[nodiscard]] virtual std::string getDisplayValidTime() const;
  [[nodiscard]] virtual std::string getDisplayIsMovement() const;
  [[nodiscard]] virtual std::string getDisplayIsDatumTransformation() const;

 public:
  // ###################################################################
  // change notifications: a plain, non-owning observer (see
  // feature_observer.h); null (nobody is told) until a job attaches itself
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
  virtual void nameWasChanged(const std::string& /*oldName*/) {}

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

  [[nodiscard]] bool rejectIfNotEditable(
      const std::string& attemptedEdit) const;

  // ######################################
  // update feature id when the job was set
  // ######################################

  virtual void setUpFeatureId();

  // ##################
  // feature attributes
  // ##################

  FeatureObserver* observer{nullptr};  // not owned, never copied
  bool notificationsBlocked{false};    // see blockSignals

  std::string name;
  std::string group;
  std::string comment;

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
  // owners; producerFunction/transformFunctions are the non-owning views
  std::unique_ptr<Function> ownedProducer;
  std::vector<std::unique_ptr<Function>> ownedTransforms;
  std::optional<oi::Guarded<Function>> producerFunction;
  std::vector<oi::Guarded<Function>> transformFunctions;

  std::vector<oi::Guarded<FeatureWrapper>>
      usedForList;  // features which need this feature to recalc
  std::map<int, oi::Guarded<FeatureWrapper>> usedForMap;
  std::vector<oi::Guarded<FeatureWrapper>>
      previouslyNeededList;  // features which are needed to recalc this feature
  std::map<int, oi::Guarded<FeatureWrapper>> previouslyNeededMap;

  // ######################################
  // this pointer wrapped by FeatureWrapper
  // ######################################

  // The feature's one FeatureWrapper. The feature owns it, so whoever owns
  // the feature (FeatureContainer, through a std::unique_ptr<Feature>) owns
  // the whole lifetime; selfFeature is the non-owning handle everything else
  // is given (getFeatureWrapper()).
  std::unique_ptr<FeatureWrapper> ownedWrapper;
  oi::Guarded<FeatureWrapper> selfFeature;

  // #############################################
  // pointer to OiJob that this feature belongs to
  // #############################################

  virtual void setJob(const oi::Guarded<OiJob>& job);

  oi::Guarded<OiJob> job;
};

}  // namespace oi

#endif  // FEATURE_H
