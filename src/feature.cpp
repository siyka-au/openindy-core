#include "feature.h"

#include <expected>
#include <format>
#include <iterator>
#include <stdexcept>
#include <vector>

#include "containers.h"
#include "featurewrapper.h"
#include "function.h"
#include "oijob.h"
#include "snapshot.h"

using namespace oi;

/*!
 * \brief Feature::Feature
 */
Feature::Feature()
    : Element(),
      ownedWrapper(std::make_unique<FeatureWrapper>()),
      selfFeature(ownedWrapper.get()) {}

/*!
 * \brief Feature::Feature
 * \param copy
 */
Feature::Feature(const Feature& copy)
    : Element(copy),
      comment(copy.comment),
      group(copy.group),
      isActiveFeature(copy.isActiveFeature),
      isSolved(copy.isSolved),
      isUpdated(copy.isUpdated),
      name(copy.name),
      ownedWrapper(std::make_unique<FeatureWrapper>()),
      selfFeature(ownedWrapper.get()) {
  // copy attributes

  // copy functions (usedFor is not copied)
  // this->functionList = copy.functionList;
  // this->previouslyNeeded = copy.previouslyNeeded;
}

/*!
 * \brief Feature::operator =
 * \param copy
 * \return
 */
Feature& Feature::operator=(const Feature& copy) {
  // copy attributes
  this->name = copy.name;
  this->comment = copy.comment;
  this->group = copy.group;
  this->isSolved = copy.isSolved;
  this->isActiveFeature = copy.isActiveFeature;
  this->isUpdated = copy.isUpdated;

  // copy functions (usedFor is not copied)
  // this->functionList = copy.functionList;
  // this->previouslyNeeded = copy.previouslyNeeded;

  // keep the existing selfFeature wrapper: assignment must not change the
  // object's identity, and replacing it here leaked the old wrapper

  return *this;
}

/*!
 * \brief Feature::~Feature
 */
Feature::~Feature() {
  // check and update features which need this feature to recalc
  if (this->usedForList.size() > 0 && !this->selfFeature.isNull()) {
    for (const oi::Guarded<FeatureWrapper>& feature :
         oi::snapshot(this->usedForList)) {
      // check feature
      if (feature.isNull() || feature->getFeature().isNull()) {
        continue;
      }

      // delete the feature from all functions
      for (const oi::Guarded<Function> function :
           oi::snapshot(feature->getFeature()->getFunctions())) {
        if (!function.isNull()) {
          function->removeInputElement(this->id);
        }
      }

      // remove this feature from previously needed list
      oi::removeOne(feature->getFeature()->previouslyNeededList,
                    this->selfFeature);
    }
  }

  // check and update features which were needed to recalc this feature
  if (this->previouslyNeededList.size() > 0) {
    for (const oi::Guarded<FeatureWrapper>& feature :
         oi::snapshot(this->previouslyNeededList)) {
      // remove this feature from usedFor list
      if (!this->selfFeature.isNull() && !feature.isNull() &&
          !feature->getFeature().isNull()) {
        oi::removeOne(feature->getFeature()->usedForList, this->selfFeature);
      }
    }
  }

  // delete all functions when deleting the feature
  if (this->producerFunction.has_value() && !this->producerFunction->isNull()) {
    delete this->producerFunction->data();
  }
  for (const oi::Guarded<Function>& function :
       oi::snapshot(this->transformFunctions)) {
    if (!function.isNull()) {
      delete function.data();
    }
  }

  // the observer is told once, then forgotten: nothing may reach it after
  // this object is gone
  this->notify(&FeatureObserver::onElementAboutToBeDeleted, this->id,
               this->name, this->group,
               this->selfFeature->getFeatureTypeEnum());
  this->observer = nullptr;

  // ownedWrapper (the self wrapper) is destroyed after this body
}

/*!
 * \brief Feature::getFeatureWrapper
 * \return
 */
const oi::Guarded<FeatureWrapper>& Feature::getFeatureWrapper() const {
  return this->selfFeature;
}

/*!
 * \brief Feature::getOwnerDescription
 * D19: empty means user-owned (editable). Base case is always
 * user-owned; see Geometry::getOwnerDescription for the fit-owned case.
 * \return
 */
std::optional<std::string> Feature::getOwnerDescription() const {
  return std::nullopt;
}

/*!
 * \brief Feature::rejectIfNotEditable
 * Enforces D19 at the API. Call at the top of a setter that represents
 * a direct user edit; internal writes performed by a function's own
 * exec() are not user edits and must not call this.
 * \param attemptedEdit human-readable name of what was being edited, for the
 * message
 * \return true if the edit was rejected
 */
