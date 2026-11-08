#include "point_cloud.h"

#include <vector>

#include "feature_wrapper.h"
#include "snapshot.h"

using namespace oi;
using namespace oi::math;

/*!
 * \brief PointCloud::PointCloud
 */
PointCloud::PointCloud() : Geometry() {
  // set up feature wrapper
  if (!this->selfFeature.isNull()) {
    this->selfFeature->setPointCloud(this);
  }
}

/*!
 * \brief PointCloud::PointCloud
 * \param copy
 */
PointCloud::PointCloud(const PointCloud& copy) : Geometry(copy) {
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
bool PointCloud::addSegment(const oi::Guarded<FeatureWrapper>& segment) {
  // check segment
  if (segment.isNull() || segment->getGeometry().isNull()) {
    return false;
  }

  // check if a segment with that name already exists
  for (const oi::Guarded<FeatureWrapper>& detectedSegment :
       oi::snapshot(this->detectedSegments)) {
    if (!detectedSegment.isNull() && !detectedSegment->getFeature().isNull() &&
        detectedSegment->getFeature()->getFeatureName().compare(
            segment->getGeometry()->getFeatureName()) == 0) {
      return false;
    }
  }

  segment->getGeometry()->setNominalSystem(this->nominalSystem);

  this->detectedSegments.push_back(segment);
  this->notify(&FeatureObserver::onGeomParametersChanged, this->id);

  return true;
}

/*!
 * \brief PointCloud::clearSegmentation
 */
void PointCloud::clearSegmentation() {
  this->detectedSegments.clear();
  this->notify(&FeatureObserver::onGeomParametersChanged, this->id);
}

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
const std::vector<std::unique_ptr<Point_PC>>& PointCloud::getPointCloudPoints()
    const {
  return this->points;
}

/*!
 * \brief PointCloud::addPointCloudPoint
 * \param point
 */
// Deliberately silent: this builds a cloud point by point (millions), before
// the cloud is added to a job; setBoundingBox, called when the cloud is
// complete, reports the change.
void PointCloud::addPointCloudPoint(std::unique_ptr<Point_PC> point) {
  if (!point) {
    return;
  }
  this->points.push_back(std::move(point));
}

/*!
 * \brief PointCloud::collectionPoints
 * PointCollection interface: a fit function asks any collection for its
 * points and doesn't care whether they came from a scanner or from a
 * thousand individually aimed shots.
 * \return
 */
std::vector<Position> PointCloud::collectionPoints() const {
  std::vector<Position> positions;
  positions.reserve(this->points.size());

  for (const auto& point : this->points) {
    // isUsed is the cloud's own inclusion flag - a point excluded by
    // segmentation or filtering is still stored, but is not part of the
    // collection a fit should see
    if (point == nullptr || !point->isUsed) {
      continue;
    }

    positions.push_back(Position(point->xyz[0], point->xyz[1], point->xyz[2]));
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
  for (const auto& point : this->points) {
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
  this->notify(&FeatureObserver::onGeomParametersChanged, this->id);
}

/*!
 * \brief PointCloud::recalc
 */
void PointCloud::recalc() { Geometry::recalc(); }
