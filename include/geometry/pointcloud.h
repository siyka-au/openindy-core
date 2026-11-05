#ifndef POINTCLOUD_H
#define POINTCLOUD_H

#include <QObject>
#include <QtXml>

#include "geometry.h"
#include "pointcollection.h"
#include "position.h"

namespace oi {

class FeatureWrapper;

/*!
 * \brief The Point_PC class
 */
class OI_CORE_EXPORT Point_PC {
 public:
  Point_PC() : xyz{0.0f, 0.0f, 0.0f}, isUsed(false) {}
  Point_PC(float x, float y, float z) : xyz{x, y, z}, isUsed(true) {}

  // a bulk scan point: position only, no per-point pedigree. That absence
  // is the defining difference from a Point feature in a PointGroup, not
  // an omission - a scanner produces undifferentiated bulk data, and
  // pretending otherwise would invent provenance that doesn't exist.
  float xyz[3];
  bool isUsed;
};

/*!
 * \brief The BoundingBox_PC class
 */
class OI_CORE_EXPORT BoundingBox_PC {
 public:
  BoundingBox_PC() {}

  float min[3]{0.0f, 0.0f, 0.0f};
  float max[3]{0.0f, 0.0f, 0.0f};
};

/*!
 * \brief The PointCloud class
 */
class OI_CORE_EXPORT PointCloud : public Geometry, public PointCollection {
  Q_OBJECT

 public:
  explicit PointCloud(QObject* parent = nullptr);

  PointCloud(const PointCloud& copy, QObject* parent = nullptr);

  PointCloud& operator=(const PointCloud& copy);

  ~PointCloud() override;

  // ###########################
  // general point cloud methods
  // ###########################

  unsigned long getPointCount() const;

  bool addSegment(const QPointer<FeatureWrapper>& segment);
  void clearSegmentation();

  // ###################################################################
  // check wether this geometry has a direction, a position and a radius
  // ###################################################################

  bool hasPosition() const override;

  // #################################
  // get or set point cloud parameters
  // #################################

  const QList<Point_PC*>& getPointCloudPoints() const;
  void addPointCloudPoint(Point_PC* point);

  // #########################
  // PointCollection interface
  // #########################

  QList<Position> collectionPoints() const override;
  int collectionPointCount() const override;

  const BoundingBox_PC& getBoundingBox() const;
  void setBoundingBox(const BoundingBox_PC& bbox);

  // ###########################
  // reexecute the function list
  // ###########################

  void recalc() override;

  // #################
  // save and load XML
  // #################

  QDomElement toOpenIndyXML(QDomDocument& xmlDoc) const override;
  bool fromOpenIndyXML(QDomElement& xmlElem) override;

  // ###############
  // display methods
  // ###############

 signals:
  void pcSegmentAdded(const QPointer<FeatureWrapper>& segment);

 private:
  // ######################
  // point cloud attributes
  // ######################

  QList<Point_PC*> points;  // all points of the pointcloud (owned)
  BoundingBox_PC bbox;      // bounding box of the pointcloud

  QList<QPointer<FeatureWrapper> >
      detectedSegments;  // geometry-segments that were detected in the
                         // pointcloud
};

}  // namespace oi

#endif  // POINTCLOUD_H