bool Feature::rejectIfNotEditable(const std::string& attemptedEdit) const {
  const std::optional<std::string> owner = this->getOwnerDescription();
  if (!owner.has_value()) {
    return false;
  }

  if (!this->job.isNull()) {
    this->job->sendMessage(
        std::format(
            "Cannot edit {} of \"{}\": it is owned by {} and is read-only.",
            attemptedEdit, this->name, *owner),
        MessageTypes::eErrorMessage, MessageDestinations::eMessageBoxMessage);
  }

  return true;
}

/*!
 * \brief Feature::getFeatureName
 * \return
 */
const std::string& Feature::getFeatureName() const { return this->name; }

/*!
 * \brief Feature::setFeatureName
 * \param name
 */
void Feature::setFeatureName(const std::string& name) {
  // check if old name = new name
  if (this->name.compare(name) == 0) {
    return;
  }

  // validate the new name before setting it
  if (!this->job.isNull() && !this->selfFeature.isNull()) {
    if (!this->job->validateFeatureName(
            name, this->selfFeature->getFeatureTypeEnum())) {
      return;
    }
  }

  // set the new name
  std::string oldName = this->name;
  this->name = name;
  if (!this->signalsBlocked()) {
    this->nameWasChanged(oldName);
  }
  this->notify(&FeatureObserver::onFeatureNameChanged, this->id, oldName);
}

/*!
 * \brief Feature::getGroupName
 * \return
 */
const std::string& Feature::getGroupName() const { return this->group; }

/*!
 * \brief Feature::setGroupName
 * \param group
 */
void Feature::setGroupName(const std::string& group) {
  if (group.compare(this->group) != 0) {
    std::string oldGroup = this->group;
    this->group = group;
    this->notify(&FeatureObserver::onFeatureGroupChanged, this->id, oldGroup);
  }
}

/*!
 * \brief Feature::getComment
 * \return
 */
const std::string& Feature::getComment() const { return this->comment; }

/*!
 * \brief Feature::setComment
 * \param comment
 */
void Feature::setComment(const std::string& comment) {
  if (comment.compare(this->comment) != 0) {
    this->comment = comment;
    this->notify(&FeatureObserver::onFeatureCommentChanged, this->id);
  }
}

/*!
 * \brief Feature::getIsUpdated
 * \return
 */
const bool& Feature::getIsUpdated() const { return this->isUpdated; }

/*!
 * \brief Feature::setIsUpdated
 * \param isUpdated
 */
void Feature::setIsUpdated(const bool& isUpdated) {
  if (isUpdated != this->isUpdated) {
    this->isUpdated = isUpdated;
    // this->notify(&FeatureObserver::onFeatureIsUpdatedChanged, this->id);
  }
}

/*!
 * \brief Feature::getIsSolved
 * \return
 */
const bool& Feature::getIsSolved() const { return this->isSolved; }

/*!
 * \brief Feature::setIsSolved
 * \param isSolved
 */
void Feature::setIsSolved(const bool& isSolved) {
  if (isSolved != this->isSolved) {
    this->isSolved = isSolved;
    this->notify(&FeatureObserver::onFeatureIsSolvedChanged, this->id);
  }
}

/*!
 * \brief Feature::getFunctions
 * Computed view - see the member comment on producerFunction/
 * transformFunctions (feature.h) and the class-level comment on the
 * accessor declarations for why this stays a vector rather than exposing
 * the split directly to every caller.
 * \return
 */
std::vector<oi::Guarded<Function> > Feature::getFunctions() const {
  std::vector<oi::Guarded<Function> > functions;
  if (this->producerFunction.has_value() && !this->producerFunction->isNull()) {
    functions.push_back(this->producerFunction.value());
  }
  oi::appendAll(functions, this->transformFunctions);
  return functions;
}

/*!
 * \brief Feature::getProducerFunction
 * \return
 */
const std::optional<oi::Guarded<Function> >& Feature::getProducerFunction()
    const {
  return this->producerFunction;
}

/*!
 * \brief Feature::getTransformFunctions
 * \return
 */
const std::vector<oi::Guarded<Function> >& Feature::getTransformFunctions()
    const {
  return this->transformFunctions;
}

