#include "pointgroup.h"

#include "featurewrapper.h"
#include "geometry/point.h"
#include "snapshot.h"

using namespace oi;

PointGroup::PointGroup(QObject* parent)
    : Group(QString(), FeatureTypes::ePointFeature, parent) {}

PointGroup::PointGroup(const QString& name, QObject* parent)
    : Group(name, FeatureTypes::ePointFeature, parent) {}

PointGroup::PointGroup(const PointGroup& copy, QObject* parent)
    : Group(copy, parent) {}

PointGroup& PointGroup::operator=(const PointGroup& copy) {
  Group::operator=(copy);
  return *this;
}

PointGroup::~PointGroup() {}

QList<QPointer<Point> > PointGroup::getPoints() const {
  QList<QPointer<Point> > points;

  for (const QPointer<FeatureWrapper>& member : oi::snapshot(this->members)) {
    // a member deleted out from under the group is skipped, not an
    // error: a group is not an owner, so outliving its members is a
    // normal state rather than a broken one
    if (member.isNull() || member->getPoint().isNull()) {
      continue;
    }

    points.append(member->getPoint());
  }

  return points;
}

QList<Position> PointGroup::collectionPoints() const {
  QList<Position> positions;

  for (const QPointer<Point>& point : oi::snapshot(this->getPoints())) {
    // an unsolved point has no meaningful position yet - handing it to a
    // fit would quietly corrupt the result, so it is not part of the
    // collection until it solves
    if (point.isNull() || !point->getIsSolved()) {
      continue;
    }

    positions.append(point->getPosition());
  }

  return positions;
}

int PointGroup::collectionPointCount() const {
  return this->collectionPoints().size();
}

QString PointGroup::getDisplayType() const { return QString("point group"); }
