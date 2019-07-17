#include "./settings.h"

#include "../misc/syncthingkiller.h"
#include "../misc/syncthinglauncher.h"

#include "../../connector/syncthingconnection.h"
#include "../../connector/syncthingnotifier.h"
#include "../../connector/syncthingprocess.h"
#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
#include "../../connector/syncthingservice.h"
#endif

// use meta-data of syncthingtray application here
#include "resources/../../tray/resources/config.h"

#include <qtutilities/settingsdialog/qtsettings.h>
#ifdef QT_UTILITIES_SUPPORT_DBUS_NOTIFICATIONS
#include <qtutilities/misc/dbusnotification.h>
#endif

#include <QApplication>
#include <QFile>
#include <QMessageBox>
#include <QSettings>
#include <QSslCertificate>
#include <QSslError>
#include <QStringBuilder>

#include <unordered_map>

using namespace std;
using namespace Data;
using namespace QtUtilities;

namespace std {

template <> struct hash<QString> {
    std::size_t operator()(const QString &str) const
    {
        return qHash(str);
    }
};
} // namespace std

namespace Settings {

/*!
 * \brief The minimum number of seconds the Syncthing service should be active before we try to connect.
 *
 * Because the REST-API is not instantly available after startup and we want to prevent connection errors.
 */
constexpr auto minActiveTimeInSeconds = 5;

/*!
 * \brief Contains the processes for launching extra tools.
 * \remarks Using std::unordered_map instead of QHash because SyncthingProcess can not be copied.
 */
static unordered_map<QString, SyncthingProcess> toolProcesses;

SyncthingProcess &Launcher::toolProcess(const QString &tool)
{
    return toolProcesses[tool];
}

std::vector<SyncthingProcess *> Launcher::allProcesses()
{
    vector<SyncthingProcess *> processes;
    processes.reserve(1 + toolProcesses.size());
    if (auto *const syncthingProcess = SyncthingProcess::mainInstance()) {
        processes.push_back(syncthingProcess);
    }
    for (auto &process : toolProcesses) {
        processes.push_back(&process.second);
    }
    return processes;
}

/*!
 * \brief Starts all processes (Syncthing and tools) if autostart is enabled.
 */
void Launcher::autostart() const
{
    auto *const launcher(SyncthingLauncher::mainInstance());
    if (autostartEnabled && launcher) {
        launcher->launch(*this);
    }
    for (auto i = tools.cbegin(), end = tools.cend(); i != end; ++i) {
        const ToolParameter &toolParams = i.value();
        if (toolParams.autostart && !toolParams.path.isEmpty()) {
            toolProcesses[i.key()].startSyncthing(toolParams.path, SyncthingProcess::splitArguments(toolParams.args));
        }
    }
}

/*!
 * \brief Terminates all launched processes.
 * \remarks Waits until all processes have terminated. If a process hangs, the user is asked to kill.
 */
void Launcher::terminate()
{
    QtGui::SyncthingKiller(allProcesses()).waitForFinished();
}

/*!
 * \brief Applies the launcher settings to the specified \a connection considering the status of the main SyncthingLauncher instance.
 * \remarks
 * - Called by TrayWidget when the launcher settings have been changed.
 * - \a currentConnectionSettings might be nullptr.
 * - Currently this is only about the auto-reconnect interval and connecting instantly.
 * \returns Returns the launcher status with respect to the specified \a connection.
 */
Launcher::LauncherStatus Launcher::apply(
    Data::SyncthingConnection &connection, const SyncthingConnectionSettings *currentConnectionSettings, bool reconnectRequired) const
{
    auto *const launcher(SyncthingLauncher::mainInstance());
    if (!launcher) {
        return LauncherStatus{};
    }
    const auto isRelevant = connection.isLocal();
    const auto isRunning = launcher->isRunning();
    const auto consideredForReconnect = considerForReconnect && isRelevant;

    if (currentConnectionSettings && (!considerForReconnect || !isRelevant || isRunning)) {
        // ensure auto-reconnect is configured according to settings
        connection.setAutoReconnectInterval(currentConnectionSettings->reconnectInterval);
    } else {
        // disable auto-reconnect regardless of the overall settings
        connection.setAutoReconnectInterval(0);
    }

    // connect instantly if service is running
    if (consideredForReconnect) {
        if (reconnectRequired) {
            if (launcher->isActiveFor(minActiveTimeInSeconds)) {
                connection.reconnect();
            } else {
                // give the service (which has just started) a few seconds to initialize
                connection.reconnectLater(minActiveTimeInSeconds * 1000);
            }
        } else if (isRunning && !connection.isConnected()) {
            if (launcher->isActiveFor(minActiveTimeInSeconds)) {
                connection.connect();
            } else {
                // give the service (which has just started) a few seconds to initialize
                connection.connectLater(minActiveTimeInSeconds * 1000);
            }
        }
    }

    return LauncherStatus{ isRelevant, isRunning, consideredForReconnect, showButton && isRelevant };
}

/*!
 * \brief Returns the launcher status with respect to the specified \a connection.
 */
Launcher::LauncherStatus Launcher::status(SyncthingConnection &connection) const
{
    auto *const launcher(SyncthingLauncher::mainInstance());
    if (!launcher) {
        return LauncherStatus{};
    }
    const auto isRelevant = connection.isLocal();
    return LauncherStatus{ isRelevant, launcher->isRunning(), considerForReconnect && isRelevant, showButton && isRelevant };
}

Settings &values()
{
    static Settings settings;
    return settings;
}

void restore()
{
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, QStringLiteral(PROJECT_NAME));
    // move old config to new location
    const QString oldConfig
        = QSettings(QSettings::IniFormat, QSettings::UserScope, QApplication::organizationName(), QApplication::applicationName()).fileName();
    QFile::rename(oldConfig, settings.fileName()) || QFile::remove(oldConfig);
    settings.sync();
    Settings &v = values();

