#include "point.h"

#include "featurewrapper.h"
#include "observation.h"
#include "snapshot.h"

using namespace oi;
using namespace oi::math;

/*!
 * \brief Point::Point
 */
Point::Point() : Geometry(), distance(-1.) {
  // set up feature wrapper
  if (!this->selfFeature.isNull()) {
    this->selfFeature->setPoint(this);
  }
}

/*!
 * \brief Point::Point
 * \param xyz
 */
Point::Point(const Position& xyz) : Geometry(), distance(-1.) {
  // set up feature wrapper
  if (!this->selfFeature.isNull()) {
    this->selfFeature->setPoint(this);
  }

  this->setPoint(xyz);
}

Point::Point(int id, const Position& xyz) : Geometry(), distance(-1.) {
  this->id = id;

  // set up feature wrapper
  if (!this->selfFeature.isNull()) {
    this->selfFeature->setPoint(this);
  }

  this->setPoint(xyz);
}

/*!
 * \brief Point::Point
 * \param copy
 */
Point::Point(const Point& copy) : Geometry(copy) {
  // set up feature wrapper
  if (!this->selfFeature.isNull()) {
    this->selfFeature->setPoint(this);
  }

  this->xyz = copy.xyz;
  this->distance = copy.distance;
}

/*!
 * \brief Point::operator =
 * \param copy
 * \return
 */
Point& Point::operator=(const Point& copy) {
  // set up feature wrapper
  if (!this->selfFeature.isNull()) {
    this->selfFeature->setPoint(this);
  }

  this->xyz = copy.xyz;
  this->distance = copy.distance;

  return *this;
}

/*!
 * \brief Point::~Point
 */
Point::~Point() {}

/*!
 * \brief Point::hasPosition
 * \return
 */
bool Point::hasPosition() const { return true; }

/*!
 * \brief Point::getPosition
 * Returns the point's position
 * \return
 */
const Position& Point::getPosition() const { return this->xyz; }

/*!
 * \brief Point::setPoint
 * \param xyz
 */
void Point::setPoint(const Position& xyz) {
  // set the given parameters
  this->xyz = xyz;

  this->notify(&FeatureObserver::onGeomParametersChanged, this->id);
}

/*!
 * \brief Point::setDistance
 * \param distance
 */
void Point::setDistance(const double distance) {
  this->distance = distance;

  this->notify(&FeatureObserver::onGeomParametersChanged, this->id);
}

/*!
 * \brief Point::getDistance depends on function
 * \return
 */
const double& Point::getDistance() const { return this->distance; }

/*!
 * \brief Point::getUnknownParameters
 * \param displayUnits
 * \param displayDigits
 * \return
 */
QMap<GeometryParameters, QString> Point::getUnknownParameters(
    const QMap<DimensionType, UnitType>& displayUnits,
    const QMap<DimensionType, int>& displayDigits) const {
  QMap<GeometryParameters, QString> parameters;

  parameters.insert(
      GeometryParameters::eUnknownX,
      this->getDisplayX(
          displayUnits.value(DimensionType::eMetric, UnitType::eUnitMeter),
          displayDigits.value(DimensionType::eMetric, 0)));
  parameters.insert(
      GeometryParameters::eUnknownY,
      this->getDisplayY(
          displayUnits.value(DimensionType::eMetric, UnitType::eUnitMeter),
          displayDigits.value(DimensionType::eMetric, 0)));
  parameters.insert(
      GeometryParameters::eUnknownZ,
      this->getDisplayZ(
          displayUnits.value(DimensionType::eMetric, UnitType::eUnitMeter),
          displayDigits.value(DimensionType::eMetric, 0)));

  return parameters;
}

/*!
 * \brief Point::applyUnknownParameters
 * \param parameters
 */
void Point::applyUnknownParameters(
    const QMap<GeometryParameters, double>& parameters) {
  // get current parameters
  OiVec position = this->xyz.getVector();

  // update parameters
  QList<GeometryParameters> keys = parameters.keys();
  for (const GeometryParameters& key : oi::snapshot(keys)) {
    switch (key) {
      case GeometryParameters::eUnknownX:
        position.setAt(0, parameters.value(GeometryParameters::eUnknownX));
        break;
      case GeometryParameters::eUnknownY:
        position.setAt(1, parameters.value(GeometryParameters::eUnknownY));
        break;
      case GeometryParameters::eUnknownZ:
        position.setAt(2, parameters.value(GeometryParameters::eUnknownZ));
        break;
      default:
        break;
    }
  }

  // update point definition
  Position pointPosition(position);
  this->setPoint(pointPosition);
}

