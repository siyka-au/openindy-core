#ifndef POINTCOLLECTION_H
#define POINTCOLLECTION_H

#include <QList>

#include "position.h"

namespace oi {

/*!
 * \brief The PointCollection class
 * "Something a fit function can get points out of."
 *
 * Deliberately a pure interface, not a base class, and deliberately not a
 * QObject: the two things that satisfy it are genuinely different kinds of
 * object that already have their own base classes, and Qt permits exactly
 * one QObject base plus any number of plain interfaces.
 *
 *   PointCloud  - a Geometry that *owns* bulk, undifferentiated scanner
 *                 data (Point_PC), acquired in one shot by a scanner-shaped
 *                 sensor (LiDAR, structured light).
 *   PointGroup  - a Group that holds *references* to real, individually
 *                 measured Point features, each with its own full accuracy
 *                 and measurement pedigree.
 *
 * A fit function asks for a PointCollection and doesn't care which it got.
 * That is the whole point: "fit a plane" is the same operation whether the
 * points came from a scan or from a thousand individually aimed
 * reflectorless shots.
 *
 * Only positions are exposed here. Anything needing per-point pedigree
 * (which observation, which instrument, what sigma) is asking a question
 * only PointGroup can answer, and should take a PointGroup.
 */
class PointCollection {
 public:
  virtual ~PointCollection() = default;

  //! positions of every point in the collection, in the collection's own
  //! current coordinate system
  virtual QList<Position> collectionPoints() const = 0;

  //! how many points the collection currently holds. Separate from
  //! collectionPoints().size() so a large point cloud can answer without
  //! materialising a copy of every position.
  virtual int collectionPointCount() const = 0;
};

}  // namespace oi

#endif  // POINTCOLLECTION_H
