#ifndef SETTINGS_H
#define SETTINGS_H

#include "../global.h"

#ifdef SYNCTHINGWIDGETS_USE_LIBSYNCTHING
#include <syncthing/interface.h>
#endif

#include <syncthingconnector/syncthingconnectionsettings.h>
#include <syncthingmodel/syncthingicons.h>

#include <qtutilities/settingsdialog/qtsettings.h>

#include <QByteArray>
#include <QFrame>
#include <QHash>
#include <QPoint>
#include <QSize>
#include <QString>
#include <QTabWidget>

#include <optional>
#include <tuple>
#include <vector>

namespace QtUtilities {
class QtSettings;
}

namespace Data {
struct SyncthingConfig;
class SyncthingProcess;
class SyncthingLauncher;
class SyncthingNotifier;
class SyncthingConnection;
class SyncthingService;
} // namespace Data

namespace QtGui {
struct ProcessWithConnection;
}

namespace Settings {

struct SYNCTHINGWIDGETS_EXPORT Connection {
    Data::SyncthingConnectionSettings primary;
    std::vector<Data::SyncthingConnectionSettings> secondary;
    bool insecure = false; // not persistent, for testing purposes only

    void addConfigFromWizard(const Data::SyncthingConfig &config);
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
    bool showDownloads = false;
    bool showTabTexts = true;
    int windowType = 0; // corresponds to TrayMenu::WindowType
    QSize trayMenuSize = QSize(575, 475);
    int frameStyle = static_cast<int>(QFrame::NoFrame) | static_cast<int>(QFrame::Plain);
    int tabPosition = QTabWidget::South;
    struct SYNCTHINGWIDGETS_EXPORT Positioning {
        QPoint assumedIconPosition;
        bool useCursorPosition = true;
        bool useAssumedIconPosition = false;
        std::optional<QPoint> positionToUse() const;
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
    QString syncthingArgs = QStringLiteral("serve --no-browser --logflags=3");
    QHash<QString, ToolParameter> tools;
    bool considerForReconnect = false;
    bool showButton = false;
    bool stopOnMeteredConnection = false;

#ifdef SYNCTHINGWIDGETS_USE_LIBSYNCTHING
    struct SYNCTHINGWIDGETS_EXPORT LibSyncthing {
        QString configDir;
        QString dataDir;
        ::LibSyncthing::LogLevel logLevel = ::LibSyncthing::LogLevel::Info;
        bool expandPaths = false;
    } libSyncthing;
#endif

    static Data::SyncthingProcess &toolProcess(const QString &tool);
    static Data::SyncthingConnection *connectionForLauncher(Data::SyncthingLauncher *launcher);
    static std::vector<QtGui::ProcessWithConnection> allProcesses();
    void autostart() const;
    static void terminate();
    struct SYNCTHINGWIDGETS_EXPORT LauncherStatus {
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
    bool systemUnit = false;
    bool showButton = false;
    bool considerForReconnect = false;
    bool stopOnMeteredConnection = false;

    struct SYNCTHINGWIDGETS_EXPORT ServiceStatus {
        bool relevant = false;
        bool running = false;
        bool consideredForReconnect = false;
        bool showStartStopButton = false;
        bool userService = true;
    };
    void setupService(Data::SyncthingService &) const;
    ServiceStatus apply(Data::SyncthingConnection &connection, const Data::SyncthingConnectionSettings *currentConnectionSettings,
        bool preventReconnect = false) const;
    ServiceStatus status(Data::SyncthingConnection &connection) const;
};
#endif

struct SYNCTHINGWIDGETS_EXPORT WebView {
    enum class Mode { Builtin, Browser, Command };
    Mode mode;
    QString customCommand;
#if defined(SYNCTHINGWIDGETS_USE_WEBENGINE) || defined(SYNCTHINGWIDGETS_USE_WEBKIT)
    double zoomFactor = 1.0;
    QByteArray geometry;
    bool keepRunning = true;
#endif
};

struct SYNCTHINGWIDGETS_EXPORT Settings {
    bool firstLaunch = false;
    bool fakeFirstLaunch = false; // not persistent, for testing purposes only
    bool enableWipFeatures = false; // not persistent, for testing purposes only
    bool isPlasmoid = false; // not persistent, set by the Plasmoid
    bool isIndependentInstance = false; // not persistent, set if --new-instance was specified
    QString error; // not persistent, whether an error occurred on last restore()/save()
    Connection connection;
    NotifyOn notifyOn;
#ifdef QT_UTILITIES_SUPPORT_DBUS_NOTIFICATIONS
    bool dbusNotifications = false;
#endif
    unsigned int ignoreInavailabilityAfterStart = 15;
    Appearance appearance;
    struct SYNCTHINGWIDGETS_EXPORT Icons {
        Data::StatusIconSettings status;
        Data::StatusIconSettings tray;
        bool distinguishTrayIcons = false;
        bool preferIconsFromTheme = false;
        bool usePaletteForStatus = false;
        bool usePaletteForTray = false;
    } icons;
    Launcher launcher;
#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
    Systemd systemd;
#endif
    WebView webView;
    QtUtilities::QtSettings qt;

    void apply(Data::SyncthingNotifier &notifier) const;
};

SYNCTHINGWIDGETS_EXPORT Settings &values();
SYNCTHINGWIDGETS_EXPORT QSettings &settings();
SYNCTHINGWIDGETS_EXPORT bool restore();
SYNCTHINGWIDGETS_EXPORT bool save();
} // namespace Settings

#endif // SETTINGS_H
