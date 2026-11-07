#ifndef OI_PROJECT_CODEC_H_
#define OI_PROJECT_CODEC_H_

// Saving and loading a project: bytes in, bytes out.
//
// The file and network format is protobuf (proto/openindy/project.proto), but
// nothing in this header depends on it. The core does not know where
// plugins live or which measurement configs the application ships, so the
// host provides those through ProjectHost.

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "guarded.h"
#include "measurementconfig.h"

namespace oi {

class OiJob;
class Function;
class BundleAdjustment;
class Sensor;

namespace codec {

/*!
 * \brief What the codec needs from the application that embeds the core.
 *
 * Plugin objects are saved as a reference (plugin name and object name) and
 * created again through the host on load. Return null when the plugin is not
 * installed: the load then fails and reports it.
 */
class ProjectHost {
 public:
  ProjectHost() = default;
  ProjectHost(const ProjectHost&) = delete;
  ProjectHost& operator=(const ProjectHost&) = delete;
  ProjectHost(ProjectHost&&) = delete;
  ProjectHost& operator=(ProjectHost&&) = delete;
  virtual ~ProjectHost() = default;

  [[nodiscard]] virtual std::unique_ptr<Function> createFunction(
      const std::string& plugin, const std::string& name) = 0;
  [[nodiscard]] virtual std::unique_ptr<BundleAdjustment>
  createBundleAdjustment(const std::string& plugin,
                         const std::string& name) = 0;
  [[nodiscard]] virtual std::unique_ptr<Sensor> createSensor(
      const std::string& plugin, const std::string& name) = 0;

  /*! The config with that key (name, type, sensor type), or an invalid
   *  config (isValid() false) if the host has none. Used when saving to find
   *  the configs a project defines, and when loading to prefer a standard
   *  config over the project's own copy. */
  [[nodiscard]] virtual MeasurementConfig measurementConfig(
      const MeasurementConfigKey& key) const = 0;
};

/*!
 * Serialises the job. Saving also stores the content digest in the job
 * (OiJob::setDigest), so the host can tell whether the project changed since
 * it was last saved. The same job always gives the same bytes, apart from the
 * save time.
 */
[[nodiscard]] std::string encodeProject(OiJob& job, const ProjectHost& host,
                                        const std::string& appVersion);

struct DecodeResult {
  /*! The loaded job, owned by the caller; null if the load failed. */
  oi::Guarded<OiJob> job;
  /*! Why the load failed. Empty on success. */
  std::string error;
  /*! "plugin/name" of every plugin object the project needs and the host
   *  could not create. A project with any of these does not load. */
  std::vector<std::string> missingPlugins;
  /*! The measurement configs the project defines (already marked as project
   *  configs), for the host to register. */
  std::vector<MeasurementConfig> projectConfigs;

  [[nodiscard]] bool ok() const { return !job.isNull(); }
};

[[nodiscard]] DecodeResult decodeProject(std::string_view bytes,
                                         ProjectHost& host);

}  // namespace codec
}  // namespace oi

#endif  // OI_PROJECT_CODEC_H_
