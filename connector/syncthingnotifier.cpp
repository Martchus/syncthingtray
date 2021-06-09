#include "./syncthingnotifier.h"
#include "./syncthingconnection.h"
#include "./syncthingprocess.h"
#include "./utils.h"

#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
#include "./syncthingservice.h"
#endif

#include <c++utilities/chrono/datetime.h>

using namespace CppUtilities;

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
    , m_service(SyncthingService::mainInstance())
#endif
    , m_process(SyncthingProcess::mainInstance())
    , m_enabledNotifications(SyncthingHighLevelNotification::None)
    , m_previousStatus(SyncthingStatus::Disconnected)
    , m_ignoreInavailabilityAfterStart(15)
    , m_initialized(false)
{
    connect(&connection, &SyncthingConnection::statusChanged, this, &SyncthingNotifier::handleStatusChangedEvent);
    connect(&connection, &SyncthingConnection::dirCompleted, this, &SyncthingNotifier::emitSyncComplete);
    connect(&connection, &SyncthingConnection::newDevAvailable, this, &SyncthingNotifier::handleNewDevEvent);
    connect(&connection, &SyncthingConnection::newDirAvailable, this, &SyncthingNotifier::handleNewDirEvent);
    if (m_process) {
        connect(m_process, &SyncthingProcess::errorOccurred, this, &SyncthingNotifier::handleSyncthingProcessError);
    }
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

void SyncthingNotifier::handleNewDevEvent(DateTime when, const QString &devId, const QString &address)
{
    CPP_UTILITIES_UNUSED(when)

    // ignore if not enabled
    if (!(m_enabledNotifications & SyncthingHighLevelNotification::NewDevice)) {
        return;
    }

    emit newDevice(devId, tr("Device %1 (%2) wants to connect.").arg(devId, address));
}

void SyncthingNotifier::handleNewDirEvent(DateTime when, const QString &devId, const SyncthingDev *dev, const QString &dirId, const QString &dirLabel)
{
    CPP_UTILITIES_UNUSED(when)

    // ignore if not enabled
    if (!(m_enabledNotifications & SyncthingHighLevelNotification::NewDir)) {
        return;
    }

    // format message
    const auto message([&devId, dev, &dirId, &dirLabel] {
        const auto devPrefix(dev ? (tr("Device ") + dev->displayName()) : (tr("Unknown device ") + devId));
        if (dirLabel.isEmpty()) {
            return devPrefix + tr(" wants to share directory %1.").arg(dirId);
        } else {
            return devPrefix + tr(" wants to share directory %1 (%2).").arg(dirLabel, dirId);
        }
    }());
    emit newDir(devId, dirId, message);
}

void SyncthingNotifier::handleSyncthingProcessError(QProcess::ProcessError processError)
{
    if (!(m_enabledNotifications & SyncthingHighLevelNotification::SyncthingProcessError)) {
        return;
    }

    const auto error = m_process->errorString();
    switch (processError) {
    case QProcess::FailedToStart:
        emit syncthingProcessError(tr("Failed to start Syncthing"),
            error.isEmpty() ? tr("Maybe the configured binary path is wrong or the binary is not marked as executable.") : error);
        break;
    case QProcess::Crashed:
        emit syncthingProcessError(tr("Syncthing crashed"), error);
        break;
    default:
        emit syncthingProcessError(tr("Syncthing launcher error occurred"), error);
    }
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
    if (m_process && m_process->isManuallyStopped()) {
        return false;
    }
#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
    if (m_service && m_service->isManuallyStopped()) {
        return false;
    }
#endif

    // ignore inavailability after start or standby-wakeup
    if (m_ignoreInavailabilityAfterStart) {
        if ((m_process && m_process->isRunning())
#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
            && ((m_service && m_service->isSystemdAvailable()
                    && !m_service->isActiveWithoutSleepFor(m_process->activeSince(), m_ignoreInavailabilityAfterStart))
                || !m_process->isActiveFor(m_ignoreInavailabilityAfterStart))
#else
            && !m_process->isActiveFor(m_ignoreInavailabilityAfterStart)
#endif
        ) {
            return false;
        }
#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
        if (m_service && m_service->isRunning() && !m_service->isActiveWithoutSleepFor(m_ignoreInavailabilityAfterStart)) {
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
void SyncthingNotifier::emitSyncComplete(CppUtilities::DateTime when, const SyncthingDir &dir, int index, const SyncthingDev *remoteDev)
{
    CPP_UTILITIES_UNUSED(when)
    CPP_UTILITIES_UNUSED(index)

    // discard event for paused directories/devices
    if (dir.paused || (remoteDev && remoteDev->paused)) {
        return;
    }

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
