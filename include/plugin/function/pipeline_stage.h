#ifndef OI_CORE_PLUGIN_FUNCTION_PIPELINE_STAGE_H_
#define OI_CORE_PLUGIN_FUNCTION_PIPELINE_STAGE_H_
#include <map>
#include <string>
#include <vector>

#include "core_export.h"
#include "types.h"

namespace oi {

/*!
 * \brief The PipelineStageType enum
 *
 * D16/domain-model.md S5: a NeededElement's input is a source (tag
 * resolution, D12) plus an ordered, declarative pipeline over the
 * resolved candidates - Java-Stream-shaped, each stage taking the
 * incoming set and yielding it unchanged, filtered, or (ExplicitList)
 * replaced outright.
 *
 * Stage 7b scope is the mechanism plus exactly one *computed* stage
 * (eResidualOutlierRejectionStage); ExplicitList and Exclude are the two
 * deterministic stages that make hand-picking and auto-tagging one
 * mechanism instead of two.
 */
enum class PipelineStageType {
  eExplicitListStage = 0,  // only these element ids survive
  eExcludeStage =
      1,  // these element ids are dropped, each carrying a reason (D16)
  eResidualOutlierRejectionStage =
      2  // drop candidates whose fit residual exceeds sigmaMultiplier * stdev
};

/*!
 * \brief The PipelineExclusion class
 *
 * D16: "rejecting a point is an Exclude pipeline stage carrying a reason,
 * not an untag and not a boolean." This is that recorded fact - produced
 * either by a user's explicit eExcludeStage entry or computed by
 * eResidualOutlierRejectionStage. Function::getPipelineExclusions()
 * reports the current set for a NeededElement position, which is the
 * traceability requirement: a silently dropped point is not acceptable
 * in an inspection context.
 */
class OI_CORE_EXPORT PipelineExclusion {
 public:
  PipelineExclusion() : elementId(-1) {}
  PipelineExclusion(int elementId, const std::string& reason)
      : elementId(elementId), reason(reason) {}

  bool operator==(const PipelineExclusion& other) const {
    return this->elementId == other.elementId;
  }

  int elementId;
  std::string reason;
};

/*!
 * \brief The PipelineStage class
 *
 * One stage in a NeededElement's declared pipeline (see
 * NeededElement::pipeline, function.h). A plain C++23 value type, not a
 * plugin-discoverable interface like Function itself - see
 * domain-model.md S5 for why: the one stage this pass ships
 * (eResidualOutlierRejectionStage) has to read whatever residual the
 * *owning* fit function already reported (Function::addDisplayResidual),
 * which only works if it is evaluated generically by Function::exec()
 * itself, not by a per-stage virtual a plugin would implement in
 * isolation. Building a second plugin-discovery ABI for exactly one
 * built-in stage is exactly the speculative filter-library the plan
 * says not to build; PipelineStageType can grow a real plugin-discoverable
 * shape later without disturbing what is persisted here (the stored shape
 * and this enum's values are independent of how a stage's *behaviour* is
 * dispatched).
 *
 * Parameterized the same shape Function uses for its own scalar inputs
 * today (plain typed fields here, since each stage type has a small,
 * fixed parameter set) rather than std::variant - Stage 10 is what
 * replaces Function's std::map<std::string,...> parameter maps project-wide,
 * and this predates it; follow the existing convention, do not scope-creep into
 * building Stage 10 early.
 */
class OI_CORE_EXPORT PipelineStage {
 public:
  //! The residual outlier rejection stage rejects residuals larger than this
  //! many standard deviations ("3 sigma") unless the stage says otherwise.
  static constexpr double kDefaultSigmaMultiplier = 3.0;

  PipelineStage()
      : type(PipelineStageType::eExcludeStage),
        sigmaMultiplier(kDefaultSigmaMultiplier) {}
  explicit PipelineStage(PipelineStageType type)
      : type(type), sigmaMultiplier(kDefaultSigmaMultiplier) {}

  static PipelineStage explicitList(const std::vector<int>& ids);
  static PipelineStage exclude(int elementId, const std::string& reason);
  static PipelineStage exclude(
      const std::vector<PipelineExclusion>& exclusions);
  static PipelineStage residualOutlierRejection(
      double sigmaMultiplier = kDefaultSigmaMultiplier);

  PipelineStageType type;

  std::vector<int> explicitIds;  // eExplicitListStage: only these ids survive
  std::vector<PipelineExclusion>
      explicitExclusions;  // eExcludeStage: these ids are dropped, each with
                           // its own reason
  double sigmaMultiplier;  // eResidualOutlierRejectionStage: reject beyond this
                           // many sigma
};

}  // namespace oi

#endif  // OI_CORE_PLUGIN_FUNCTION_PIPELINE_STAGE_H_
