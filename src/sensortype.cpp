#include "sensortype.h"

#include <QMap>

namespace oi {

namespace {

//! turns "laserTracker" into "Laser Tracker" so a type nobody registered
//! still presents reasonably
QString humanize(const QString& id) {
  QString result;
  for (int i = 0; i < id.size(); i++) {
    const QChar c = id.at(i);
    if (i == 0) {
      result.append(c.toUpper());
    } else if (c.isUpper()) {
      result.append(' ');
      result.append(c);
    } else {
      result.append(c);
    }
  }
  return result;
}

QMap<QString, SensorType>& registry() {
  static QMap<QString, SensorType> instance;
  return instance;
}

bool seeded = false;

}  // namespace

SensorType::SensorType(const QString& id, const QString& displayName,
                       const QString& iconKey)
    : m_id(id), m_displayName(displayName), m_iconKey(iconKey) {}

/*!
 * \brief SensorType::fromInterfaceId
 * A sensor interface id looks like
 * de.openIndy.plugin.sensor.<typeId>.v<version>. The bare sensor interface
 * (de.openIndy.plugin.sensor.v<version>) carries no type of its own and yields
 * an invalid SensorType.
 */
SensorType SensorType::fromInterfaceId(const QString& interfaceId) {
  const QString marker = QStringLiteral("plugin.sensor.");
  const int start = interfaceId.indexOf(marker);
  if (start < 0) {
    return SensorType();
  }

  QString remainder = interfaceId.mid(start + marker.size());

  // strip the trailing version segment
  const int versionAt = remainder.lastIndexOf(QStringLiteral(".v"));
  if (versionAt >= 0) {
    remainder = remainder.left(versionAt);
  }

  // the bare sensor interface has nothing between the marker and the version
  if (remainder.isEmpty() || remainder.startsWith(QLatin1Char('v'))) {
    return SensorType();
  }

  if (const std::optional<SensorType> known =
          SensorTypeRegistry::byId(remainder)) {
    return known.value();
  }

  return SensorType(remainder, humanize(remainder));
}

const QString& SensorType::id() const { return this->m_id; }

const QString& SensorType::displayName() const { return this->m_displayName; }

const QString& SensorType::iconKey() const { return this->m_iconKey; }

bool SensorType::isValid() const { return !this->m_id.isEmpty(); }

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
  registry().insert(QStringLiteral("laserTracker"),
                    SensorType(QStringLiteral("laserTracker"),
                               QStringLiteral("Laser Tracker"),
                               QStringLiteral("laserTracker")));
  registry().insert(QStringLiteral("totalStation"),
                    SensorType(QStringLiteral("totalStation"),
                               QStringLiteral("Total Station"),
                               QStringLiteral("totalStation")));
}

void SensorTypeRegistry::registerType(const SensorType& type) {
  SensorTypeRegistry::ensureSeeded();
  if (!type.isValid()) {
    return;
  }
  registry().insert(type.id(), type);
}

std::optional<SensorType> SensorTypeRegistry::byId(const QString& id) {
  SensorTypeRegistry::ensureSeeded();
  const auto it = registry().constFind(id);
  if (it == registry().constEnd()) {
    return std::nullopt;
  }
  return it.value();
}

QList<SensorType> SensorTypeRegistry::all() {
  SensorTypeRegistry::ensureSeeded();
  return registry().values();
}

}  // namespace oi
