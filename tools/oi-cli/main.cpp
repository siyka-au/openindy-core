// oi-cli: end-to-end proof that the core is usable without Qt.
//
// Loads a project (protobuf), runs a plane fit through a compute plugin
// loaded with oi::PluginLoader, reports progress through an oi::ProgressSink
// and diagnostics through oi::log, saves the project with the fitted plane,
// loads it again and checks that nothing was lost. Exits non-zero on any
// mismatch. This program and the libraries it links contain no Qt.
//
//   oi-cli --plugin <libp_xxx.so> --make-fixture <out.oip>
//   oi-cli --plugin <libp_xxx.so> --project <in.oip> [--out <out.oip>] [-v]
//   oi-cli --plugin <libp_xxx.so> --selftest [-v]     (77 = plugin not found)

#include <cmath>
#include <cstdio>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "chooselalib.h"
#include "coordinatesystem.h"
#include "featurewrapper.h"
#include "function.h"
#include "geometry/plane.h"
#include "geometry/point.h"
#include "log.h"
#include "oijob.h"
#include "plugin.h"
#include "pluginloader.h"
#include "progress.h"
#include "project_codec.h"
#include "station.h"

namespace {

constexpr int kExitOk = 0;
constexpr int kExitFailed = 1;
constexpr int kExitUsage = 2;
constexpr int kExitSkipped = 77;  // the CTest convention for "skipped"

constexpr const char* kFitFunction = "BestFitPlane";
constexpr const char* kFittedName = "oi-cli fit";

// prints progress on stderr, one line per report
class PrintingSink final : public oi::ProgressSink {
 public:
  void onProgress(const oi::Progress& progress) override {
    if (progress.isDeterminate()) {
      std::cerr << std::format("[{:3.0f}%] {}\n", progress.fraction * 100.0,
                               progress.message);
    } else {
      std::cerr << std::format("[ ...] {}\n", progress.message);
    }
  }
};

// The CLI creates no plugin objects from a project file: it only needs a host
// to save and load the points and planes of a project.
class NoPluginsHost final : public oi::codec::ProjectHost {
 public:
  std::unique_ptr<oi::Function> createFunction(const std::string&,
                                               const std::string&) override {
    return nullptr;
  }
  std::unique_ptr<oi::BundleAdjustment> createBundleAdjustment(
      const std::string&, const std::string&) override {
    return nullptr;
  }
  std::unique_ptr<oi::Sensor> createSensor(const std::string&,
                                           const std::string&) override {
    return nullptr;
  }
  oi::MeasurementConfig measurementConfig(
      const oi::MeasurementConfigKey&) const override {
    return {};
  }
};

bool readFile(const std::string& path, std::string& bytes) {
  std::ifstream in(path, std::ios::binary);
  if (!in) {
    return false;
  }
  bytes.assign(std::istreambuf_iterator<char>(in),
               std::istreambuf_iterator<char>());
  return true;
}

bool writeFile(const std::string& path, const std::string& bytes) {
  std::ofstream out(path, std::ios::binary | std::ios::trunc);
  out.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
  return static_cast<bool>(out);
}

// a fixed plane z = 0.5 + 0.1 x - 0.05 y sampled at six points
oi::Guarded<oi::OiJob> makeFixtureJob() {
  oi::Guarded<oi::OiJob> job = new oi::OiJob();

  // a project always has an active station
  auto own_station = std::make_unique<oi::Station>();
  oi::Guarded<oi::Station> station = own_station.get();
  oi::Guarded<oi::FeatureWrapper> stationWrapper = station->getFeatureWrapper();
  station->setFeatureName("STATION01");
  job->addFeature(std::move(own_station));
  auto own_part = std::make_unique<oi::CoordinateSystem>();
  oi::Guarded<oi::CoordinateSystem> part = own_part.get();
  oi::Guarded<oi::FeatureWrapper> partWrapper = part->getFeatureWrapper();
  part->setFeatureName("PART");
  job->addFeature(std::move(own_part));
  station->setActiveStationState(true);
  station->getCoordinateSystem()->setActiveCoordinateSystemState(true);

  const double xs[] = {0.0, 1.0, 0.0, 2.0, 1.5, 3.0};
  const double ys[] = {0.0, 0.0, 1.0, 1.0, 2.5, 2.0};
  for (int i = 0; i < 6; ++i) {
    const double z = 0.5 + 0.1 * xs[i] - 0.05 * ys[i];
    auto own_point = std::make_unique<oi::Point>(oi::Position(xs[i], ys[i], z));
    oi::Guarded<oi::Point> point = own_point.get();
    oi::Guarded<oi::FeatureWrapper> wrapper = point->getFeatureWrapper();
    point->setFeatureName(std::format("P{}", i + 1));
    point->setIsSolved(true);
    job->addFeature(std::move(own_point));
  }
  return job;
}

oi::Guarded<oi::Plane> fittedPlaneOf(const oi::OiJob& job) {
  for (const auto& wrapper : job.getFeaturesByName(kFittedName)) {
    if (!wrapper.isNull() && !wrapper->getPlane().isNull()) {
      return wrapper->getPlane();
    }
  }
  return {};
}

// unit normal of the plane, sign-normalised so that k >= 0
std::vector<double> normalOf(const oi::Plane& plane) {
  const auto v = plane.getDirection().getVector();
  std::vector<double> n = {v.getAt(0), v.getAt(1), v.getAt(2)};
  const double length = std::sqrt(n[0] * n[0] + n[1] * n[1] + n[2] * n[2]);
  const double sign = n[2] < 0.0 ? -1.0 : 1.0;
  for (double& c : n) {
    c = sign * c / length;
  }
  return n;
}

int fail(std::string_view what) {
  std::cerr << "oi-cli: FAILED: " << what << "\n";
  return kExitFailed;
}

int runProject(const std::string& pluginPath, const std::string& projectPath,
               const std::string& outPath, bool expectFixtureNormal = false) {
  // 1. the compute plugin
  oi::PluginLoadResult loaded = oi::PluginLoader::load(pluginPath);
  if (!loaded) {
    return fail(loaded.error);
  }
  oi::log::info("plugin {} loaded (ABI {})", pluginPath,
                loaded.plugin.abiVersion());

  // 2. the project
  std::string bytes;
  if (!readFile(projectPath, bytes)) {
    return fail("cannot read " + projectPath);
  }
  NoPluginsHost host;
  oi::codec::DecodeResult decoded = oi::codec::decodeProject(bytes, host);
  if (!decoded.ok()) {
    return fail("cannot decode " + projectPath + ": " + decoded.error);
  }
  oi::OiJob& job = *decoded.job;

  std::vector<oi::Guarded<oi::Point>> points;
  for (const auto& wrapper : job.getFeaturesList()) {
    if (!wrapper.isNull() && !wrapper->getPoint().isNull() &&
        wrapper->getPoint()->getIsSolved()) {
      points.push_back(wrapper->getPoint());
    }
  }
  oi::log::info("{} solved points in {}", points.size(), projectPath);
  if (points.size() < 3) {
    return fail("a plane fit needs at least three solved points");
  }

  // 3. the fit, through the plugin
  oi::Guarded<oi::Function> fit = loaded.plugin->createFunction(kFitFunction);
  if (fit.isNull()) {
    return fail(std::string("plugin has no function ") + kFitFunction);
  }
  fit->init();
  for (const auto& point : points) {
    oi::InputElement element(point->getId());
    element.typeOfElement = oi::ElementTypes::ePointElement;
    element.point = point;
    fit->addInputElement(element, 0);
  }
  auto own_plane = std::make_unique<oi::Plane>();
  oi::Guarded<oi::Plane> plane = own_plane.get();
  oi::Guarded<oi::FeatureWrapper> planeWrapper = plane->getFeatureWrapper();
  plane->setFeatureName(kFittedName);

  PrintingSink sink;
  const auto result =
      fit->execChecked(planeWrapper, oi::OperationContext(&sink));
  if (!result.has_value()) {
    return fail("the fit failed: " + result.error());
  }
  const std::vector<double> fitted = normalOf(*plane);
  std::cout << std::format("plane normal: {:.6f} {:.6f} {:.6f}\n", fitted[0],
                           fitted[1], fitted[2]);
  if (expectFixtureNormal) {
    // the fixture lies exactly on z = 0.5 + 0.1 x - 0.05 y
    const double len = std::sqrt(0.1 * 0.1 + 0.05 * 0.05 + 1.0);
    const double expected[] = {-0.1 / len, 0.05 / len, 1.0 / len};
    for (int i = 0; i < 3; ++i) {
      if (std::abs(fitted[i] - expected[i]) > 1e-6) {
        return fail("the fitted normal is not the fixture's plane normal");
      }
    }
  }
  plane->setIsSolved(true);
  if (!job.addFeature(std::move(own_plane))) {
    return fail("cannot add the fitted plane to the job");
  }
  const int featureCount = job.getFeatureCount();

  // 4. save, load again, compare
  const std::string saved = oi::codec::encodeProject(job, host, "oi-cli");
  if (!outPath.empty() && !writeFile(outPath, saved)) {
    return fail("cannot write " + outPath);
  }
  oi::codec::DecodeResult reloaded = oi::codec::decodeProject(saved, host);
  if (!reloaded.ok()) {
    return fail("the saved project does not load: " + reloaded.error);
  }
  if (reloaded.job->getFeatureCount() != featureCount) {
    return fail(std::format("feature count {} after reload, expected {}",
                            reloaded.job->getFeatureCount(), featureCount));
  }
  const oi::Guarded<oi::Plane> again = fittedPlaneOf(*reloaded.job);
  if (again.isNull()) {
    return fail("the fitted plane is missing after reload");
  }
  const std::vector<double> back = normalOf(*again);
  for (int i = 0; i < 3; ++i) {
    if (std::abs(back[i] - fitted[i]) > 1e-12) {
      return fail(
          std::format("plane normal differs after reload (axis {})", i));
    }
  }
  // saving the reloaded job must reproduce the same project
  const std::string savedAgain =
      oi::codec::encodeProject(*reloaded.job, host, "oi-cli");
  if (savedAgain.size() != saved.size()) {
    return fail("saving the reloaded project gives a different size");
  }
  std::cout << std::format("ok: {} features, saved {} bytes, reload matches\n",
                           featureCount, saved.size());
  return kExitOk;
}

int usage() {
  std::cerr
      << "usage: oi-cli --plugin <lib> --make-fixture <out.oip>\n"
         "       oi-cli --plugin <lib> --project <in.oip> [--out <file>]\n"
         "       oi-cli --plugin <lib> --selftest\n"
         "options: -v  more diagnostics\n";
  return kExitUsage;
}

}  // namespace