    settings.beginGroup(QStringLiteral("tray"));
    const int connectionCount = settings.beginReadArray(QStringLiteral("connections"));
    auto &primaryConnectionSettings = v.connection.primary;
    if (connectionCount > 0) {
        auto &secondaryConnectionSettings = v.connection.secondary;
        secondaryConnectionSettings.clear();
        secondaryConnectionSettings.reserve(static_cast<size_t>(connectionCount));
        for (int i = 0; i < connectionCount; ++i) {
            SyncthingConnectionSettings *connectionSettings;
            if (i == 0) {
                connectionSettings = &primaryConnectionSettings;
            } else {
                secondaryConnectionSettings.emplace_back();
                connectionSettings = &secondaryConnectionSettings.back();
            }
            settings.setArrayIndex(i);
            connectionSettings->label = settings.value(QStringLiteral("label")).toString();
            if (connectionSettings->label.isEmpty()) {
                connectionSettings->label = (i == 0 ? QStringLiteral("Primary instance") : QStringLiteral("Secondary instance %1").arg(i));
            }
            connectionSettings->syncthingUrl = settings.value(QStringLiteral("syncthingUrl"), connectionSettings->syncthingUrl).toString();
            connectionSettings->authEnabled = settings.value(QStringLiteral("authEnabled"), connectionSettings->authEnabled).toBool();
            connectionSettings->userName = settings.value(QStringLiteral("userName")).toString();
            connectionSettings->password = settings.value(QStringLiteral("password")).toString();
            connectionSettings->apiKey = settings.value(QStringLiteral("apiKey")).toByteArray();
            connectionSettings->trafficPollInterval
                = settings.value(QStringLiteral("trafficPollInterval"), connectionSettings->trafficPollInterval).toInt();
            connectionSettings->devStatsPollInterval
                = settings.value(QStringLiteral("devStatsPollInterval"), connectionSettings->devStatsPollInterval).toInt();
            connectionSettings->errorsPollInterval
                = settings.value(QStringLiteral("errorsPollInterval"), connectionSettings->errorsPollInterval).toInt();
            connectionSettings->reconnectInterval
                = settings.value(QStringLiteral("reconnectInterval"), connectionSettings->reconnectInterval).toInt();
            connectionSettings->httpsCertPath = settings.value(QStringLiteral("httpsCertPath")).toString();
            if (!connectionSettings->loadHttpsCert()) {
                QMessageBox::critical(nullptr, QCoreApplication::applicationName(),
                    QCoreApplication::translate("Settings::restore", "Unable to load certificate \"%1\" when restoring settings.")
                        .arg(connectionSettings->httpsCertPath));
            }
        }
    } else {
        v.firstLaunch = true;
        primaryConnectionSettings.label = QStringLiteral("Primary instance");
    }
    settings.endArray();

