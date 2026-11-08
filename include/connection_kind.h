#ifndef OI_CORE_CONNECTION_KIND_H_
#define OI_CORE_CONNECTION_KIND_H_
#include <string>
#include <vector>

#include "core_export.h"

namespace oi {

/*!
 * \brief The ConnectionKind class
 * How a driver reaches its instrument - network, serial, or (later) anything
 * a driver wants to define itself. Open by the same reasoning as SensorType:
 * a closed enum means a core release for every new transport a driver wants,
 * and every driver so far only ever needed two.
 *
 * Deliberately narrow for now: this replaces the closed 2-value enum with an
 * open identity, but keeps the existing typed network/serial field shapes in
 * SensorConfiguration::ConnectionConfig as-is. A driver defining a genuinely
 * new transport (e.g. a custom TCP bridge) and the UI's corresponding
 * per-kind widget registry are real future work, not built here - doing
 * both at once was conflating "drivers own connections" with "one specific
 * driver's specific bridge", which are separate decisions.
 */
class OI_CORE_EXPORT ConnectionKind {
 public:
  ConnectionKind() = default;
  ConnectionKind(const std::string& id, const std::string& displayName);

  static const ConnectionKind& network();
  static const ConnectionKind& serial();

  static std::vector<ConnectionKind> knownKinds();
  static ConnectionKind byId(
      const std::string& id);  // returns an invalid kind if unknown

  [[nodiscard]] const std::string& id() const;
  [[nodiscard]] const std::string& displayName() const;

  [[nodiscard]] bool isValid() const;

  bool operator==(const ConnectionKind& other) const;
  bool operator!=(const ConnectionKind& other) const;

 private:
  std::string m_id;
  std::string m_displayName;
};

}  // namespace oi

#endif  // OI_CORE_CONNECTION_KIND_H_
