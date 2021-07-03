#ifndef DATA_SYNCTHINGNOTIFIER_H
#define DATA_SYNCTHINGNOTIFIER_H

#include "./global.h"

#include <c++utilities/misc/flagenumclass.h>

#include <QObject>
#include <QProcess>

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include "./syncthingdev.h"
#include "./syncthingprocess.h"
#include "./syncthingservice.h"
#endif

namespace CppUtilities {
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
    None = 0,
    ConnectedDisconnected = (1 << 0),
    LocalSyncComplete = (1 << 1),
    RemoteSyncComplete = (1 << 2),
    NewDevice = (1 << 3),
    NewDir = (1 << 4),
    SyncthingProcessError = (1 << 5),
};

class LIB_SYNCTHING_CONNECTOR_EXPORT SyncthingNotifier : public QObject {
    Q_OBJECT
    Q_PROPERTY(SyncthingHighLevelNotification enabledNotifications READ enabledNotifications WRITE setEnabledNotifications)
    Q_PROPERTY(bool ignoreInavailabilityAfterStart READ ignoreInavailabilityAfterStart WRITE setIgnoreInavailabilityAfterStart)
#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
    Q_PROPERTY(const SyncthingService *service READ service WRITE setService)
#endif
    Q_PROPERTY(const SyncthingProcess *process READ process WRITE setProcess)

public:
    SyncthingNotifier(const SyncthingConnection &connection, QObject *parent = nullptr);

    const SyncthingConnection &connection() const;
    SyncthingHighLevelNotification enabledNotifications() const;
    unsigned int ignoreInavailabilityAfterStart() const;
#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
    const SyncthingService *service() const;
#endif
    const SyncthingProcess *process() const;

public Q_SLOTS:
    void setEnabledNotifications(SyncthingHighLevelNotification enabledNotifications);
    void setIgnoreInavailabilityAfterStart(unsigned int seconds);
#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
    void setService(const SyncthingService *service);
#endif
    void setProcess(const SyncthingProcess *process);

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
    ///! \brief Emitted when the Syncthing process fails to start or crashes.
    void syncthingProcessError(const QString &message, const QString &additionalInfo);

private Q_SLOTS:
    void handleStatusChangedEvent(SyncthingStatus newStatus);
    void handleNewDevEvent(CppUtilities::DateTime when, const QString &devId, const QString &address);
    void handleNewDirEvent(CppUtilities::DateTime when, const QString &devId, const SyncthingDev *dev, const QString &dirId, const QString &dirLabel);
    void handleSyncthingProcessError(QProcess::ProcessError syncthingProcessError);

private:
    bool isDisconnectRelevant() const;
    void emitConnectedAndDisconnected(SyncthingStatus newStatus);
    void emitSyncComplete(CppUtilities::DateTime when, const SyncthingDir &dir, int index, const SyncthingDev *remoteDev);

    const SyncthingConnection &m_connection;
#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
    const SyncthingService *m_service;
#endif
    const SyncthingProcess *m_process;
    SyncthingHighLevelNotification m_enabledNotifications;
    SyncthingStatus m_previousStatus;
    unsigned int m_ignoreInavailabilityAfterStart;
    bool m_initialized;
};

/*!
 * \brief Returns the associated connection.
 */
inline const SyncthingConnection &SyncthingNotifier::connection() const
{
    return m_connection;
}

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
 * \brief Returns the number of seconds after startup or standby-wakeup to suppress disconnect notifications.
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

#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
/*!
 * \brief Returns the SyncthingService to be taken into account. Might be nullptr.
 */
inline const SyncthingService *SyncthingNotifier::service() const
{
    return m_service;
}

/*!
 * \brief Sets the SyncthingService to be taken into account. Might be nullptr.
 */
inline void SyncthingNotifier::setService(const SyncthingService *service)
{
    m_service = service;
}
#endif

/*!
 * \brief Returns the SyncthingProcess to be taken into account. Might be nullptr.
 */
inline const SyncthingProcess *SyncthingNotifier::process() const
{
    return m_process;
}

/*!
 * \brief Sets the SyncthingProcess to be taken into account. Might be nullptr.
 */
inline void SyncthingNotifier::setProcess(const SyncthingProcess *process)
{
    m_process = process;
}

} // namespace Data

CPP_UTILITIES_MARK_FLAG_ENUM_CLASS(Data, Data::SyncthingHighLevelNotification)

#endif // DATA_SYNCTHINGNOTIFIER_H
