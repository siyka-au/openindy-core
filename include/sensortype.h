#ifndef SENSORTYPE_H
#define SENSORTYPE_H

#include <QList>
#include <QMetaType>
#include <QString>
#include <optional>

#if defined(OI_CORE_LIB)
#define OI_CORE_EXPORT Q_DECL_EXPORT
#else
#define OI_CORE_EXPORT Q_DECL_IMPORT
#endif

namespace oi {

/*!
 * \brief The SensorType class
 * What kind of instrument a sensor is - laser tracker, total station, and
 * whatever else a plugin cares to introduce.
 *
 * This replaces a closed three-value enum. The identity is an open string so
 * a plugin can bring a type the core has never heard of without a core
 * release: the id is derived from the plugin's own interface id, which
 * already encodes it (de.openIndy.plugin.sensor.<typeId>.v<version>).
 *
 * Behavioural contracts that are genuinely shared within a type continue to
 * live in the abstract base classes (LaserTracker, TotalStation, ...); this
 * type carries identity and presentation, and is the place to hang per-type
 * defaults if a need for them appears.
 */
class OI_CORE_EXPORT SensorType {
 public:
  SensorType() = default;
  SensorType(const QString& id, const QString& displayName,
             const QString& iconKey = QString());

  //! derives a type from a plugin interface id such as
  //! "de.openIndy.plugin.sensor.laserTracker.v1"; returns an invalid type
  //! for the bare sensor interface or anything unparseable
  static SensorType fromInterfaceId(const QString& interfaceId);

  const QString& id() const;
  const QString& displayName() const;
  const QString& iconKey() const;

  bool isValid() const;

  bool operator==(const SensorType& other) const;
  bool operator!=(const SensorType& other) const;

 private:
  QString m_id;
  QString m_displayName;
  QString m_iconKey;
};

/*!
 * \brief The SensorTypeRegistry class
 * Known sensor types. Seeded with the ones the core ships base classes for so
 * their display names and icons are right; a plugin introducing a new type is
 * resolved by derivation even when it never registers.
 */
class OI_CORE_EXPORT SensorTypeRegistry {
 public:
  static void registerType(const SensorType& type);

  static std::optional<SensorType> byId(const QString& id);

  static QList<SensorType> all();

 private:
  static void ensureSeeded();
};

}  // namespace oi

Q_DECLARE_METATYPE(oi::SensorType)

#endif  // SENSORTYPE_H