    auto &notifyOn = v.notifyOn;
    notifyOn.disconnect = settings.value(QStringLiteral("notifyOnDisconnect"), notifyOn.disconnect).toBool();
    notifyOn.internalErrors = settings.value(QStringLiteral("notifyOnErrors"), notifyOn.internalErrors).toBool();
    notifyOn.launcherErrors = settings.value(QStringLiteral("notifyOnLauncherErrors"), notifyOn.launcherErrors).toBool();
    notifyOn.localSyncComplete = settings.value(QStringLiteral("notifyOnLocalSyncComplete"), notifyOn.localSyncComplete).toBool();
    notifyOn.remoteSyncComplete = settings.value(QStringLiteral("notifyOnRemoteSyncComplete"), notifyOn.remoteSyncComplete).toBool();
    notifyOn.syncthingErrors = settings.value(QStringLiteral("showSyncthingNotifications"), notifyOn.syncthingErrors).toBool();
    notifyOn.newDeviceConnects = settings.value(QStringLiteral("notifyOnNewDeviceConnects"), notifyOn.newDeviceConnects).toBool();
    notifyOn.newDirectoryShared = settings.value(QStringLiteral("notifyOnNewDirectoryShared"), notifyOn.newDirectoryShared).toBool();
#ifdef QT_UTILITIES_SUPPORT_DBUS_NOTIFICATIONS
    v.dbusNotifications = settings.value(QStringLiteral("dbusNotifications"), DBusNotification::isAvailable()).toBool();
#endif
    v.ignoreInavailabilityAfterStart = settings.value(QStringLiteral("ignoreInavailabilityAfterStart"), v.ignoreInavailabilityAfterStart).toUInt();
    auto &appearance = v.appearance;
    appearance.showTraffic = settings.value(QStringLiteral("showTraffic"), appearance.showTraffic).toBool();
    appearance.trayMenuSize = settings.value(QStringLiteral("trayMenuSize"), appearance.trayMenuSize).toSize();
    appearance.frameStyle = settings.value(QStringLiteral("frameStyle"), appearance.frameStyle).toInt();
    appearance.tabPosition = settings.value(QStringLiteral("tabPos"), appearance.tabPosition).toInt();
    appearance.brightTextColors = settings.value(QStringLiteral("brightTextColors"), appearance.brightTextColors).toBool();
    v.statusIcons = StatusIconSettings(settings.value(QStringLiteral("statusIcons")).toString());
    settings.endGroup();

