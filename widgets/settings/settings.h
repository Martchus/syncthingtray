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

#include <vector>

namespace Dialogs {
class QtSettings;
}

namespace Data {
class SyncthingProcess;
}

namespace Settings {

struct SYNCTHINGWIDGETS_EXPORT Connection {
    Data::SyncthingConnectionSettings primary;
    std::vector<Data::SyncthingConnectionSettings> secondary;
};

struct SYNCTHINGWIDGETS_EXPORT NotifyOn {
    bool disconnect = true;
    bool internalErrors = true;
    bool syncComplete = true;
    bool syncthingErrors = true;
};

struct SYNCTHINGWIDGETS_EXPORT Appearance {
    bool showTraffic = true;
    QSize trayMenuSize = QSize(450, 400);
    int frameStyle = QFrame::StyledPanel | QFrame::Sunken;
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
    QString syncthingPath =
#ifdef PLATFORM_WINDOWS
        QStringLiteral("syncthing.exe");
#else
        QStringLiteral("syncthing");
#endif
    QString syncthingArgs;
    QHash<QString, ToolParameter> tools;
    QString syncthingCmd() const;
    QString toolCmd(const QString &tool) const;
    static Data::SyncthingProcess &toolProcess(const QString &tool);
    void autostart() const;
    static void terminate();
};

#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
struct SYNCTHINGWIDGETS_EXPORT Systemd {
    QString syncthingUnit = QStringLiteral("syncthing.service");
    bool showButton = false;
    bool considerForReconnect = false;
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
};

Settings SYNCTHINGWIDGETS_EXPORT &values();
void SYNCTHINGWIDGETS_EXPORT restore();
void SYNCTHINGWIDGETS_EXPORT save();
}

#endif // SETTINGS_H
