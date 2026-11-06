#ifndef FEATUREWRAPPER_H
#define FEATUREWRAPPER_H

#include <QObject>
#include <QPointer>

#include "circle.h"
#include "cone.h"
#include "coordinatesystem.h"
#include "core_export.h"
#include "cylinder.h"
#include "ellipse.h"
#include "ellipsoid.h"
#include "feature.h"
#include "geometry.h"
#include "group.h"
#include "hyperboloid.h"
#include "line.h"
#include "nurbs.h"
#include "paraboloid.h"
#include "plane.h"
#include "point.h"
#include "pointcloud.h"
#include "pointgroup.h"
#include "scalarentityangle.h"
#include "scalarentitydistance.h"
#include "scalarentitymeasurementseries.h"
#include "scalarentitytemperature.h"
#include "slottedhole.h"
#include "sphere.h"
#include "station.h"
#include "torus.h"
#include "trafoparam.h"
#include "types.h"
#include "util.h"

namespace oi {

class OI_CORE_EXPORT FeatureWrapper : public QObject {
  Q_OBJECT

 public:
  explicit FeatureWrapper(QObject* parent = nullptr);

  bool operator==(const FeatureWrapper& other);

  // ################
  // type information
  // ################

  [[nodiscard]] QString getFeatureTypeString() const;
  [[nodiscard]] const FeatureTypes& getFeatureTypeEnum() const;

  // ###################
  // get feature pointer
  // ###################

  [[nodiscard]] const QPointer<Feature>& getFeature() const;
  [[nodiscard]] const QPointer<Geometry>& getGeometry() const;

  [[nodiscard]] const QPointer<CoordinateSystem>& getCoordinateSystem() const;
  [[nodiscard]] const QPointer<TrafoParam>& getTrafoParam() const;
  [[nodiscard]] const QPointer<Station>& getStation() const;
  [[nodiscard]] const QPointer<Group>& getGroup() const;
  [[nodiscard]] const QPointer<PointGroup>& getPointGroup() const;

  [[nodiscard]] const QPointer<Point>& getPoint() const;
  [[nodiscard]] const QPointer<Line>& getLine() const;
  [[nodiscard]] const QPointer<Plane>& getPlane() const;
  [[nodiscard]] const QPointer<Sphere>& getSphere() const;
  [[nodiscard]] const QPointer<Circle>& getCircle() const;
  [[nodiscard]] const QPointer<Cone>& getCone() const;
  [[nodiscard]] const QPointer<Cylinder>& getCylinder() const;
  [[nodiscard]] const QPointer<Ellipsoid>& getEllipsoid() const;
  [[nodiscard]] const QPointer<Hyperboloid>& getHyperboloid() const;
  [[nodiscard]] const QPointer<Paraboloid>& getParaboloid() const;
  [[nodiscard]] const QPointer<PointCloud>& getPointCloud() const;
  [[nodiscard]] const QPointer<Nurbs>& getNurbs() const;
  [[nodiscard]] const QPointer<Ellipse>& getEllipse() const;
  [[nodiscard]] const QPointer<SlottedHole>& getSlottedHole() const;
  [[nodiscard]] const QPointer<Torus>& getTorus() const;

  [[nodiscard]] const QPointer<ScalarEntityAngle>& getScalarEntityAngle() const;
  [[nodiscard]] const QPointer<ScalarEntityDistance>& getScalarEntityDistance()
      const;
  [[nodiscard]] const QPointer<ScalarEntityTemperature>&
  getScalarEntityTemperature() const;
  [[nodiscard]] const QPointer<ScalarEntityMeasurementSeries>&
  getScalarEntityMeasurementSeries() const;

  // ###################
  // set feature pointer
  // ###################

  void setCoordinateSystem(const QPointer<CoordinateSystem>& coordSystem);
  void setTrafoParam(const QPointer<TrafoParam>& trafoParam);
  void setStation(const QPointer<Station>& station);
  void setGroup(const QPointer<Group>& group);
  void setPointGroup(const QPointer<PointGroup>& pointGroup);

  void setPoint(const QPointer<Point>& point);
  void setLine(const QPointer<Line>& line);
  void setPlane(const QPointer<Plane>& plane);
  void setSphere(const QPointer<Sphere>& sphere);
  void setCircle(const QPointer<Circle>& circle);
  void setCone(const QPointer<Cone>& cone);
  void setCylinder(const QPointer<Cylinder>& cylinder);
  void setEllipsoid(const QPointer<Ellipsoid>& ellipsoid);
  void setHyperboloid(const QPointer<Hyperboloid>& hyperboloid);
  void setParaboloid(const QPointer<Paraboloid>& paraboloid);
  void setPointCloud(const QPointer<PointCloud>& pointCloud);
  void setNurbs(const QPointer<Nurbs>& nurbs);
  void setEllipse(const QPointer<Ellipse>& ellipse);
  void setSlottedHole(const QPointer<SlottedHole>& slottedHole);
  void setTorus(const QPointer<Torus>& torus);

  void setScalarEntityAngle(const QPointer<ScalarEntityAngle>& angle);
  void setScalarEntityDistance(const QPointer<ScalarEntityDistance>& distance);
  void setScalarEntityTemperature(
      const QPointer<ScalarEntityTemperature>& temperature);
  void setScalarEntityMeasurementSeries(
      const QPointer<ScalarEntityMeasurementSeries>& measurementSeries);

 private:
  // #################################################################
  // private feature pointer that hold a feature or are set to invalid
  // #################################################################

  QPointer<Feature> feature;
  QPointer<Geometry> geometry;

  QPointer<CoordinateSystem> coordinateSystem;
  QPointer<TrafoParam> trafoParam;

  QPointer<Station> station;

  QPointer<Point> point;
  QPointer<Line> line;
  QPointer<Plane> plane;
  QPointer<Sphere> sphere;
  QPointer<Circle> circle;
  QPointer<Cone> cone;
  QPointer<Cylinder> cylinder;
  QPointer<Ellipsoid> ellipsoid;
  QPointer<Hyperboloid> hyperboloid;
  QPointer<Paraboloid> paraboloid;
  QPointer<PointCloud> pointCloud;
  QPointer<Group> groupFeature;
  QPointer<PointGroup> pointGroup;
  QPointer<Nurbs> nurbs;
  QPointer<Ellipse> ellipse;
  QPointer<SlottedHole> slottedHole;
  QPointer<Torus> torus;
  QPointer<ScalarEntityAngle> angle;
  QPointer<ScalarEntityDistance> distance;
  QPointer<ScalarEntityTemperature> temperature;
  QPointer<ScalarEntityMeasurementSeries> measurementSeries;

  FeatureTypes typeOfFeature;
};

}  // namespace oi

Q_DECLARE_METATYPE(oi::FeatureWrapper*)

#endif  // FEATUREWRAPPER_H
