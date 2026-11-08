#include "calibrated_constellation.h"

#include <vector>

namespace oi {

LocalPoint::LocalPoint(const std::string& label, const OiVec& position,
                       bool isMeasured)
    : m_label(label), m_position(position), m_isMeasured(isMeasured) {}

const std::string& LocalPoint::label() const { return this->m_label; }

const OiVec& LocalPoint::position() const { return this->m_position; }

bool LocalPoint::isMeasured() const { return this->m_isMeasured; }

CalibratedConstellation::CalibratedConstellation(
    const std::string& name, const std::vector<LocalPoint>& points,
    const std::string& certificateId,
    const std::optional<std::chrono::year_month_day>& calibratedAt)
    : m_name(name),
      m_points(points),
      m_certificateId(certificateId),
      m_calibratedAt(calibratedAt) {}

const std::string& CalibratedConstellation::name() const {
  return this->m_name;
}

const std::vector<LocalPoint>& CalibratedConstellation::points() const {
  return this->m_points;
}

std::vector<LocalPoint> CalibratedConstellation::measuredPoints() const {
  std::vector<LocalPoint> result;
  for (const LocalPoint& p : this->m_points) {
    if (p.isMeasured()) {
      result.push_back(p);
    }
  }
  return result;
}

std::vector<LocalPoint> CalibratedConstellation::derivedPoints() const {
  std::vector<LocalPoint> result;
  for (const LocalPoint& p : this->m_points) {
    if (!p.isMeasured()) {
      result.push_back(p);
    }
  }
  return result;
}

std::optional<LocalPoint> CalibratedConstellation::pointByLabel(
    const std::string& label) const {
  for (const LocalPoint& p : this->m_points) {
    if (p.label() == label) {
      return p;
    }
  }
  return std::nullopt;
}

const std::string& CalibratedConstellation::certificateId() const {
  return this->m_certificateId;
}

const std::optional<std::chrono::year_month_day>&
CalibratedConstellation::calibratedAt() const {
  return this->m_calibratedAt;
}

bool CalibratedConstellation::isValid() const {
  return !this->m_name.empty() && this->m_points.size() >= 2;
}

}  // namespace oi
