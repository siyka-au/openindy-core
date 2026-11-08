#ifndef BUNDLEADJUSTMENT_H
#define BUNDLEADJUSTMENT_H
#include <map>
#include <string>
#include <vector>

#include "core_export.h"
#include "oi_job.h"
#include "plugin_metadata.h"
#include "progress.h"
#include "types.h"

namespace oi {

class BundleException : public std::exception {
 public:
  explicit BundleException(const char* const& message, int featureId = -1)
      : message(message), featureId(featureId) {}

  int getFeatureId() { return featureId; }

  [[nodiscard]] const char* what() const noexcept override { return message; }

 private:
  const char* message;
  int featureId;
};

/*!
 * \brief The BundleTransformation class
 * Defines a transformation for bundle adjustment
 */
class BundleTransformation {
 public:
  BundleTransformation() {}

  bool operator==(const BundleTransformation& other) const {
    if (this->id == other.id) {
      return true;
    }
    return false;
  }

  // station feature id
  int id{-1};

  // transformation parameters
  std::map<TrafoParamParameters, double> parameters;
};

/*!
 * \brief The BundleGeometry class
 * Defines a geometry for bundle adjustment
 */
class BundleGeometry {
 public:
  BundleGeometry() {}

  bool operator==(const BundleGeometry& other) const {
    if (this->id == other.id) {
      return true;
    }
    return false;
  }

  // geometry feature id
  int id{-1};

  // geometry parameters
  std::map<GeometryParameters, double> parameters;
};

/*!
 * \brief The BundleStation class
 * Defines a station used for bundle adjustment
 */
class BundleStation {
 public:
  BundleStation()

  {}

  bool operator==(const BundleStation& other) const {
    if (this->id == other.id) {
      return true;
    }
    return false;
  }

  // station feature id
  int id{-1};

  // defines which parameters to calculate
  bool tx{true}, ty{true}, tz{true};
  bool rx{true}, ry{true}, rz{true};
  bool m{true};

  // geometries solved in station system
  std::vector<BundleGeometry> geometries;
};

/*!
 * \brief The BundleAdjustment class
 * Interface for implementing bundle adjustment plugins.
 */
class OI_CORE_EXPORT BundleAdjustment : public oi::Trackable {
  friend struct codec::Access;  // the protobuf codec (see src/codec)

 public:
  explicit BundleAdjustment();
  ~BundleAdjustment() override;

  // ############################
  // bundle initialization method
  // ############################

  virtual void init();

  // ################################
  // calculates the bundle adjustment
  // ################################

  // `ctx`: progress and cancellation from the host; implementations report
  // through it and poll it between iterations (throwing OperationCancelled is
  // fine, runBundle()'s caller treats it as "cancelled by the user").
  virtual bool runBundle(const OperationContext& ctx = {});

  // ###########################################################
  // methods to get or set further information to solve a bundle
  // ###########################################################

  // meta data
  [[nodiscard]] const PluginMetaData& getMetaData() const;

  // available scalar parameters
  [[nodiscard]] const std::map<std::string, int>& getIntegerParameter() const;
  [[nodiscard]] const std::map<std::string, double>& getDoubleParameter() const;
  [[nodiscard]] const std::multimap<std::string, std::string>&
  getStringParameter() const;

  // scalar input parameters
  const ScalarInputParams& getScalarInputParams();
  void setScalarInputParams(const ScalarInputParams& params);

  // input stations
  [[nodiscard]] const std::vector<BundleStation>& getInputStations() const;
  void setInputStations(const std::vector<BundleStation>& stations);
  [[nodiscard]] const BundleStation& getBaseStation() const;
  void setBaseStation(const BundleStation& station);

  // bundle coordinate system
  [[nodiscard]] const oi::Guarded<CoordinateSystem>& getBundleSystem() const;
  void setBundleSystem(const oi::Guarded<CoordinateSystem>& system);

  // OpenIndy job
  [[nodiscard]] const oi::Guarded<OiJob>& getCurrentJob() const;
  void setCurrentJob(const oi::Guarded<OiJob>& job);

  // ###############
  // general getters
  // ###############

  // id
  [[nodiscard]] const int& getId() const;

  // bundle results
  [[nodiscard]] const std::vector<BundleGeometry>& getOutputGeometries() const;
  [[nodiscard]] const std::vector<BundleTransformation>&
  getOutputTransformations() const;

  // #############
  // clear results
  // #############

  virtual void clear();
  virtual void clearResults();

  // #################################################
  // report bundle issues to the host (formerly a signal)
  // #################################################

  // messages go to the sink if there is one (non-owning; see MessageSink for
  // its lifetime rules), otherwise they are dropped. setCurrentJob makes the
  // job the sink, as it used to connect to the job's message slot.
  void setMessageSink(MessageSink* sink) { this->messageSink = sink; }
  [[nodiscard]] MessageSink* getMessageSink() const {
    return this->messageSink;
  }

  void sendMessage(const std::string& msg, const MessageTypes& msgType,
                   const MessageDestinations& msgDest =
                       MessageDestinations::eConsoleMessage) {
    if (this->messageSink != nullptr) {
      this->messageSink->onMessage(msg, msgType, msgDest);
    }
  }

 private:
  // ###########
  // connect job
  // ###########

  void connectJob();
  void disconnectJob();

  MessageSink* messageSink = nullptr;  // non-owning

 protected:
  // ###########################
  // input and output parameters
  // ###########################

  // meta information about the bundle
  PluginMetaData metaData;

  // the needed scalar input parameters including default values
  std::map<std::string, int> integerParameters;
  std::map<std::string, double> doubleParameters;
  std::multimap<std::string, std::string> stringParameters;

  // user defined scalar input parameters
  ScalarInputParams scalarInputParams;

  // input stations
  std::vector<BundleStation> stations;
  BundleStation baseSystem;

  // output geometries and transformation parameters
  std::vector<BundleGeometry> geometries;
  std::vector<BundleTransformation> transformations;

  // bundle coordinate system
  oi::Guarded<CoordinateSystem> bundleSystem;

  // OpenIndy job
  oi::Guarded<OiJob> currentJob;

  // ##################
  // general attributes
  // ##################

  int id{};
};

}  // namespace oi

#ifndef STR
#define STR(x) #x
#endif
#ifndef STR2
#define STR2(x) STR(x)
#endif

#ifndef CONCAT
#define CONCAT(a, b) a##b
#endif
#ifndef CONCAT2
#define CONCAT2(a, b) CONCAT(a, b)
#endif

#include "plugin_abi.h"

#define BUNDLEADJUSTMENT_PREFIX de.openIndy.plugin.bundleAdjustment.v
#define BundleAdjustment_iidd \
  STR2(CONCAT2(BUNDLEADJUSTMENT_PREFIX, OI_PLUGIN_ABI_VERSION))

#endif  // BUNDLEADJUSTMENT_H