/*!
 * \brief Feature::addFunction
 * Stage 7d/D21 (domain-model.md S7): the attach-time enforcement point.
 * Which slot a function goes into is decided by what it actually
 * implements, not by attach order: a function only takes the producer
 * slot if it implements Producer<T> for this feature's type *and* the
 * slot is still empty; otherwise, if it implements Transform<T>, it is
 * appended as a transform regardless of whether a producer exists yet
 * (a station-owned Point's value can come from measurement directly, with
 * no producer Function ever attached - a Transform must still be
 * attachable on top of that). A function implementing neither mixin for
 * this feature type - or a second Producer once the slot is filled - is
 * rejected outright rather than silently accepted into a container shape
 * that cannot represent it.
 *
 * Runtime, not compile-time: the real attach paths (the UI's function
 * picker, ProjectExchanger restoring a saved project) only ever hold a
 * dynamically-loaded Function* with no compile-time geometry type, so a
 * template-constrained attach API could not be used at those call sites.
 * See domain-model.md S7 "Dispatch boundary" and D21's left-open call.
 * \param function
 */
void Feature::addFunction(const oi::Guarded<Function>& function) {
  if (function.isNull() || this->selfFeature.isNull()) {
    return;
  }

  const FeatureTypes type = this->selfFeature->getFeatureTypeEnum();
  const bool producerCapable =
      Function::canActAsProducer(type, function.data());
  const bool transformCapable =
      Function::canActAsTransform(type, function.data());

  bool attachAsProducer = false;

  if (producerCapable && !this->producerFunction.has_value()) {
    attachAsProducer = true;
  } else if (transformCapable) {
    attachAsProducer = false;
  } else {
    const std::string reason =
        (producerCapable && this->producerFunction.has_value())
            ? std::format(
                  "Function \"{}\" cannot be attached to \"{}\": a producer is "
                  "already set ({})",
                  function->getMetaData().name, this->getFeatureName(),
                  this->producerFunction.value()->getMetaData().name)
            : std::format(
                  "Function \"{}\" cannot be attached to \"{}\": it implements "
                  "neither Producer<T> nor Transform<T> for this feature type",
                  function->getMetaData().name, this->getFeatureName());
    function->sendMessage(reason, MessageTypes::eCriticalMessage,
                          MessageDestinations::eMessageBoxMessage);
    return;
  }

  // let the current job generate a unique id if it is valid
  if (!this->job.isNull()) {
    function->id = this->job->generateUniqueId();
  }

  if (attachAsProducer) {
    this->producerFunction = function;
  } else {
    this->transformFunctions.push_back(function);
  }

  this->isUpdated = false;
  this->notify(&FeatureObserver::onFeatureFunctionListChanged, this->id);
}

/*!
 * \brief Feature::removeFunction
 * Index space matches getFunctions(): 0 is the producer (if one is set),
 * everything after is transformFunctions in order.
 * \param index
 */
void Feature::removeFunction(const int& index) {
  if (index < 0) {
    return;
  }

  if (this->producerFunction.has_value()) {
    if (index == 0) {
      this->producerFunction = std::nullopt;
      this->isUpdated = false;
      this->notify(&FeatureObserver::onFeatureFunctionListChanged, this->id);
      return;
    }
    const int transformIndex = index - 1;
    if (transformIndex < std::ssize(this->transformFunctions)) {
      oi::removeAt(this->transformFunctions, transformIndex);
      this->isUpdated = false;
      this->notify(&FeatureObserver::onFeatureFunctionListChanged, this->id);
    }
    return;
  }

  if (index < std::ssize(this->transformFunctions)) {
    oi::removeAt(this->transformFunctions, index);
    this->isUpdated = false;
    this->notify(&FeatureObserver::onFeatureFunctionListChanged, this->id);
  }
}

/*!
 * \brief Feature::getIsActiveFeature
 * \return
 */
const bool& Feature::getIsActiveFeature() const {
  return this->isActiveFeature;
}

/*!
 * \brief Feature::setActiveFeatureState
 * \param isActiveFeature
 */
void Feature::setActiveFeatureState(const bool& isActiveFeature) {
  if (isActiveFeature != this->isActiveFeature) {
    this->isActiveFeature = isActiveFeature;
    this->notify(&FeatureObserver::onFeatureIsActiveChanged, this->id);
  }
}

/*!
 * \brief Feature::getUsedFor
 * \return
 */
const std::vector<oi::Guarded<FeatureWrapper> >& Feature::getUsedFor() const {
  return this->usedForList;
}

/*!
 * \brief Feature::addUsedFor
 * \param feature
 * \return
 */
