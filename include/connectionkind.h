#ifndef CONNECTIONKIND_H
#define CONNECTIONKIND_H

#include <QList>
#include <QMetaType>
#include <QString>

#if defined(OI_CORE_LIB)
#  define OI_CORE_EXPORT Q_DECL_EXPORT
#else
#  define OI_CORE_EXPORT Q_DECL_IMPORT
#endif

namespace oi{

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
class OI_CORE_EXPORT ConnectionKind
{

public:

    ConnectionKind() = default;
    ConnectionKind(const QString &id, const QString &displayName);

    static const ConnectionKind &network();
    static const ConnectionKind &serial();

    static QList<ConnectionKind> knownKinds();
    static ConnectionKind byId(const QString &id);   //returns an invalid kind if unknown

    const QString &id() const;
    const QString &displayName() const;

    bool isValid() const;

    bool operator==(const ConnectionKind &other) const;
    bool operator!=(const ConnectionKind &other) const;

private:

    QString m_id;
    QString m_displayName;

};

}

Q_DECLARE_METATYPE( oi::ConnectionKind )

#endif // CONNECTIONKIND_H
