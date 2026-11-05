#include "featurewrapper.h"

using namespace oi;

/*!
 * \brief FeatureWrapper::FeatureWrapper
 * \param parent
 */
FeatureWrapper::FeatureWrapper(QObject* parent)
    : QObject(parent), typeOfFeature(FeatureTypes::eUndefinedFeature) {}

/*!
 * \brief FeatureWrapper::operator ==
 * Compare 2 FeatureWrappers by their underlying feature id
 * \param other
 * \return
 */
bool FeatureWrapper::operator==(const FeatureWrapper& other) {
  // check if both feature wrappers have got a valid feature
  if (this->feature.isNull() || other.getFeature().isNull()) {
    return false;
  }

  // compare the feature ids
  if (this->feature->getId() == other.getFeature()->getId()) {
    return true;
  }

  return false;
}

/*!
 * \brief FeatureWrapper::getFeatureTypeString
 * \return
 */
QString FeatureWrapper::getFeatureTypeString() const {
  return getFeatureTypeName(this->typeOfFeature);
}

/*!
 * \brief FeatureWrapper::getFeatureTypeEnum
 * \return
 */
const FeatureTypes& FeatureWrapper::getFeatureTypeEnum() const {
  return this->typeOfFeature;
}

/*!
 * \brief FeatureWrapper::getFeature
 * \return
 */
const QPointer<Feature>& FeatureWrapper::getFeature() const {
  return this->feature;
}

/*!
 * \brief FeatureWrapper::getGeometry
 * \return
 */
const QPointer<Geometry>& FeatureWrapper::getGeometry() const {
  return this->geometry;
}

/*!
 * \brief FeatureWrapper::getCoordinateSystem
 * \return
 */
const QPointer<CoordinateSystem>& FeatureWrapper::getCoordinateSystem() const {
  return this->coordinateSystem;
}

/*!
 * \brief FeatureWrapper::getTrafoParam
 * \return
 */
const QPointer<TrafoParam>& FeatureWrapper::getTrafoParam() const {
  return this->trafoParam;
}

/*!
 * \brief FeatureWrapper::getStation
 * \return
 */
const QPointer<Station>& FeatureWrapper::getStation() const {
  return this->station;
}

/*!
 * \brief FeatureWrapper::getPoint
 * \return
 */
const QPointer<Point>& FeatureWrapper::getPoint() const { return this->point; }

/*!
 * \brief FeatureWrapper::getLine
 * \return
 */
const QPointer<Line>& FeatureWrapper::getLine() const { return this->line; }

/*!
 * \brief FeatureWrapper::getPlane
 * \return
 */
const QPointer<Plane>& FeatureWrapper::getPlane() const { return this->plane; }

/*!
 * \brief FeatureWrapper::getSphere
 * \return
 */
const QPointer<Sphere>& FeatureWrapper::getSphere() const {
  return this->sphere;
}

/*!
 * \brief FeatureWrapper::getCircle
 * \return
 */
const QPointer<Circle>& FeatureWrapper::getCircle() const {
  return this->circle;
}

/*!
 * \brief FeatureWrapper::getCone
 * \return
 */
const QPointer<Cone>& FeatureWrapper::getCone() const { return this->cone; }

/*!
 * \brief FeatureWrapper::getCylinder
 * \return
 */
const QPointer<Cylinder>& FeatureWrapper::getCylinder() const {
  return this->cylinder;
}

/*!
 * \brief FeatureWrapper::getEllipsoid
 * \return
 */
const QPointer<Ellipsoid>& FeatureWrapper::getEllipsoid() const {
  return this->ellipsoid;
}

/*!
 * \brief FeatureWrapper::getHyperboloid
 * \return
 */
const QPointer<Hyperboloid>& FeatureWrapper::getHyperboloid() const {
  return this->hyperboloid;
}

/*!
 * \brief FeatureWrapper::getParaboloid
 * \return
 */
const QPointer<Paraboloid>& FeatureWrapper::getParaboloid() const {
  return this->paraboloid;
}

/*!
 * \brief FeatureWrapper::getGroup
 * \return
 */
const QPointer<Group>& FeatureWrapper::getGroup() const {
  return this->groupFeature;
}

/*!
 * \brief FeatureWrapper::getPointGroup
 * \return
 */
const QPointer<PointGroup>& FeatureWrapper::getPointGroup() const {
  return this->pointGroup;
}

/*!
 * \brief FeatureWrapper::getPointCloud
 * \return
 */