bool Feature::addUsedFor(const oi::Guarded<FeatureWrapper>& feature) {
  // check feature
  if (feature.isNull() || feature->getFeature().isNull()) {
    return false;
  }

  // check if the feature is already included
  if (this->usedForMap.contains(feature->getFeature()->getId())) {
    return false;
  }

  // check if feature is in same job
  if (!this->job.isNull()) {
    oi::Guarded<FeatureWrapper> jobFeature =
        this->job->getFeatureById(feature->getFeature()->getId());
    if (jobFeature.isNull() || jobFeature->getFeature().isNull() ||
        feature->getFeature() != jobFeature->getFeature()) {
      return false;
    }
  }

  this->usedForList.push_back(feature);
  this->usedForMap.insert_or_assign(feature->getFeature()->getId(), feature);

  // add this feature to the previously needed features of feature
  if (!this->selfFeature.isNull()) {
    feature->getFeature()->addPreviouslyNeeded(this->selfFeature);
  }

  return true;
}

/*!
 * \brief Feature::removeUsedFor
 * \param feature
 * \return
 */
bool Feature::removeUsedFor(const oi::Guarded<FeatureWrapper>& feature) {
  // check feature
  if (feature.isNull() || feature->getFeature().isNull()) {
    return false;
  }

  // check if the feature is included
  if (!this->usedForMap.contains(feature->getFeature()->getId())) {
    return false;
  }

  // check if feature is in same job
  if (!this->job.isNull()) {
    oi::Guarded<FeatureWrapper> jobFeature =
        this->job->getFeatureById(feature->getFeature()->getId());
    if (jobFeature.isNull() || jobFeature->getFeature().isNull() ||
        feature->getFeature() != jobFeature->getFeature()) {
      return false;
    }
  }

  oi::removeOne(this->usedForList, feature);
  this->usedForMap.erase(feature->getFeature()->getId());

  // remove this feature from the previously needed features of feature
  if (!this->selfFeature.isNull()) {
    feature->getFeature()->removePreviouslyNeeded(this->selfFeature);
  }

  return true;
}

/*!
 * \brief Feature::getPreviouslyNeeded
 * \return
 */
const std::vector<oi::Guarded<FeatureWrapper> >& Feature::getPreviouslyNeeded()
    const {
  return this->previouslyNeededList;
}

/*!
 * \brief Feature::addPreviouslyNeeded
 * \param feature
 * \return
 */
bool Feature::addPreviouslyNeeded(const oi::Guarded<FeatureWrapper>& feature) {
  // check feature
  if (feature.isNull() || feature->getFeature().isNull()) {
    return false;
  }

  // check if the feature is already included
  if (this->previouslyNeededMap.contains(feature->getFeature()->getId())) {
    return false;
  }

  // check if feature is in same job
  if (!this->job.isNull()) {
    oi::Guarded<FeatureWrapper> jobFeature =
        this->job->getFeatureById(feature->getFeature()->getId());
    if (jobFeature.isNull() || jobFeature->getFeature().isNull() ||
        feature->getFeature() != jobFeature->getFeature()) {
      return false;
    }
  }

  this->previouslyNeededList.push_back(feature);
  this->previouslyNeededMap.insert_or_assign(feature->getFeature()->getId(),
                                             feature);

  this->isUpdated = false;

  // add this feature to the used for features of feature
  if (!this->selfFeature.isNull()) {
    feature->getFeature()->addUsedFor(this->selfFeature);
  }

  return true;
}

/*!
 * \brief Feature::removePreviouslyNeeded
 * \param feature
 * \return
 */
bool Feature::removePreviouslyNeeded(
    const oi::Guarded<FeatureWrapper>& feature) {
  // check feature
  if (feature.isNull() || feature->getFeature().isNull()) {
    return false;
  }

  // check if the feature is included
  if (!this->previouslyNeededMap.contains(feature->getFeature()->getId())) {
    return false;
  }

  // check if feature is in same job
  if (!this->job.isNull()) {
    oi::Guarded<FeatureWrapper> jobFeature =
        this->job->getFeatureById(feature->getFeature()->getId());
    if (jobFeature.isNull() || jobFeature->getFeature().isNull() ||
        feature->getFeature() != jobFeature->getFeature()) {
      return false;
    }
  }

  oi::removeOne(this->previouslyNeededList, feature);
  this->previouslyNeededMap.erase(feature->getFeature()->getId());

  this->isUpdated = false;

  // remove this feature from the used for features of feature
  if (!this->selfFeature.isNull()) {
    feature->getFeature()->removeUsedFor(this->selfFeature);
  }

  return true;
}

