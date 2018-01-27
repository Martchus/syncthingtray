#ifndef DATA_SYNCTHINGNOTIFIER_H
#define DATA_SYNCTHINGNOTIFIER_H

#include "./global.h"

#include <QObject>

namespace Data {

enum class SyncthingStatus;
class SyncthingConnection;
class SyncthingService;
struct SyncthingDir;
struct SyncthingDev;

/*!
 * \brief The SyncthingHighLevelNotification enum specifies the high-level notifications provided by the SyncthingNotifier class.
 * \remarks The enum is supposed to be used as flag-enum.
 */
enum class SyncthingHighLevelNotification {
    None = 0x0,
    ConnectedDisconnected = 0x1,
    SyncComplete = 0x2,
};

/// \cond
constexpr SyncthingHighLevelNotification operator|(SyncthingHighLevelNotification lhs, SyncthingHighLevelNotification rhs)
{
    return static_cast<SyncthingHighLevelNotification>(static_cast<unsigned char>(lhs) | static_cast<unsigned char>(rhs));
}

constexpr SyncthingHighLevelNotification &operator|=(SyncthingHighLevelNotification &lhs, SyncthingHighLevelNotification rhs)
{
    return lhs = static_cast<SyncthingHighLevelNotification>(static_cast<unsigned char>(lhs) | static_cast<unsigned char>(rhs));
}

constexpr bool operator&(SyncthingHighLevelNotification lhs, SyncthingHighLevelNotification rhs)
{
    return static_cast<bool>(static_cast<unsigned char>(lhs) & static_cast<unsigned char>(rhs));
}
/// \endcond

class LIB_SYNCTHING_CONNECTOR_EXPORT SyncthingNotifier : public QObject {
    Q_OBJECT
    Q_PROPERTY(SyncthingHighLevelNotification enabledNotifications READ enabledNotifications WRITE setEnabledNotifications)

public:
    SyncthingNotifier(const SyncthingConnection &connection, QObject *parent = nullptr);

    SyncthingHighLevelNotification enabledNotifications() const;

public Q_SLOTS:
    void setEnabledNotifications(SyncthingHighLevelNotification enabledNotifications);

Q_SIGNALS:
    ///! \brief Emitted when the connection status changes. Also provides the previous status.
    void statusChanged(SyncthingStatus previousStatus, SyncthingStatus newStatus);
    ///! \brief Emitted when the connection to Syncthing has been established.
    void connected();
    ///! \brief Emitted when the connection to Syncthing has been interrupted.
    void disconnected();
    ///! \brief Emitted when the specified \a dirs have been completed synchronization.
    void syncComplete(const QString &message);

private Q_SLOTS:
    void handleStatusChangedEvent(SyncthingStatus newStatus);

private:
    void emitConnectedAndDisconnected(SyncthingStatus newStatus);
    void emitSyncComplete(const SyncthingDir &dir, int index, const SyncthingDev *remoteDev);

    const SyncthingConnection &m_connection;
#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
    const SyncthingService &m_service;
#endif
    SyncthingHighLevelNotification m_enabledNotifications;
    SyncthingStatus m_previousStatus;
    bool m_initialized;
};

/*!
 * \brief Returns which notifications are enabled (by default none).
 */
inline SyncthingHighLevelNotification SyncthingNotifier::enabledNotifications() const
{
    return m_enabledNotifications;
}

/*!
 * \brief Sets which notifications are enabled.
 */
inline void SyncthingNotifier::setEnabledNotifications(SyncthingHighLevelNotification enabledNotifications)
{
    m_enabledNotifications = enabledNotifications;
}

} // namespace Data

#endif // DATA_SYNCTHINGNOTIFIER_H