int main(int argc, char** argv) {
  std::string plugin;
  std::string project;
  std::string out;
  std::string fixture;
  bool selftest = false;
  for (int i = 1; i < argc; ++i) {
    const std::string_view arg = argv[i];
    auto value = [&](std::string& into) {
      if (i + 1 >= argc) {
        return false;
      }
      into = argv[++i];
      return true;
    };
    if (arg == "--plugin") {
      if (!value(plugin)) return usage();
    } else if (arg == "--project") {
      if (!value(project)) return usage();
    } else if (arg == "--out") {
      if (!value(out)) return usage();
    } else if (arg == "--make-fixture") {
      if (!value(fixture)) return usage();
    } else if (arg == "--selftest") {
      selftest = true;
    } else if (arg == "-v") {
      oi::log::setLevel(oi::log::Level::kDebug);
    } else {
      return usage();
    }
  }

  ChooseLALib::setLinearAlgebra(ChooseLALib::LinearAlgebraLibrary::Armadillo);

  if (plugin.empty() || !std::filesystem::exists(plugin)) {
    if (selftest) {
      std::cerr << "oi-cli: plugin library not found, skipping\n";
      return kExitSkipped;
    }
    return usage();
  }

  if (!fixture.empty()) {
    NoPluginsHost host;
    oi::Guarded<oi::OiJob> job = makeFixtureJob();
    if (!writeFile(fixture, oi::codec::encodeProject(*job, host, "oi-cli"))) {
      return fail("cannot write " + fixture);
    }
    return kExitOk;
  }
  if (selftest) {
    const auto path =
        (std::filesystem::temp_directory_path() / "oi-cli-selftest.oip")
            .string();
    NoPluginsHost host;
    oi::Guarded<oi::OiJob> job = makeFixtureJob();
    if (!writeFile(path, oi::codec::encodeProject(*job, host, "oi-cli"))) {
      return fail("cannot write " + path);
    }
    const int code = runProject(plugin, path, "", true);
    std::filesystem::remove(path);
    return code;
  }
  if (project.empty()) {
    return usage();
  }
  return runProject(plugin, project, out);
}
