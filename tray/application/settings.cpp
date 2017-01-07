#include "./settings.h"

#include <qtutilities/settingsdialog/qtsettings.h>
#ifdef QT_UTILITIES_SUPPORT_DBUS_NOTIFICATIONS
# include <qtutilities/misc/dbusnotification.h>
#endif

#include <QStringBuilder>
#include <QApplication>
#include <QSettings>
#include <QSslCertificate>
#include <QSslError>
#include <QMessageBox>

using namespace std;
using namespace Data;
#ifdef QT_UTILITIES_SUPPORT_DBUS_NOTIFICATIONS
using namespace MiscUtils;
#endif

namespace Settings {

QString Launcher::syncthingCmd() const
{
    return syncthingPath % QChar(' ') % syncthingArgs;
}

Settings &values()
{
    static Settings settings;
    return settings;
}

void restore()
{
    QSettings settings(QSettings::IniFormat, QSettings::UserScope,  QApplication::organizationName(), QApplication::applicationName());
    Settings &v = values();

    settings.beginGroup(QStringLiteral("tray"));
    const int connectionCount = settings.beginReadArray(QStringLiteral("connections"));
    auto &primaryConnectionSettings = v.connection.primary;
    if(connectionCount > 0) {
        auto &secondaryConnectionSettings = v.connection.secondary;
        secondaryConnectionSettings.clear();
        secondaryConnectionSettings.reserve(static_cast<size_t>(connectionCount));
        for(int i = 0; i < connectionCount; ++i) {
            SyncthingConnectionSettings *connectionSettings;
            if(i == 0) {
                connectionSettings = &primaryConnectionSettings;
            } else {
                secondaryConnectionSettings.emplace_back();
                connectionSettings = &secondaryConnectionSettings.back();
            }
            settings.setArrayIndex(i);
            connectionSettings->label = settings.value(QStringLiteral("label")).toString();
            if(connectionSettings->label.isEmpty()) {
                connectionSettings->label = (i == 0 ? QStringLiteral("Primary instance") : QStringLiteral("Secondary instance %1").arg(i));
            }
            connectionSettings->syncthingUrl = settings.value(QStringLiteral("syncthingUrl"), connectionSettings->syncthingUrl).toString();
            connectionSettings->authEnabled = settings.value(QStringLiteral("authEnabled"), connectionSettings->authEnabled).toBool();
            connectionSettings->userName = settings.value(QStringLiteral("userName")).toString();
            connectionSettings->password = settings.value(QStringLiteral("password")).toString();
            connectionSettings->apiKey = settings.value(QStringLiteral("apiKey")).toByteArray();
            connectionSettings->trafficPollInterval = settings.value(QStringLiteral("trafficPollInterval"), connectionSettings->trafficPollInterval).toInt();
            connectionSettings->devStatsPollInterval = settings.value(QStringLiteral("devStatsPollInterval"), connectionSettings->devStatsPollInterval).toInt();
            connectionSettings->reconnectInterval = settings.value(QStringLiteral("reconnectInterval"), connectionSettings->reconnectInterval).toInt();
            connectionSettings->httpsCertPath = settings.value(QStringLiteral("httpsCertPath")).toString();
            if(!connectionSettings->loadHttpsCert()) {
                QMessageBox::critical(nullptr, QCoreApplication::applicationName(), QCoreApplication::translate("Settings::restore", "Unable to load certificate \"%1\" when restoring settings.").arg(connectionSettings->httpsCertPath));
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
    notifyOn.syncComplete = settings.value(QStringLiteral("notifyOnSyncComplete"), notifyOn.syncComplete).toBool();
    notifyOn.syncthingErrors = settings.value(QStringLiteral("showSyncthingNotifications"), notifyOn.syncthingErrors).toBool();
#ifdef QT_UTILITIES_SUPPORT_DBUS_NOTIFICATIONS
    v.dbusNotifications = settings.value(QStringLiteral("dbusNotifications"), DBusNotification::isAvailable()).toBool();
#endif
    auto &appearance = v.appearance;
    appearance.showTraffic = settings.value(QStringLiteral("showTraffic"), appearance.showTraffic).toBool();
    appearance.trayMenuSize = settings.value(QStringLiteral("trayMenuSize"), appearance.trayMenuSize).toSize();
    appearance.frameStyle = settings.value(QStringLiteral("frameStyle"), appearance.frameStyle).toInt();
    appearance.tabPosition = settings.value(QStringLiteral("tabPos"), appearance.tabPosition).toInt();
    appearance.brightTextColors = settings.value(QStringLiteral("brightTextColors"), appearance.brightTextColors).toBool();
    settings.endGroup();

    settings.beginGroup(QStringLiteral("startup"));
    auto &launcher = v.launcher;
    launcher.enabled = settings.value(QStringLiteral("launchSynchting"), launcher.enabled).toBool();
    launcher.syncthingPath = settings.value(QStringLiteral("syncthingPath"), launcher.syncthingPath).toString();
    launcher.syncthingArgs = settings.value(QStringLiteral("syncthingArgs"), launcher.syncthingArgs).toString();
#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
    auto &systemd = v.systemd;
    systemd.syncthingUnit = settings.value(QStringLiteral("syncthingUnit"), systemd.syncthingUnit).toString();
    systemd.showButton = settings.value(QStringLiteral("showButton"), systemd.showButton).toBool();
    systemd.considerForReconnect = settings.value(QStringLiteral("considerForReconnect"), systemd.considerForReconnect).toBool();
#endif
    settings.endGroup();

#if defined(SYNCTHINGTRAY_USE_WEBENGINE) || defined(SYNCTHINGTRAY_USE_WEBKIT)
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
    QSettings settings(QSettings::IniFormat, QSettings::UserScope,  QApplication::organizationName(), QApplication::applicationName());
    const Settings &v = values();

    settings.beginGroup(QStringLiteral("tray"));
    const auto &primaryConnectionSettings = v.connection.primary;
    const auto &secondaryConnectionSettings = v.connection.secondary;
    const int connectionCount = static_cast<int>(1 + secondaryConnectionSettings.size());
    settings.beginWriteArray(QStringLiteral("connections"), connectionCount);
    for(int i = 0; i < connectionCount; ++i) {
        const SyncthingConnectionSettings *connectionSettings = (i == 0 ? &primaryConnectionSettings : &secondaryConnectionSettings[static_cast<size_t>(i - 1)]);
        settings.setArrayIndex(i);
        settings.setValue(QStringLiteral("label"), connectionSettings->label);
        settings.setValue(QStringLiteral("syncthingUrl"), connectionSettings->syncthingUrl);
        settings.setValue(QStringLiteral("authEnabled"), connectionSettings->authEnabled);
        settings.setValue(QStringLiteral("userName"), connectionSettings->userName);
        settings.setValue(QStringLiteral("password"), connectionSettings->password);
        settings.setValue(QStringLiteral("apiKey"), connectionSettings->apiKey);
        settings.setValue(QStringLiteral("trafficPollInterval"), connectionSettings->trafficPollInterval);
        settings.setValue(QStringLiteral("devStatsPollInterval"), connectionSettings->devStatsPollInterval);
        settings.setValue(QStringLiteral("reconnectInterval"), connectionSettings->reconnectInterval);
        settings.setValue(QStringLiteral("httpsCertPath"), connectionSettings->httpsCertPath);
    }
    settings.endArray();

    const auto &notifyOn = v.notifyOn;
    settings.setValue(QStringLiteral("notifyOnDisconnect"), notifyOn.disconnect);
    settings.setValue(QStringLiteral("notifyOnErrors"), notifyOn.internalErrors);
    settings.setValue(QStringLiteral("notifyOnSyncComplete"), notifyOn.syncComplete);
    settings.setValue(QStringLiteral("showSyncthingNotifications"), notifyOn.syncthingErrors);
#ifdef QT_UTILITIES_SUPPORT_DBUS_NOTIFICATIONS
    settings.setValue(QStringLiteral("dbusNotifications"), v.dbusNotifications);
#endif
    const auto &appearance = v.appearance;
    settings.setValue(QStringLiteral("showTraffic"), appearance.showTraffic);
    settings.setValue(QStringLiteral("trayMenuSize"), appearance.trayMenuSize);
    settings.setValue(QStringLiteral("frameStyle"), appearance.frameStyle);
    settings.setValue(QStringLiteral("tabPos"), appearance.tabPosition);
    settings.setValue(QStringLiteral("brightTextColors"), appearance.brightTextColors);
    settings.endGroup();

    settings.beginGroup(QStringLiteral("startup"));
    const auto &launcher = v.launcher;
    settings.setValue(QStringLiteral("launchSynchting"), launcher.enabled);
    settings.setValue(QStringLiteral("syncthingPath"), launcher.syncthingPath);
    settings.setValue(QStringLiteral("syncthingArgs"), launcher.syncthingArgs);
#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
    const auto &systemd = v.systemd;
    settings.setValue(QStringLiteral("syncthingUnit"), systemd.syncthingUnit);
    settings.setValue(QStringLiteral("showButton"), systemd.showButton);
    settings.setValue(QStringLiteral("considerForReconnect"), systemd.considerForReconnect);
#endif
    settings.endGroup();

#if defined(SYNCTHINGTRAY_USE_WEBENGINE) || defined(SYNCTHINGTRAY_USE_WEBKIT)
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

}
