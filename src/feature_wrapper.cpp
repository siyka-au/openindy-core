#include "feature_wrapper.h"

using namespace oi;

/*!
 * \brief FeatureWrapper::FeatureWrapper
 */
FeatureWrapper::FeatureWrapper()
    : typeOfFeature(FeatureTypes::eUndefinedFeature) {}

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
std::string FeatureWrapper::getFeatureTypeString() const {
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
const oi::Guarded<Feature>& FeatureWrapper::getFeature() const {
  return this->feature;
}

/*!
 * \brief FeatureWrapper::getGeometry
 * \return
 */
const oi::Guarded<Geometry>& FeatureWrapper::getGeometry() const {
  return this->geometry;
}

/*!
 * \brief FeatureWrapper::getCoordinateSystem
 * \return
 */
const oi::Guarded<CoordinateSystem>& FeatureWrapper::getCoordinateSystem()
    const {
  return this->coordinateSystem;
}

/*!
 * \brief FeatureWrapper::getTrafoParam
 * \return
 */
const oi::Guarded<TrafoParam>& FeatureWrapper::getTrafoParam() const {
  return this->trafoParam;
}

/*!
 * \brief FeatureWrapper::getStation
 * \return
 */
const oi::Guarded<Station>& FeatureWrapper::getStation() const {
  return this->station;
}

/*!
 * \brief FeatureWrapper::getPoint
 * \return
 */
const oi::Guarded<Point>& FeatureWrapper::getPoint() const {
  return this->point;
}

/*!
 * \brief FeatureWrapper::getLine
 * \return
 */
const oi::Guarded<Line>& FeatureWrapper::getLine() const { return this->line; }

/*!
 * \brief FeatureWrapper::getPlane
 * \return
 */
const oi::Guarded<Plane>& FeatureWrapper::getPlane() const {
  return this->plane;
}

/*!
 * \brief FeatureWrapper::getSphere
 * \return
 */
const oi::Guarded<Sphere>& FeatureWrapper::getSphere() const {
  return this->sphere;
}

/*!
 * \brief FeatureWrapper::getCircle
 * \return
 */
const oi::Guarded<Circle>& FeatureWrapper::getCircle() const {
  return this->circle;
}

/*!
 * \brief FeatureWrapper::getCone
 * \return
 */
const oi::Guarded<Cone>& FeatureWrapper::getCone() const { return this->cone; }

/*!
 * \brief FeatureWrapper::getCylinder
 * \return
 */
const oi::Guarded<Cylinder>& FeatureWrapper::getCylinder() const {
  return this->cylinder;
}

/*!
 * \brief FeatureWrapper::getEllipsoid
 * \return
 */
const oi::Guarded<Ellipsoid>& FeatureWrapper::getEllipsoid() const {
  return this->ellipsoid;
}

/*!
 * \brief FeatureWrapper::getHyperboloid
 * \return
 */
const oi::Guarded<Hyperboloid>& FeatureWrapper::getHyperboloid() const {
  return this->hyperboloid;
}

/*!
 * \brief FeatureWrapper::getParaboloid
 * \return
 */
const oi::Guarded<Paraboloid>& FeatureWrapper::getParaboloid() const {
  return this->paraboloid;
}

/*!
 * \brief FeatureWrapper::getGroup
 * \return
 */
const oi::Guarded<Group>& FeatureWrapper::getGroup() const {
  return this->groupFeature;
}

/*!
 * \brief FeatureWrapper::getPointGroup
 * \return
 */
const oi::Guarded<PointGroup>& FeatureWrapper::getPointGroup() const {
  return this->pointGroup;
}

/*!
 * \brief FeatureWrapper::getPointCloud
 * \return
 */
const oi::Guarded<PointCloud>& FeatureWrapper::getPointCloud() const {
  return this->pointCloud;
}

/*!
 * \brief FeatureWrapper::getNurbs
 * \return
 */
const oi::Guarded<Nurbs>& FeatureWrapper::getNurbs() const {
  return this->nurbs;
}

/*!
 * \brief FeatureWrapper::getEllipse
 * \return
 */
const oi::Guarded<Ellipse>& FeatureWrapper::getEllipse() const {
  return this->ellipse;
}

/*!
 * \brief FeatureWrapper::getSlottedHole
 * \return
 */
const oi::Guarded<SlottedHole>& FeatureWrapper::getSlottedHole() const {
  return this->slottedHole;
}

/*!
 * \brief FeatureWrapper::getTorus
 * \return
 */
const oi::Guarded<Torus>& FeatureWrapper::getTorus() const {
  return this->torus;
}

/*!
 * \brief FeatureWrapper::getScalarEntityAngle
 * \return
 */
const oi::Guarded<ScalarEntityAngle>& FeatureWrapper::getScalarEntityAngle()
    const {
  return this->angle;
}

/*!
 * \brief FeatureWrapper::getScalarEntityDistance
 * \return
 */
const oi::Guarded<ScalarEntityDistance>&
FeatureWrapper::getScalarEntityDistance() const {
  return this->distance;
}

/*!
 * \brief FeatureWrapper::getScalarEntityTemperature
 * \return
 */
const oi::Guarded<ScalarEntityTemperature>&
FeatureWrapper::getScalarEntityTemperature() const {
  return this->temperature;
}

/*!
 * \brief FeatureWrapper::getScalarEntityMeasurementSeries
 * \return
 */
const oi::Guarded<ScalarEntityMeasurementSeries>&
FeatureWrapper::getScalarEntityMeasurementSeries() const {
  return this->measurementSeries;
}

/*!
 * \brief FeatureWrapper::setCoordinateSystem
 * \param coordSystem
 */
void FeatureWrapper::setCoordinateSystem(
    const oi::Guarded<CoordinateSystem>& coordSystem) {
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
void FeatureWrapper::setTrafoParam(const oi::Guarded<TrafoParam>& trafoParam) {
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
void FeatureWrapper::setStation(const oi::Guarded<Station>& station) {
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
void FeatureWrapper::setPoint(const oi::Guarded<Point>& point) {
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
void FeatureWrapper::setLine(const oi::Guarded<Line>& line) {
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
void FeatureWrapper::setPlane(const oi::Guarded<Plane>& plane) {
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
void FeatureWrapper::setSphere(const oi::Guarded<Sphere>& sphere) {
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
void FeatureWrapper::setCircle(const oi::Guarded<Circle>& circle) {
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
void FeatureWrapper::setCone(const oi::Guarded<Cone>& cone) {
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
void FeatureWrapper::setCylinder(const oi::Guarded<Cylinder>& cylinder) {
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
void FeatureWrapper::setEllipsoid(const oi::Guarded<Ellipsoid>& ellipsoid) {
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
void FeatureWrapper::setHyperboloid(
    const oi::Guarded<Hyperboloid>& hyperboloid) {
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
void FeatureWrapper::setParaboloid(const oi::Guarded<Paraboloid>& paraboloid) {
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
void FeatureWrapper::setGroup(const oi::Guarded<Group>& group) {
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
void FeatureWrapper::setPointGroup(const oi::Guarded<PointGroup>& pointGroup) {
  // a PointGroup is built on a Group, whose constructor has already tagged
  // the wrapper as a plain group: that is upgraded here
  if (!pointGroup.isNull() &&
      (this->typeOfFeature == FeatureTypes::eUndefinedFeature ||
       (this->typeOfFeature == FeatureTypes::eGroupFeature &&
        this->feature.data() == pointGroup.data()))) {
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
void FeatureWrapper::setPointCloud(const oi::Guarded<PointCloud>& pointCloud) {
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
void FeatureWrapper::setNurbs(const oi::Guarded<Nurbs>& nurbs) {
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
void FeatureWrapper::setEllipse(const oi::Guarded<Ellipse>& ellipse) {
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
void FeatureWrapper::setSlottedHole(
    const oi::Guarded<SlottedHole>& slottedHole) {
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
void FeatureWrapper::setTorus(const oi::Guarded<Torus>& torus) {
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
    const oi::Guarded<ScalarEntityAngle>& angle) {
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
    const oi::Guarded<ScalarEntityDistance>& distance) {
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
    const oi::Guarded<ScalarEntityTemperature>& temperature) {
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
    const oi::Guarded<ScalarEntityMeasurementSeries>& measurementSeries) {
  if (!measurementSeries.isNull() &&
      this->typeOfFeature == FeatureTypes::eUndefinedFeature) {
    this->feature = measurementSeries;
    this->geometry = measurementSeries;
    this->measurementSeries = measurementSeries;
    this->typeOfFeature = FeatureTypes::eScalarEntityMeasurementSeriesFeature;
  }
}