    settings.beginGroup(QStringLiteral("startup"));
    auto &launcher = v.launcher;
    launcher.autostartEnabled = settings.value(QStringLiteral("syncthingAutostart"), launcher.autostartEnabled).toBool();
    launcher.useLibSyncthing = settings.value(QStringLiteral("useLibSyncthing"), launcher.useLibSyncthing).toBool();
    launcher.libSyncthing.configDir = settings.value(QStringLiteral("libSyncthingConfigDir"), launcher.libSyncthing.configDir).toString();
    launcher.syncthingPath = settings.value(QStringLiteral("syncthingPath"), launcher.syncthingPath).toString();
    launcher.syncthingArgs = settings.value(QStringLiteral("syncthingArgs"), launcher.syncthingArgs).toString();
    launcher.considerForReconnect = settings.value(QStringLiteral("considerLauncherForReconnect"), launcher.considerForReconnect).toBool();
    launcher.showButton = settings.value(QStringLiteral("showLauncherButton"), launcher.showButton).toBool();
    settings.beginGroup(QStringLiteral("tools"));
    for (const QString &tool : settings.childGroups()) {
        settings.beginGroup(tool);
        ToolParameter &toolParams = launcher.tools[tool];
        toolParams.autostart = settings.value(QStringLiteral("autostart"), toolParams.autostart).toBool();
        toolParams.path = settings.value(QStringLiteral("path"), toolParams.path).toString();
        toolParams.args = settings.value(QStringLiteral("args"), toolParams.args).toString();
        settings.endGroup();
    }
    settings.endGroup();
#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
    auto &systemd = v.systemd;
    systemd.syncthingUnit = settings.value(QStringLiteral("syncthingUnit"), systemd.syncthingUnit).toString();
    systemd.showButton = settings.value(QStringLiteral("showButton"), systemd.showButton).toBool();
    systemd.considerForReconnect = settings.value(QStringLiteral("considerForReconnect"), systemd.considerForReconnect).toBool();
#endif
    settings.endGroup();

#if defined(SYNCTHINGWIDGETS_USE_WEBENGINE) || defined(SYNCTHINGWIDGETS_USE_WEBKIT)
    settings.beginGroup(QStringLiteral("webview"));
    auto &webView = v.webView;
    webView.disabled = settings.value(QStringLiteral("disabled"), webView.disabled).toBool();
    webView.zoomFactor = settings.value(QStringLiteral("zoomFactor"), webView.zoomFactor).toDouble();
    webView.geometry = settings.value(QStringLiteral("geometry")).toByteArray();
    webView.keepRunning = settings.value(QStringLiteral("keepRunning"), webView.keepRunning).toBool();
    settings.endGroup();
#endif

    v.qt.restore(settings);
}

void save()
{
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, QStringLiteral(PROJECT_NAME));

    const Settings &v = values();

    settings.beginGroup(QStringLiteral("tray"));
    const auto &primaryConnectionSettings = v.connection.primary;
    const auto &secondaryConnectionSettings = v.connection.secondary;
    const int connectionCount = static_cast<int>(1 + secondaryConnectionSettings.size());
    settings.beginWriteArray(QStringLiteral("connections"), connectionCount);
    for (int i = 0; i < connectionCount; ++i) {
        const SyncthingConnectionSettings *connectionSettings
            = (i == 0 ? &primaryConnectionSettings : &secondaryConnectionSettings[static_cast<size_t>(i - 1)]);
        settings.setArrayIndex(i);
        settings.setValue(QStringLiteral("label"), connectionSettings->label);
        settings.setValue(QStringLiteral("syncthingUrl"), connectionSettings->syncthingUrl);
        settings.setValue(QStringLiteral("authEnabled"), connectionSettings->authEnabled);
        settings.setValue(QStringLiteral("userName"), connectionSettings->userName);
        settings.setValue(QStringLiteral("password"), connectionSettings->password);
        settings.setValue(QStringLiteral("apiKey"), connectionSettings->apiKey);
        settings.setValue(QStringLiteral("trafficPollInterval"), connectionSettings->trafficPollInterval);
        settings.setValue(QStringLiteral("devStatsPollInterval"), connectionSettings->devStatsPollInterval);
        settings.setValue(QStringLiteral("errorsPollInterval"), connectionSettings->errorsPollInterval);
        settings.setValue(QStringLiteral("reconnectInterval"), connectionSettings->reconnectInterval);
        settings.setValue(QStringLiteral("httpsCertPath"), connectionSettings->httpsCertPath);
    }
    settings.endArray();

    const auto &notifyOn = v.notifyOn;
    settings.setValue(QStringLiteral("notifyOnDisconnect"), notifyOn.disconnect);
    settings.setValue(QStringLiteral("notifyOnErrors"), notifyOn.internalErrors);
    settings.setValue(QStringLiteral("notifyOnLauncherErrors"), notifyOn.launcherErrors);
    settings.setValue(QStringLiteral("notifyOnLocalSyncComplete"), notifyOn.localSyncComplete);
    settings.setValue(QStringLiteral("notifyOnRemoteSyncComplete"), notifyOn.remoteSyncComplete);
    settings.setValue(QStringLiteral("showSyncthingNotifications"), notifyOn.syncthingErrors);
    settings.setValue(QStringLiteral("notifyOnNewDeviceConnects"), notifyOn.newDeviceConnects);
    settings.setValue(QStringLiteral("notifyOnNewDirectoryShared"), notifyOn.newDirectoryShared);