/*!
 * \brief Feature::recalc
 * Stage 7d/D21 (domain-model.md S7): run the producer once to obtain (or
 * refresh) this feature's value, then run each transform on the result in
 * order - std::expected<void, std::string> propagating a reason on failure
 * instead of the old "iterate one list, break on first bool false". No
 * bool-return survives at this boundary: both producerFunction and every
 * transformFunctions entry are run through Function::execChecked(), never
 * the bool-returning Function::exec() kept for pre-existing callers.
 *
 * A feature with no producer at all (functionList was empty pre-7d) stays
 * unsolved, matching the old behaviour exactly: the loop below never ran,
 * so isSolved stayed false from the top of the method.
 */
void Feature::recalc() {
  this->isSolved = false;

  try {
    if (!this->producerFunction.has_value() ||
        this->producerFunction->isNull()) {
      return;
    }

    std::expected<void, std::string> result =
        this->producerFunction.value()->execChecked(this->selfFeature);

    if (result.has_value()) {
      for (const oi::Guarded<Function>& transform :
           oi::snapshot(this->transformFunctions)) {
        if (transform.isNull()) {
          result = std::unexpected(
              std::format("a transform function on \"{}\" is no longer valid",
                          this->getFeatureName()));
          break;
        }
        result = transform->execChecked(this->selfFeature);
        if (!result.has_value()) {
          break;
        }
      }
    }

    this->isSolved = result.has_value();

  } catch (const std::exception& e) {
    throw std::runtime_error(
        std::format("can not calculate feature: \"{}\", error: {}",
                    this->getFeatureName(), e.what()));
  } catch (...) {
    throw std::runtime_error(std::format("can not calculate feature: \"{}\"",
                                         this->getFeatureName()));
  }
}

/*!
 * \brief Feature::getDisplayType
 * \return
 */
std::string Feature::getDisplayType() const {
  if (!this->selfFeature.isNull()) {
    return this->selfFeature->getFeatureTypeString();
  }
  return std::string("-/-");
}

/*!
 * \brief Feature::getDisplayIsSolved
 * \return
 */
std::string Feature::getDisplayIsSolved() const {
  return this->isSolved ? "true" : "false";
}

/*!
 * \brief Feature::getDisplayIsUpdated
 * \return
 */
std::string Feature::getDisplayIsUpdated() const {
  return this->isUpdated ? "true" : "false";
}

/*!
 * \brief Feature::getDisplayFunctions
 * \return
 */
std::string Feature::getDisplayFunctions() const {
  std::string display = "";
  const std::vector<oi::Guarded<Function> > functions = this->getFunctions();
  for (int i = 0; i < std::ssize(functions); i++) {
    oi::Guarded<Function> function = functions.at(i);
    if (!function.isNull()) {
      display += function->getMetaData().name +
                 (i < std::ssize(functions) - 1 ? ", " : "");
    }
  }
  return display;
}

/*!
 * \brief Feature::getDisplayUsedFor
 * \return
 */
std::string Feature::getDisplayUsedFor() const {
  std::string display = "";
  for (int i = 0; i < std::ssize(this->usedForList); i++) {
    oi::Guarded<FeatureWrapper> feature = this->usedForList.at(i);
    if (!feature.isNull() && !feature->getFeature().isNull()) {
      display += feature->getFeature()->getFeatureName() +
                 (i < std::ssize(this->usedForList) - 1 ? ", " : "");
    }
  }
  return display;
}

/*!
 * \brief Feature::getDisplayPreviouslyNeeded
 * \return
 */
std::string Feature::getDisplayPreviouslyNeeded() const {
  std::string display = "";
  for (int i = 0; i < std::ssize(this->previouslyNeededList); i++) {
    oi::Guarded<FeatureWrapper> feature = this->previouslyNeededList.at(i);
    if (!feature.isNull() && !feature->getFeature().isNull()) {
      display += feature->getFeature()->getFeatureName() +
                 (i < std::ssize(this->previouslyNeededList) - 1 ? ", " : "");
    }
  }
  return display;
}

/*!
 * \brief Feature::getDisplayStDev
 * \param type
 * \param digits
 * \return
 */
std::string Feature::getDisplayStDev(const UnitType& /*type*/,
                                     const int& /*digits*/) const {
  return std::string("-/-");
}

/*!
 * \brief Feature::getDisplayMeasurementConfig
 * \return
 */
std::string Feature::getDisplayMeasurementConfig() const {
  return std::string("-/-");
}

/*!
 * \brief Feature::getDisplayObservations
 * \return
 */
std::string Feature::getDisplayObservations() const {
  return std::string("-/-");
}

/*!
 * \brief Feature::getDisplayIsCommon
 * \return
 */
