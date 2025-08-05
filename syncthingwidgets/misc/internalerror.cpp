#include "./internalerror.h"
#include "./syncthinglauncher.h"

#if defined(SYNCTHINGWIDGETS_GUI_QTWIDGETS)
#include "../settings/settings.h"
#endif

#include <syncthingconnector/syncthingconnection.h>
#include <syncthingconnector/syncthingservice.h>

#include <QNetworkReply>

using namespace Data;

namespace QtGui {

/*!
 * \brief Returns whether to ignore inavailability after start or standby-wakeup.
 */
static bool ignoreInavailabilityAfterStart(unsigned int ignoreInavailabilityAfterStartSec, const SyncthingLauncher *launcher,
#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
    const SyncthingService *service,
#endif
    const QString &message, int networkError)
{
    if (!ignoreInavailabilityAfterStartSec) {
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
                && !service->isActiveWithoutSleepFor(launcher->activeSince(), ignoreInavailabilityAfterStartSec))
            || !launcher->isActiveWithoutSleepFor(ignoreInavailabilityAfterStartSec))
#else
        && !launcher->isActiveWithoutSleepFor(ignoreInavailabilityAfterStartSec)
#endif
    ) {
        return true;
    }
#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
    if (service && !service->isActiveWithoutSleepFor(ignoreInavailabilityAfterStartSec)) {
        return true;
    }
#endif
    return false;
}

/*!
 * \brief Returns whether the error is relevant. Only in this case a notification for the error should be shown.
 * \todo Unify with SyncthingNotifier::isDisconnectRelevant().
 * \remarks
 * The function automatically checks automatically whether the launcher or service have been manually stopped. The argument \a isManuallyStopped can be used in
 * addition in case the launcher/service is not available but the information is known by other means (e.g. in the Android UI process which is informed by this
 * via a Binder message).
 *
 */
bool InternalError::isRelevant(const SyncthingConnection &connection, SyncthingErrorCategory category, const QString &message, int networkError,
    bool useGlobalSettings, bool isManuallyStopped)
{
    // ignore overall connection errors when auto reconnect tries >= 1
    if (category != SyncthingErrorCategory::OverallConnection && category != SyncthingErrorCategory::TLS && connection.autoReconnectTries() >= 1) {
        return false;
    }

    // skip further considerations if connection is remote
    if (!connection.syncthingUrl().isEmpty() && !connection.isLocal()) {
        return true;
    }

    // define default settings
    constexpr auto considerForReconnectDefault = true;
    constexpr auto ignoreInavailabilityAfterStartSecDefault = 15;

#if defined(SYNCTHINGWIDGETS_GUI_QTWIDGETS)
    // ignore configuration errors on first launch (to avoid greeting people with an error message)
    const auto &settings = Settings::values();
    if ((settings.firstLaunch || settings.fakeFirstLaunch)
        && (category == SyncthingErrorCategory::OverallConnection && networkError == QNetworkReply::NoError)) {
        return false;
    }

    // decide based on global user settings what to consider
    const auto considerLauncherForReconnect = useGlobalSettings ? settings.launcher.considerForReconnect : considerForReconnectDefault;
#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
    const auto considerServiceForReconnect = useGlobalSettings ? settings.systemd.considerForReconnect : considerForReconnectDefault;
#endif
    const auto ignoreInavailabilityAfterStartSec
        = useGlobalSettings ? settings.ignoreInavailabilityAfterStart : ignoreInavailabilityAfterStartSecDefault;
#else

    // use always the default settings when not building with Qt Widgets GUI at all
    Q_UNUSED(useGlobalSettings)
    constexpr auto considerLauncherForReconnect = considerForReconnectDefault;
#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
    constexpr auto considerServiceForReconnect = considerForReconnectDefault;
#endif
    constexpr auto ignoreInavailabilityAfterStartSec = ignoreInavailabilityAfterStartSecDefault;
#endif

    // ignore "remote host closed" error if we've just stopped Syncthing ourselves (or "connection refused" which can also be the result of stopping Syncthing ourselves)
    const auto remoteHostClosed = networkError == QNetworkReply::ConnectionRefusedError || networkError == QNetworkReply::RemoteHostClosedError
        || networkError == QNetworkReply::ProxyConnectionClosedError;
    if (remoteHostClosed && isManuallyStopped) {
        return false;
    }
    // consider process/launcher or systemd unit status
    const auto *launcher = SyncthingLauncher::mainInstance();
    if (considerLauncherForReconnect && remoteHostClosed && launcher && launcher->isManuallyStopped()) {
        return false;
    }
#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
    const auto *const service = SyncthingService::mainInstance();
    if (considerServiceForReconnect && remoteHostClosed && service && service->isManuallyStopped()) {
        return false;
    }
#endif

    return !ignoreInavailabilityAfterStart(ignoreInavailabilityAfterStartSec, launcher,
#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
        service,
#endif
        message, networkError);
}
} // namespace QtGui
