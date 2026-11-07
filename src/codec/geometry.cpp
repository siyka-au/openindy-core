// Conversions for geometries and groups.
//
// A geometry message is the state every geometry shares (position, direction,
// radius, statistic, references) plus a oneof naming the concrete type and
// holding that type's extra state. The old XML loader for six of the types
// could never succeed (its attribute checks were inverted), so none of them
// round-tripped; here each type is covered by a test.

#include <memory>

#include "access.h"
#include "convert.h"
#include "geometry/circle.h"
#include "geometry/cone.h"
#include "geometry/cylinder.h"
#include "geometry/ellipse.h"
#include "geometry/ellipsoid.h"
#include "geometry/hyperboloid.h"
#include "geometry/line.h"
#include "geometry/nurbs.h"
#include "geometry/paraboloid.h"
#include "geometry/plane.h"
#include "geometry/point.h"
#include "geometry/pointcloud.h"
#include "geometry/scalarentityangle.h"
#include "geometry/scalarentitydistance.h"
#include "geometry/scalarentitymeasurementseries.h"
#include "geometry/scalarentitytemperature.h"
#include "geometry/slottedhole.h"
#include "geometry/sphere.h"
#include "geometry/torus.h"
#include "observation.h"
#include "pointgroup.h"

namespace oi::codec {

namespace {

oipb::Vec3 vec3Of(const Position& p) { return vec3(p.getVector()); }
oipb::Vec3 vec3Of(const Direction& d) { return vec3(d.getVector()); }

Position positionOf(const oipb::Vec3& v) {
  return Position(v.x(), v.y(), v.z());
}
Direction directionOf(const oipb::Vec3& v) {
  return Direction(v.x(), v.y(), v.z());
}

}  // namespace

oipb::FeatureKind toProto(FeatureTypes type) {
  switch (type) {
    case FeatureTypes::eCircleFeature:
      return oipb::FEATURE_KIND_CIRCLE;
    case FeatureTypes::eConeFeature:
      return oipb::FEATURE_KIND_CONE;
    case FeatureTypes::eCylinderFeature:
      return oipb::FEATURE_KIND_CYLINDER;
    case FeatureTypes::eEllipseFeature:
      return oipb::FEATURE_KIND_ELLIPSE;
    case FeatureTypes::eEllipsoidFeature:
      return oipb::FEATURE_KIND_ELLIPSOID;
    case FeatureTypes::eHyperboloidFeature:
      return oipb::FEATURE_KIND_HYPERBOLOID;
    case FeatureTypes::eLineFeature:
      return oipb::FEATURE_KIND_LINE;
    case FeatureTypes::eNurbsFeature:
      return oipb::FEATURE_KIND_NURBS;
    case FeatureTypes::eParaboloidFeature:
      return oipb::FEATURE_KIND_PARABOLOID;
    case FeatureTypes::ePlaneFeature:
      return oipb::FEATURE_KIND_PLANE;
    case FeatureTypes::ePointFeature:
      return oipb::FEATURE_KIND_POINT;
    case FeatureTypes::ePointCloudFeature:
      return oipb::FEATURE_KIND_POINT_CLOUD;
    case FeatureTypes::eScalarEntityAngleFeature:
      return oipb::FEATURE_KIND_SCALAR_ANGLE;
    case FeatureTypes::eScalarEntityDistanceFeature:
      return oipb::FEATURE_KIND_SCALAR_DISTANCE;
    case FeatureTypes::eScalarEntityMeasurementSeriesFeature:
      return oipb::FEATURE_KIND_SCALAR_MEASUREMENT_SERIES;
    case FeatureTypes::eScalarEntityTemperatureFeature:
      return oipb::FEATURE_KIND_SCALAR_TEMPERATURE;
    case FeatureTypes::eSlottedHoleFeature:
      return oipb::FEATURE_KIND_SLOTTED_HOLE;
    case FeatureTypes::eSphereFeature:
      return oipb::FEATURE_KIND_SPHERE;
    case FeatureTypes::eTorusFeature:
      return oipb::FEATURE_KIND_TORUS;
    case FeatureTypes::eCoordinateSystemFeature:
      return oipb::FEATURE_KIND_COORDINATE_SYSTEM;
    case FeatureTypes::eStationFeature:
      return oipb::FEATURE_KIND_STATION;
    case FeatureTypes::eTrafoParamFeature:
      return oipb::FEATURE_KIND_TRAFO_PARAM;
    case FeatureTypes::eLevelFeature:
      return oipb::FEATURE_KIND_LEVEL;
    case FeatureTypes::eGroupFeature:
      return oipb::FEATURE_KIND_GROUP;
    case FeatureTypes::ePointGroupFeature:
      return oipb::FEATURE_KIND_POINT_GROUP;
    case FeatureTypes::eUndefinedFeature:
      break;
  }
  return oipb::FEATURE_KIND_UNSPECIFIED;
}

std::optional<FeatureTypes> fromProto(oipb::FeatureKind kind) {
  switch (kind) {
    case oipb::FEATURE_KIND_CIRCLE:
      return FeatureTypes::eCircleFeature;
    case oipb::FEATURE_KIND_CONE:
      return FeatureTypes::eConeFeature;
    case oipb::FEATURE_KIND_CYLINDER:
      return FeatureTypes::eCylinderFeature;
    case oipb::FEATURE_KIND_ELLIPSE:
      return FeatureTypes::eEllipseFeature;
    case oipb::FEATURE_KIND_ELLIPSOID:
      return FeatureTypes::eEllipsoidFeature;
    case oipb::FEATURE_KIND_HYPERBOLOID:
      return FeatureTypes::eHyperboloidFeature;
    case oipb::FEATURE_KIND_LINE:
      return FeatureTypes::eLineFeature;
    case oipb::FEATURE_KIND_NURBS:
      return FeatureTypes::eNurbsFeature;
    case oipb::FEATURE_KIND_PARABOLOID:
      return FeatureTypes::eParaboloidFeature;
    case oipb::FEATURE_KIND_PLANE:
      return FeatureTypes::ePlaneFeature;
    case oipb::FEATURE_KIND_POINT:
      return FeatureTypes::ePointFeature;
    case oipb::FEATURE_KIND_POINT_CLOUD:
      return FeatureTypes::ePointCloudFeature;
    case oipb::FEATURE_KIND_SCALAR_ANGLE:
      return FeatureTypes::eScalarEntityAngleFeature;
    case oipb::FEATURE_KIND_SCALAR_DISTANCE:
      return FeatureTypes::eScalarEntityDistanceFeature;
    case oipb::FEATURE_KIND_SCALAR_MEASUREMENT_SERIES:
      return FeatureTypes::eScalarEntityMeasurementSeriesFeature;
    case oipb::FEATURE_KIND_SCALAR_TEMPERATURE:
      return FeatureTypes::eScalarEntityTemperatureFeature;
    case oipb::FEATURE_KIND_SLOTTED_HOLE:
      return FeatureTypes::eSlottedHoleFeature;
    case oipb::FEATURE_KIND_SPHERE:
      return FeatureTypes::eSphereFeature;
    case oipb::FEATURE_KIND_TORUS:
      return FeatureTypes::eTorusFeature;
    case oipb::FEATURE_KIND_COORDINATE_SYSTEM:
      return FeatureTypes::eCoordinateSystemFeature;
    case oipb::FEATURE_KIND_STATION:
      return FeatureTypes::eStationFeature;
    case oipb::FEATURE_KIND_TRAFO_PARAM:
      return FeatureTypes::eTrafoParamFeature;
    case oipb::FEATURE_KIND_LEVEL:
      return FeatureTypes::eLevelFeature;
    case oipb::FEATURE_KIND_GROUP:
      return FeatureTypes::eGroupFeature;
    case oipb::FEATURE_KIND_POINT_GROUP:
      return FeatureTypes::ePointGroupFeature;
    default:
      break;
  }
  return std::nullopt;
}

namespace {

void encodeStatistic(const Statistic& s, oipb::GeometryStatistic* m) {
  m->set_valid(s.getIsValid());
  m->set_s0_apriori(s.getS0APriori());
  m->set_s0_aposteriori(s.getS0APosteriori());
  m->set_stdev(s.getStdev());
  m->set_form_error(s.getFormError());
}

void decodeStatistic(const oipb::GeometryStatistic& m, Statistic* s) {
  s->setIsValid(m.valid());
  s->setS0APriori(m.s0_apriori());
  s->setS0APosteriori(m.s0_aposteriori());
  s->setStdev(m.stdev());
  s->setFormError(m.form_error());
}

}  // namespace

// Builds a geometry of type T and restores the state every geometry shares.
template <class T>
oi::Guarded<Geometry> Access::restoreBase(const oipb::Geometry& m) {
  oi::Guarded<T> created = new T();
  // Address the shared members through Geometry: SlottedHole declares its own
  // `radius`, which would otherwise hide the base one.
  Geometry& geometry = *created;
  Access::restoreCommon(geometry, m.common());
  geometry.isCommon = m.is_common();
  geometry.xyz = positionOf(m.position());
  geometry.ijk = directionOf(m.direction());
  geometry.radius.setRadius(m.radius());
  decodeStatistic(m.statistic(), &geometry.statistic);
  if (m.has_measurement_config()) {
    geometry.activeMeasurementConfig = configKeyOf(m.measurement_config());
  }
  return created;
}

void Access::encodeGeometry(const Geometry& g, oipb::Geometry* m) {
  *m->mutable_common() = commonOf(g);
  m->set_is_common(g.isCommon);
  *m->mutable_position() = vec3Of(g.xyz);
  *m->mutable_direction() = vec3Of(g.ijk);
  m->set_radius(g.radius.getRadius());
  encodeStatistic(g.statistic, m->mutable_statistic());

  for (const auto& observation : g.observations) {
    if (!observation.isNull()) {
      m->add_observations(observation->getId());
    }
  }
  m->set_nominal_system(idOf(g.nominalSystem.data()));
  if (g.realises.has_value()) {
    m->set_realises(idOf(g.realises->data()));
  }
  if (g.ownerFeature.has_value()) {
    m->set_owner_feature(idOf(g.ownerFeature->data()));
  }
  // by name: a freshly created geometry has not had its config type resolved
  // yet (that happens when the project is linked), which makes the key
  // "invalid" although the name is there
  if (!g.activeMeasurementConfig.getName().isEmpty()) {
    *m->mutable_measurement_config() = configRefOf(g.activeMeasurementConfig);
  }

  if (dynamic_cast<const Circle*>(&g) != nullptr) {
    m->mutable_circle();
  } else if (const auto* cone = dynamic_cast<const Cone*>(&g)) {
    m->mutable_cone()->set_aperture(cone->aperture);
  } else if (dynamic_cast<const Cylinder*>(&g) != nullptr) {
    m->mutable_cylinder();
  } else if (const auto* ellipse = dynamic_cast<const Ellipse*>(&g)) {
    auto* shape = m->mutable_ellipse();
    shape->set_a(ellipse->a);
    shape->set_b(ellipse->b);
    *shape->mutable_semi_major_axis() = vec3Of(ellipse->semiMajorAxis);
  } else if (const auto* ellipsoid = dynamic_cast<const Ellipsoid*>(&g)) {
    auto* shape = m->mutable_ellipsoid();
    shape->set_a(ellipsoid->a);
    shape->set_b(ellipsoid->b);
  } else if (const auto* hyperboloid = dynamic_cast<const Hyperboloid*>(&g)) {
    auto* shape = m->mutable_hyperboloid();
    shape->set_a(hyperboloid->a);
    shape->set_c(hyperboloid->c);
  } else if (dynamic_cast<const Line*>(&g) != nullptr) {
    m->mutable_line();
  } else if (dynamic_cast<const Nurbs*>(&g) != nullptr) {
    m->mutable_nurbs();
  } else if (const auto* paraboloid = dynamic_cast<const Paraboloid*>(&g)) {
    m->mutable_paraboloid()->set_a(paraboloid->a);
  } else if (const auto* plane = dynamic_cast<const Plane*>(&g)) {
    m->mutable_plane()->set_distance_from_origin(plane->d);
  } else if (const auto* point = dynamic_cast<const Point*>(&g)) {
    auto* shape = m->mutable_point();
    for (const MeasurementIntent& intent : point->intents) {
      auto* entry = shape->add_intents();
      entry->set_feature_id(intent.featureId);
      entry->set_role(intent.roleName.toStdString());
    }
  } else if (const auto* cloud = dynamic_cast<const PointCloud*>(&g)) {
    auto* shape = m->mutable_point_cloud();
    for (const Point_PC* p : cloud->points) {
      shape->add_coordinates(p->xyz[0]);
      shape->add_coordinates(p->xyz[1]);
      shape->add_coordinates(p->xyz[2]);
      shape->add_used(p->isUsed);
    }
    for (int i = 0; i < 3; ++i) {
      shape->add_bbox_min(cloud->bbox.min[static_cast<std::size_t>(i)]);
      shape->add_bbox_max(cloud->bbox.max[static_cast<std::size_t>(i)]);
    }
    for (const auto& segment : cloud->detectedSegments) {
      if (!segment.isNull() && !segment->getFeature().isNull()) {
        shape->add_segments(segment->getFeature()->getId());
      }
    }
  } else if (const auto* angle = dynamic_cast<const ScalarEntityAngle*>(&g)) {
    m->mutable_scalar_angle()->set_angle(angle->angle);
  } else if (const auto* distance =
                 dynamic_cast<const ScalarEntityDistance*>(&g)) {
    m->mutable_scalar_distance()->set_distance(distance->distance);
  } else if (const auto* series =
                 dynamic_cast<const ScalarEntityMeasurementSeries*>(&g)) {
    m->mutable_scalar_measurement_series()->set_series_value(
        series->seriesValue);
  } else if (const auto* temperature =
                 dynamic_cast<const ScalarEntityTemperature*>(&g)) {
    m->mutable_scalar_temperature()->set_temperature(temperature->temperature);
  } else if (const auto* hole = dynamic_cast<const SlottedHole*>(&g)) {
    auto* shape = m->mutable_slotted_hole();
    *shape->mutable_center() = vec3Of(hole->center);
    *shape->mutable_normal() = vec3Of(hole->normal);
    shape->set_radius(hole->radius.getRadius());
    shape->set_length(hole->length);
    *shape->mutable_hole_axis() = vec3Of(hole->holeAxis);
    *shape->mutable_circle_center_a() = vec3Of(hole->circleCenterA);
    *shape->mutable_circle_center_b() = vec3Of(hole->circleCenterB);
  } else if (dynamic_cast<const Sphere*>(&g) != nullptr) {
    m->mutable_sphere();
  } else if (const auto* torus = dynamic_cast<const Torus*>(&g)) {
    m->mutable_torus()->set_radius_b(torus->radiusB.getRadius());
  }
}

oi::Guarded<Geometry> Access::decodeGeometry(const oipb::Geometry& m) {
  switch (m.shape_case()) {
    case oipb::Geometry::kCircle:
      return restoreBase<Circle>(m);
    case oipb::Geometry::kCone: {
      auto geometry = restoreBase<Cone>(m);
      static_cast<Cone*>(geometry.data())->aperture = m.cone().aperture();
      return geometry;
    }
    case oipb::Geometry::kCylinder:
      return restoreBase<Cylinder>(m);
    case oipb::Geometry::kEllipse: {
      auto geometry = restoreBase<Ellipse>(m);
      auto* ellipse = static_cast<Ellipse*>(geometry.data());
      ellipse->a = m.ellipse().a();
      ellipse->b = m.ellipse().b();
      ellipse->semiMajorAxis = directionOf(m.ellipse().semi_major_axis());
      return geometry;
    }
    case oipb::Geometry::kEllipsoid: {
      auto geometry = restoreBase<Ellipsoid>(m);
      auto* ellipsoid = static_cast<Ellipsoid*>(geometry.data());
      ellipsoid->a = m.ellipsoid().a();
      ellipsoid->b = m.ellipsoid().b();
      return geometry;
    }
    case oipb::Geometry::kHyperboloid: {
      auto geometry = restoreBase<Hyperboloid>(m);
      auto* hyperboloid = static_cast<Hyperboloid*>(geometry.data());
      hyperboloid->a = m.hyperboloid().a();
      hyperboloid->c = m.hyperboloid().c();
      return geometry;
    }
    case oipb::Geometry::kLine:
      return restoreBase<Line>(m);
    case oipb::Geometry::kNurbs:
      return restoreBase<Nurbs>(m);
    case oipb::Geometry::kParaboloid: {
      auto geometry = restoreBase<Paraboloid>(m);
      static_cast<Paraboloid*>(geometry.data())->a = m.paraboloid().a();
      return geometry;
    }
    case oipb::Geometry::kPlane: {
      auto geometry = restoreBase<Plane>(m);
      static_cast<Plane*>(geometry.data())->d =
          m.plane().distance_from_origin();
      return geometry;
    }
    case oipb::Geometry::kPoint: {
      auto geometry = restoreBase<Point>(m);
      auto* point = static_cast<Point*>(geometry.data());
      for (const auto& intent : m.point().intents()) {
        point->intents.push_back(MeasurementIntent(
            intent.feature_id(), QString::fromStdString(intent.role())));
      }
      return geometry;
    }
    case oipb::Geometry::kPointCloud: {
      auto geometry = restoreBase<PointCloud>(m);
      auto* cloud = static_cast<PointCloud*>(geometry.data());
      const auto& shape = m.point_cloud();
      const int count = shape.coordinates_size() / 3;
      for (int i = 0; i < count; ++i) {
        auto* p =
            new Point_PC(shape.coordinates(3 * i), shape.coordinates(3 * i + 1),
                         shape.coordinates(3 * i + 2));
        p->isUsed = i < shape.used_size() ? shape.used(i) : true;
        cloud->points.push_back(p);
      }
      if (shape.bbox_min_size() == 3 && shape.bbox_max_size() == 3) {
        for (int i = 0; i < 3; ++i) {
          cloud->bbox.min[static_cast<std::size_t>(i)] = shape.bbox_min(i);
          cloud->bbox.max[static_cast<std::size_t>(i)] = shape.bbox_max(i);
        }
      }
      return geometry;
    }
    case oipb::Geometry::kScalarAngle: {
      auto geometry = restoreBase<ScalarEntityAngle>(m);
      static_cast<ScalarEntityAngle*>(geometry.data())->angle =
          m.scalar_angle().angle();
      return geometry;
    }
    case oipb::Geometry::kScalarDistance: {
      auto geometry = restoreBase<ScalarEntityDistance>(m);
      static_cast<ScalarEntityDistance*>(geometry.data())->distance =
          m.scalar_distance().distance();
      return geometry;
    }
    case oipb::Geometry::kScalarMeasurementSeries: {
      auto geometry = restoreBase<ScalarEntityMeasurementSeries>(m);
      static_cast<ScalarEntityMeasurementSeries*>(geometry.data())
          ->seriesValue = m.scalar_measurement_series().series_value();
      return geometry;
    }
    case oipb::Geometry::kScalarTemperature: {
      auto geometry = restoreBase<ScalarEntityTemperature>(m);
      static_cast<ScalarEntityTemperature*>(geometry.data())->temperature =
          m.scalar_temperature().temperature();
      return geometry;
    }
    case oipb::Geometry::kSlottedHole: {
      auto geometry = restoreBase<SlottedHole>(m);
      auto* hole = static_cast<SlottedHole*>(geometry.data());
      const auto& shape = m.slotted_hole();
      hole->center = positionOf(shape.center());
      hole->normal = directionOf(shape.normal());
      hole->radius.setRadius(shape.radius());
      hole->length = shape.length();
      hole->holeAxis = directionOf(shape.hole_axis());
      hole->circleCenterA = positionOf(shape.circle_center_a());
      hole->circleCenterB = positionOf(shape.circle_center_b());
      return geometry;
    }
    case oipb::Geometry::kSphere:
      return restoreBase<Sphere>(m);
    case oipb::Geometry::kTorus: {
      auto geometry = restoreBase<Torus>(m);
      static_cast<Torus*>(geometry.data())
          ->radiusB.setRadius(m.torus().radius_b());
      return geometry;
    }
    case oipb::Geometry::SHAPE_NOT_SET:
      break;
  }
  return {};
}

void Access::encodeGroup(const Group& g, oipb::Group* m) {
  *m->mutable_common() = commonOf(g);
  if (g.constraint.has_value()) {
    m->set_constraint(toProto(g.constraint.value()));
  }
  appendIds(g.members, m->mutable_members());
  m->set_point_group(dynamic_cast<const PointGroup*>(&g) != nullptr);
}

oi::Guarded<Group> Access::decodeGroup(const oipb::Group& m) {
  oi::Guarded<Group> group;
  if (m.point_group()) {
    group = new PointGroup();
  } else {
    group = new Group();
  }
  restoreCommon(*group, m.common());
  if (!m.point_group()) {
    group->constraint = fromProto(m.constraint());
  }
  return group;
}

oipb::Geometry toProto(const Geometry& geometry) {
  oipb::Geometry m;
  Access::encodeGeometry(geometry, &m);
  return m;
}

oi::Guarded<Geometry> createGeometry(const oipb::Geometry& message) {
  return Access::decodeGeometry(message);
}

oipb::Group toProto(const Group& group) {
  oipb::Group m;
  Access::encodeGroup(group, &m);
  return m;
}

oi::Guarded<Group> createGroup(const oipb::Group& message) {
  return Access::decodeGroup(message);
}

}  // namespace oi::codec
