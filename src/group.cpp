#include "group.h"

#include "featurewrapper.h"
#include "snapshot.h"

using namespace oi;

Group::Group(QObject* parent) : Feature(parent) {}

Group::Group(const QString& name, QObject* parent) : Feature(parent) {
  this->name = name;
}

Group::Group(const QString& name, const FeatureTypes& constraint,
             QObject* parent)
    : Feature(parent) {
  this->name = name;
  this->constraint = constraint;
}

Group::Group(const Group& copy, QObject* parent)
    : Feature(copy, parent),
      constraint(copy.constraint),
      members(copy.members) {
  // members are references, so a copied group references the same
  // features - it does not clone them. That is the whole contract.
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

bool Group::accepts(const QPointer<FeatureWrapper>& feature) const {
  if (feature.isNull() || feature->getFeature().isNull()) {
    return false;
  }

  if (!this->constraint.has_value()) {
    return true;
  }

  return feature->getFeatureTypeEnum() == this->constraint.value();
}

const QList<QPointer<FeatureWrapper> >& Group::getMembers() const {
  return this->members;
}

bool Group::addMember(const QPointer<FeatureWrapper>& feature) {
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

  this->members.append(feature);

  emit this->groupMembersChanged(this->id);

  return true;
}

bool Group::removeMember(const QPointer<FeatureWrapper>& feature) {
  if (feature.isNull() || feature->getFeature().isNull()) {
    return false;
  }

  return this->removeMemberById(feature->getFeature()->getId());
}

bool Group::removeMemberById(const int& featureId) {
  for (int i = 0; i < this->members.size(); ++i) {
    const QPointer<FeatureWrapper>& member = this->members.at(i);
    if (member.isNull() || member->getFeature().isNull()) {
      continue;
    }

    if (member->getFeature()->getId() == featureId) {
      this->members.removeAt(i);
      emit this->groupMembersChanged(this->id);
      return true;
    }
  }

  return false;
}

bool Group::containsMember(const QPointer<FeatureWrapper>& feature) const {
  if (feature.isNull() || feature->getFeature().isNull()) {
    return false;
  }

  const Feature* target = feature->getFeature().data();

  for (const QPointer<FeatureWrapper>& member : oi::snapshot(this->members)) {
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
  for (const QPointer<FeatureWrapper>& member : oi::snapshot(this->members)) {
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
  for (const QPointer<FeatureWrapper>& member : oi::snapshot(this->members)) {
    if (!member.isNull() && !member->getFeature().isNull()) {
      ++count;
    }
  }
  return count;
}

void Group::compact() {
  const int before = this->members.size();

  QList<QPointer<FeatureWrapper> > live;
  for (const QPointer<FeatureWrapper>& member : oi::snapshot(this->members)) {
    if (!member.isNull() && !member->getFeature().isNull()) {
      live.append(member);
    }
  }
  this->members = live;

  if (this->members.size() != before) {
    emit this->groupMembersChanged(this->id);
  }
}

void Group::recalc() {
  // a group has no functions and solves to nothing of its own; it is
  // organisational. It is "solved" whenever it holds anything.
  this->isSolved = this->memberCount() > 0;
}

QString Group::getDisplayType() const {
  if (this->constraint.has_value()) {
    return QString("%1 group")
        .arg(getFeatureTypeName(this->constraint.value()));
  }
  return QString("group");
}
