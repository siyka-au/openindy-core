#ifndef OI_CORE_SRC_CODEC_ACCESS_H_
#define OI_CORE_SRC_CODEC_ACCESS_H_

// Internal. Access is the friend declared in the model classes (Element,
// Feature, Station, ...). It exists so the codec can restore private state
// directly, the way the classes' own deserialisers used to.
#include <map>
#include <vector>

#include "convert.h"
#include "feature_wrapper.h"
#include "geometry/point.h"
#include "geometry/point_cloud.h"
#include "guarded.h"
#include "oi_job.h"
#include "wire.h"

namespace oi::codec {

struct Access {
  // ---- Element / Feature ----
  static void setId(Element& element, int id) { element.id = id; }

  static void restoreCommon(Feature& feature, const oipb::FeatureCommon& c) {
    feature.id = c.id();
    feature.name = (c.name());
    feature.group = (c.group());
    feature.comment = (c.comment());
    feature.isSolved = c.solved();
    feature.isActiveFeature = c.active();
  }

  // ---- Station ----
  static void restoreStation(Station& station, const oipb::Station& m) {
    station.isActiveStation = m.active_station();
  }

  // ---- CoordinateSystem ----
  static void restoreSystem(CoordinateSystem& system,
                            const oipb::CoordinateSystem& m) {
    system.isActiveCoordinateSystem = m.active_system();
    system.isBundleSystem = m.bundle_system();
    system.origin = Position(m.origin().x(), m.origin().y(), m.origin().z());
    system.xAxis = Direction(m.x_axis().x(), m.x_axis().y(), m.x_axis().z());
    system.yAxis = Direction(m.y_axis().x(), m.y_axis().y(), m.y_axis().z());
    system.zAxis = Direction(m.z_axis().x(), m.z_axis().y(), m.z_axis().z());
    system.expansionOrigin =
        Position(m.expansion_origin().x(), m.expansion_origin().y(),
                 m.expansion_origin().z());
  }

  static void setBundleTemplate(CoordinateSystem& system,
                                const std::string& value) {
    system.bundleTemplate = value;
  }

