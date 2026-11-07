#ifndef SIMULATIONMODEL_H
#define SIMULATIONMODEL_H
#include <QString>
#include <map>
#include <vector>

#include "core_export.h"
#include "direction.h"
#include "guarded.h"
#include "messagesink.h"
#include "pluginmetadata.h"
#include "position.h"
#include "radius.h"
#include "reading.h"

namespace oi {

// ##############
// helper classes
// ##############

/*!
 * \brief The UncertaintyComponent class
 * Describe an uncertainty component of a simulation model
 */
class OI_CORE_EXPORT UncertaintyComponent {
 public:
  QString name;

  double value{};        // magnitude of the error
  double uncertainty{};  // uncertainty of the error
  QString distribution;  // the distribution of the uncertainty component

  QString errorUnit;  // unit of the uncertainty value (mm, m ,...)

  QString description;

  std::vector<QString> distributions;  // distributions in which the error could
                                       // be determined at random
};

/*!
 * \brief The Uncertainties class
 */
class OI_CORE_EXPORT Uncertainties {
 public:
  std::map<QString, UncertaintyComponent> sensorUncertainties;
  std::map<QString, UncertaintyComponent> objectUncertainties;
  std::map<QString, UncertaintyComponent> enviromentUncertainties;
  std::map<QString, UncertaintyComponent> humanUncertainties;
};

/*!
 * \brief The UncertaintyData class
 * Save information about simulation results for one geometry parameter
 */
class OI_CORE_EXPORT UncertaintyData {
 public:
  std::vector<double> values;  // randomly shuffled values produced distortion
                               // of readings and recalculation

  // maximum and minimum of the data series
  double maxValue{};
  double minValue{};

  double expectation{};
  double uncertainty{};  // uncertainty of the data series

  QString distribution;  // name of the distribution of the data series

  // function pointers that calculate density and distribution values
  double (*densityFunction)(const double& x, const double& expectation,
                            const double& uncertainty, const double& lowerLimit,
                            const double& upperLimit){};
  double (*distributionFunction)(const double& x, const double& expectation,
                                 const double& uncertainty,
                                 const double& lowerLimit,
                                 const double& upperLimit){};

  std::map<QString, QString> info;  // custom information
};

/*!
 * \brief The SimulationData class
 * Save simulation results for a geometry
 */
class OI_CORE_EXPORT SimulationData {
 public:
  // ####################################################
  // uncertainty data for each unknown geometry parameter
  // ####################################################

  UncertaintyData uncertaintyX;
  UncertaintyData uncertaintyY;
  UncertaintyData uncertaintyZ;

  UncertaintyData uncertaintyPrimaryI;
  UncertaintyData uncertaintyPrimaryJ;
  UncertaintyData uncertaintyPrimaryK;

  UncertaintyData uncertaintySecondaryI;
  UncertaintyData uncertaintySecondaryJ;
  UncertaintyData uncertaintySecondaryK;

  UncertaintyData uncertaintyRadiusA;
  UncertaintyData uncertaintyRadiusB;

  UncertaintyData uncertaintyAperture;

  UncertaintyData uncertaintyAngle;
  UncertaintyData uncertaintyDistance;
  UncertaintyData uncertaintyMeasurementSeries;
  UncertaintyData uncertaintyTemperature;

  UncertaintyData uncertaintyLength;

  // ###############################
  // correlations between parameters
  // ###############################

  std::map<QString, double> correlations;
};

// TODO set simulation config as separate class like sensor config an add xml
// save/load

/*!
 * \brief The SimulationConfiguration class
 */
class OI_CORE_EXPORT SimulationConfiguration {
 public:
  Uncertainties uncertainties;

  std::map<QString, int> integerParameters;
  std::map<QString, double> doubleParameters;
  std::map<QString, QString> stringParameters;
};

// #######################
// simulation class itself
// #######################

/*!
 * \brief The SimulationModel class
 * Interface for creating an uncertainty model for a simulation in OpenIndy.
 * You can provide four major error influences (sensor, object, enviroment and
 * human).
 */
class OI_CORE_EXPORT SimulationModel : public oi::Trackable {
 public:
  explicit SimulationModel();

  ~SimulationModel() override;

  // ################################
  // simulation initialization method
  // ################################

  virtual void init();

  // #########################################
  // methods to get or set further information
  // #########################################

  [[nodiscard]] const PluginMetaData& getMetaData() const;

  [[nodiscard]] const SimulationConfiguration& getSimulationConfiguration()
      const;
  void setSimulationConfiguration(const SimulationConfiguration& sConfig);

  [[nodiscard]] const std::map<QString, int>& getIntegerParameter() const;
  [[nodiscard]] const std::map<QString, double>& getDoubleParameter() const;
  [[nodiscard]] const std::multimap<QString, QString>& getStringParameter()
      const;

  // ##################
  // simulation methods
  // ##################

  [[nodiscard]] const std::map<QString, UncertaintyComponent>&
  getSensorUncertainties() const;
  [[nodiscard]] const std::map<QString, UncertaintyComponent>&
  getObjectUncertainties() const;
  [[nodiscard]] const std::map<QString, UncertaintyComponent>&
  getEnviromentUncertainties() const;
  [[nodiscard]] const std::map<QString, UncertaintyComponent>&
  getHumanInfluence() const;

  virtual bool distort(const oi::Guarded<Reading>& r,
                       const OiMat& objectRelation,
                       const bool& newIterationStart);
  virtual bool analyseSimulationData(UncertaintyData& d);
  virtual double getCorrelationCoefficient(const std::vector<double>& x,
                                           const std::vector<double>& y);

  // ##################################################
  // report simulation issues to the host (formerly a signal)
  // ##################################################

  // messages go to the sink if there is one (non-owning; see MessageSink for
  // its lifetime rules), otherwise they are dropped
  void setMessageSink(MessageSink* sink) { this->messageSink = sink; }
  [[nodiscard]] MessageSink* getMessageSink() const {
    return this->messageSink;
  }

  void sendMessage(const QString& msg, const MessageTypes& msgType,
                   const MessageDestinations& msgDest =
                       MessageDestinations::eConsoleMessage) {
    if (this->messageSink != nullptr) {
      this->messageSink->onMessage(msg, msgType, msgDest);
    }
  }

 protected:
  // ###########################
  // input and output parameters
  // ###########################

  SimulationConfiguration sConfig;

  PluginMetaData metaData;

  // the needed scalar input parameters including default values
  std::map<QString, int> integerParameters;
  std::map<QString, double> doubleParameters;
  std::multimap<QString, QString> stringParameters;

  // uncertainties provided by plugin writer
  std::map<QString, UncertaintyComponent> sensorUncertainties;
  std::map<QString, UncertaintyComponent> objectUncertainties;
  std::map<QString, UncertaintyComponent> environmentUncertainties;
  std::map<QString, UncertaintyComponent> humanInfluence;

 private:
  MessageSink* messageSink = nullptr;  // non-owning
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

#define SIMULATION_PREFIX de.openIndy.plugin.simulation.v
#define Simulation_iidd STR2(CONCAT2(SIMULATION_PREFIX, OI_PLUGIN_ABI_VERSION))

#endif  // SIMULATIONMODEL_H
