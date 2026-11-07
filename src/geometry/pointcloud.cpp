#include "pointcloud.h"

#include "featurewrapper.h"
#include "snapshot.h"

using namespace oi;
using namespace oi::math;

/*!
 * \brief PointCloud::PointCloud
 * \param parent
 */
PointCloud::PointCloud(QObject* parent) : Geometry(parent) {
  // set up feature wrapper
  if (!this->selfFeature.isNull()) {
    this->selfFeature->setPointCloud(this);
  }
}

/*!
 * \brief PointCloud::PointCloud
 * \param copy
 */
PointCloud::PointCloud(const PointCloud& copy, QObject* parent)
    : Geometry(copy, parent) {
  // set up feature wrapper
  if (!this->selfFeature.isNull()) {
    this->selfFeature->setPointCloud(this);
  }

  this->xyz = copy.xyz;
  this->setBoundingBox(copy.bbox);
}

/*!
 * \brief PointCloud::operator =
 * \param copy
 * \return
 */
PointCloud& PointCloud::operator=(const PointCloud& copy) {
  // set up feature wrapper
  if (!this->selfFeature.isNull()) {
    this->selfFeature->setPointCloud(this);
  }

  this->xyz = copy.xyz;
  this->setBoundingBox(copy.bbox);

  return *this;
}

/*!
 * \brief PointCloud::~PointCloud
 */
PointCloud::~PointCloud() {}

/*!
 * \brief PointCloud::getPointCount
 * \return
 */
unsigned long PointCloud::getPointCount() const { return this->points.size(); }

/*!
 * \brief PointCloud::addSegment
 * \param segment
 * \return
 */
bool PointCloud::addSegment(const QPointer<FeatureWrapper>& segment) {
  // check segment
  if (segment.isNull() || segment->getGeometry().isNull()) {
    return false;
  }

  // check if a segment with that name already exists
  for (const QPointer<FeatureWrapper>& detectedSegment :
       oi::snapshot(this->detectedSegments)) {
    if (!detectedSegment.isNull() && !detectedSegment->getFeature().isNull() &&
        detectedSegment->getFeature()->getFeatureName().compare(
            segment->getGeometry()->getFeatureName()) == 0) {
      return false;
    }
  }

  segment->getGeometry()->setNominalSystem(this->nominalSystem);

  this->detectedSegments.append(segment);

  emit this->pcSegmentAdded(segment);

  return true;
}

/*!
 * \brief PointCloud::clearSegmentation
 */
void PointCloud::clearSegmentation() { this->detectedSegments.clear(); }

/*!
 * \brief PointCloud::hasPosition
 * \return
 */
bool PointCloud::hasPosition() const { return true; }

/*!
 * \brief PointCloud::getPointCloudPoints
 * Returns a list of all point cloud points
 * \return
 */
const QList<Point_PC*>& PointCloud::getPointCloudPoints() const {
  return this->points;
}

/*!
 * \brief PointCloud::addPointCloudPoint
 * \param point
 */
void PointCloud::addPointCloudPoint(Point_PC* point) {
  if (point == nullptr) {
    return;
  }
  this->points.append(point);
}

/*!
 * \brief PointCloud::collectionPoints
 * PointCollection interface: a fit function asks any collection for its
 * points and doesn't care whether they came from a scanner or from a
 * thousand individually aimed shots.
 * \return
 */
QList<Position> PointCloud::collectionPoints() const {
  QList<Position> positions;
  positions.reserve(this->points.size());

  for (Point_PC* point : oi::snapshot(this->points)) {
    // isUsed is the cloud's own inclusion flag - a point excluded by
    // segmentation or filtering is still stored, but is not part of the
    // collection a fit should see
    if (point == nullptr || !point->isUsed) {
      continue;
    }

    positions.append(Position(point->xyz[0], point->xyz[1], point->xyz[2]));
  }

  return positions;
}

/*!
 * \brief PointCloud::collectionPointCount
 * \return
 */
int PointCloud::collectionPointCount() const {
  // counted without materialising positions - a real scan is millions of
  // points and this gets asked just to answer "is there enough data yet"
  int count = 0;
  for (Point_PC* point : oi::snapshot(this->points)) {
    if (point != nullptr && point->isUsed) {
      ++count;
    }
  }
  return count;
}

/*!
 * \brief PointCloud::getBoundingBox
 * Returns the bounding box of the point cloud
 * \return
 */
const BoundingBox_PC& PointCloud::getBoundingBox() const { return this->bbox; }

/*!
 * \brief PointCloud::setBoundingBox
 * \param bbox
 */
void PointCloud::setBoundingBox(const BoundingBox_PC& bbox) {
  this->bbox = bbox;
}

/*!
 * \brief PointCloud::recalc
 */
void PointCloud::recalc() { Geometry::recalc(); }
