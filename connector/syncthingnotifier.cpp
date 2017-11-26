#include "./syncthingnotifier.h"
#include "./syncthingconnection.h"

#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
#include "./syncthingservice.h"
#endif

namespace Data {

/*!
 * \class SyncthingNotifier
 * \brief The SyncthingNotifier class emits high-level notification for a given SyncthingConnection.
 *
 * In contrast to the signals provided by the SyncthingConnection class, these signals take further apply
 * further logic and take additional information into account (previous status, service status if known, ...).
 *
 * \remarks Not tested yet. Supposed to simplify
 * - SyncthingApplet::handleConnectionStatusChanged(SyncthingStatus status)
 * - and TrayIcon::showStatusNotification(SyncthingStatus status).
 */

/*!
 * \brief Constructs a new SyncthingNotifier instance for the specified \a connection.
 * \remarks Use setEnabledNotifications() to enable notifications (only statusChanged() is always emitted).
 */
SyncthingNotifier::SyncthingNotifier(const SyncthingConnection &connection, QObject *parent)
    : QObject(parent)
    , m_connection(connection)
#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
    , m_service(syncthingService())
#endif
    , m_enabledNotifications(SyncthingHighLevelNotification::None)
    , m_previousStatus(SyncthingStatus::Disconnected)
    , m_initialized(false)
{
}

void SyncthingNotifier::handleStatusChangedEvent(SyncthingStatus newStatus)
{
    // skip redundant status updates
    if (m_initialized && m_previousStatus == newStatus) {
        return;
    }

    // emit signals
    emit statusChanged(m_previousStatus, newStatus);
    emitConnectedAndDisconnected(newStatus);
    emitSyncComplete(newStatus);

    // update status variables
    m_initialized = true;
    m_previousStatus = newStatus;
}

/*!
 * \brief Emits the connected() or disconnected() signal.
 */
void SyncthingNotifier::emitConnectedAndDisconnected(SyncthingStatus newStatus)
{
    if (!(m_enabledNotifications & SyncthingHighLevelNotification::ConnectedDisconnected)) {
        return;
    }

    switch (newStatus) {
    case SyncthingStatus::Disconnected:
        if (m_initialized
#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
            && m_service.isManuallyStopped()
#endif
        ) {
            emit disconnected();
        }
        break;
    default:
        switch (m_previousStatus) {
        case SyncthingStatus::Disconnected:
        case SyncthingStatus::Reconnecting:
            emit connected();
            break;
        default:;
        }
    }
}

/*!
 * \brief Emits the syncComplete() signal.
 */
void SyncthingNotifier::emitSyncComplete(SyncthingStatus newStatus)
{
    if (!(m_enabledNotifications & SyncthingHighLevelNotification::SyncComplete)) {
        return;
    }

    switch (newStatus) {
    case SyncthingStatus::Disconnected:
    case SyncthingStatus::Reconnecting:
    case SyncthingStatus::Synchronizing:
        break;
    default:
        if (m_previousStatus == SyncthingStatus::Synchronizing) {
            const auto &completedDirs = m_connection.completedDirs();
            if (!completedDirs.empty()) {
                emit syncComplete(completedDirs);
            }
        }
    }
}

} // namespace Data
