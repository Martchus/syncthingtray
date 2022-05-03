#include "./internalerror.h"
#include "./syncthinglauncher.h"

#include "../settings/settings.h"

#include <syncthingconnector/syncthingconnection.h>
#include <syncthingconnector/syncthingservice.h>

#include <QNetworkReply>

using namespace Data;

namespace QtGui {

/*!
 * \brief Returns whether the error is relevant. Only in this case a notification for the error should be shown.
 * \todo Unify with SyncthingNotifier::isDisconnectRelevant().
 */
bool InternalError::isRelevant(const SyncthingConnection &connection, SyncthingErrorCategory category, int networkError)
{
    // ignore overall connection errors when auto reconnect tries >= 1
    if (category != SyncthingErrorCategory::OverallConnection && category != SyncthingErrorCategory::TLS && connection.autoReconnectTries() >= 1) {
        return false;
    }

    // skip further considerations if connection is remote
    if (!connection.isLocal()) {
        return true;
    }

    // consider process/launcher or systemd unit status
    const auto remoteHostClosed(networkError == QNetworkReply::RemoteHostClosedError || networkError == QNetworkReply::ProxyConnectionClosedError);
    // ignore "remote host closed" error if we've just stopped Syncthing ourselves
    const auto *launcher(SyncthingLauncher::mainInstance());
    const auto &settings = Settings::values();
    if (settings.launcher.considerForReconnect && remoteHostClosed && launcher && launcher->isManuallyStopped()) {
        return false;
    }
#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
    const auto *const service(SyncthingService::mainInstance());
    if (settings.systemd.considerForReconnect && remoteHostClosed && service && service->isManuallyStopped()) {
        return false;
    }
#endif

    // ignore inavailability after start or standby-wakeup
    if (settings.ignoreInavailabilityAfterStart) {
        switch (networkError) {
        case QNetworkReply::ConnectionRefusedError:
        case QNetworkReply::HostNotFoundError:
        case QNetworkReply::TemporaryNetworkFailureError:
        case QNetworkReply::NetworkSessionFailedError:
        case QNetworkReply::ProxyConnectionRefusedError:
        case QNetworkReply::ProxyNotFoundError:
            if ((launcher && launcher->isRunning())
#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
                && ((service && service->isSystemdAvailable()
                        && !service->isActiveWithoutSleepFor(launcher->activeSince(), settings.ignoreInavailabilityAfterStart))
                    || !launcher->isActiveFor(settings.ignoreInavailabilityAfterStart))
#else
                && !launcher->isActiveFor(settings.ignoreInavailabilityAfterStart)
#endif
            ) {
                return false;
            }
#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
            if (service && !service->isActiveWithoutSleepFor(settings.ignoreInavailabilityAfterStart)) {
                return false;
            }
#endif
            break;
        }
    }

    return true;
}
} // namespace QtGui
