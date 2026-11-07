#ifndef GROUP_H
#define GROUP_H

#include <QList>
#include <QPointer>
#include <QtCore>
#include <optional>

#include "core_export.h"
#include "feature.h"

namespace oi {

class FeatureWrapper;

/*!
 * \brief The Group class
 * A named collection of *references* to other features.
 *
 * One class for every kind of grouping rather than PointGroup/PlaneGroup/
 * SphereGroup as separate types: a set of registration spheres, a series of
 * slicing planes, and a wall's worth of surveyed points differ only in what
 * they contain, not in how grouping works. Consumers that need homogeneity
 * declare a type constraint; consumers that don't, don't.
 *
 * A Group never owns or copies feature data. Membership is many-to-many by
 * construction - the same Point may belong to as many Groups as you like,
 * measured once and stored once. That is the direct answer to having to
 * copy and paste the same physical point between contexts in other
 * metrology packages just because the software insists each use needs its
 * own copy.
 *
 * A Group is not a Geometry: it has no position, solves to nothing, and
 * carries no measurement of its own. It is organisational, and it is a
 * first-class Feature so it persists, appears in the job, and can be an
 * input to a function.
 */
class OI_CORE_EXPORT Group : public Feature {
  friend class OiJob;
  friend struct codec::Access;  // the protobuf codec (see src/codec)
  Q_OBJECT

 public:
  explicit Group(QObject* parent = nullptr);
  explicit Group(const QString& name, QObject* parent = nullptr);

  //! a group that will only accept members of one feature type
  Group(const QString& name, const FeatureTypes& constraint,
        QObject* parent = nullptr);

  Group(const Group& copy, QObject* parent = nullptr);
  Group& operator=(const Group& copy);

  ~Group() override;

  // #################
  // group constraints
  // #################

  //! the feature type this group accepts, if it is constrained at all
  [[nodiscard]] const std::optional<FeatureTypes>& getConstraint() const;

  //! true if the feature may be added: either the group is unconstrained,
  //! or the feature's type matches the constraint
  [[nodiscard]] bool accepts(const QPointer<FeatureWrapper>& feature) const;

  // ##########
  // membership
  // ##########

  [[nodiscard]] const QList<QPointer<FeatureWrapper>>& getMembers() const;

  //! adds a reference. Rejects a null feature, a duplicate, a feature
  //! whose type the constraint forbids, and the group itself.
  bool addMember(const QPointer<FeatureWrapper>& feature);

  bool removeMember(const QPointer<FeatureWrapper>& feature);
  bool removeMemberById(const int& featureId);

  //! identity is the underlying Feature, not its id: a feature that has
  //! not been added to a job yet still carries the default id of -1, so
  //! id comparison would collapse every unmanaged feature into one member
  [[nodiscard]] bool containsMember(
      const QPointer<FeatureWrapper>& feature) const;

  //! only meaningful for job-managed features, which are the only ones
  //! guaranteed to have been assigned a real id
  [[nodiscard]] bool containsMemberId(const int& featureId) const;

  //! how many members the group holds, ignoring any whose feature has
  //! since been deleted out from under it
  [[nodiscard]] int memberCount() const;

  //! drops references whose target has been deleted. Deleting a feature
  //! that several groups reference is legal and does not disturb them
  //! beyond leaving a null to be compacted away - a group is not an owner
  //! and must never keep a feature alive or block its removal.
  void compact();

  // ###########################
  // reexecute the function list
  // ###########################

  void recalc() override;

  // ###############
  // display methods
  // ###############

  [[nodiscard]] QString getDisplayType() const override;

 protected:
  QList<QPointer<FeatureWrapper>> members;
  std::optional<FeatureTypes> constraint;
};

}  // namespace oi

#endif  // GROUP_H
