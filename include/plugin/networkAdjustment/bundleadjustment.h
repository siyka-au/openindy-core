#ifndef BUNDLEADJUSTMENT_H
#define BUNDLEADJUSTMENT_H

#include <QDomDocument>
#include <QDomElement>
#include <QMap>
#include <QMultiMap>
#include <QObject>
#include <QString>

#include "oijob.h"
#include "pluginmetadata.h"
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
  QMap<TrafoParamParameters, double> parameters;
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
  QMap<GeometryParameters, double> parameters;
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
  QList<BundleGeometry> geometries;
};

/*!
 * \brief The BundleAdjustment class
 * Interface for implementing bundle adjustment plugins.
 */
class OI_CORE_EXPORT BundleAdjustment : public QObject {
  Q_OBJECT
  Q_DISABLE_COPY_MOVE(BundleAdjustment)

 public:
  explicit BundleAdjustment(QObject* parent = nullptr);
  ~BundleAdjustment() override;

  // ############################
  // bundle initialization method
  // ############################

  virtual void init();

  // ################################
  // calculates the bundle adjustment
  // ################################

  virtual bool runBundle();

  // ###########################################################
  // methods to get or set further information to solve a bundle
  // ###########################################################

  // meta data
  [[nodiscard]] const PluginMetaData& getMetaData() const;

  // available scalar parameters
  [[nodiscard]] const QMap<QString, int>& getIntegerParameter() const;
  [[nodiscard]] const QMap<QString, double>& getDoubleParameter() const;
  [[nodiscard]] const QMultiMap<QString, QString>& getStringParameter() const;

  // scalar input parameters
  const ScalarInputParams& getScalarInputParams();
  void setScalarInputParams(const ScalarInputParams& params);

  // input stations
  [[nodiscard]] const QList<BundleStation>& getInputStations() const;
  void setInputStations(const QList<BundleStation>& stations);
  [[nodiscard]] const BundleStation& getBaseStation() const;
  void setBaseStation(const BundleStation& station);

  // bundle coordinate system
  [[nodiscard]] const QPointer<CoordinateSystem>& getBundleSystem() const;
  void setBundleSystem(const QPointer<CoordinateSystem>& system);

  // OpenIndy job
  [[nodiscard]] const QPointer<OiJob>& getCurrentJob() const;
  void setCurrentJob(const QPointer<OiJob>& job);

  // ###############
  // general getters
  // ###############

  // id
  [[nodiscard]] const int& getId() const;

  // bundle results
  [[nodiscard]] const QList<BundleGeometry>& getOutputGeometries() const;
  [[nodiscard]] const QList<BundleTransformation>& getOutputTransformations()
      const;

  // #############
  // clear results
  // #############

  virtual void clear();
  virtual void clearResults();

  // #################
  // save and load XML
  // #################

  QDomElement toOpenIndyXML(QDomDocument& xmlDoc) const;
  bool fromOpenIndyXML(QDomElement& xmlElem);

 signals:

  // ################################################
  // signals to inform OpenIndy about function issues
  // ################################################

  // messaging
  void sendMessage(const QString& msg, const MessageTypes& msgType,
                   const MessageDestinations& msgDest =
                       MessageDestinations::eConsoleMessage);

  // input parameters changed
  void scalarInputParametersChanged();
  void inputStationsChanged();
  void baseStationChanged();
  void bundleSystemChanged();
  void jobChanged();

 private:
  // ###########
  // connect job
  // ###########

  void connectJob();
  void disconnectJob();

 protected:
  // ###########################
  // input and output parameters
  // ###########################

  // meta information about the bundle
  PluginMetaData metaData;

  // the needed scalar input parameters including default values
  QMap<QString, int> integerParameters;
  QMap<QString, double> doubleParameters;
  QMultiMap<QString, QString> stringParameters;

  // user defined scalar input parameters
  ScalarInputParams scalarInputParams;

  // input stations
  QList<BundleStation> stations;
  BundleStation baseSystem;

  // output geometries and transformation parameters
  QList<BundleGeometry> geometries;
  QList<BundleTransformation> transformations;

  // bundle coordinate system
  QPointer<CoordinateSystem> bundleSystem;

  // OpenIndy job
  QPointer<OiJob> currentJob;

  // ##################
  // general attributes
  // ##################

  int id{};
};

}  // namespace oi

Q_DECLARE_METATYPE(oi::BundleAdjustment*)

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

#define BUNDLEADJUSTMENT_PREFIX de.openIndy.plugin.bundleAdjustment.v
#define BundleAdjustment_iidd \
  STR2(CONCAT2(BUNDLEADJUSTMENT_PREFIX, OI_PLUGIN_ABI_VERSION))

#endif  // BUNDLEADJUSTMENT_H
