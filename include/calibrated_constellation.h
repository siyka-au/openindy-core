#ifndef OI_CORE_CALIBRATED_CONSTELLATION_H_
#define OI_CORE_CALIBRATED_CONSTELLATION_H_
#include <chrono>
#include <optional>
#include <string>
#include <vector>

#include "core_export.h"
#include "oi_vec.h"

namespace oi {

using namespace math;

/*!
 * \brief The LocalPoint class
 * One point of a CalibratedConstellation, in the constellation's own local
 * frame. A scale bar's two ends, a vector bar's two measured ends plus its
 * inferred tip, a calibration board's array of targets - all just lists of
 * these.
 *
 * isMeasured distinguishes a point a sensor can actually observe from one
 * that's only known through its fixed geometric relationship to the others
 * (e.g. a vector bar's tip, worked out once at calibration time from the
 * two end points). Both kinds are stored the same way - a derived point's
 * position in the local frame is just as fixed as a measured one's, it was
 * simply computed rather than touched. No separate "derivation rule" object
 * is needed: whatever rule produced this coordinate was already applied
 * once, when the constellation was calibrated/authored.
 */
class OI_CORE_EXPORT LocalPoint {
 public:
  LocalPoint() = default;
  LocalPoint(const std::string& label, const OiVec& position,
             bool isMeasured = true);

  [[nodiscard]] const std::string& label() const;
  [[nodiscard]] const OiVec& position() const;
  [[nodiscard]] bool isMeasured() const;

 private:
  std::string m_label;
  OiVec m_position;
  bool m_isMeasured = true;
};

/*!
 * \brief The CalibratedConstellation class
 * A rigid, portable calibration reference: N points with certified relative
 * geometry, in their own local frame. Unlike a DCN, whose points are already
 * in absolute/job coordinates with uncertainty on their *position*, a
 * constellation's uncertainty is on its *internal* geometry - it has to be
 * observed and pose-solved into job coordinates before its points mean
 * anything there, which is exactly what makes it portable: pick it up,
 * move it, observe it again, solve again.
 *
 * A scale bar is the 2-point case (one certified distance, no derived
 * points). A vector bar is 3 points, one derived. A 7-point calibration
 * board is the same shape at N=7. A 6DoF active probe (T-Probe/T-Mac
 * style) is 6-7 points plus a derived tip, pose-solved continuously instead
 * of once.
 */
class OI_CORE_EXPORT CalibratedConstellation {
 public:
  CalibratedConstellation() = default;
  CalibratedConstellation(const std::string& name,
                          const std::vector<LocalPoint>& points,
                          const std::string& certificateId = std::string(),
                          const std::optional<std::chrono::year_month_day>&
                              calibratedAt = std::nullopt);

  [[nodiscard]] const std::string& name() const;
  [[nodiscard]] const std::vector<LocalPoint>& points() const;

  [[nodiscard]] std::vector<LocalPoint> measuredPoints() const;
  [[nodiscard]] std::vector<LocalPoint> derivedPoints() const;

  [[nodiscard]] std::optional<LocalPoint> pointByLabel(
      const std::string& label) const;

  [[nodiscard]] const std::string& certificateId() const;
  [[nodiscard]] const std::optional<std::chrono::year_month_day>& calibratedAt()
      const;

  [[nodiscard]] bool isValid() const;

 private:
  std::string m_name;
  std::vector<LocalPoint> m_points;
  std::string m_certificateId;
  std::optional<std::chrono::year_month_day> m_calibratedAt;
};

}  // namespace oi

#endif  // OI_CORE_CALIBRATED_CONSTELLATION_H_
