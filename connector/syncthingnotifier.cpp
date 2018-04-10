#include "./syncthingnotifier.h"
#include "./syncthingconnection.h"
#include "./syncthingprocess.h"
#include "./utils.h"

#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
#include "./syncthingservice.h"
#endif

#include <c++utilities/chrono/datetime.h>

using namespace ChronoUtilities;

namespace Data {

/*!
 * \class SyncthingNotifier
 * \brief The SyncthingNotifier class emits high-level notification for a given SyncthingConnection.
 *
 * In contrast to the signals provided by the SyncthingConnection class, these signals take further apply
 * further logic and take additional information into account (previous status, service status if known, ...).
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
    , m_process(syncthingProcess())
    , m_enabledNotifications(SyncthingHighLevelNotification::None)
    , m_previousStatus(SyncthingStatus::Disconnected)
    , m_ignoreInavailabilityAfterStart(15)
    , m_initialized(false)
{
    connect(&connection, &SyncthingConnection::statusChanged, this, &SyncthingNotifier::handleStatusChangedEvent);
    connect(&connection, &SyncthingConnection::dirCompleted, this, &SyncthingNotifier::emitSyncComplete);
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

    // update status variables
    m_initialized = true;
    m_previousStatus = newStatus;
}

/*!
 * \brief Returns whether a "disconnected" notification should be shown.
 * \todo Unify with InternalError::isRelevant().
 */
bool SyncthingNotifier::isDisconnectRelevant() const
{
    // skip disconnect if not initialized
    if (!m_initialized) {
        return false;
    }

    // skip further considerations if connection is remote
    if (!m_connection.isLocal()) {
        return true;
    }

    // consider process/launcher or systemd unit status
    if (m_process.isManuallyStopped()) {
        return false;
    }
#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
    const SyncthingService &service(syncthingService());
    if (m_service.isManuallyStopped()) {
        return false;
    }
#endif

    // ignore inavailability after start or standby-wakeup
    if (m_ignoreInavailabilityAfterStart) {
        if (m_process.isRunning()
#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
            && ((m_service.isSystemdAvailable() && !service.isActiveWithoutSleepFor(m_process.activeSince(), m_ignoreInavailabilityAfterStart))
                   || !m_process.isActiveFor(m_ignoreInavailabilityAfterStart))
#else
            && !m_process.isActiveFor(m_ignoreInavailabilityAfterStart)
#endif
        ) {
            return false;
        }
#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
        if (m_service.isRunning() && !m_service.isActiveWithoutSleepFor(m_ignoreInavailabilityAfterStart)) {
            return false;
        }
#endif
    }

    return true;
}

/*!
 * \brief Emits the connected() or disconnected() signal.
 */
void SyncthingNotifier::emitConnectedAndDisconnected(SyncthingStatus newStatus)
{
    // discard event if not enabled
    if (!(m_enabledNotifications & SyncthingHighLevelNotification::ConnectedDisconnected)) {
        return;
    }

    switch (newStatus) {
    case SyncthingStatus::Disconnected:
        if (isDisconnectRelevant()) {
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
void SyncthingNotifier::emitSyncComplete(ChronoUtilities::DateTime when, const SyncthingDir &dir, int index, const SyncthingDev *remoteDev)
{
    VAR_UNUSED(when)
    VAR_UNUSED(index)

    // discard event if not enabled
    if (!m_initialized || (!remoteDev && (m_enabledNotifications & SyncthingHighLevelNotification::LocalSyncComplete) == 0)
        || (remoteDev && (m_enabledNotifications & SyncthingHighLevelNotification::LocalSyncComplete) == 0)) {
        return;
    }

    // format the notification message
    const auto message(syncCompleteString(std::vector<const SyncthingDir *>{ &dir }, remoteDev));
    if (!message.isEmpty()) {
        emit syncComplete(message);
    }
}

} // namespace Data