const QPointer<PointCloud>& FeatureWrapper::getPointCloud() const {
  return this->pointCloud;
}

/*!
 * \brief FeatureWrapper::getNurbs
 * \return
 */
const QPointer<Nurbs>& FeatureWrapper::getNurbs() const { return this->nurbs; }

/*!
 * \brief FeatureWrapper::getEllipse
 * \return
 */
const QPointer<Ellipse>& FeatureWrapper::getEllipse() const {
  return this->ellipse;
}

/*!
 * \brief FeatureWrapper::getSlottedHole
 * \return
 */
const QPointer<SlottedHole>& FeatureWrapper::getSlottedHole() const {
  return this->slottedHole;
}

/*!
 * \brief FeatureWrapper::getTorus
 * \return
 */
const QPointer<Torus>& FeatureWrapper::getTorus() const { return this->torus; }

/*!
 * \brief FeatureWrapper::getScalarEntityAngle
 * \return
 */
const QPointer<ScalarEntityAngle>& FeatureWrapper::getScalarEntityAngle()
    const {
  return this->angle;
}

/*!
 * \brief FeatureWrapper::getScalarEntityDistance
 * \return
 */
const QPointer<ScalarEntityDistance>& FeatureWrapper::getScalarEntityDistance()
    const {
  return this->distance;
}

/*!
 * \brief FeatureWrapper::getScalarEntityTemperature
 * \return
 */
const QPointer<ScalarEntityTemperature>&
FeatureWrapper::getScalarEntityTemperature() const {
  return this->temperature;
}

/*!
 * \brief FeatureWrapper::getScalarEntityMeasurementSeries
 * \return
 */
const QPointer<ScalarEntityMeasurementSeries>&
FeatureWrapper::getScalarEntityMeasurementSeries() const {
  return this->measurementSeries;
}

/*!
 * \brief FeatureWrapper::setCoordinateSystem
 * \param coordSystem
 */
void FeatureWrapper::setCoordinateSystem(
    const QPointer<CoordinateSystem>& coordSystem) {
  if (!coordSystem.isNull() &&
      this->typeOfFeature == FeatureTypes::eUndefinedFeature) {
    this->feature = coordSystem;
    this->coordinateSystem = coordSystem;
    this->typeOfFeature = FeatureTypes::eCoordinateSystemFeature;
  }
}

/*!
 * \brief FeatureWrapper::setTrafoParam
 * \param trafoParam
 */
void FeatureWrapper::setTrafoParam(const QPointer<TrafoParam>& trafoParam) {
  if (!trafoParam.isNull() &&
      this->typeOfFeature == FeatureTypes::eUndefinedFeature) {
    this->feature = trafoParam;
    this->trafoParam = trafoParam;
    this->typeOfFeature = FeatureTypes::eTrafoParamFeature;
  }
}

/*!
 * \brief FeatureWrapper::setStation
 * \param station
 */
void FeatureWrapper::setStation(const QPointer<Station>& station) {
  if (!station.isNull() &&
      this->typeOfFeature == FeatureTypes::eUndefinedFeature) {
    this->feature = station;
    this->station = station;
    this->typeOfFeature = FeatureTypes::eStationFeature;
  }
}

/*!
 * \brief FeatureWrapper::setPoint
 * \param point
 */
void FeatureWrapper::setPoint(const QPointer<Point>& point) {
  if (!point.isNull() &&
      this->typeOfFeature == FeatureTypes::eUndefinedFeature) {
    this->feature = point;
    this->geometry = point;
    this->point = point;
    this->typeOfFeature = FeatureTypes::ePointFeature;
  }
}

/*!
 * \brief FeatureWrapper::setLine
 * \param line
 */
void FeatureWrapper::setLine(const QPointer<Line>& line) {
  if (!line.isNull() &&
      this->typeOfFeature == FeatureTypes::eUndefinedFeature) {
    this->feature = line;
    this->geometry = line;
    this->line = line;
    this->typeOfFeature = FeatureTypes::eLineFeature;
  }
}

/*!
 * \brief FeatureWrapper::setPlane
 * \param plane
 */
void FeatureWrapper::setPlane(const QPointer<Plane>& plane) {
  if (!plane.isNull() &&
      this->typeOfFeature == FeatureTypes::eUndefinedFeature) {
    this->feature = plane;
    this->geometry = plane;
    this->plane = plane;
    this->typeOfFeature = FeatureTypes::ePlaneFeature;
  }
}

/*!
 * \brief FeatureWrapper::setSphere
 * \param sphere
 */
