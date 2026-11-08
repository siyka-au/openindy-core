#ifndef FEATUREWRAPPER_H
#define FEATUREWRAPPER_H

#include "circle.h"
#include "cone.h"
#include "coordinate_system.h"
#include "core_export.h"
#include "cylinder.h"
#include "ellipse.h"
#include "ellipsoid.h"
#include "feature.h"
#include "geometry.h"
#include "group.h"
#include "guarded.h"
#include "hyperboloid.h"
#include "line.h"
#include "nurbs.h"
#include "paraboloid.h"
#include "plane.h"
#include "point.h"
#include "point_cloud.h"
#include "point_group.h"
#include "scalar_entity_angle.h"
#include "scalar_entity_distance.h"
#include "scalar_entity_measurement_series.h"
#include "scalar_entity_temperature.h"
#include "slotted_hole.h"
#include "sphere.h"
#include "station.h"
#include "torus.h"
#include "trafo_param.h"
#include "types.h"
#include "util.h"

namespace oi {

class OI_CORE_EXPORT FeatureWrapper : public oi::Trackable {
 public:
  explicit FeatureWrapper();

  bool operator==(const FeatureWrapper& other);

  // ################
  // type information
  // ################

  [[nodiscard]] std::string getFeatureTypeString() const;
  [[nodiscard]] const FeatureTypes& getFeatureTypeEnum() const;

  // ###################
  // get feature pointer
  // ###################

  [[nodiscard]] const oi::Guarded<Feature>& getFeature() const;
  [[nodiscard]] const oi::Guarded<Geometry>& getGeometry() const;

  [[nodiscard]] const oi::Guarded<CoordinateSystem>& getCoordinateSystem()
      const;
  [[nodiscard]] const oi::Guarded<TrafoParam>& getTrafoParam() const;
  [[nodiscard]] const oi::Guarded<Station>& getStation() const;
  [[nodiscard]] const oi::Guarded<Group>& getGroup() const;
  [[nodiscard]] const oi::Guarded<PointGroup>& getPointGroup() const;

  [[nodiscard]] const oi::Guarded<Point>& getPoint() const;
  [[nodiscard]] const oi::Guarded<Line>& getLine() const;
  [[nodiscard]] const oi::Guarded<Plane>& getPlane() const;
  [[nodiscard]] const oi::Guarded<Sphere>& getSphere() const;
  [[nodiscard]] const oi::Guarded<Circle>& getCircle() const;
  [[nodiscard]] const oi::Guarded<Cone>& getCone() const;
  [[nodiscard]] const oi::Guarded<Cylinder>& getCylinder() const;
  [[nodiscard]] const oi::Guarded<Ellipsoid>& getEllipsoid() const;
  [[nodiscard]] const oi::Guarded<Hyperboloid>& getHyperboloid() const;
  [[nodiscard]] const oi::Guarded<Paraboloid>& getParaboloid() const;
  [[nodiscard]] const oi::Guarded<PointCloud>& getPointCloud() const;
  [[nodiscard]] const oi::Guarded<Nurbs>& getNurbs() const;
  [[nodiscard]] const oi::Guarded<Ellipse>& getEllipse() const;
  [[nodiscard]] const oi::Guarded<SlottedHole>& getSlottedHole() const;
  [[nodiscard]] const oi::Guarded<Torus>& getTorus() const;

  [[nodiscard]] const oi::Guarded<ScalarEntityAngle>& getScalarEntityAngle()
      const;
  [[nodiscard]] const oi::Guarded<ScalarEntityDistance>&
  getScalarEntityDistance() const;
  [[nodiscard]] const oi::Guarded<ScalarEntityTemperature>&
  getScalarEntityTemperature() const;
  [[nodiscard]] const oi::Guarded<ScalarEntityMeasurementSeries>&
  getScalarEntityMeasurementSeries() const;

  // ###################
  // set feature pointer
  // ###################

  void setCoordinateSystem(const oi::Guarded<CoordinateSystem>& coordSystem);
  void setTrafoParam(const oi::Guarded<TrafoParam>& trafoParam);
  void setStation(const oi::Guarded<Station>& station);
  void setGroup(const oi::Guarded<Group>& group);
  void setPointGroup(const oi::Guarded<PointGroup>& pointGroup);

  void setPoint(const oi::Guarded<Point>& point);
  void setLine(const oi::Guarded<Line>& line);
  void setPlane(const oi::Guarded<Plane>& plane);
  void setSphere(const oi::Guarded<Sphere>& sphere);
  void setCircle(const oi::Guarded<Circle>& circle);
  void setCone(const oi::Guarded<Cone>& cone);
  void setCylinder(const oi::Guarded<Cylinder>& cylinder);
  void setEllipsoid(const oi::Guarded<Ellipsoid>& ellipsoid);
  void setHyperboloid(const oi::Guarded<Hyperboloid>& hyperboloid);
  void setParaboloid(const oi::Guarded<Paraboloid>& paraboloid);
  void setPointCloud(const oi::Guarded<PointCloud>& pointCloud);
  void setNurbs(const oi::Guarded<Nurbs>& nurbs);
  void setEllipse(const oi::Guarded<Ellipse>& ellipse);
  void setSlottedHole(const oi::Guarded<SlottedHole>& slottedHole);
  void setTorus(const oi::Guarded<Torus>& torus);

  void setScalarEntityAngle(const oi::Guarded<ScalarEntityAngle>& angle);
  void setScalarEntityDistance(
      const oi::Guarded<ScalarEntityDistance>& distance);
  void setScalarEntityTemperature(
      const oi::Guarded<ScalarEntityTemperature>& temperature);
  void setScalarEntityMeasurementSeries(
      const oi::Guarded<ScalarEntityMeasurementSeries>& measurementSeries);

 private:
  // #################################################################
  // private feature pointer that hold a feature or are set to invalid
  // #################################################################

  oi::Guarded<Feature> feature;
  oi::Guarded<Geometry> geometry;

  oi::Guarded<CoordinateSystem> coordinateSystem;
  oi::Guarded<TrafoParam> trafoParam;

  oi::Guarded<Station> station;

  oi::Guarded<Point> point;
  oi::Guarded<Line> line;
  oi::Guarded<Plane> plane;
  oi::Guarded<Sphere> sphere;
  oi::Guarded<Circle> circle;
  oi::Guarded<Cone> cone;
  oi::Guarded<Cylinder> cylinder;
  oi::Guarded<Ellipsoid> ellipsoid;
  oi::Guarded<Hyperboloid> hyperboloid;
  oi::Guarded<Paraboloid> paraboloid;
  oi::Guarded<PointCloud> pointCloud;
  oi::Guarded<Group> groupFeature;
  oi::Guarded<PointGroup> pointGroup;
  oi::Guarded<Nurbs> nurbs;
  oi::Guarded<Ellipse> ellipse;
  oi::Guarded<SlottedHole> slottedHole;
  oi::Guarded<Torus> torus;
  oi::Guarded<ScalarEntityAngle> angle;
  oi::Guarded<ScalarEntityDistance> distance;
  oi::Guarded<ScalarEntityTemperature> temperature;
  oi::Guarded<ScalarEntityMeasurementSeries> measurementSeries;

  FeatureTypes typeOfFeature;
};

}  // namespace oi

#endif  // FEATUREWRAPPER_H
