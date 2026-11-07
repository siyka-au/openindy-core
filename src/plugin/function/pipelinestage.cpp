#include "pipelinestage.h"

#include <vector>

#include "snapshot.h"

using namespace oi;

/*!
 * \brief PipelineStage::explicitList
 * \param ids
 * \return
 */
PipelineStage PipelineStage::explicitList(const std::vector<int>& ids) {
  PipelineStage stage(PipelineStageType::eExplicitListStage);
  stage.explicitIds = ids;
  return stage;
}

/*!
 * \brief PipelineStage::exclude
 * D16: an exclusion always carries a reason.
 * \param elementId
 * \param reason
 * \return
 */
PipelineStage PipelineStage::exclude(int elementId, const QString& reason) {
  PipelineStage stage(PipelineStageType::eExcludeStage);
  stage.explicitExclusions.push_back(PipelineExclusion(elementId, reason));
  return stage;
}

/*!
 * \brief PipelineStage::exclude
 * \param exclusions
 * \return
 */
PipelineStage PipelineStage::exclude(
    const std::vector<PipelineExclusion>& exclusions) {
  PipelineStage stage(PipelineStageType::eExcludeStage);
  stage.explicitExclusions = exclusions;
  return stage;
}

/*!
 * \brief PipelineStage::residualOutlierRejection
 * \param sigmaMultiplier
 * \return
 */
PipelineStage PipelineStage::residualOutlierRejection(double sigmaMultiplier) {
  PipelineStage stage(PipelineStageType::eResidualOutlierRejectionStage);
  stage.sigmaMultiplier = sigmaMultiplier;
  return stage;
}