std::string Feature::getDisplayIsCommon() const { return std::string("-/-"); }

/*!
 * \brief Feature::getDisplayIsActual
 * \return
 */
std::string Feature::getDisplayIsActual() const { return std::string("-/-"); }

/*!
 * \brief Feature::getDisplayX
 * \param type
 * \param digits
 * \param showDiff
 * \return
 */
std::string Feature::getDisplayX(const UnitType& /*type*/,
                                 const int& /*digits*/,
                                 const bool& /*showDiff*/) const {
  return std::string("-/-");
}

/*!
 * \brief Feature::getDisplayY
 * \param type
 * \param digits
 * \param showDiff
 * \return
 */
std::string Feature::getDisplayY(const UnitType& /*type*/,
                                 const int& /*digits*/,
                                 const bool& /*showDiff*/) const {
  return std::string("-/-");
}

/*!
 * \brief Feature::getDisplayZ
 * \param type
 * \param digits
 * \param showDiff
 * \return
 */
std::string Feature::getDisplayZ(const UnitType& /*type*/,
                                 const int& /*digits*/,
                                 const bool& /*showDiff*/) const {
  return std::string("-/-");
}

/*!
 * \brief Feature::getDisplayPrimaryI
 * \param digits
 * \param showDiff
 * \return
 */
std::string Feature::getDisplayPrimaryI(const int& /*digits*/,
                                        const bool& /*showDiff*/) const {
  return std::string("-/-");
}

/*!
 * \brief Feature::getDisplayPrimaryJ
 * \param digits
 * \param showDiff
 * \return
 */
std::string Feature::getDisplayPrimaryJ(const int& /*digits*/,
                                        const bool& /*showDiff*/) const {
  return std::string("-/-");
}

/*!
 * \brief Feature::getDisplayPrimaryK
 * \param digits
 * \param showDiff
 * \return
 */
std::string Feature::getDisplayPrimaryK(const int& /*digits*/,
                                        const bool& /*showDiff*/) const {
  return std::string("-/-");
}

/*!
 * \brief Feature::getDisplayRadiusA
 * \param type
 * \param digits
 * \param showDiff
 * \return
 */
std::string Feature::getDisplayRadiusA(const UnitType& /*type*/,
                                       const int& /*digits*/,
                                       const bool& /*showDiff*/) const {
  return std::string("-/-");
}

/*!
 * \brief Feature::getDisplayRadiusB
 * \param type
 * \param digits
 * \param showDiff
 * \return
 */
std::string Feature::getDisplayRadiusB(const UnitType& /*type*/,
                                       const int& /*digits*/,
                                       const bool& /*showDiff*/) const {
  return std::string("-/-");
}

/*!
 * \brief Feature::getDisplaySecondaryI
 * \param digits
 * \param showDiff
 * \return
 */
std::string Feature::getDisplaySecondaryI(const int& /*digits*/,
                                          const bool& /*showDiff*/) const {
  return std::string("-/-");
}

/*!
 * \brief Feature::getDisplaySecondaryJ
 * \param digits
 * \param showDiff
 * \return
 */
std::string Feature::getDisplaySecondaryJ(const int& /*digits*/,
                                          const bool& /*showDiff*/) const {
  return std::string("-/-");
}

/*!
 * \brief Feature::getDisplaySecondaryK
 * \param digits
 * \param showDiff
 * \return
 */
std::string Feature::getDisplaySecondaryK(const int& /*digits*/,
                                          const bool& /*showDiff*/) const {
  return std::string("-/-");
}

/*!
 * \brief Feature::getDisplayAperture
 * \param type
 * \param digits
 * \param showDiff
 * \return
 */
std::string Feature::getDisplayAperture(const UnitType& /*type*/,
                                        const int& /*digits*/,
                                        const bool& /*showDiff*/) const {
  return std::string("-/-");
}

/*!
 * \brief Feature::getDisplayA
 * \param type
 * \param digits
 * \param showDiff
 * \return
 */
std::string Feature::getDisplayA(const UnitType& /*type*/,
                                 const int& /*digits*/,
                                 const bool& /*showDiff*/) const {
  return std::string("-/-");
}

/*!
 * \brief Feature::getDisplayB
 * \param type
 * \param digits
 * \param showDiff
 * \return
 */
std::string Feature::getDisplayB(const UnitType& /*type*/,
                                 const int& /*digits*/,
                                 const bool& /*showDiff*/) const {
  return std::string("-/-");
}

/*!
 * \brief Feature::getDisplayC
 * \param type
 * \param digits
 * \param showDiff
 * \return
 */