  // ---- TrafoParam ----
  static void restoreTrafo(TrafoParam& trafo, const oipb::TrafoParam& m) {
    const auto set3 = [](OiVec& target, const oipb::Vec3& v) {
      target.setAt(0, v.x());
      target.setAt(1, v.y());
      target.setAt(2, v.z());
    };
    set3(trafo.translation, m.translation());
    set3(trafo.rotation, m.rotation());
    set3(trafo.scale, m.scale());
    trafo.isDatumTrafo = m.datum_trafo();
    trafo.isUsed = m.used();
    if (m.homogen_matrix_size() == 16) {
      for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
          trafo.homogenMatrix.setAt(row, col, m.homogen_matrix(row * 4 + col));
        }
      }
    }
  }

  static int fromId(const TrafoParam& t) {
    return t.from ? t.from->getId() : 0;
  }
  static int toId(const TrafoParam& t) { return t.to ? t.to->getId() : 0; }

  // ---- Observation ----
  static void restoreObservation(Observation& o, const oipb::Observation& m) {
    o.id = m.id();
    o.isValid = m.valid();
    o.isSolved = m.solved();
    o.isDummyPoint = m.dummy_point();
    o.hasDirection = m.has_direction();
    const auto set4 = [](OiVec& target, const oipb::Vec3& v) {
      target.setAt(0, v.x());
      target.setAt(1, v.y());
      target.setAt(2, v.z());
      target.setAt(3, 1.0);
    };
    set4(o.xyz, m.xyz());
    set4(o.originalXyz, m.original_xyz());
    set4(o.sigmaXyz, m.sigma_xyz());
    set4(o.originalSigmaXyz, m.original_sigma_xyz());
    set4(o.ijk, m.ijk());
    set4(o.originalIjk, m.original_ijk());
    set4(o.sigmaIjk, m.sigma_ijk());
    set4(o.originalSigmaIjk, m.original_sigma_ijk());
  }

  static void encodeObservation(const Observation& o, oipb::Observation* m);

  // ---- Reading ----
  static void encodeReading(const Reading& r, oipb::Reading* m);
  static std::unique_ptr<Reading> decodeReading(const oipb::Reading& m);

  // ---- Geometry (geometry.cpp) ----
  template <class T>
  static std::unique_ptr<Geometry> restoreBase(const oipb::Geometry& m);
  static void encodeGeometry(const Geometry& g, oipb::Geometry* m);
  static std::unique_ptr<Geometry> decodeGeometry(const oipb::Geometry& m);

  // ---- Function (functions.cpp) ----
  static void encodeFunction(const Function& f, oipb::FunctionRecord* m);
  static void applyFunction(const oipb::FunctionRecord& m, Function& f);

  // ---- Bundle adjustment (functions.cpp) ----
  static void encodeBundle(const BundleAdjustment& b,
                           oipb::BundleAdjustmentRecord* m);
  static void applyBundle(const oipb::BundleAdjustmentRecord& m,
                          BundleAdjustment& b);

  // ---- Linking (project.cpp): state the linker restores after decoding ----
  static std::map<int, std::vector<InputElement>>& inputs(Function& f) {
    return f.inputElements;
  }

  // The station comes with a default position and station system; the saved
  // ones replace them.
  static void setStationPosition(Station& s, std::unique_ptr<Point> point) {
    s.ownedPosition = std::move(point);
    s.position = s.ownedPosition.get();
  }
  static void setStationSystem(Station& s,
                               std::unique_ptr<CoordinateSystem> system) {
    s.ownedSystem = std::move(system);
    s.stationSystem = s.ownedSystem.get();
    s.ownedSystem->isStationSystem = true;
    s.ownedSystem->station = &s;
  }
  static void setUsedSensors(Station& s, const std::vector<Sensor>& sensors) {
    s.setUsedSensors(sensors);
  }
  static void addStationReading(Station& s,
                                const oi::Guarded<Reading>& reading) {
    switch (reading->getTypeOfReading()) {
      case ReadingTypes::eCartesianReading:
        s.cartesianReadings.push_back(reading);
        break;
      case ReadingTypes::ePolarReading:
        s.polarReadings.push_back(reading);
        break;
      case ReadingTypes::eDirectionReading:
        s.directionReadings.push_back(reading);
        break;
      case ReadingTypes::eDistanceReading:
        s.distanceReadings.push_back(reading);
        break;
      case ReadingTypes::eLevelReading:
        s.levelReadings.push_back(reading);
        break;
      case ReadingTypes::eTemperatureReading:
        s.temperatureRadings.push_back(reading);
        break;
      default:
        break;
    }
  }
  static void addSystemObservation(CoordinateSystem& system,
                                   std::unique_ptr<Observation> owned) {
    const oi::Guarded<Observation> observation(owned.get());
    system.ownedObservations.push_back(std::move(owned));
    system.observationsList.push_back(observation);
    system.observationsMap.insert_or_assign(observation->getId(), observation);
  }
  static void addSystemNominal(CoordinateSystem& system,
                               const oi::Guarded<FeatureWrapper>& nominal) {
    system.nominalsList.push_back(nominal);
    system.nominalsMap.insert_or_assign(nominal->getFeature()->getId(),
                                        nominal);
  }

  static void addSegment(PointCloud& cloud,
                         const oi::Guarded<FeatureWrapper>& segment) {
    cloud.detectedSegments.push_back(segment);
  }

  static int nextId(const OiJob& job) { return job.nextId; }
  static void setNextId(OiJob& job, int next) { job.nextId = next; }
  static void adopt(OiJob& job, std::vector<std::unique_ptr<Feature>> f) {
    job.addImportedFeatures(std::move(f));
  }

  // ---- Group (geometry.cpp) ----
  static void encodeGroup(const Group& g, oipb::Group* m);
  static std::unique_ptr<Group> decodeGroup(const oipb::Group& m);
};

}  // namespace oi::codec

#endif  // OI_CORE_SRC_CODEC_ACCESS_H_
