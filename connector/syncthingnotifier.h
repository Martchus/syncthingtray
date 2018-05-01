#ifndef DATA_SYNCTHINGNOTIFIER_H
#define DATA_SYNCTHINGNOTIFIER_H

#include "./global.h"

#include <QObject>

namespace ChronoUtilities {
class DateTime;
}

namespace Data {

enum class SyncthingStatus;
class SyncthingConnection;
class SyncthingService;
class SyncthingProcess;
struct SyncthingDir;
struct SyncthingDev;

/*!
 * \brief The SyncthingHighLevelNotification enum specifies the high-level notifications provided by the SyncthingNotifier class.
 * \remarks The enum is supposed to be used as flag-enum.
 */
enum class SyncthingHighLevelNotification {
    None = 0x0,
    ConnectedDisconnected = 0x1,
    LocalSyncComplete = 0x2,
    RemoteSyncComplete = 0x4,
    NewDevice = 0x8,
    NewDir = 0x10,
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
    Q_PROPERTY(bool ignoreInavailabilityAfterStart READ ignoreInavailabilityAfterStart WRITE setIgnoreInavailabilityAfterStart)

public:
    SyncthingNotifier(const SyncthingConnection &connection, QObject *parent = nullptr);

    SyncthingHighLevelNotification enabledNotifications() const;
    unsigned int ignoreInavailabilityAfterStart() const;

public Q_SLOTS:
    void setEnabledNotifications(SyncthingHighLevelNotification enabledNotifications);
    void setIgnoreInavailabilityAfterStart(unsigned int seconds);

Q_SIGNALS:
    ///! \brief Emitted when the connection status changes. Also provides the previous status.
    void statusChanged(SyncthingStatus previousStatus, SyncthingStatus newStatus);
    ///! \brief Emitted when the connection to Syncthing has been established.
    void connected();
    ///! \brief Emitted when the connection to Syncthing has been interrupted.
    void disconnected();
    ///! \brief Emitted when one or more directories have completed synchronization.
    ///! \remarks Both, local and remote devices, are taken into account.
    void syncComplete(const QString &message);
    ///! \brief Emitted when a new device talks to us.
    void newDevice(const QString &devId, const QString &message);
    ///! \brief Emitted when a new directory is shared with us.
    void newDir(const QString &devId, const QString &dirId, const QString &message);

private Q_SLOTS:
    void handleStatusChangedEvent(SyncthingStatus newStatus);
    void handleNewDevEvent(ChronoUtilities::DateTime when, const QString &devId, const QString &address);
    void handleNewDirEvent(
        ChronoUtilities::DateTime when, const QString &devId, const SyncthingDev *dev, const QString &dirId, const QString &dirLabel);

private:
    bool isDisconnectRelevant() const;
    void emitConnectedAndDisconnected(SyncthingStatus newStatus);
    void emitSyncComplete(ChronoUtilities::DateTime when, const SyncthingDir &dir, int index, const SyncthingDev *remoteDev);

    const SyncthingConnection &m_connection;
#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
    const SyncthingService *const m_service;
#endif
    const SyncthingProcess *const m_process;
    SyncthingHighLevelNotification m_enabledNotifications;
    SyncthingStatus m_previousStatus;
    unsigned int m_ignoreInavailabilityAfterStart;
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

/*!
 * \brief Returns the number of seconds after startup or standby-wakeup to supress disconnect notifications.
 */
inline unsigned int SyncthingNotifier::ignoreInavailabilityAfterStart() const
{
    return m_ignoreInavailabilityAfterStart;
}

/*!
 * \brief Prevents disconnect notifications in the first \a seconds after startup or standby-wakeup.
 */
inline void SyncthingNotifier::setIgnoreInavailabilityAfterStart(unsigned int seconds)
{
    m_ignoreInavailabilityAfterStart = seconds;
}

} // namespace Data

#endif // DATA_SYNCTHINGNOTIFIER_H