/*!
 * \brief Point::recalc
 * D15a/D20: a station-owned point (no producer function, but carrying the
 * raw Observations one acquisition produced - see
 * OiJob::addMeasurementResults) computes its own position here, as the
 * centroid of its observations' xyz. This has to happen in recalc()
 * rather than at acquisition time: FeatureUpdater::recalcFeature (app
 * layer) transforms a feature's own observations into the job's active
 * coordinate system immediately before calling recalc() on it, and core
 * cannot depend on that app-level transform machinery directly. Geometry's
 * base recalc() leaves isSolved false for an empty functionList (D19's
 * "no producer function" case - Feature::recalc() only sets it inside the
 * function-execution loop), so this fills the same role a FitFunction's
 * exec() would for an owned geometry, without being one.
 */
void Point::recalc() {
  Geometry::recalc();

  if (!this->producerFunction.has_value() && !this->observations.isEmpty()) {
    OiVec centroid(3);
    centroid.setAt(0, 0.0);
    centroid.setAt(1, 0.0);
    centroid.setAt(2, 0.0);
    int usableCount = 0;

    for (const oi::Guarded<Observation>& observation :
         oi::snapshot(this->observations)) {
      if (observation.isNull() || !observation->getIsSolved()) {
        continue;
      }
      const OiVec& obsXyz = observation->getXYZ();
      if (obsXyz.getSize() < 3) {
        continue;
      }
      centroid.setAt(0, centroid.getAt(0) + obsXyz.getAt(0));
      centroid.setAt(1, centroid.getAt(1) + obsXyz.getAt(1));
      centroid.setAt(2, centroid.getAt(2) + obsXyz.getAt(2));
      ++usableCount;
    }

    if (usableCount > 0) {
      centroid.setAt(0, centroid.getAt(0) / usableCount);
      centroid.setAt(1, centroid.getAt(1) / usableCount);
      centroid.setAt(2, centroid.getAt(2) / usableCount);
      this->setPoint(Position(centroid));
      this->isSolved = true;
    }
  }

  // reset point definition if not solved and no nominal
  if (!this->isSolved && this->getOwnerDescription().has_value()) {
    this->xyz.setVector(0.0, 0.0, 0.0);
  }
}

/*!
 * \brief Point::getOwnerDescription
 * D19/D20: Geometry::getOwnerDescription only recognises a FitFunction at
 * functionList[0] - a Point deposited directly by the acquisition path
 * (OiJob::addMeasurementResults stamps its observations and tags but never
 * runs a Function::exec() on it) has an empty functionList, which the base
 * rule would misread as user-owned. A Point carrying observations with no
 * producer is a measurement, not a draft: it is owned by the station that
 * produced those observations. A Point with neither (freshly constructed,
 * or a derived Point built from other Points via ePointElement inputs -
 * AverageFromPoints, a fitted Plane/Line/Sphere/etc's own geometry, and so
 * on, none of which populate this Point's own observations list) falls
 * through to the base rule unchanged.
 * \return
 */
std::optional<QString> Point::getOwnerDescription() const {
  if (!this->producerFunction.has_value() && !this->observations.isEmpty()) {
    const oi::Guarded<Observation> firstObs = this->observations.constFirst();
    if (!firstObs.isNull() && !firstObs->getStation().isNull()) {
      return QString("its station \"%1\"")
          .arg(firstObs->getStation()->getFeatureName());
    }
    return QString("its station");
  }

  return Geometry::getOwnerDescription();
}

/*!
 * \brief Point::getIntents
 * D12: what this point was shot for (domain-model.md S3). See point.h.
 * \return
 */
const QList<MeasurementIntent>& Point::getIntents() const {
  return this->intents;
}

/*!
 * \brief Point::addIntent
 * \param intent
 */
void Point::addIntent(const MeasurementIntent& intent) {
  if (!this->intents.contains(intent)) {
    this->intents.append(intent);
  }
}

/*!
 * \brief Point::removeIntent
 * \param intent
 * \return
 */
bool Point::removeIntent(const MeasurementIntent& intent) {
  return this->intents.removeOne(intent);
}

/*!
 * \brief Point::hasIntent
 * \param featureId
 * \param roleName
 * \return
 */
bool Point::hasIntent(int featureId, const QString& roleName) const {
  return this->intents.contains(MeasurementIntent(featureId, roleName));
}

/*!
 * \brief Point::getDisplayDistance
 * \param type
 * \param digits
 * \param showDiff
 * \return
 */
QString Point::getDisplayDistance(const UnitType& type, const int& digits,
                                  const bool& /*showDiff*/) const {
  if (this->isSolved && this->distance > 0.) {
    return QString::number(convertFromDefault(this->distance, type), 'f',
                           digits);
  }
  return QString("");
}