#ifdef QT_UTILITIES_SUPPORT_DBUS_NOTIFICATIONS
    settings.setValue(QStringLiteral("dbusNotifications"), v.dbusNotifications);
#endif
    settings.setValue(QStringLiteral("ignoreInavailabilityAfterStart"), v.ignoreInavailabilityAfterStart);
    const auto &appearance = v.appearance;
    settings.setValue(QStringLiteral("showTraffic"), appearance.showTraffic);
    settings.setValue(QStringLiteral("trayMenuSize"), appearance.trayMenuSize);
    settings.setValue(QStringLiteral("frameStyle"), appearance.frameStyle);
    settings.setValue(QStringLiteral("tabPos"), appearance.tabPosition);
    settings.setValue(QStringLiteral("brightTextColors"), appearance.brightTextColors);
    settings.setValue(QStringLiteral("statusIcons"), v.statusIcons.toString());
    settings.endGroup();

    settings.beginGroup(QStringLiteral("startup"));
    const auto &launcher = v.launcher;
    settings.setValue(QStringLiteral("syncthingAutostart"), launcher.autostartEnabled);
    settings.setValue(QStringLiteral("useLibSyncthing"), launcher.useLibSyncthing);
    settings.setValue(QStringLiteral("libSyncthingConfigDir"), launcher.libSyncthing.configDir);
    settings.setValue(QStringLiteral("syncthingPath"), launcher.syncthingPath);
    settings.setValue(QStringLiteral("syncthingArgs"), launcher.syncthingArgs);
    settings.setValue(QStringLiteral("considerLauncherForReconnect"), launcher.considerForReconnect);
    settings.setValue(QStringLiteral("showLauncherButton"), launcher.showButton);
    settings.beginGroup(QStringLiteral("tools"));
    for (auto i = launcher.tools.cbegin(), end = launcher.tools.cend(); i != end; ++i) {
        const ToolParameter &toolParams = i.value();
        settings.beginGroup(i.key());
        settings.setValue(QStringLiteral("autostart"), toolParams.autostart);
        settings.setValue(QStringLiteral("path"), toolParams.path);
        settings.setValue(QStringLiteral("args"), toolParams.args);
        settings.endGroup();
    }
    settings.endGroup();
#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
    const auto &systemd = v.systemd;
    settings.setValue(QStringLiteral("syncthingUnit"), systemd.syncthingUnit);
    settings.setValue(QStringLiteral("showButton"), systemd.showButton);
    settings.setValue(QStringLiteral("considerForReconnect"), systemd.considerForReconnect);
#endif
    settings.endGroup();

#if defined(SYNCTHINGWIDGETS_USE_WEBENGINE) || defined(SYNCTHINGWIDGETS_USE_WEBKIT)
    settings.beginGroup(QStringLiteral("webview"));
    const auto &webView = v.webView;
    settings.setValue(QStringLiteral("disabled"), webView.disabled);
    settings.setValue(QStringLiteral("zoomFactor"), webView.zoomFactor);
    settings.setValue(QStringLiteral("geometry"), webView.geometry);
    settings.setValue(QStringLiteral("keepRunning"), webView.keepRunning);
    settings.endGroup();
