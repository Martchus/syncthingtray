#include "./internalerror.h"
#include "./syncthinglauncher.h"

#include "../settings/settings.h"

#include <syncthingconnector/syncthingconnection.h>
#include <syncthingconnector/syncthingservice.h>

#include <QNetworkReply>

using namespace Data;

namespace QtGui {

/*!
 * \brief Returns whether to ignore inavailability after start or standby-wakeup.
 */
static bool ignoreInavailabilityAfterStart(const Settings::Settings &settings, const SyncthingLauncher *launcher,
#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
    const SyncthingService *service,
#endif
    const QString &message, int networkError)
{
    if (!settings.ignoreInavailabilityAfterStart) {
        return false;
    }

    // ignore only certain types of errors
    // note: Not sure how to check for "Forbidden" except for checking the error message.
    switch (networkError) {
    case QNetworkReply::ConnectionRefusedError:
    case QNetworkReply::HostNotFoundError:
    case QNetworkReply::TemporaryNetworkFailureError:
    case QNetworkReply::NetworkSessionFailedError:
    case QNetworkReply::ProxyConnectionRefusedError:
    case QNetworkReply::ProxyNotFoundError:
        break;
    default:
        if (message.contains(QLatin1String("Forbidden"))) {
            break;
        }
        return false;
    };

    // ignore inavailable shorty after the start
    if ((launcher && launcher->isRunning())
#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
        && ((service && service->isSystemdAvailable()
                && !service->isActiveWithoutSleepFor(launcher->activeSince(), settings.ignoreInavailabilityAfterStart))
            || !launcher->isActiveFor(settings.ignoreInavailabilityAfterStart))
#else
        && !launcher->isActiveFor(settings.ignoreInavailabilityAfterStart)
#endif
    ) {
        return true;
    }
#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
    if (service && !service->isActiveWithoutSleepFor(settings.ignoreInavailabilityAfterStart)) {
        return true;
    }
#endif
    return false;
}

/*!
 * \brief Returns whether the error is relevant. Only in this case a notification for the error should be shown.
 * \todo Unify with SyncthingNotifier::isDisconnectRelevant().
 */
bool InternalError::isRelevant(const SyncthingConnection &connection, SyncthingErrorCategory category, const QString &message, int networkError)
{
    // ignore overall connection errors when auto reconnect tries >= 1
    if (category != SyncthingErrorCategory::OverallConnection && category != SyncthingErrorCategory::TLS && connection.autoReconnectTries() >= 1) {
        return false;
    }

    // skip further considerations if connection is remote
    if (!connection.syncthingUrl().isEmpty() && !connection.isLocal()) {
        return true;
    }

    // ignore configuration errors on first launch (to avoid greeting people with an error message)
    const auto &settings = Settings::values();
    if ((settings.firstLaunch || settings.fakeFirstLaunch)
        && (category == SyncthingErrorCategory::OverallConnection && networkError == QNetworkReply::NoError)) {
        return false;
    }

    // consider process/launcher or systemd unit status
    const auto remoteHostClosed = networkError == QNetworkReply::ConnectionRefusedError || networkError == QNetworkReply::RemoteHostClosedError
        || networkError == QNetworkReply::ProxyConnectionClosedError;
    // ignore "remote host closed" error if we've just stopped Syncthing ourselves (or "connection refused" which can also be the result of stopping Syncthing ourselves)
    const auto *launcher = SyncthingLauncher::mainInstance();
    if (settings.launcher.considerForReconnect && remoteHostClosed && launcher && launcher->isManuallyStopped()) {
        return false;
    }
#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
    const auto *const service = SyncthingService::mainInstance();
    if (settings.systemd.considerForReconnect && remoteHostClosed && service && service->isManuallyStopped()) {
        return false;
    }
#endif

    return !ignoreInavailabilityAfterStart(settings, launcher,
#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
        service,
#endif
        message, networkError);
}
} // namespace QtGui
