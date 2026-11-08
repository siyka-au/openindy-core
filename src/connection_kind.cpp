#include "connection_kind.h"

#include <vector>

namespace oi {

ConnectionKind::ConnectionKind(const std::string& id,
                               const std::string& displayName)
    : m_id(id), m_displayName(displayName) {}

const ConnectionKind& ConnectionKind::network() {
  static const ConnectionKind kind("network", "Network");
  return kind;
}

const ConnectionKind& ConnectionKind::serial() {
  static const ConnectionKind kind("serial", "Serial");
  return kind;
}

std::vector<ConnectionKind> ConnectionKind::knownKinds() {
  return {ConnectionKind::network(), ConnectionKind::serial()};
}

ConnectionKind ConnectionKind::byId(const std::string& id) {
  for (const ConnectionKind& kind : ConnectionKind::knownKinds()) {
    if (kind.id() == id) {
      return kind;
    }
  }
  return ConnectionKind();
}

const std::string& ConnectionKind::id() const { return this->m_id; }

const std::string& ConnectionKind::displayName() const {
  return this->m_displayName;
}

bool ConnectionKind::isValid() const { return !this->m_id.empty(); }

bool ConnectionKind::operator==(const ConnectionKind& other) const {
  return this->m_id == other.m_id;
}

bool ConnectionKind::operator!=(const ConnectionKind& other) const {
  return !(*this == other);
}

}  // namespace oi
