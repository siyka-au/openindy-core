#include "simulationmodel.h"

#include <map>
#include <vector>

using namespace oi;

/*!
 * \brief SimulationModel::SimulationModel
 */
SimulationModel::SimulationModel() {}

/*!
 * \brief SimulationModel::~SimulationModel
 */
SimulationModel::~SimulationModel() {}

/*!
 * \brief SimulationModel::getMetaData
 * \return
 */
const PluginMetaData& SimulationModel::getMetaData() const {
  return this->metaData;
}

/*!
 * \brief SimulationModel::getSimulationConfiguration
 * \return
 */
const SimulationConfiguration& SimulationModel::getSimulationConfiguration()
    const {
  return this->sConfig;
}

/*!
 * \brief SimulationModel::setSimulationConfiguration
 * \param sConfig
 */
void SimulationModel::setSimulationConfiguration(
    const SimulationConfiguration& sConfig) {
  this->sConfig = sConfig;
}

/*!
 * \brief SimulationModel::getIntegerParameter
 * \return
 */
const std::map<std::string, int>& SimulationModel::getIntegerParameter() const {
  return this->integerParameters;
}

/*!
 * \brief SimulationModel::getDoubleParameter
 * \return
 */
const std::map<std::string, double>& SimulationModel::getDoubleParameter()
    const {
  return this->doubleParameters;
}

/*!
 * \brief SimulationModel::getStringParameter
 * \return
 */
const std::multimap<std::string, std::string>&
SimulationModel::getStringParameter() const {
  return this->stringParameters;
}

/*!
 * \brief SimulationModel::getSensorUncertainties
 * \return
 */
const std::map<std::string, UncertaintyComponent>&
SimulationModel::getSensorUncertainties() const {
  return this->sensorUncertainties;
}

/*!
 * \brief SimulationModel::getObjectUncertainties
 * \return
 */
const std::map<std::string, UncertaintyComponent>&
SimulationModel::getObjectUncertainties() const {
  return objectUncertainties;
}

/*!
 * \brief SimulationModel::getEnviromentUncertainties
 * \return
 */
const std::map<std::string, UncertaintyComponent>&
SimulationModel::getEnviromentUncertainties() const {
  return environmentUncertainties;
}

/*!
 * \brief SimulationModel::getHumanInfluence
 * \return
 */
const std::map<std::string, UncertaintyComponent>&
SimulationModel::getHumanInfluence() const {
  return this->humanInfluence;
}

/*!
 * \brief SimulationModel::distort
 * Distort a reading with the given uncertainties
 * \param r
 * \param objectRelation homogeneous matrix (4x4) which describes the relation
 * between station and object
 * \param newIterationStart
 * \return
 */
bool SimulationModel::distort(const oi::Guarded<Reading>& /*r*/,
                              const OiMat& /*objectRelation*/,
                              const bool& /*newIterationStart*/) {
  this->sendMessage(
      std::string(
          "Simulation Model not implemented correctly: distort method missing"),
      MessageTypes::eCriticalMessage, MessageDestinations::eMessageBoxMessage);
  return false;
}

/*!
 * \brief SimulationModel::analyseSimulationData
 * Analyse the simulation values saved in d.values
 * \param d
 * \return
 */
bool SimulationModel::analyseSimulationData(UncertaintyData& /*d*/,
                                            const OperationContext& /*ctx*/) {
  this->sendMessage(
      std::string(
          "Simulation Model not implemented correctly: analyse method missing"),
      MessageTypes::eCriticalMessage, MessageDestinations::eMessageBoxMessage);
  return false;
}

/*!
 * \brief SimulationModel::getCorrelationCoefficient
 * Determine the correlation coefficient of the two quantities x and y.
 * \param x
 * \param y
 * \return
 */
double SimulationModel::getCorrelationCoefficient(
    const std::vector<double>& /*x*/, const std::vector<double>& /*y*/) {
  this->sendMessage(std::string("Simulation Model not implemented correctly: "
                                "correlation method missing"),
                    MessageTypes::eCriticalMessage,
                    MessageDestinations::eMessageBoxMessage);
  return 0.0;
}

/*!
 * \brief SimulationModel::init
 */
void SimulationModel::init() {}
