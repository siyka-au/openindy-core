#ifndef EXCHANGEINTERFACE_H
#define EXCHANGEINTERFACE_H
#include <iosfwd>
#include <map>
#include <vector>

#include "coordinatesystem.h"
#include "core_export.h"
#include "exchangeobserver.h"
#include "featurewrapper.h"
#include "guarded.h"
#include "messagesink.h"
#include "pluginmetadata.h"

namespace oi {

/*!
 * \brief The OiExchangeInterface class
 * Base class for implementing import/export plugins
 */
class OI_CORE_EXPORT ExchangeInterface : public oi::Trackable {
 public:
  explicit ExchangeInterface();

  ~ExchangeInterface() override;

  // ##############################
  // exchange initialization method
  // ##############################

  virtual void init();

  // #########################################
  // methods to get or set further information
  // #########################################

  [[nodiscard]] const PluginMetaData& getMetaData() const;
  const std::vector<GeometryTypes>& getSupportedGeometries();

  // the stream used for the im- or export (a file stream, a string stream);
  // non-owning, it must outlive the exchange run
  [[nodiscard]] std::iostream* getStream() const;
  void setStream(std::iostream* stream);

  [[nodiscard]] const std::vector<oi::Guarded<FeatureWrapper>>& getFeatures()
      const;
  void setFeatures(const std::vector<oi::Guarded<FeatureWrapper>>& features);

  [[nodiscard]] const bool& getExportObservations() const;
  void setExportObservations(const bool& exportObservations);

  [[nodiscard]] const std::string& getGroupName() const;
  void setGroupName(const std::string& groupName);

  [[nodiscard]] const oi::Guarded<CoordinateSystem>& getNominalSystem() const;
  void setNominalSystem(const oi::Guarded<CoordinateSystem>& nominalSystem);

  [[nodiscard]] const std::map<DimensionType, UnitType>& getUnits() const;
  void setUnit(const DimensionType& dimension, const UnitType& unit);

 public:
  // #########################
  // import and export methods
  // #########################

  virtual void importOiData();
  virtual void exportOiData();

  // ################################################
  // report exchange issues to the host (formerly signals)
  // ################################################

  // messages go to the sink, progress and completion to the observer; both
  // are optional, non-owning, and must outlive the exchange (or be removed
  // first). They are called synchronously on the thread the exchange runs
  // on, which is usually not the host's UI thread.
  void setMessageSink(MessageSink* sink) { this->messageSink = sink; }
  [[nodiscard]] MessageSink* getMessageSink() const {
    return this->messageSink;
  }
  void setObserver(ExchangeObserver* observer) {
    this->exchangeObserver = observer;
  }
  [[nodiscard]] ExchangeObserver* getObserver() const {
    return this->exchangeObserver;
  }

  void sendMessage(const std::string& msg, const MessageTypes& msgType,
                   const MessageDestinations& msgDest =
                       MessageDestinations::eConsoleMessage) {
    if (this->messageSink != nullptr) {
      this->messageSink->onMessage(msg, msgType, msgDest);
    }
  }
  void updateProgress(const int& progress,
                      const std::string& msg) {  // 0 <= progress <= 100
    if (this->exchangeObserver != nullptr) {
      this->exchangeObserver->onProgress(progress, msg);
    }
  }
  void importFinished(const bool& success) {
    if (this->exchangeObserver != nullptr) {
      this->exchangeObserver->onImportFinished(success);
    }
  }
  void exportFinished(const bool& success) {
    if (this->exchangeObserver != nullptr) {
      this->exchangeObserver->onExportFinished(success);
    }
  }

 protected:
  // ###########################
  // input and output parameters
  // ###########################

  std::map<DimensionType, UnitType> units;

  bool exportObservations{};  // true if only the observations of the selected
                              // geometries shall be exported
  std::string groupName;  // group that the imported nominals shall be added to
  oi::Guarded<CoordinateSystem>
      nominalSystem;  // nominal coordinate system the imported geometries are
                      // referenced to

  std::iostream* stream{nullptr};  // non-owning, see setStream
  std::vector<oi::Guarded<FeatureWrapper>>
      features;  // the features that shall be exported or were imported

  PluginMetaData metaData;
  std::vector<GeometryTypes> supportedGeometries;

 private:
  MessageSink* messageSink = nullptr;            // non-owning
  ExchangeObserver* exchangeObserver = nullptr;  // non-owning
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

#include "pluginabi.h"

#define EXCHANGE_PREFIX de.openIndy.plugin.exchange.v
#define Exchange_iidd STR2(CONCAT2(EXCHANGE_PREFIX, OI_PLUGIN_ABI_VERSION))

#endif  // EXCHANGEINTERFACE_H
