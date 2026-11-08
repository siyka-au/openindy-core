#include "group.h"

#include <format>
#include <iterator>
#include <vector>

#include "containers.h"
#include "feature_wrapper.h"
#include "snapshot.h"

using namespace oi;

Group::Group() : Feature() { this->selfFeature->setGroup(this); }

Group::Group(const std::string& name) : Feature() {
  this->name = name;
  this->selfFeature->setGroup(this);
}

Group::Group(const std::string& name, const FeatureTypes& constraint)
    : Feature() {
  this->name = name;
  this->constraint = constraint;
  this->selfFeature->setGroup(this);
}

Group::Group(const Group& copy)
    : Feature(copy), constraint(copy.constraint), members(copy.members) {
  // members are references, so a copied group references the same
  // features - it does not clone them. That is the whole contract.
  this->selfFeature->setGroup(this);
}

Group& Group::operator=(const Group& copy) {
  Feature::operator=(copy);

  this->members = copy.members;
  this->constraint = copy.constraint;

  return *this;
}

Group::~Group() {
  // a group owns nothing - dropping references is the entire teardown.
  // Deleting a group must never take its members with it.
  this->members.clear();
}

const std::optional<FeatureTypes>& Group::getConstraint() const {
  return this->constraint;
}

bool Group::accepts(const oi::Guarded<FeatureWrapper>& feature) const {
  if (feature.isNull() || feature->getFeature().isNull()) {
    return false;
  }

  if (!this->constraint.has_value()) {
    return true;
  }

  return feature->getFeatureTypeEnum() == this->constraint.value();
}

const std::vector<oi::Guarded<FeatureWrapper> >& Group::getMembers() const {
  return this->members;
}

bool Group::addMember(const oi::Guarded<FeatureWrapper>& feature) {
  if (!this->accepts(feature)) {
    return false;
  }

  // a group containing itself would recurse forever on any traversal.
  // Compared by pointer, not id, for the same reason as containsMember.
  if (feature->getFeature().data() == static_cast<const Feature*>(this)) {
    return false;
  }

  if (this->containsMember(feature)) {
    return false;
  }

  this->members.push_back(feature);

  this->notify(&FeatureObserver::onGroupMembersChanged, this->id);

  return true;
}

bool Group::removeMember(const oi::Guarded<FeatureWrapper>& feature) {
  if (feature.isNull() || feature->getFeature().isNull()) {
    return false;
  }

  return this->removeMemberById(feature->getFeature()->getId());
}

bool Group::removeMemberById(const int& featureId) {
  for (int i = 0; i < std::ssize(this->members); ++i) {
    const oi::Guarded<FeatureWrapper>& member = this->members.at(i);
    if (member.isNull() || member->getFeature().isNull()) {
      continue;
    }

    if (member->getFeature()->getId() == featureId) {
      oi::removeAt(this->members, i);
      this->notify(&FeatureObserver::onGroupMembersChanged, this->id);
      return true;
    }
  }

  return false;
}

bool Group::containsMember(const oi::Guarded<FeatureWrapper>& feature) const {
  if (feature.isNull() || feature->getFeature().isNull()) {
    return false;
  }

  const Feature* target = feature->getFeature().data();

  for (const oi::Guarded<FeatureWrapper>& member :
       oi::snapshot(this->members)) {
    if (member.isNull() || member->getFeature().isNull()) {
      continue;
    }
    if (member->getFeature().data() == target) {
      return true;
    }
  }

  return false;
}

bool Group::containsMemberId(const int& featureId) const {
  for (const oi::Guarded<FeatureWrapper>& member :
       oi::snapshot(this->members)) {
    if (member.isNull() || member->getFeature().isNull()) {
      continue;
    }
    if (member->getFeature()->getId() == featureId) {
      return true;
    }
  }

  return false;
}

int Group::memberCount() const {
  int count = 0;
  for (const oi::Guarded<FeatureWrapper>& member :
       oi::snapshot(this->members)) {
    if (!member.isNull() && !member->getFeature().isNull()) {
      ++count;
    }
  }
  return count;
}

void Group::compact() {
  const int before = this->members.size();

  std::vector<oi::Guarded<FeatureWrapper> > live;
  for (const oi::Guarded<FeatureWrapper>& member :
       oi::snapshot(this->members)) {
    if (!member.isNull() && !member->getFeature().isNull()) {
      live.push_back(member);
    }
  }
  this->members = live;

  if (std::ssize(this->members) != before) {
    this->notify(&FeatureObserver::onGroupMembersChanged, this->id);
  }
}

void Group::recalc() {
  // a group has no functions and solves to nothing of its own; it is
  // organisational. It is "solved" whenever it holds anything.
  this->isSolved = this->memberCount() > 0;
}

std::string Group::getDisplayType() const {
  if (this->constraint.has_value()) {
    return std::format("{} group",
                       getFeatureTypeName(this->constraint.value()));
  }
  return std::string("group");
}
