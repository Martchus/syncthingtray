#include "./settings.h"

#include "../misc/syncthingkiller.h"
#include "../misc/syncthinglauncher.h"

#include <syncthingconnector/qstringhash.h>
#include <syncthingconnector/syncthingconfig.h>
#include <syncthingconnector/syncthingconnection.h>
#include <syncthingconnector/syncthingconnectionsettings.h>
#include <syncthingconnector/syncthingnotifier.h>
#include <syncthingconnector/syncthingprocess.h>
#include <syncthingconnector/utils.h>
#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
#include <syncthingconnector/syncthingservice.h>
#endif

// use meta-data of syncthingtray application here
#include "resources/../../tray/resources/config.h"

#include <qtutilities/resources/resources.h>
#include <qtutilities/settingsdialog/qtsettings.h>
#ifdef QT_UTILITIES_SUPPORT_DBUS_NOTIFICATIONS
#include <qtutilities/misc/dbusnotification.h>
#endif

#include <c++utilities/application/commandlineutils.h>
#include <c++utilities/io/ansiescapecodes.h>

#include <QApplication>
#include <QCursor>
#include <QFile>
#include <QMessageBox>
#include <QSettings>
#include <QStringBuilder>
#include <QVersionNumber>

#include <iostream>
#include <type_traits>
#include <unordered_map>

using namespace std;
using namespace Data;
using namespace CppUtilities::EscapeCodes;
using namespace QtUtilities;

