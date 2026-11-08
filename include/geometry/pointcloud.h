#ifndef POINTCLOUD_H
#define POINTCLOUD_H
#include <array>
#include <string>
#include <vector>

#include "core_export.h"
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
  std::array<float, 3> xyz{};
  bool isUsed;
};

/*!
 * \brief The BoundingBox_PC class
 */
class OI_CORE_EXPORT BoundingBox_PC {
 public:
  BoundingBox_PC() {}

  std::array<float, 3> min{0.0f, 0.0f, 0.0f};
  std::array<float, 3> max{0.0f, 0.0f, 0.0f};
};

/*!
 * \brief The PointCloud class
 */
class OI_CORE_EXPORT PointCloud : public Geometry, public PointCollection {
  friend struct codec::Access;  // the protobuf codec (see src/codec)

 public:
  explicit PointCloud();

  PointCloud(const PointCloud& copy);

  PointCloud& operator=(const PointCloud& copy);

  ~PointCloud() override;

  // ###########################
  // general point cloud methods
  // ###########################

  [[nodiscard]] unsigned long getPointCount() const;

  bool addSegment(const oi::Guarded<FeatureWrapper>& segment);
  void clearSegmentation();

  // ###################################################################
  // check wether this geometry has a direction, a position and a radius
  // ###################################################################

  [[nodiscard]] bool hasPosition() const override;

  // #################################
  // get or set point cloud parameters
  // #################################

  [[nodiscard]] const std::vector<std::unique_ptr<Point_PC>>&
  getPointCloudPoints() const;
  void addPointCloudPoint(std::unique_ptr<Point_PC> point);

  // #########################
  // PointCollection interface
  // #########################

  [[nodiscard]] std::vector<Position> collectionPoints() const override;
  [[nodiscard]] int collectionPointCount() const override;

  [[nodiscard]] const BoundingBox_PC& getBoundingBox() const;
  void setBoundingBox(const BoundingBox_PC& bbox);

  // ###########################
  // reexecute the function list
  // ###########################

  void recalc() override;

  // ###############
  // display methods
  // ###############

 private:
  // ######################
  // point cloud attributes
  // ######################

  std::vector<std::unique_ptr<Point_PC>>
      points;           // all points of the pointcloud (owned)
  BoundingBox_PC bbox;  // bounding box of the pointcloud

  std::vector<oi::Guarded<FeatureWrapper>>
      detectedSegments;  // geometry-segments that were detected in the
                         // pointcloud
};

}  // namespace oi

#endif  // POINTCLOUD_H