std::string Feature::getDisplayC(const UnitType& /*type*/,
                                 const int& /*digits*/,
                                 const bool& /*showDiff*/) const {
  return std::string("-/-");
}

/*!
 * \brief Feature::getDisplayAngle
 * \param type
 * \param digits
 * \param showDiff
 * \return
 */
std::string Feature::getDisplayAngle(const UnitType& /*type*/,
                                     const int& /*digits*/,
                                     const bool& /*showDiff*/) const {
  return std::string("-/-");
}

/*!
 * \brief Feature::getDisplayDistance
 * \param type
 * \param digits
 * \param showDiff
 * \return
 */
std::string Feature::getDisplayDistance(const UnitType& /*type*/,
                                        const int& /*digits*/,
                                        const bool& /*showDiff*/) const {
  return std::string("-/-");
}

/*!
 * \brief Feature::getDisplayMeasurementSeries
 * \param type
 * \param digits
 * \param showDiff
 * \return
 */
std::string Feature::getDisplayMeasurementSeries(
    const UnitType& /*type*/, const int& /*digits*/,
    const bool& /*showDiff*/) const {
  return std::string("-/-");
}

/*!
 * \brief Feature::getDisplayTemperature
 * \param type
 * \param digits
 * \param showDiff
 * \return
 */
std::string Feature::getDisplayTemperature(const UnitType& /*type*/,
                                           const int& /*digits*/,
                                           const bool& /*showDiff*/) const {
  return std::string("-/-");
}

/*!
 * \brief Feature::getDisplayLength
 * \param type
 * \param digits
 * \param showDiff
 * \return
 */
std::string Feature::getDisplayLength(const UnitType& /*type*/,
                                      const int& /*digits*/,
                                      const bool& /*showDiff*/) const {
  return std::string("-/-");
}

/*!
 * \brief Feature::getDisplayExpansionOriginX
 * \param type
 * \param digits
 * \return
 */
std::string Feature::getDisplayExpansionOriginX(const UnitType& /*type*/,
                                                const int& /*digits*/) const {
  return std::string("-/-");
}

/*!
 * \brief Feature::getDisplayExpansionOriginY
 * \param type
 * \param digits
 * \return
 */
std::string Feature::getDisplayExpansionOriginY(const UnitType& /*type*/,
                                                const int& /*digits*/) const {
  return std::string("-/-");
}

/*!
 * \brief Feature::getDisplayExpansionOriginZ
 * \param type
 * \param digits
 * \return
 */
std::string Feature::getDisplayExpansionOriginZ(const UnitType& /*type*/,
                                                const int& /*digits*/) const {
  return std::string("-/-");
}

/*!
 * \brief Feature::getDisplayXAxisI
 * \param digits
 * \return
 */
std::string Feature::getDisplayXAxisI(const int& /*digits*/) const {
  return std::string("-/-");
}

/*!
 * \brief Feature::getDisplayXAxisJ
 * \param digits
 * \return
 */
std::string Feature::getDisplayXAxisJ(const int& /*digits*/) const {
  return std::string("-/-");
}

/*!
 * \brief Feature::getDisplayXAxisK
 * \param digits
 * \return
 */
std::string Feature::getDisplayXAxisK(const int& /*digits*/) const {
  return std::string("-/-");
}

/*!
 * \brief Feature::getDisplayYAxisI
 * \param digits
 * \return
 */
std::string Feature::getDisplayYAxisI(const int& /*digits*/) const {
  return std::string("-/-");
}

/*!
 * \brief Feature::getDisplayYAxisJ
 * \param digits
 * \return
 */
std::string Feature::getDisplayYAxisJ(const int& /*digits*/) const {
  return std::string("-/-");
}

/*!
 * \brief Feature::getDisplayYAxisK
 * \param digits
 * \return
 */
std::string Feature::getDisplayYAxisK(const int& /*digits*/) const {
  return std::string("-/-");
}

/*!
 * \brief Feature::getDisplayZAxisI
 * \param digits
 * \return
 */
std::string Feature::getDisplayZAxisI(const int& /*digits*/) const {
  return std::string("-/-");
}

/*!
 * \brief Feature::getDisplayZAxisJ
 * \param digits
 * \return
 */
std::string Feature::getDisplayZAxisJ(const int& /*digits*/) const {
  return std::string("-/-");
}

/*!
 * \brief Feature::getDisplayZAxisK
 * \param digits
 * \return
 */
std::string Feature::getDisplayZAxisK(const int& /*digits*/) const {
  return std::string("-/-");
}