namespace Settings {

/*!
 * \brief The minimum number of seconds the Syncthing service should be active before we try to connect.
 *
 * Because the REST-API is not instantly available after startup and we want to prevent connection errors.
 */
constexpr auto minActiveTimeInSeconds = 5;

/*!
 * \brief Returns the position to use.
 */
std::optional<QPoint> Appearance::Positioning::positionToUse() const
{
    return useCursorPosition ? std::optional<QPoint>(QCursor::pos())
                             : (useAssumedIconPosition ? std::optional<QPoint>(assumedIconPosition) : std::nullopt);
}

/*!
 * \brief Contains the processes for launching extra tools.
 * \remarks Using std::unordered_map instead of QHash because SyncthingProcess can not be copied.
 */
static std::unordered_map<QString, SyncthingProcess> &toolProcesses()
{
    static auto toolProcesses = std::unordered_map<QString, SyncthingProcess>();
    return toolProcesses;
}

SyncthingProcess &Launcher::toolProcess(const QString &tool)
{
    return toolProcesses()[tool];
}

static bool isLocalAndMatchesPort(const Data::SyncthingConnectionSettings &settings, int port)
{
    if (settings.syncthingUrl.isEmpty() || settings.apiKey.isEmpty()) {
        return false;
    }
    const auto url = QUrl(settings.syncthingUrl);
    return ::Data::isLocal(url) && port == url.port(url.scheme() == QLatin1String("https") ? 443 : 80);
}

Data::SyncthingConnection *Launcher::connectionForLauncher(Data::SyncthingLauncher *launcher)
{
    const auto port = launcher->guiUrl().port(-1);
    if (port < 0) {
        return nullptr;
    }
    auto &connectionSettings = values().connection;
    auto *relevantSetting = isLocalAndMatchesPort(connectionSettings.primary, port) ? &connectionSettings.primary : nullptr;
    if (!relevantSetting) {
        for (auto &secondarySetting : connectionSettings.secondary) {
            if (isLocalAndMatchesPort(secondarySetting, port)) {
                relevantSetting = &secondarySetting;
                continue;
            }
        }
    }
    if (!relevantSetting) {
        return nullptr;
    }
    auto *const connection = new SyncthingConnection();
    connection->setParent(launcher);
    connection->applySettings(*relevantSetting);
    std::cerr << Phrases::Info << "Considering configured connection \"" << relevantSetting->label.toStdString()
              << "\" (URL: " << relevantSetting->syncthingUrl.toStdString() << ") to terminate Syncthing" << Phrases::End;
    return connection;
}

std::vector<QtGui::ProcessWithConnection> Launcher::allProcesses()
{
    auto &tools = toolProcesses();
    auto processes = std::vector<QtGui::ProcessWithConnection>();
    processes.reserve(1 + tools.size());
    if (auto *const launcher = SyncthingLauncher::mainInstance()) {
        processes.emplace_back(launcher->process(), connectionForLauncher(launcher));
    } else if (auto *const process = SyncthingProcess::mainInstance()) {
        processes.emplace_back(process);
    }
    for (auto &[tool, process] : tools) {
        processes.emplace_back(&process);
    }
    return processes;
}

/*!
 * \brief Starts all processes (Syncthing and tools) if autostart is enabled.
 */
void Launcher::autostart() const
{
    auto *const launcher(SyncthingLauncher::mainInstance());
    if (autostartEnabled && launcher && (!stopOnMeteredConnection || !launcher->isNetworkConnectionMetered().value_or(false))) {
        launcher->launch(*this);
    }
    auto &toolProcs = toolProcesses();
    for (auto i = tools.cbegin(), end = tools.cend(); i != end; ++i) {
        const ToolParameter &toolParams = i.value();
        if (toolParams.autostart && !toolParams.path.isEmpty()) {
            toolProcs[i.key()].startSyncthing(toolParams.path, SyncthingProcess::splitArguments(toolParams.args));
        }
    }
}

/*!
 * \brief Terminates all launched processes.
 * \remarks Waits until all processes have terminated. If a process hangs, the user is asked to kill.
 */
void Launcher::terminate()
{
    // trigger termination of all pending processes
    // note: This covers the process launched via SyncthingLauncher and additional tool processes. It does *not* cover
    //       the built-in Syncthing instance.
    auto killer = QtGui::SyncthingKiller(allProcesses());
    // stop the built-in Syncthing instance and wait until it has terminated
#ifdef SYNCTHINGWIDGETS_USE_LIBSYNCTHING
    auto *const launcher = SyncthingLauncher::mainInstance();
    if (launcher) {
        launcher->stopLibSyncthing();
    }
#endif
    // wait until all pending processes have terminated, possibly ask user to kill a process if it does not react
    killer.waitForFinished();
}

/*!
 * \brief Configures the reconnect interval and establishes a connection according to the specified settings/status.
 */
template <typename SyncthingMonitor>
static inline void connectAccordingToSettings(Data::SyncthingConnection &connection, const SyncthingConnectionSettings *currentConnectionSettings,
    const SyncthingMonitor &monitor, bool reconnectRequired, bool considerForReconnect, bool isRelevant, bool isRunning, bool consideredForReconnect)
{
    // set reconnect interval and connect if auto-reconnect is configured
    if (currentConnectionSettings && (!considerForReconnect || !isRelevant || isRunning)) {
        // ensure reconnect interval is configured according to settings and connect if auto-connect is configured
        connection.setAutoReconnectInterval(currentConnectionSettings->reconnectInterval);
        if (!consideredForReconnect && currentConnectionSettings->autoConnect) {
            if (reconnectRequired) {
                connection.reconnect();
            } else {
                connection.connect();
            }
        }
    } else {
        // disable auto-reconnect regardless of the overall settings
        connection.setAutoReconnectInterval(0);
    }

    // connect instantly if service is running
    if (consideredForReconnect) {
        if (reconnectRequired) {
            if (monitor.isActiveWithoutSleepFor(minActiveTimeInSeconds)) {
                connection.reconnect();
            } else if (isRunning) {
                // give the service (which has just started) a few seconds to initialize
                connection.reconnectLater(minActiveTimeInSeconds * 1000);
            }
        } else if (isRunning && !connection.isConnected()) {
            if (monitor.isActiveWithoutSleepFor(minActiveTimeInSeconds)) {
                connection.connect();
            } else {
                // give the service (which has just started) a few seconds to initialize
                connection.connectLater(minActiveTimeInSeconds * 1000);
            }
        }
    }
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
    const auto isRunning = launcher->isRunning() && !launcher->guiUrl().isEmpty();
    const auto consideredForReconnect = considerForReconnect && isRelevant;
    connectAccordingToSettings(
        connection, currentConnectionSettings, *launcher, reconnectRequired, considerForReconnect, isRelevant, isRunning, consideredForReconnect);
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

static void setVarFromEnv(bool &var, const char *envVar)
{
    if (const auto val = CppUtilities::isEnvVariableSet(envVar)) {
        var = val.value();
    }
}

QSettings &settings()
{
    static auto s = QtUtilities::getSettings(QStringLiteral(PROJECT_NAME));
    return *s;
}

bool restore()
{
    auto &settings = ::Settings::settings();
    auto &v = values();
    v.error = QtUtilities::errorMessageForSettings(settings);

    const auto version = QVersionNumber::fromString(settings.value(QStringLiteral("v")).toString());
    settings.beginGroup(QStringLiteral("tray"));
    const int connectionCount = settings.beginReadArray(QStringLiteral("connections"));
    auto &primaryConnectionSettings = v.connection.primary;
    using UnderlyingFlagType = std::underlying_type_t<Data::SyncthingStatusComputionFlags>;
    if (connectionCount > 0) {
        auto &secondaryConnectionSettings = v.connection.secondary;
        secondaryConnectionSettings.clear();
        secondaryConnectionSettings.reserve(static_cast<std::size_t>(connectionCount));
        for (int i = 0; i < connectionCount; ++i) {
            SyncthingConnectionSettings *const connectionSettings = i == 0 ? &primaryConnectionSettings : &secondaryConnectionSettings.emplace_back();
            settings.setArrayIndex(i);
            connectionSettings->label = settings.value(QStringLiteral("label")).toString();
            if (connectionSettings->label.isEmpty()) {
                connectionSettings->label = (i == 0 ? QStringLiteral("Primary instance") : QStringLiteral("Secondary instance %1").arg(i));
            }
            connectionSettings->syncthingUrl = settings.value(QStringLiteral("syncthingUrl"), connectionSettings->syncthingUrl).toString();
            connectionSettings->localPath = settings.value(QStringLiteral("localPath"), connectionSettings->localPath).toString();
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
            connectionSettings->requestTimeout = settings.value(QStringLiteral("requestTimeout"), connectionSettings->requestTimeout).toInt();
            connectionSettings->longPollingTimeout
                = settings.value(QStringLiteral("longPollingTimeout"), connectionSettings->longPollingTimeout).toInt();
            connectionSettings->diskEventLimit = settings.value(QStringLiteral("diskEventLimit"), connectionSettings->diskEventLimit).toInt();
            connectionSettings->autoConnect = settings.value(QStringLiteral("autoConnect"), connectionSettings->autoConnect).toBool();
            connectionSettings->pauseOnMeteredConnection
                = settings.value(QStringLiteral("pauseOnMetered"), connectionSettings->pauseOnMeteredConnection).toBool();
            const auto statusComputionFlags = settings.value(QStringLiteral("statusComputionFlags"),
                QVariant::fromValue(static_cast<UnderlyingFlagType>(connectionSettings->statusComputionFlags)));
            if (statusComputionFlags.canConvert<UnderlyingFlagType>()) {
                connectionSettings->statusComputionFlags
                    = static_cast<Data::SyncthingStatusComputionFlags>(statusComputionFlags.value<UnderlyingFlagType>());
                if (version < QVersionNumber(1, 2)) {
                    connectionSettings->statusComputionFlags
                        += Data::SyncthingStatusComputionFlags::OutOfSync | Data::SyncthingStatusComputionFlags::UnreadNotifications;
                }
            }
#ifndef QT_NO_SSL
            connectionSettings->httpsCertPath = settings.value(QStringLiteral("httpsCertPath")).toString();
            if (!connectionSettings->loadHttpsCert()) {
                QMessageBox::critical(nullptr, QCoreApplication::applicationName(),
                    QCoreApplication::translate("Settings::restore", "Unable to load certificate \"%1\" when restoring settings.")
                        .arg(connectionSettings->httpsCertPath));
            }
#endif
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
    appearance.showDownloads = settings.value(QStringLiteral("showDownloads"), appearance.showDownloads).toBool();
    appearance.showTabTexts = settings.value(QStringLiteral("showTabTexts"), appearance.showTabTexts).toBool();
    if (auto windowType = settings.value(QStringLiteral("windowType")); windowType.isValid()) {
        appearance.windowType = windowType.toInt();
    } else if (auto windowed = settings.value(QStringLiteral("windowed")); windowed.isValid()) {
        appearance.windowType = !windowed.toBool() ? 0 : 1;
    }
    appearance.trayMenuSize = settings.value(QStringLiteral("trayMenuSize"), appearance.trayMenuSize).toSize();
    appearance.frameStyle = settings.value(QStringLiteral("frameStyle"), appearance.frameStyle).toInt();
    appearance.tabPosition = settings.value(QStringLiteral("tabPos"), appearance.tabPosition).toInt();
    v.icons.status = StatusIconSettings(settings.value(QStringLiteral("statusIcons")).toString());
    v.icons.tray = StatusIconSettings(settings.value(QStringLiteral("trayIcons")).toString());
    v.icons.status.renderSize = settings.value(QStringLiteral("statusIconsRenderSize"), v.icons.status.renderSize).toSize();
    v.icons.tray.renderSize = settings.value(QStringLiteral("trayIconsRenderSize"), v.icons.tray.renderSize).toSize();
    v.icons.status.strokeWidth = static_cast<StatusIconStrokeWidth>(
        settings.value(QStringLiteral("statusIconsStrokeWidth"), static_cast<int>(v.icons.status.strokeWidth)).toInt());
    v.icons.tray.strokeWidth = static_cast<StatusIconStrokeWidth>(
        settings.value(QStringLiteral("trayIconsStrokeWidth"), static_cast<int>(v.icons.tray.strokeWidth)).toInt());
    v.icons.distinguishTrayIcons = settings.value(QStringLiteral("distinguishTrayIcons")).toBool();
    v.icons.preferIconsFromTheme = settings.value(QStringLiteral("preferIconsFromTheme")).toBool();
    v.icons.usePaletteForStatus = settings.value(QStringLiteral("usePaletteForStatusIcons")).toBool();
    v.icons.usePaletteForTray = settings.value(QStringLiteral("usePaletteForTrayIcons")).toBool();
    settings.beginGroup(QStringLiteral("positioning"));
    auto &positioning = appearance.positioning;
    positioning.useCursorPosition = settings.value(QStringLiteral("useCursorPos"), positioning.useCursorPosition).toBool();
    positioning.useAssumedIconPosition = settings.value(QStringLiteral("useAssumedIconPosition"), positioning.useAssumedIconPosition).toBool();
    positioning.assumedIconPosition = settings.value(QStringLiteral("assumedIconPos"), positioning.assumedIconPosition).toPoint();
    settings.endGroup();
    settings.endGroup();

    settings.beginGroup(QStringLiteral("startup"));
    auto &launcher = v.launcher;
    launcher.autostartEnabled = settings.value(QStringLiteral("syncthingAutostart"), launcher.autostartEnabled).toBool();
    launcher.useLibSyncthing = settings.value(QStringLiteral("useLibSyncthing"), launcher.useLibSyncthing).toBool();
#ifdef SYNCTHINGWIDGETS_USE_LIBSYNCTHING
    launcher.libSyncthing.configDir = settings.value(QStringLiteral("libSyncthingConfigDir"), launcher.libSyncthing.configDir).toString();
    launcher.libSyncthing.dataDir = settings.value(QStringLiteral("libSyncthingDataDir"), launcher.libSyncthing.dataDir).toString();
    launcher.libSyncthing.logLevel = static_cast<LibSyncthing::LogLevel>(
        settings.value(QStringLiteral("libSyncthing2LogLevel"), static_cast<int>(launcher.libSyncthing.logLevel)).toInt());
    launcher.libSyncthing.expandPaths = settings.value(QStringLiteral("libSyncthingExpandPaths")).toBool();
#endif
    launcher.syncthingPath = settings.value(QStringLiteral("syncthingPath"), launcher.syncthingPath).toString();
    launcher.syncthingArgs = settings.value(QStringLiteral("syncthingArgs"), launcher.syncthingArgs).toString();
    launcher.considerForReconnect = settings.value(QStringLiteral("considerLauncherForReconnect"), launcher.considerForReconnect).toBool();
    launcher.showButton = settings.value(QStringLiteral("showLauncherButton"), launcher.showButton).toBool();
    launcher.stopOnMeteredConnection = settings.value(QStringLiteral("stopOnMetered"), launcher.stopOnMeteredConnection).toBool();
    settings.beginGroup(QStringLiteral("tools"));
    const auto childGroups = settings.childGroups();
    for (const QString &tool : childGroups) {
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
    systemd.systemUnit = settings.value(QStringLiteral("systemUnit"), systemd.systemUnit).toBool();
    systemd.showButton = settings.value(QStringLiteral("showButton"), systemd.showButton).toBool();
    systemd.considerForReconnect = settings.value(QStringLiteral("considerForReconnect"), systemd.considerForReconnect).toBool();
    systemd.stopOnMeteredConnection = settings.value(QStringLiteral("stopServiceOnMetered"), systemd.stopOnMeteredConnection).toBool();
#endif
    settings.endGroup();

    settings.beginGroup(QStringLiteral("webview"));
    auto &webView = v.webView;
    if (auto mode = settings.value(QStringLiteral("mode")); mode.isValid()) {
        webView.mode = static_cast<WebView::Mode>(mode.toInt());
    } else if (auto disabled = settings.value(QStringLiteral("disabled")); disabled.isValid()) {
        webView.mode = disabled.toBool() ? WebView::Mode::Browser : WebView::Mode::Builtin;
    }
    webView.customCommand = settings.value(QStringLiteral("customCommand"), webView.customCommand).toString();
#if defined(SYNCTHINGWIDGETS_USE_WEBENGINE) || defined(SYNCTHINGWIDGETS_USE_WEBKIT)
    webView.zoomFactor = settings.value(QStringLiteral("zoomFactor"), webView.zoomFactor).toDouble();
    webView.geometry = settings.value(QStringLiteral("geometry")).toByteArray();
    webView.keepRunning = settings.value(QStringLiteral("keepRunning"), webView.keepRunning).toBool();
#endif
    settings.endGroup();

    v.qt.restore(settings);

    // restore developer settings from environment variables
    setVarFromEnv(v.fakeFirstLaunch, PROJECT_VARNAME_UPPER "_FAKE_FIRST_LAUNCH");
    setVarFromEnv(v.enableWipFeatures, PROJECT_VARNAME_UPPER "_ENABLE_WIP_FEATURES");
    setVarFromEnv(v.connection.insecure, PROJECT_VARNAME_UPPER "_INSECURE");

    return v.error.isEmpty();
}

bool save()
{
    auto &settings = ::Settings::settings();
    auto &v = values();

    settings.setValue(QStringLiteral("v"), QStringLiteral(APP_VERSION));
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
        settings.setValue(QStringLiteral("localPath"), connectionSettings->localPath);
        settings.setValue(QStringLiteral("authEnabled"), connectionSettings->authEnabled);
        settings.setValue(QStringLiteral("userName"), connectionSettings->userName);
        settings.setValue(QStringLiteral("password"), connectionSettings->password);
        settings.setValue(QStringLiteral("apiKey"), connectionSettings->apiKey);
        settings.setValue(QStringLiteral("trafficPollInterval"), connectionSettings->trafficPollInterval);
        settings.setValue(QStringLiteral("devStatsPollInterval"), connectionSettings->devStatsPollInterval);
        settings.setValue(QStringLiteral("errorsPollInterval"), connectionSettings->errorsPollInterval);
        settings.setValue(QStringLiteral("reconnectInterval"), connectionSettings->reconnectInterval);
        settings.setValue(QStringLiteral("diskEventLimit"), connectionSettings->diskEventLimit);
        settings.setValue(QStringLiteral("requestTimeout"), connectionSettings->requestTimeout);
        settings.setValue(QStringLiteral("longPollingTimeout"), connectionSettings->longPollingTimeout);
        settings.setValue(QStringLiteral("autoConnect"), connectionSettings->autoConnect);
        settings.setValue(QStringLiteral("pauseOnMetered"), connectionSettings->pauseOnMeteredConnection);
        settings.setValue(QStringLiteral("statusComputionFlags"),
            QVariant::fromValue(static_cast<std::underlying_type_t<Data::SyncthingStatusComputionFlags>>(connectionSettings->statusComputionFlags)));
#ifndef QT_NO_SSL
        settings.setValue(QStringLiteral("httpsCertPath"), connectionSettings->httpsCertPath);
#endif
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
    settings.setValue(QStringLiteral("showDownloads"), appearance.showDownloads);
    settings.setValue(QStringLiteral("showTabTexts"), appearance.showTabTexts);
    settings.setValue(QStringLiteral("windowType"), appearance.windowType);
    settings.setValue(QStringLiteral("trayMenuSize"), appearance.trayMenuSize);
    settings.setValue(QStringLiteral("frameStyle"), appearance.frameStyle);
    settings.setValue(QStringLiteral("tabPos"), appearance.tabPosition);
    settings.setValue(QStringLiteral("statusIcons"), v.icons.status.toString());
    settings.setValue(QStringLiteral("trayIcons"), v.icons.tray.toString());
    settings.setValue(QStringLiteral("statusIconsRenderSize"), v.icons.status.renderSize);
    settings.setValue(QStringLiteral("trayIconsRenderSize"), v.icons.tray.renderSize);
    settings.setValue(QStringLiteral("statusIconsStrokeWidth"), static_cast<int>(v.icons.status.strokeWidth));
    settings.setValue(QStringLiteral("trayIconsStrokeWidth"), static_cast<int>(v.icons.tray.strokeWidth));
    settings.setValue(QStringLiteral("distinguishTrayIcons"), v.icons.distinguishTrayIcons);
    settings.setValue(QStringLiteral("preferIconsFromTheme"), v.icons.preferIconsFromTheme);
    settings.setValue(QStringLiteral("usePaletteForStatusIcons"), v.icons.usePaletteForStatus);
    settings.setValue(QStringLiteral("usePaletteForTrayIcons"), v.icons.usePaletteForTray);
    settings.beginGroup(QStringLiteral("positioning"));
    settings.setValue(QStringLiteral("useCursorPos"), appearance.positioning.useCursorPosition);
    settings.setValue(QStringLiteral("useAssumedIconPosition"), appearance.positioning.useAssumedIconPosition);
    settings.setValue(QStringLiteral("assumedIconPos"), appearance.positioning.assumedIconPosition);
    settings.endGroup();
    settings.endGroup();

    settings.beginGroup(QStringLiteral("startup"));
    const auto &launcher = v.launcher;
    settings.setValue(QStringLiteral("syncthingAutostart"), launcher.autostartEnabled);
    settings.setValue(QStringLiteral("useLibSyncthing"), launcher.useLibSyncthing);
#ifdef SYNCTHINGWIDGETS_USE_LIBSYNCTHING
    settings.setValue(QStringLiteral("libSyncthingConfigDir"), launcher.libSyncthing.configDir);
    settings.setValue(QStringLiteral("libSyncthingDataDir"), launcher.libSyncthing.dataDir);
    settings.setValue(QStringLiteral("libSyncthing2LogLevel"), static_cast<int>(launcher.libSyncthing.logLevel));
    settings.setValue(QStringLiteral("libSyncthingExpandPaths"), launcher.libSyncthing.expandPaths);
#endif
    settings.setValue(QStringLiteral("syncthingPath"), launcher.syncthingPath);
    settings.setValue(QStringLiteral("syncthingArgs"), launcher.syncthingArgs);
    settings.setValue(QStringLiteral("considerLauncherForReconnect"), launcher.considerForReconnect);
    settings.setValue(QStringLiteral("showLauncherButton"), launcher.showButton);
    settings.setValue(QStringLiteral("stopOnMetered"), launcher.stopOnMeteredConnection);
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
    settings.setValue(QStringLiteral("systemUnit"), systemd.systemUnit);
    settings.setValue(QStringLiteral("showButton"), systemd.showButton);
    settings.setValue(QStringLiteral("considerForReconnect"), systemd.considerForReconnect);
    settings.setValue(QStringLiteral("stopServiceOnMetered"), systemd.stopOnMeteredConnection);
#endif
    settings.endGroup();

    settings.beginGroup(QStringLiteral("webview"));
    const auto &webView = v.webView;
    settings.setValue(QStringLiteral("mode"), static_cast<int>(webView.mode));
    settings.setValue(QStringLiteral("customCommand"), webView.customCommand);
    settings.setValue(QStringLiteral("disabled"), webView.mode == WebView::Mode::Browser);
#if defined(SYNCTHINGWIDGETS_USE_WEBENGINE) || defined(SYNCTHINGWIDGETS_USE_WEBKIT)
    settings.setValue(QStringLiteral("zoomFactor"), webView.zoomFactor);
    settings.setValue(QStringLiteral("geometry"), webView.geometry);
    settings.setValue(QStringLiteral("keepRunning"), webView.keepRunning);
#endif
    settings.endGroup();

    v.qt.save(settings);

    QtUtilities::saveSettingsWithLogging(settings);
    v.error = QtUtilities::errorMessageForSettings(settings);
    return v.error.isEmpty();
}

/*!
 * \brief Applies the notification settings on the specified \a notifier.
 */
void Settings::apply(SyncthingNotifier &notifier) const
{
    auto notifications = SyncthingHighLevelNotification::None;
    auto integrations = SyncthingStartupIntegration::None;
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
    if (launcher.considerForReconnect) {
        integrations |= SyncthingStartupIntegration::Process;
    }
#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
    if (systemd.considerForReconnect) {
        integrations |= SyncthingStartupIntegration::Service;
    }
#endif
    notifier.setEnabledNotifications(notifications);
    notifier.setConsideredIntegrations(integrations);
    notifier.setIgnoreInavailabilityAfterStart(ignoreInavailabilityAfterStart);
}

#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
/*!
 * \brief Sets the scope and unit name of the specified \a service according to the settings.
 */
void Systemd::setupService(SyncthingService &service) const
{
    service.setStoppingOnMeteredConnection(stopOnMeteredConnection);
    service.setScopeAndUnitName(systemUnit ? SystemdScope::System : SystemdScope::User, syncthingUnit);
}

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
    const auto unitAvailable = service->isUnitAvailable();
    const auto isRunning = unitAvailable && service->isRunning();
    const auto consideredForReconnect = considerForReconnect && isRelevant;
    connectAccordingToSettings(
        connection, currentConnectionSettings, *service, reconnectRequired, considerForReconnect, isRelevant, isRunning, consideredForReconnect);
    return ServiceStatus{ isRelevant, isRunning, consideredForReconnect, showButton && isRelevant, service->isUserScope() };
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
    return ServiceStatus{ isRelevant, service->isRunning(), considerForReconnect && isRelevant, showButton && isRelevant, service->isUserScope() };
}
#endif

/*!
 * \brief Add the specified \a config as primary config possibly backing up the current primary config as secondary config.
 */
void Connection::addConfigFromWizard(const Data::SyncthingConfig &config)
{
    // skip if settings basically don't change
    const auto url = config.syncthingUrl();
    const auto apiKey = config.guiApiKey.toUtf8();
    if (url == primary.syncthingUrl && config.guiUser == primary.userName && config.guiApiKey == primary.apiKey) {
        primary.authEnabled = false; // just disable auth to solely rely on the API key
        primary.autoConnect = true; // ensure the connection is actually established when applying
        return;
    }

    // backup previous primary config unless fields going to be overridden are empty anyway
    if (!primary.syncthingUrl.isEmpty() || !primary.userName.isEmpty() || !primary.password.isEmpty() || !primary.apiKey.isEmpty()) {
        auto &backup = secondary.emplace_back(primary);
        backup.label = QCoreApplication::translate("Settings::Connection", "Backup of %1 (created by wizard)").arg(backup.label);
    }

    primary.syncthingUrl = url;
    primary.localPath.clear();
    primary.userName = config.guiUser;
    primary.authEnabled = false;
    primary.password.clear();
    primary.apiKey = apiKey;
    primary.autoConnect = true; // ensure the connection is actually established when applying
#ifndef QT_NO_SSL
    primary.httpsCertPath = Data::SyncthingConfig::locateHttpsCertificate();
#endif
}

} // namespace Settings
