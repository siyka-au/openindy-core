// Conversions for function records: which function, its inputs, declared
// pipelines and parameters.
#include <iterator>
#include <map>
#include <optional>
#include <vector>

#include "access.h"
#include "containers.h"
#include "convert.h"
#include "wire.h"

namespace oi::codec {

namespace {

oipb::PipelineStageKind toProto(PipelineStageType type) {
  switch (type) {
    case PipelineStageType::eExplicitListStage:
      return oipb::PIPELINE_STAGE_KIND_EXPLICIT_LIST;
    case PipelineStageType::eExcludeStage:
      return oipb::PIPELINE_STAGE_KIND_EXCLUDE;
    case PipelineStageType::eResidualOutlierRejectionStage:
      return oipb::PIPELINE_STAGE_KIND_RESIDUAL_OUTLIER_REJECTION;
  }
  return oipb::PIPELINE_STAGE_KIND_UNSPECIFIED;
}

std::optional<PipelineStageType> stageTypeOf(oipb::PipelineStageKind kind) {
  switch (kind) {
    case oipb::PIPELINE_STAGE_KIND_EXPLICIT_LIST:
      return PipelineStageType::eExplicitListStage;
    case oipb::PIPELINE_STAGE_KIND_EXCLUDE:
      return PipelineStageType::eExcludeStage;
    case oipb::PIPELINE_STAGE_KIND_RESIDUAL_OUTLIER_REJECTION:
      return PipelineStageType::eResidualOutlierRejectionStage;
    default:
      break;
  }
  return std::nullopt;
}

oipb::ElementKind toProto(ElementTypes value) {
  switch (value) {
    case ElementTypes::eCircleElement:
      return oipb::ELEMENT_KIND_CIRCLE;
    case ElementTypes::eConeElement:
      return oipb::ELEMENT_KIND_CONE;
    case ElementTypes::eCylinderElement:
      return oipb::ELEMENT_KIND_CYLINDER;
    case ElementTypes::eEllipseElement:
      return oipb::ELEMENT_KIND_ELLIPSE;
    case ElementTypes::eEllipsoidElement:
      return oipb::ELEMENT_KIND_ELLIPSOID;
    case ElementTypes::eHyperboloidElement:
      return oipb::ELEMENT_KIND_HYPERBOLOID;
    case ElementTypes::eLineElement:
      return oipb::ELEMENT_KIND_LINE;
    case ElementTypes::eNurbsElement:
      return oipb::ELEMENT_KIND_NURBS;
    case ElementTypes::eParaboloidElement:
      return oipb::ELEMENT_KIND_PARABOLOID;
    case ElementTypes::ePlaneElement:
      return oipb::ELEMENT_KIND_PLANE;
    case ElementTypes::ePointElement:
      return oipb::ELEMENT_KIND_POINT;
    case ElementTypes::ePointCloudElement:
      return oipb::ELEMENT_KIND_POINT_CLOUD;
    case ElementTypes::eScalarEntityAngleElement:
      return oipb::ELEMENT_KIND_SCALAR_ENTITY_ANGLE;
    case ElementTypes::eScalarEntityDistanceElement:
      return oipb::ELEMENT_KIND_SCALAR_ENTITY_DISTANCE;
    case ElementTypes::eScalarEntityMeasurementSeriesElement:
      return oipb::ELEMENT_KIND_SCALAR_ENTITY_MEASUREMENT_SERIES;
    case ElementTypes::eScalarEntityTemperatureElement:
      return oipb::ELEMENT_KIND_SCALAR_ENTITY_TEMPERATURE;
    case ElementTypes::eSlottedHoleElement:
      return oipb::ELEMENT_KIND_SLOTTED_HOLE;
    case ElementTypes::eSphereElement:
      return oipb::ELEMENT_KIND_SPHERE;
    case ElementTypes::eTorusElement:
      return oipb::ELEMENT_KIND_TORUS;
    case ElementTypes::eDirectionElement:
      return oipb::ELEMENT_KIND_DIRECTION;
    case ElementTypes::ePositionElement:
      return oipb::ELEMENT_KIND_POSITION;
    case ElementTypes::eRadiusElement:
      return oipb::ELEMENT_KIND_RADIUS;
    case ElementTypes::eCoordinateSystemElement:
      return oipb::ELEMENT_KIND_COORDINATE_SYSTEM;
    case ElementTypes::eStationElement:
      return oipb::ELEMENT_KIND_STATION;
    case ElementTypes::eTrafoParamElement:
      return oipb::ELEMENT_KIND_TRAFO_PARAM;
    case ElementTypes::eObservationElement:
      return oipb::ELEMENT_KIND_OBSERVATION;
    case ElementTypes::eReadingCartesianElement:
      return oipb::ELEMENT_KIND_READING_CARTESIAN;
    case ElementTypes::eReadingPolarElement:
      return oipb::ELEMENT_KIND_READING_POLAR;
    case ElementTypes::eReadingDistanceElement:
      return oipb::ELEMENT_KIND_READING_DISTANCE;
    case ElementTypes::eReadingDirectionElement:
      return oipb::ELEMENT_KIND_READING_DIRECTION;
    case ElementTypes::eReadingTemperatureElement:
      return oipb::ELEMENT_KIND_READING_TEMPERATURE;
    case ElementTypes::eReadingLevelElement:
      return oipb::ELEMENT_KIND_READING_LEVEL;
    case ElementTypes::eUndefinedElement:
      return oipb::ELEMENT_KIND_UNDEFINED;
    case ElementTypes::eLevelElement:
      return oipb::ELEMENT_KIND_LEVEL;
    case ElementTypes::ePointCollectionElement:
      return oipb::ELEMENT_KIND_POINT_COLLECTION;
  }
  return oipb::ELEMENT_KIND_UNSPECIFIED;
}

std::optional<ElementTypes> elementTypeOf(oipb::ElementKind value) {
  switch (value) {
    case oipb::ELEMENT_KIND_CIRCLE:
      return ElementTypes::eCircleElement;
    case oipb::ELEMENT_KIND_CONE:
      return ElementTypes::eConeElement;
    case oipb::ELEMENT_KIND_CYLINDER:
      return ElementTypes::eCylinderElement;
    case oipb::ELEMENT_KIND_ELLIPSE:
      return ElementTypes::eEllipseElement;
    case oipb::ELEMENT_KIND_ELLIPSOID:
      return ElementTypes::eEllipsoidElement;
    case oipb::ELEMENT_KIND_HYPERBOLOID:
      return ElementTypes::eHyperboloidElement;
    case oipb::ELEMENT_KIND_LINE:
      return ElementTypes::eLineElement;
    case oipb::ELEMENT_KIND_NURBS:
      return ElementTypes::eNurbsElement;
    case oipb::ELEMENT_KIND_PARABOLOID:
      return ElementTypes::eParaboloidElement;
    case oipb::ELEMENT_KIND_PLANE:
      return ElementTypes::ePlaneElement;
    case oipb::ELEMENT_KIND_POINT:
      return ElementTypes::ePointElement;
    case oipb::ELEMENT_KIND_POINT_CLOUD:
      return ElementTypes::ePointCloudElement;
    case oipb::ELEMENT_KIND_SCALAR_ENTITY_ANGLE:
      return ElementTypes::eScalarEntityAngleElement;
    case oipb::ELEMENT_KIND_SCALAR_ENTITY_DISTANCE:
      return ElementTypes::eScalarEntityDistanceElement;
    case oipb::ELEMENT_KIND_SCALAR_ENTITY_MEASUREMENT_SERIES:
      return ElementTypes::eScalarEntityMeasurementSeriesElement;
    case oipb::ELEMENT_KIND_SCALAR_ENTITY_TEMPERATURE:
      return ElementTypes::eScalarEntityTemperatureElement;
    case oipb::ELEMENT_KIND_SLOTTED_HOLE:
      return ElementTypes::eSlottedHoleElement;
    case oipb::ELEMENT_KIND_SPHERE:
      return ElementTypes::eSphereElement;
    case oipb::ELEMENT_KIND_TORUS:
      return ElementTypes::eTorusElement;
    case oipb::ELEMENT_KIND_DIRECTION:
      return ElementTypes::eDirectionElement;
    case oipb::ELEMENT_KIND_POSITION:
      return ElementTypes::ePositionElement;
    case oipb::ELEMENT_KIND_RADIUS:
      return ElementTypes::eRadiusElement;
    case oipb::ELEMENT_KIND_COORDINATE_SYSTEM:
      return ElementTypes::eCoordinateSystemElement;
    case oipb::ELEMENT_KIND_STATION:
      return ElementTypes::eStationElement;
    case oipb::ELEMENT_KIND_TRAFO_PARAM:
      return ElementTypes::eTrafoParamElement;
    case oipb::ELEMENT_KIND_OBSERVATION:
      return ElementTypes::eObservationElement;
    case oipb::ELEMENT_KIND_READING_CARTESIAN:
      return ElementTypes::eReadingCartesianElement;
    case oipb::ELEMENT_KIND_READING_POLAR:
      return ElementTypes::eReadingPolarElement;
    case oipb::ELEMENT_KIND_READING_DISTANCE:
      return ElementTypes::eReadingDistanceElement;
    case oipb::ELEMENT_KIND_READING_DIRECTION:
      return ElementTypes::eReadingDirectionElement;
    case oipb::ELEMENT_KIND_READING_TEMPERATURE:
      return ElementTypes::eReadingTemperatureElement;
    case oipb::ELEMENT_KIND_READING_LEVEL:
      return ElementTypes::eReadingLevelElement;
    case oipb::ELEMENT_KIND_UNDEFINED:
      return ElementTypes::eUndefinedElement;
    case oipb::ELEMENT_KIND_LEVEL:
      return ElementTypes::eLevelElement;
    case oipb::ELEMENT_KIND_POINT_COLLECTION:
      return ElementTypes::ePointCollectionElement;
    default:
      break;
  }
  return std::nullopt;
}

oipb::GeometryParameter toProto(GeometryParameters value) {
  switch (value) {
    case GeometryParameters::eUnknownX:
      return oipb::GEOMETRY_PARAMETER_UNKNOWN_X;
    case GeometryParameters::eUnknownY:
      return oipb::GEOMETRY_PARAMETER_UNKNOWN_Y;
    case GeometryParameters::eUnknownZ:
      return oipb::GEOMETRY_PARAMETER_UNKNOWN_Z;
    case GeometryParameters::eUnknownPrimaryI:
      return oipb::GEOMETRY_PARAMETER_UNKNOWN_PRIMARY_I;
    case GeometryParameters::eUnknownPrimaryJ:
      return oipb::GEOMETRY_PARAMETER_UNKNOWN_PRIMARY_J;
    case GeometryParameters::eUnknownPrimaryK:
      return oipb::GEOMETRY_PARAMETER_UNKNOWN_PRIMARY_K;
    case GeometryParameters::eUnknownSecondaryI:
      return oipb::GEOMETRY_PARAMETER_UNKNOWN_SECONDARY_I;
    case GeometryParameters::eUnknownSecondaryJ:
      return oipb::GEOMETRY_PARAMETER_UNKNOWN_SECONDARY_J;
    case GeometryParameters::eUnknownSecondaryK:
      return oipb::GEOMETRY_PARAMETER_UNKNOWN_SECONDARY_K;
    case GeometryParameters::eUnknownRadiusA:
      return oipb::GEOMETRY_PARAMETER_UNKNOWN_RADIUS_A;
    case GeometryParameters::eUnknownRadiusB:
      return oipb::GEOMETRY_PARAMETER_UNKNOWN_RADIUS_B;
    case GeometryParameters::eUnknownAperture:
      return oipb::GEOMETRY_PARAMETER_UNKNOWN_APERTURE;
    case GeometryParameters::eUnknownA:
      return oipb::GEOMETRY_PARAMETER_UNKNOWN_A;
    case GeometryParameters::eUnknownB:
      return oipb::GEOMETRY_PARAMETER_UNKNOWN_B;
    case GeometryParameters::eUnknownC:
      return oipb::GEOMETRY_PARAMETER_UNKNOWN_C;
    case GeometryParameters::eUnknownAngle:
      return oipb::GEOMETRY_PARAMETER_UNKNOWN_ANGLE;
    case GeometryParameters::eUnknownDistance:
      return oipb::GEOMETRY_PARAMETER_UNKNOWN_DISTANCE;
    case GeometryParameters::eUnknownMeasurementSeries:
      return oipb::GEOMETRY_PARAMETER_UNKNOWN_MEASUREMENT_SERIES;
    case GeometryParameters::eUnknownTemperature:
      return oipb::GEOMETRY_PARAMETER_UNKNOWN_TEMPERATURE;
    case GeometryParameters::eUnknownLength:
      return oipb::GEOMETRY_PARAMETER_UNKNOWN_LENGTH;
  }
  return oipb::GEOMETRY_PARAMETER_UNSPECIFIED;
}

std::optional<GeometryParameters> parameterOf(oipb::GeometryParameter value) {
  switch (value) {
    case oipb::GEOMETRY_PARAMETER_UNKNOWN_X:
      return GeometryParameters::eUnknownX;
    case oipb::GEOMETRY_PARAMETER_UNKNOWN_Y:
      return GeometryParameters::eUnknownY;
    case oipb::GEOMETRY_PARAMETER_UNKNOWN_Z:
      return GeometryParameters::eUnknownZ;
    case oipb::GEOMETRY_PARAMETER_UNKNOWN_PRIMARY_I:
      return GeometryParameters::eUnknownPrimaryI;
    case oipb::GEOMETRY_PARAMETER_UNKNOWN_PRIMARY_J:
      return GeometryParameters::eUnknownPrimaryJ;
    case oipb::GEOMETRY_PARAMETER_UNKNOWN_PRIMARY_K:
      return GeometryParameters::eUnknownPrimaryK;
    case oipb::GEOMETRY_PARAMETER_UNKNOWN_SECONDARY_I:
      return GeometryParameters::eUnknownSecondaryI;
    case oipb::GEOMETRY_PARAMETER_UNKNOWN_SECONDARY_J:
      return GeometryParameters::eUnknownSecondaryJ;
    case oipb::GEOMETRY_PARAMETER_UNKNOWN_SECONDARY_K:
      return GeometryParameters::eUnknownSecondaryK;
    case oipb::GEOMETRY_PARAMETER_UNKNOWN_RADIUS_A:
      return GeometryParameters::eUnknownRadiusA;
    case oipb::GEOMETRY_PARAMETER_UNKNOWN_RADIUS_B:
      return GeometryParameters::eUnknownRadiusB;
    case oipb::GEOMETRY_PARAMETER_UNKNOWN_APERTURE:
      return GeometryParameters::eUnknownAperture;
    case oipb::GEOMETRY_PARAMETER_UNKNOWN_A:
      return GeometryParameters::eUnknownA;
    case oipb::GEOMETRY_PARAMETER_UNKNOWN_B:
      return GeometryParameters::eUnknownB;
    case oipb::GEOMETRY_PARAMETER_UNKNOWN_C:
      return GeometryParameters::eUnknownC;
    case oipb::GEOMETRY_PARAMETER_UNKNOWN_ANGLE:
      return GeometryParameters::eUnknownAngle;
    case oipb::GEOMETRY_PARAMETER_UNKNOWN_DISTANCE:
      return GeometryParameters::eUnknownDistance;
    case oipb::GEOMETRY_PARAMETER_UNKNOWN_MEASUREMENT_SERIES:
      return GeometryParameters::eUnknownMeasurementSeries;
    case oipb::GEOMETRY_PARAMETER_UNKNOWN_TEMPERATURE:
      return GeometryParameters::eUnknownTemperature;
    case oipb::GEOMETRY_PARAMETER_UNKNOWN_LENGTH:
      return GeometryParameters::eUnknownLength;
    default:
      break;
  }
  return std::nullopt;
}

oipb::TrafoParameter toProto(TrafoParamParameters value) {
  switch (value) {
    case TrafoParamParameters::eUnknownTX:
      return oipb::TRAFO_PARAMETER_TRANSLATION_X;
    case TrafoParamParameters::eUnknownTY:
      return oipb::TRAFO_PARAMETER_TRANSLATION_Y;
    case TrafoParamParameters::eUnknownTZ:
      return oipb::TRAFO_PARAMETER_TRANSLATION_Z;
    case TrafoParamParameters::eUnknownRX:
      return oipb::TRAFO_PARAMETER_ROTATION_X;
    case TrafoParamParameters::eUnknownRY:
      return oipb::TRAFO_PARAMETER_ROTATION_Y;
    case TrafoParamParameters::eUnknownRZ:
      return oipb::TRAFO_PARAMETER_ROTATION_Z;
    case TrafoParamParameters::eUnknownSX:
      return oipb::TRAFO_PARAMETER_SCALE_X;
    case TrafoParamParameters::eUnknownSY:
      return oipb::TRAFO_PARAMETER_SCALE_Y;
    case TrafoParamParameters::eUnknownSZ:
      return oipb::TRAFO_PARAMETER_SCALE_Z;
  }
  return oipb::TRAFO_PARAMETER_UNSPECIFIED;
}

std::optional<TrafoParamParameters> trafoParameterOf(
    oipb::TrafoParameter value) {
  switch (value) {
    case oipb::TRAFO_PARAMETER_TRANSLATION_X:
      return TrafoParamParameters::eUnknownTX;
    case oipb::TRAFO_PARAMETER_TRANSLATION_Y:
      return TrafoParamParameters::eUnknownTY;
    case oipb::TRAFO_PARAMETER_TRANSLATION_Z:
      return TrafoParamParameters::eUnknownTZ;
    case oipb::TRAFO_PARAMETER_ROTATION_X:
      return TrafoParamParameters::eUnknownRX;
    case oipb::TRAFO_PARAMETER_ROTATION_Y:
      return TrafoParamParameters::eUnknownRY;
    case oipb::TRAFO_PARAMETER_ROTATION_Z:
      return TrafoParamParameters::eUnknownRZ;
    case oipb::TRAFO_PARAMETER_SCALE_X:
      return TrafoParamParameters::eUnknownSX;
    case oipb::TRAFO_PARAMETER_SCALE_Y:
      return TrafoParamParameters::eUnknownSY;
    case oipb::TRAFO_PARAMETER_SCALE_Z:
      return TrafoParamParameters::eUnknownSZ;
    default:
      break;
  }
  return std::nullopt;
}

void addParameters(const std::vector<GeometryParameters>& source,
                   google::protobuf::RepeatedField<int>* out) {
  for (GeometryParameters parameter : source) {
    out->Add(toProto(parameter));
  }
}

std::vector<GeometryParameters> parametersOf(
    const google::protobuf::RepeatedField<int>& source) {
  std::vector<GeometryParameters> out;
  for (int value : source) {
    if (const auto parameter =
            parameterOf(static_cast<oipb::GeometryParameter>(value))) {
      out.push_back(*parameter);
    }
  }
  return out;
}

void encodeBundleGeometry(const BundleGeometry& g, oipb::BundleGeometry* m) {
  m->set_id(g.id);
  for (auto it = g.parameters.cbegin(); it != g.parameters.cend(); ++it) {
    auto* p = m->add_parameters();
    p->set_parameter(toProto(it->first));
    p->set_value(it->second);
  }
}

BundleGeometry bundleGeometryOf(const oipb::BundleGeometry& m) {
  BundleGeometry g;
  g.id = m.id();
  for (const auto& p : m.parameters()) {
    if (const auto parameter = parameterOf(p.parameter())) {
      g.parameters.insert_or_assign(*parameter, p.value());
    }
  }
  return g;
}

void encodeBundleStation(const BundleStation& s, oipb::BundleStation* m) {
  m->set_id(s.id);
  m->set_tx(s.tx);
  m->set_ty(s.ty);
  m->set_tz(s.tz);
  m->set_rx(s.rx);
  m->set_ry(s.ry);
  m->set_rz(s.rz);
  m->set_m(s.m);
  for (const BundleGeometry& g : s.geometries) {
    encodeBundleGeometry(g, m->add_geometries());
  }
}

BundleStation bundleStationOf(const oipb::BundleStation& m) {
  BundleStation s;
  s.id = m.id();
  s.tx = m.tx();
  s.ty = m.ty();
  s.tz = m.tz();
  s.rx = m.rx();
  s.ry = m.ry();
  s.rz = m.rz();
  s.m = m.m();
  for (const auto& g : m.geometries()) {
    s.geometries.push_back(bundleGeometryOf(g));
  }
  return s;
}

}  // namespace

void Access::encodeFunction(const Function& f, oipb::FunctionRecord* m) {
  *m->mutable_plugin() = pluginRef(f.metaData);

  for (auto it = f.inputElements.cbegin(); it != f.inputElements.cend(); ++it) {
    for (const InputElement& input : it->second) {
      auto* entry = m->add_inputs();
      entry->set_position(it->first);
      entry->set_element_id(input.id);
      entry->set_kind(toProto(input.typeOfElement));
      entry->set_label(input.label.toStdString());
      addParameters(input.ignoredDestinationParams,
                    entry->mutable_ignored_destination());
      addParameters(input.ignoredTargetParams, entry->mutable_ignored_target());
      entry->set_should_be_used(input.shouldBeUsed);
      entry->set_is_used(input.isUsed);
    }
  }

  for (int position = 0; position < std::ssize(f.neededElements); ++position) {
    const std::vector<PipelineStage>& stages =
        f.neededElements.at(position).pipeline;
    if (stages.empty()) {
      continue;
    }
    auto* pipeline = m->add_pipelines();
    pipeline->set_position(position);
    for (const PipelineStage& stage : stages) {
      auto* entry = pipeline->add_stages();
      entry->set_kind(toProto(stage.type));
      for (int id : stage.explicitIds) {
        entry->add_explicit_ids(id);
      }
      for (const PipelineExclusion& exclusion : stage.explicitExclusions) {
        auto* e = entry->add_exclusions();
        e->set_element_id(exclusion.elementId);
        e->set_reason(exclusion.reason.toStdString());
      }
      entry->set_sigma_multiplier(stage.sigmaMultiplier);
    }
  }

  // the values the user set, or the function's defaults if never edited
  if (f.scalarInputParams.isValid) {
    for (auto it = f.scalarInputParams.intParameter.cbegin();
         it != f.scalarInputParams.intParameter.cend(); ++it) {
      auto* p = m->add_int_parameters();
      p->set_name(it->first.toStdString());
      p->set_value(it->second);
    }
    for (auto it = f.scalarInputParams.doubleParameter.cbegin();
         it != f.scalarInputParams.doubleParameter.cend(); ++it) {
      auto* p = m->add_double_parameters();
      p->set_name(it->first.toStdString());
      p->set_value(it->second);
    }
    for (auto it = f.scalarInputParams.stringParameter.cbegin();
         it != f.scalarInputParams.stringParameter.cend(); ++it) {
      auto* p = m->add_string_parameters();
      p->set_name(it->first.toStdString());
      p->set_value(it->second.toStdString());
    }
  } else {
    for (auto it = f.integerParameters.cbegin();
         it != f.integerParameters.cend(); ++it) {
      auto* p = m->add_int_parameters();
      p->set_name(it->first.toStdString());
      p->set_value(it->second);
    }
    for (auto it = f.doubleParameters.cbegin(); it != f.doubleParameters.cend();
         ++it) {
      auto* p = m->add_double_parameters();
      p->set_name(it->first.toStdString());
      p->set_value(it->second);
    }
    for (auto it = f.stringParameters.cbegin(); it != f.stringParameters.cend();
         ++it) {
      auto* p = m->add_string_parameters();
      p->set_name(it->first.toStdString());
      p->set_value(it->second.toStdString());
    }
  }

  for (const FixedParameter& fixed : f.fixedParameters) {
    auto* p = m->add_fixed_parameters();
    p->set_parameter(toProto(fixed.parameter));
    p->set_value(fixed.value);
  }
}

void Access::applyFunction(const oipb::FunctionRecord& m, Function& f) {
  f.scalarInputParams.intParameter.clear();
  f.scalarInputParams.doubleParameter.clear();
  f.scalarInputParams.stringParameter.clear();
  for (const auto& p : m.int_parameters()) {
    f.scalarInputParams.intParameter.insert_or_assign(
        QString::fromStdString(p.name()), p.value());
  }
  for (const auto& p : m.double_parameters()) {
    f.scalarInputParams.doubleParameter.insert_or_assign(
        QString::fromStdString(p.name()), p.value());
  }
  for (const auto& p : m.string_parameters()) {
    f.scalarInputParams.stringParameter.insert_or_assign(
        QString::fromStdString(p.name()), QString::fromStdString(p.value()));
  }
  f.scalarInputParams.isValid = true;

  f.fixedParameters.clear();
  for (const auto& p : m.fixed_parameters()) {
    if (const auto parameter = parameterOf(p.parameter())) {
      FixedParameter fixed(*parameter);
      fixed.value = p.value();
      f.fixedParameters.push_back(fixed);
    }
  }

  // pipelines go onto the positions the function declared in init(); a
  // position it no longer declares is dropped
  for (const auto& pipeline : m.pipelines()) {
    const int position = pipeline.position();
    if (position < 0 || position >= std::ssize(f.neededElements)) {
      continue;
    }
    std::vector<PipelineStage> stages;
    for (const auto& entry : pipeline.stages()) {
      const auto type = stageTypeOf(entry.kind());
      if (!type) {
        continue;
      }
      PipelineStage stage(*type);
      for (int id : entry.explicit_ids()) {
        stage.explicitIds.push_back(id);
      }
      for (const auto& e : entry.exclusions()) {
        stage.explicitExclusions.push_back(PipelineExclusion(
            e.element_id(), QString::fromStdString(e.reason())));
      }
      stage.sigmaMultiplier = entry.sigma_multiplier();
      stages.push_back(stage);
    }
    f.neededElements[position].pipeline = stages;
  }

  f.inputElements.clear();
  for (const auto& entry : m.inputs()) {
    InputElement input(entry.element_id());
    input.typeOfElement =
        elementTypeOf(entry.kind()).value_or(ElementTypes::eUndefinedElement);
    input.label = QString::fromStdString(entry.label());
    input.ignoredDestinationParams = parametersOf(entry.ignored_destination());
    input.ignoredTargetParams = parametersOf(entry.ignored_target());
    input.shouldBeUsed = entry.should_be_used();
    input.isUsed = entry.is_used();
    f.inputElements[entry.position()].push_back(input);
  }
}

void Access::encodeBundle(const BundleAdjustment& b,
                          oipb::BundleAdjustmentRecord* m) {
  *m->mutable_plugin() = pluginRef(b.metaData);
  for (const BundleStation& station : b.stations) {
    encodeBundleStation(station, m->add_stations());
  }
  m->set_base_station(b.baseSystem.id);
  for (const BundleGeometry& g : b.geometries) {
    encodeBundleGeometry(g, m->add_geometries());
  }
  for (const BundleTransformation& t : b.transformations) {
    auto* entry = m->add_transformations();
    entry->set_id(t.id);
    for (auto it = t.parameters.cbegin(); it != t.parameters.cend(); ++it) {
      auto* p = entry->add_parameters();
      p->set_parameter(toProto(it->first));
      p->set_value(it->second);
    }
  }
}

void Access::applyBundle(const oipb::BundleAdjustmentRecord& m,
                         BundleAdjustment& b) {
  b.stations.clear();
  b.geometries.clear();
  b.transformations.clear();
  b.baseSystem = BundleStation();

  for (const auto& station : m.stations()) {
    b.stations.push_back(bundleStationOf(station));
  }
  // the base station is one of the stations, found by id
  if (m.base_station() != 0) {
    BundleStation key;
    key.id = m.base_station();
    const int index = oi::indexOf(b.stations, key);
    if (index >= 0) {
      b.baseSystem = b.stations.at(index);
    }
  }
  for (const auto& g : m.geometries()) {
    b.geometries.push_back(bundleGeometryOf(g));
  }
  for (const auto& entry : m.transformations()) {
    BundleTransformation t;
    t.id = entry.id();
    for (const auto& p : entry.parameters()) {
      if (const auto parameter = trafoParameterOf(p.parameter())) {
        t.parameters.insert_or_assign(*parameter, p.value());
      }
    }
    b.transformations.push_back(t);
  }
}

oipb::BundleAdjustmentRecord toProto(const BundleAdjustment& bundle) {
  oipb::BundleAdjustmentRecord m;
  Access::encodeBundle(bundle, &m);
  return m;
}

void applyRecord(const oipb::BundleAdjustmentRecord& record,
                 BundleAdjustment& bundle) {
  Access::applyBundle(record, bundle);
}

oipb::FunctionRecord toProto(const Function& function) {
  oipb::FunctionRecord m;
  Access::encodeFunction(function, &m);
  return m;
}

void applyRecord(const oipb::FunctionRecord& record, Function& function) {
  Access::applyFunction(record, function);
}

}  // namespace oi::codec