/*!
 * \brief Feature::getDisplayStartSystem
 * \return
 */
std::string Feature::getDisplayStartSystem() const {
  return std::string("-/-");
}

/*!
 * \brief Feature::getDisplayDestinationSystem
 * \return
 */
std::string Feature::getDisplayDestinationSystem() const {
  return std::string("-/-");
}

/*!
 * \brief Feature::getDisplayTranslationX
 * \param type
 * \param digits
 * \return
 */
std::string Feature::getDisplayTranslationX(const UnitType& /*type*/,
                                            const int& /*digits*/) const {
  return std::string("-/-");
}

/*!
 * \brief Feature::getDisplayTranslationY
 * \param type
 * \param digits
 * \return
 */
std::string Feature::getDisplayTranslationY(const UnitType& /*type*/,
                                            const int& /*digits*/) const {
  return std::string("-/-");
}

/*!
 * \brief Feature::getDisplayTranslationZ
 * \param type
 * \param digits
 * \return
 */
std::string Feature::getDisplayTranslationZ(const UnitType& /*type*/,
                                            const int& /*digits*/) const {
  return std::string("-/-");
}

/*!
 * \brief Feature::getDisplayRotationX
 * \param type
 * \param digits
 * \return
 */
std::string Feature::getDisplayRotationX(const UnitType& /*type*/,
                                         const int& /*digits*/) const {
  return std::string("-/-");
}

/*!
 * \brief Feature::getDisplayRotationY
 * \param type
 * \param digits
 * \return
 */
std::string Feature::getDisplayRotationY(const UnitType& /*type*/,
                                         const int& /*digits*/) const {
  return std::string("-/-");
}

/*!
 * \brief Feature::getDisplayRotationZ
 * \param type
 * \param digits
 * \return
 */
std::string Feature::getDisplayRotationZ(const UnitType& /*type*/,
                                         const int& /*digits*/) const {
  return std::string("-/-");
}

/*!
 * \brief Feature::getDisplayScaleX
 * \param digits
 * \return
 */
std::string Feature::getDisplayScaleX(const int& /*digits*/) const {
  return std::string("-/-");
}

/*!
 * \brief Feature::getDisplayScaleY
 * \param digits
 * \return
 */
std::string Feature::getDisplayScaleY(const int& /*digits*/) const {
  return std::string("-/-");
}

/*!
 * \brief Feature::getDisplayScaleZ
 * \param digits
 * \return
 */
std::string Feature::getDisplayScaleZ(const int& /*digits*/) const {
  return std::string("-/-");
}

/*!
 * \brief Feature::getDisplayIsUsed
 * \return
 */
std::string Feature::getDisplayIsUsed() const { return std::string("-/-"); }

/*!
 * \brief Feature::getDisplayValidTime
 * \return
 */
std::string Feature::getDisplayValidTime() const { return std::string("-/-"); }

/*!
 * \brief Feature::getDisplayIsMovement
 * \return
 */
std::string Feature::getDisplayIsMovement() const { return std::string("-/-"); }

/*!
 * \brief Feature::getDisplayIsDatumTransformation
 * \return
 */
std::string Feature::getDisplayIsDatumTransformation() const {
  return std::string("-/-");
}

/*!
 * \brief Feature::getDisplayFormError
 * \param type
 * \param digits
 * \param showDiff
 * \return
 */
std::string Feature::getDisplayFormError(const UnitType& /*type*/,
                                         const int& /*digits*/) const {
  return std::string("-/-");
}

/*!
 * \brief Feature::setUpFeatureId
 * Generate a new unique id when the current job was set
 */
void Feature::setUpFeatureId() {
  // check job
  if (this->job.isNull()) {
    return;
  }

  // let the job generate a new unique id for this feature
  this->id = job->generateUniqueId();

  // generate unique ids for the feature's functions
  if (this->producerFunction.has_value() && !this->producerFunction->isNull()) {
    this->producerFunction.value()->id = this->job->generateUniqueId();
  }
  for (const oi::Guarded<Function>& function :
       oi::snapshot(this->transformFunctions)) {
    if (!function.isNull()) {
      function->id = this->job->generateUniqueId();
    }
  }
}

/*!
 * \brief Feature::setJob
 * Called by OiJob. Sets the current job this feature belongs to and checks its
 * id
 * \param job
 */
void Feature::setJob(const oi::Guarded<OiJob>& job) {
  // check and set job
  if (job.isNull()) {
    return;
  }
  this->job = job;

  // generate feature id
  this->setUpFeatureId();
}