void FeatureWrapper::setSphere(const QPointer<Sphere>& sphere) {
  if (!sphere.isNull() &&
      this->typeOfFeature == FeatureTypes::eUndefinedFeature) {
    this->feature = sphere;
    this->geometry = sphere;
    this->sphere = sphere;
    this->typeOfFeature = FeatureTypes::eSphereFeature;
  }
}

/*!
 * \brief FeatureWrapper::setCircle
 * \param circle
 */
void FeatureWrapper::setCircle(const QPointer<Circle>& circle) {
  if (!circle.isNull() &&
      this->typeOfFeature == FeatureTypes::eUndefinedFeature) {
    this->feature = circle;
    this->geometry = circle;
    this->circle = circle;
    this->typeOfFeature = FeatureTypes::eCircleFeature;
  }
}

/*!
 * \brief FeatureWrapper::setCone
 * \param cone
 */
void FeatureWrapper::setCone(const QPointer<Cone>& cone) {
  if (!cone.isNull() &&
      this->typeOfFeature == FeatureTypes::eUndefinedFeature) {
    this->feature = cone;
    this->geometry = cone;
    this->cone = cone;
    this->typeOfFeature = FeatureTypes::eConeFeature;
  }
}

/*!
 * \brief FeatureWrapper::setCylinder
 * \param cylinder
 */
void FeatureWrapper::setCylinder(const QPointer<Cylinder>& cylinder) {
  if (!cylinder.isNull() &&
      this->typeOfFeature == FeatureTypes::eUndefinedFeature) {
    this->feature = cylinder;
    this->geometry = cylinder;
    this->cylinder = cylinder;
    this->typeOfFeature = FeatureTypes::eCylinderFeature;
  }
}

/*!
 * \brief FeatureWrapper::setEllipsoid
 * \param ellipsoid
 */
void FeatureWrapper::setEllipsoid(const QPointer<Ellipsoid>& ellipsoid) {
  if (!ellipsoid.isNull() &&
      this->typeOfFeature == FeatureTypes::eUndefinedFeature) {
    this->feature = ellipsoid;
    this->geometry = ellipsoid;
    this->ellipsoid = ellipsoid;
    this->typeOfFeature = FeatureTypes::eEllipsoidFeature;
  }
}

/*!
 * \brief FeatureWrapper::setHyperboloid
 * \param hyperboloid
 */
void FeatureWrapper::setHyperboloid(const QPointer<Hyperboloid>& hyperboloid) {
  if (!hyperboloid.isNull() &&
      this->typeOfFeature == FeatureTypes::eUndefinedFeature) {
    this->feature = hyperboloid;
    this->geometry = hyperboloid;
    this->hyperboloid = hyperboloid;
    this->typeOfFeature = FeatureTypes::eHyperboloidFeature;
  }
}

/*!
 * \brief FeatureWrapper::setParaboloid
 * \param paraboloid
 */
void FeatureWrapper::setParaboloid(const QPointer<Paraboloid>& paraboloid) {
  if (!paraboloid.isNull() &&
      this->typeOfFeature == FeatureTypes::eUndefinedFeature) {
    this->feature = paraboloid;
    this->geometry = paraboloid;
    this->paraboloid = paraboloid;
    this->typeOfFeature = FeatureTypes::eParaboloidFeature;
  }
}

/*!
 * \brief FeatureWrapper::setPointCloud
 * \param pointCloud
 */
void FeatureWrapper::setGroup(const QPointer<Group>& group) {
  if (!group.isNull() &&
      this->typeOfFeature == FeatureTypes::eUndefinedFeature) {
    this->feature = group;
    this->groupFeature = group;
    this->typeOfFeature = FeatureTypes::eGroupFeature;
  }
}

/*!
 * \brief FeatureWrapper::setPointGroup
 * \param pointGroup
 */
void FeatureWrapper::setPointGroup(const QPointer<PointGroup>& pointGroup) {
  if (!pointGroup.isNull() &&
      this->typeOfFeature == FeatureTypes::eUndefinedFeature) {
    this->feature = pointGroup;
    // a PointGroup is-a Group, so both handles resolve - a consumer that
    // only cares about grouping doesn't need to know which kind it got
    this->groupFeature = pointGroup;
    this->pointGroup = pointGroup;
    this->typeOfFeature = FeatureTypes::ePointGroupFeature;
  }
}

/*!
 * \brief FeatureWrapper::setPointCloud
 * \param pointCloud
 */
