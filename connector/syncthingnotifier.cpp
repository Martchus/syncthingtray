#include "./syncthingnotifier.h"
#include "./syncthingconnection.h"
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
    , m_enabledNotifications(SyncthingHighLevelNotification::None)
    , m_previousStatus(SyncthingStatus::Disconnected)
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
void SyncthingNotifier::emitSyncComplete(ChronoUtilities::DateTime when, const SyncthingDir &dir, int index, const SyncthingDev *remoteDev)
{
    VAR_UNUSED(index)
    VAR_UNUSED(remoteDev)

    // discard event if not enabled
    if ((m_enabledNotifications & SyncthingHighLevelNotification::SyncComplete) == 0 || !m_initialized) {
        return;
    }

    // discard event if too old so we don't get "sync complete" messages for all dirs on startup
    if ((DateTime::gmtNow() - when) > TimeSpan::fromSeconds(5)) {
        return;
    }

    // format the notification message
    const auto message(syncCompleteString(std::vector<const SyncthingDir *>{ &dir }));
    if (!message.isEmpty()) {
        emit syncComplete(message);
    }
}

} // namespace Data
