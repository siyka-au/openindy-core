#include "sensortype.h"

#include <cctype>
#include <map>
#include <vector>

#include "containers.h"

namespace oi {

namespace {

//! turns "laserTracker" into "Laser Tracker" so a type nobody registered
//! still presents reasonably
std::string humanize(const std::string& id) {
  std::string result;
  for (std::size_t i = 0; i < id.size(); i++) {
    const char c = id.at(i);
    if (i == 0) {
      result.push_back(
          static_cast<char>(std::toupper(static_cast<unsigned char>(c))));
    } else if (std::isupper(static_cast<unsigned char>(c)) != 0) {
      result.push_back(' ');
      result.push_back(c);
    } else {
      result.push_back(c);
    }
  }
  return result;
}

std::map<std::string, SensorType>& registry() {
  static std::map<std::string, SensorType> instance;
  return instance;
}

bool seeded = false;

}  // namespace

SensorType::SensorType(const std::string& id, const std::string& displayName,
                       const std::string& iconKey)
    : m_id(id), m_displayName(displayName), m_iconKey(iconKey) {}

/*!
 * \brief SensorType::fromInterfaceId
 * A sensor interface id looks like
 * de.openIndy.plugin.sensor.<typeId>.v<version>. The bare sensor interface
 * (de.openIndy.plugin.sensor.v<version>) carries no type of its own and yields
 * an invalid SensorType.
 */
SensorType SensorType::fromInterfaceId(const std::string& interfaceId) {
  const std::string marker = std::string("plugin.sensor.");
  const std::size_t start = interfaceId.find(marker);
  if (start == std::string::npos) {
    return SensorType();
  }

  std::string remainder = interfaceId.substr(start + marker.size());

  // strip the trailing version segment
  const std::size_t versionAt = remainder.rfind(".v");
  if (versionAt != std::string::npos) {
    remainder.resize(versionAt);
  }

  // the bare sensor interface has nothing between the marker and the version
  if (remainder.empty() || remainder.starts_with('v')) {
    return SensorType();
  }

  if (const std::optional<SensorType> known =
          SensorTypeRegistry::byId(remainder)) {
    return known.value();
  }

  return SensorType(remainder, humanize(remainder));
}

const std::string& SensorType::id() const { return this->m_id; }

const std::string& SensorType::displayName() const {
  return this->m_displayName;
}

const std::string& SensorType::iconKey() const { return this->m_iconKey; }

bool SensorType::isValid() const { return !this->m_id.empty(); }

bool SensorType::operator==(const SensorType& other) const {
  return this->m_id == other.m_id;
}

bool SensorType::operator!=(const SensorType& other) const {
  return !(*this == other);
}

void SensorTypeRegistry::ensureSeeded() {
  if (seeded) {
    return;
  }
  seeded = true;
  registry().insert_or_assign(
      std::string("laserTracker"),
      SensorType(std::string("laserTracker"), std::string("Laser Tracker"),
                 std::string("laserTracker")));
  registry().insert_or_assign(
      std::string("totalStation"),
      SensorType(std::string("totalStation"), std::string("Total Station"),
                 std::string("totalStation")));
}

void SensorTypeRegistry::registerType(const SensorType& type) {
  SensorTypeRegistry::ensureSeeded();
  if (!type.isValid()) {
    return;
  }
  registry().insert_or_assign(type.id(), type);
}

std::optional<SensorType> SensorTypeRegistry::byId(const std::string& id) {
  SensorTypeRegistry::ensureSeeded();
  const auto it = registry().find(id);
  if (it == registry().cend()) {
    return std::nullopt;
  }
  return it->second;
}

std::vector<SensorType> SensorTypeRegistry::all() {
  SensorTypeRegistry::ensureSeeded();
  return oi::values(registry());
}

}  // namespace oi