void FeatureWrapper::setPointCloud(const QPointer<PointCloud>& pointCloud) {
  if (!pointCloud.isNull() &&
      this->typeOfFeature == FeatureTypes::eUndefinedFeature) {
    this->feature = pointCloud;
    this->geometry = pointCloud;
    this->pointCloud = pointCloud;
    this->typeOfFeature = FeatureTypes::ePointCloudFeature;
  }
}

/*!
 * \brief FeatureWrapper::setNurbs
 * \param nurbs
 */
void FeatureWrapper::setNurbs(const QPointer<Nurbs>& nurbs) {
  if (!nurbs.isNull() &&
      this->typeOfFeature == FeatureTypes::eUndefinedFeature) {
    this->feature = nurbs;
    this->geometry = nurbs;
    this->nurbs = nurbs;
    this->typeOfFeature = FeatureTypes::eNurbsFeature;
  }
}

/*!
 * \brief FeatureWrapper::setEllipse
 * \param ellipse
 */
void FeatureWrapper::setEllipse(const QPointer<Ellipse>& ellipse) {
  if (!ellipse.isNull() &&
      this->typeOfFeature == FeatureTypes::eUndefinedFeature) {
    this->feature = ellipse;
    this->geometry = ellipse;
    this->ellipse = ellipse;
    this->typeOfFeature = FeatureTypes::eEllipseFeature;
  }
}

/*!
 * \brief FeatureWrapper::setSlottedHole
 * \param slottedHole
 */
void FeatureWrapper::setSlottedHole(const QPointer<SlottedHole>& slottedHole) {
  if (!slottedHole.isNull() &&
      this->typeOfFeature == FeatureTypes::eUndefinedFeature) {
    this->feature = slottedHole;
    this->geometry = slottedHole;
    this->slottedHole = slottedHole;
    this->typeOfFeature = FeatureTypes::eSlottedHoleFeature;
  }
}

/*!
 * \brief FeatureWrapper::setTorus
 * \param torus
 */
void FeatureWrapper::setTorus(const QPointer<Torus>& torus) {
  if (!torus.isNull() &&
      this->typeOfFeature == FeatureTypes::eUndefinedFeature) {
    this->feature = torus;
    this->geometry = torus;
    this->torus = torus;
    this->typeOfFeature = FeatureTypes::eTorusFeature;
  }
}

/*!
 * \brief FeatureWrapper::setScalarEntityAngle
 * \param angle
 */
void FeatureWrapper::setScalarEntityAngle(
    const QPointer<ScalarEntityAngle>& angle) {
  if (!angle.isNull() &&
      this->typeOfFeature == FeatureTypes::eUndefinedFeature) {
    this->feature = angle;
    this->geometry = angle;
    this->angle = angle;
    this->typeOfFeature = FeatureTypes::eScalarEntityAngleFeature;
  }
}

/*!
 * \brief FeatureWrapper::setScalarEntityDistance
 * \param distance
 */
void FeatureWrapper::setScalarEntityDistance(
    const QPointer<ScalarEntityDistance>& distance) {
  if (!distance.isNull() &&
      this->typeOfFeature == FeatureTypes::eUndefinedFeature) {
    this->feature = distance;
    this->geometry = distance;
    this->distance = distance;
    this->typeOfFeature = FeatureTypes::eScalarEntityDistanceFeature;
  }
}

/*!
 * \brief FeatureWrapper::setScalarEntityTemperature
 * \param temperature
 */
void FeatureWrapper::setScalarEntityTemperature(
    const QPointer<ScalarEntityTemperature>& temperature) {
  if (!temperature.isNull() &&
      this->typeOfFeature == FeatureTypes::eUndefinedFeature) {
    this->feature = temperature;
    this->geometry = temperature;
    this->temperature = temperature;
    this->typeOfFeature = FeatureTypes::eScalarEntityTemperatureFeature;
  }
}

/*!
 * \brief FeatureWrapper::setScalarEntityMeasurementSeries
 * \param measurementSeries
 */
void FeatureWrapper::setScalarEntityMeasurementSeries(
    const QPointer<ScalarEntityMeasurementSeries>& measurementSeries) {
  if (!measurementSeries.isNull() &&
      this->typeOfFeature == FeatureTypes::eUndefinedFeature) {
    this->feature = measurementSeries;
    this->geometry = measurementSeries;
    this->measurementSeries = measurementSeries;
    this->typeOfFeature = FeatureTypes::eScalarEntityMeasurementSeriesFeature;
  }
}