#endif

    v.qt.save(settings);
}

/*!
 * \brief Applies the notification settings on the specified \a notifier.
 */
void Settings::apply(SyncthingNotifier &notifier) const
{
    auto notifications(SyncthingHighLevelNotification::None);
    if (notifyOn.disconnect) {
        notifications |= SyncthingHighLevelNotification::ConnectedDisconnected;
    }
    if (notifyOn.localSyncComplete) {
        notifications |= SyncthingHighLevelNotification::LocalSyncComplete;
    }
    if (notifyOn.remoteSyncComplete) {
        notifications |= SyncthingHighLevelNotification::RemoteSyncComplete;
    }
    if (notifyOn.newDeviceConnects) {
        notifications |= SyncthingHighLevelNotification::NewDevice;
    }
    if (notifyOn.newDirectoryShared) {
        notifications |= SyncthingHighLevelNotification::NewDir;
    }
    if (notifyOn.launcherErrors) {
        notifications |= SyncthingHighLevelNotification::SyncthingProcessError;
    }
    notifier.setEnabledNotifications(notifications);
    notifier.setIgnoreInavailabilityAfterStart(ignoreInavailabilityAfterStart);
}

#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
/*!
 * \brief Applies the systemd settings to the specified \a connection considering the status of the global SyncthingService instance.
 * \remarks
 * - Called by SyncthingApplet and TrayWidget when the status of the SyncthingService changes or the systemd settings have been changed.
 * - \a currentConnectionSettings might be nullptr.
 * - Currently this is only about the auto-reconnect interval and connecting instantly.
 * \returns Returns the service status with respect to the specified \a connection.
 */
Systemd::ServiceStatus Systemd::apply(
    Data::SyncthingConnection &connection, const SyncthingConnectionSettings *currentConnectionSettings, bool reconnectRequired) const
{
    auto *const service(SyncthingService::mainInstance());
    if (!service) {
        return ServiceStatus{};
    }
    const auto isRelevant = service->isSystemdAvailable() && connection.isLocal();
    const auto isRunning = service->isRunning();
    const auto consideredForReconnect = considerForReconnect && isRelevant;

    if (currentConnectionSettings && (!considerForReconnect || !isRelevant || isRunning)) {
        // ensure auto-reconnect is configured according to settings
        connection.setAutoReconnectInterval(currentConnectionSettings->reconnectInterval);
    } else {
        // disable auto-reconnect regardless of the overall settings
        connection.setAutoReconnectInterval(0);
    }

    // connect instantly if service is running
    if (consideredForReconnect) {
        if (reconnectRequired) {
            if (service->isActiveWithoutSleepFor(minActiveTimeInSeconds)) {
                connection.reconnect();
            } else {
                // give the service (which has just started) a few seconds to initialize
                connection.reconnectLater(minActiveTimeInSeconds * 1000);
            }
        } else if (isRunning && !connection.isConnected()) {
            if (service->isActiveWithoutSleepFor(minActiveTimeInSeconds)) {
                connection.connect();
            } else {
                // give the service (which has just started) a few seconds to initialize
                connection.connectLater(minActiveTimeInSeconds * 1000);
            }
        }
    }

    return ServiceStatus{ isRelevant, isRunning, consideredForReconnect, showButton && isRelevant };
}

/*!
 * \brief Returns the service status with respect to the specified \a connection.
 */
Systemd::ServiceStatus Systemd::status(SyncthingConnection &connection) const
{
    auto *const service(SyncthingService::mainInstance());
    if (!service) {
        return ServiceStatus{};
    }
    const auto isRelevant = service->isSystemdAvailable() && connection.isLocal();
    return ServiceStatus{ isRelevant, service->isRunning(), considerForReconnect && isRelevant, showButton && isRelevant };
}
#endif

} // namespace Settings
