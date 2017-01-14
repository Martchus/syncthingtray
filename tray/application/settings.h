#ifndef SETTINGS_H
#define SETTINGS_H

#include "../../connector/syncthingconnectionsettings.h"

#include <qtutilities/settingsdialog/qtsettings.h>

#include <c++utilities/conversion/types.h>

#include <QString>
#include <QByteArray>
#include <QSize>
#include <QFrame>
#include <QTabWidget>

#include <vector>

namespace Media {
enum class TagUsage;
enum class ElementPosition;
}

namespace Dialogs {
class QtSettings;
}

namespace Settings {

struct Connection
{
    Data::SyncthingConnectionSettings primary;
    std::vector<Data::SyncthingConnectionSettings> secondary;
};

struct NotifyOn
{
    bool disconnect = true;
    bool internalErrors = true;
    bool syncComplete = true;
    bool syncthingErrors = true;
};

struct Appearance
{
    bool showTraffic = true;
    QSize trayMenuSize = QSize(450, 400);
    int frameStyle = QFrame::StyledPanel | QFrame::Sunken;
    int tabPosition = QTabWidget::South;
    bool brightTextColors = false;
};

struct Launcher
{
    bool enabled = false;
    QString syncthingPath =
#ifdef PLATFORM_WINDOWS
            QStringLiteral("syncthing.exe");
#else
            QStringLiteral("syncthing");
#endif
    QString syncthingArgs;
    QString syncthingCmd() const;
};

#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
struct Systemd
{
    QString syncthingUnit = QStringLiteral("syncthing.service");
    bool showButton = false;
    bool considerForReconnect = false;
};
#endif

#if defined(SYNCTHINGTRAY_USE_WEBENGINE) || defined(SYNCTHINGTRAY_USE_WEBKIT)
struct WebView
{
    bool disabled = false;
    double zoomFactor = 1.0;
    QByteArray geometry;
    bool keepRunning = true;
};
#endif

struct Settings
{
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
#if defined(SYNCTHINGTRAY_USE_WEBENGINE) || defined(SYNCTHINGTRAY_USE_WEBKIT)
    WebView webView;
#endif
    Dialogs::QtSettings qt;
};

Settings &values();
void restore();
void save();

}

#endif // SETTINGS_H
