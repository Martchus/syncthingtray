#ifndef SETTINGS_H
#define SETTINGS_H

#include "../../connector/syncthingconnectionsettings.h"
#include "../../model/syncthingicons.h"
#include "../global.h"

#include <qtutilities/settingsdialog/qtsettings.h>

#include <QByteArray>
#include <QFrame>
#include <QHash>
#include <QPoint>
#include <QSize>
#include <QString>
#include <QTabWidget>

#include <tuple>
#include <vector>

namespace QtUtilities {
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
    bool launcherErrors = true;
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
    struct Positioning {
        QPoint assumedIconPosition;
        bool useCursorPosition = true;
        QPoint positionToUse() const;
    } positioning;
};

struct SYNCTHINGWIDGETS_EXPORT ToolParameter {
    QString path;
    QString args;
    bool autostart = false;
};

struct SYNCTHINGWIDGETS_EXPORT Launcher {
    bool autostartEnabled = false;
    bool useLibSyncthing = false;
    QString syncthingPath =
#ifdef PLATFORM_WINDOWS
        QStringLiteral("syncthing.exe");
#else
        QStringLiteral("syncthing");
#endif
    QString syncthingArgs = QStringLiteral("-no-browser -no-restart -logflags=3");
    QHash<QString, ToolParameter> tools;
    bool considerForReconnect = false;
    bool showButton = false;

    struct LibSyncthing {
        QString configDir;
    } libSyncthing;

    static Data::SyncthingProcess &toolProcess(const QString &tool);
    static std::vector<Data::SyncthingProcess *> allProcesses();
    void autostart() const;
    static void terminate();
    struct LauncherStatus {
        bool relevant = false;
        bool running = false;
        bool consideredForReconnect = false;
        bool showStartStopButton = false;
    };
    LauncherStatus apply(Data::SyncthingConnection &connection, const Data::SyncthingConnectionSettings *currentConnectionSettings,
        bool preventReconnect = false) const;
    LauncherStatus status(Data::SyncthingConnection &connection) const;
};

#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
struct SYNCTHINGWIDGETS_EXPORT Systemd {
    QString syncthingUnit = QStringLiteral("syncthing.service");
    bool showButton = false;
    bool considerForReconnect = false;

    struct ServiceStatus {
        bool relevant = false;
        bool running = false;
        bool consideredForReconnect = false;
        bool showStartStopButton = false;
    };
    ServiceStatus apply(Data::SyncthingConnection &connection, const Data::SyncthingConnectionSettings *currentConnectionSettings,
        bool preventReconnect = false) const;
    ServiceStatus status(Data::SyncthingConnection &connection) const;
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
    Data::StatusIconSettings statusIcons;
    Launcher launcher;
#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
    Systemd systemd;
#endif
#if defined(SYNCTHINGWIDGETS_USE_WEBENGINE) || defined(SYNCTHINGWIDGETS_USE_WEBKIT)
    WebView webView;
#endif
    QtUtilities::QtSettings qt;

    void apply(Data::SyncthingNotifier &notifier) const;
};

Settings SYNCTHINGWIDGETS_EXPORT &values();
void SYNCTHINGWIDGETS_EXPORT restore();
void SYNCTHINGWIDGETS_EXPORT save();
} // namespace Settings

#endif // SETTINGS_H
