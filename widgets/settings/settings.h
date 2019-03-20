#ifndef SETTINGS_H
#define SETTINGS_H

#include "../../connector/syncthingconnectionsettings.h"
#include "../global.h"

#include <qtutilities/settingsdialog/qtsettings.h>

#include <c++utilities/conversion/types.h>

#include <QByteArray>
#include <QFrame>
#include <QHash>
#include <QSize>
#include <QString>
#include <QTabWidget>

#include <tuple>
#include <vector>

namespace Dialogs {
class QtSettings;
}

namespace Data {
class SyncthingProcess;
class SyncthingNotifier;
class SyncthingConnection;
} // namespace Data

namespace Settings {

struct SYNCTHINGWIDGETS_EXPORT Connection {
    Data::SyncthingConnectionSettings primary;
    std::vector<Data::SyncthingConnectionSettings> secondary;
};

struct SYNCTHINGWIDGETS_EXPORT NotifyOn {
    bool disconnect = true;
    bool internalErrors = true;
    bool localSyncComplete = false;
    bool remoteSyncComplete = false;
    bool syncthingErrors = true;
    bool newDeviceConnects = false;
    bool newDirectoryShared = false;
};

struct SYNCTHINGWIDGETS_EXPORT Appearance {
    bool showTraffic = true;
    QSize trayMenuSize = QSize(450, 400);
    int frameStyle = QFrame::NoFrame | QFrame::Plain;
    int tabPosition = QTabWidget::South;
    bool brightTextColors = false;
};

struct SYNCTHINGWIDGETS_EXPORT ToolParameter {
    QString path;
    QString args;
    bool autostart = false;
};

struct SYNCTHINGWIDGETS_EXPORT Launcher {
    bool enabled = false;
    bool useLibSyncthing = false;
    QString syncthingPath =
#ifdef PLATFORM_WINDOWS
        QStringLiteral("syncthing.exe");
#else
        QStringLiteral("syncthing");
#endif
    QString syncthingArgs;
    QHash<QString, ToolParameter> tools;
    bool considerForReconnect = false;
    static Data::SyncthingProcess &toolProcess(const QString &tool);
    static std::vector<Data::SyncthingProcess *> allProcesses();
    void autostart() const;
    static void terminate();
};

#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
struct SYNCTHINGWIDGETS_EXPORT Systemd {
    QString syncthingUnit = QStringLiteral("syncthing.service");
    bool showButton = false;
    bool considerForReconnect = false;

#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
    std::tuple<bool, bool> apply(Data::SyncthingConnection &connection, const Data::SyncthingConnectionSettings *currentConnectionSettings,
        bool preventReconnect = false) const;
#endif
};
#endif

#if defined(SYNCTHINGWIDGETS_USE_WEBENGINE) || defined(SYNCTHINGWIDGETS_USE_WEBKIT)
struct SYNCTHINGWIDGETS_EXPORT WebView {
    bool disabled = false;
    double zoomFactor = 1.0;
    QByteArray geometry;
    bool keepRunning = true;
};
#endif

struct SYNCTHINGWIDGETS_EXPORT Settings {
    bool firstLaunch = false;
    Connection connection;
    NotifyOn notifyOn;
#ifdef QT_UTILITIES_SUPPORT_DBUS_NOTIFICATIONS
    bool dbusNotifications = false;
#endif
    unsigned int ignoreInavailabilityAfterStart = 15;
    Appearance appearance;
    Launcher launcher;
#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
    Systemd systemd;
#endif
#if defined(SYNCTHINGWIDGETS_USE_WEBENGINE) || defined(SYNCTHINGWIDGETS_USE_WEBKIT)
    WebView webView;
#endif
    Dialogs::QtSettings qt;

    void apply(Data::SyncthingNotifier &notifier) const;
};

Settings SYNCTHINGWIDGETS_EXPORT &values();
void SYNCTHINGWIDGETS_EXPORT restore();
void SYNCTHINGWIDGETS_EXPORT save();
} // namespace Settings

#endif // SETTINGS_H
