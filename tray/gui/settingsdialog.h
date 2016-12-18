#ifndef SETTINGS_DIALOG_H
#define SETTINGS_DIALOG_H

#include "../application/settings.h"

#include <qtutilities/settingsdialog/settingsdialog.h>
#include <qtutilities/settingsdialog/optionpage.h>
#include <qtutilities/settingsdialog/qtsettings.h>

#include <QWidget>
#include <QProcess>

namespace Data {
class SyncthingConnection;
class SyncthingService;
}

namespace QtGui {

BEGIN_DECLARE_UI_FILE_BASED_OPTION_PAGE_CUSTOM_CTOR(ConnectionOptionPage)
public:
    ConnectionOptionPage(Data::SyncthingConnection *connection, QWidget *parentWidget = nullptr);
private:
    DECLARE_SETUP_WIDGETS
    void insertFromConfigFile();
    void updateConnectionStatus();
    void applyAndReconnect();
    bool showConnectionSettings(int index);
    bool cacheCurrentSettings(bool applying);
    void saveCurrentConnectionName(const QString &name);
    void addConnectionSettings();
    void removeConnectionSettings();
    Data::SyncthingConnection *m_connection;
    Data::SyncthingConnectionSettings m_primarySettings;
    std::vector<Data::SyncthingConnectionSettings> m_secondarySettings;
    int m_currentIndex;
END_DECLARE_OPTION_PAGE

DECLARE_UI_FILE_BASED_OPTION_PAGE(NotificationsOptionPage)

DECLARE_UI_FILE_BASED_OPTION_PAGE(AppearanceOptionPage)

DECLARE_UI_FILE_BASED_OPTION_PAGE_CUSTOM_SETUP(AutostartOptionPage)

BEGIN_DECLARE_UI_FILE_BASED_OPTION_PAGE(LauncherOptionPage)
private:
    DECLARE_SETUP_WIDGETS
    void handleSyncthingReadyRead();
    void handleSyncthingExited(int exitCode, QProcess::ExitStatus exitStatus);
    void launch();
    void stop();
    QList<QMetaObject::Connection> m_connections;
    bool m_kill;
END_DECLARE_OPTION_PAGE

#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
BEGIN_DECLARE_UI_FILE_BASED_OPTION_PAGE(SystemdOptionPage)
private:
    DECLARE_SETUP_WIDGETS
    void handleDescriptionChanged(const QString &description);
    void handleStatusChanged(const QString &activeState, const QString &subState);
    void handleEnabledChanged(const QString &unitFileState);
    Data::SyncthingService &m_service;
END_DECLARE_OPTION_PAGE
#endif

#ifndef SYNCTHINGTRAY_NO_WEBVIEW
DECLARE_UI_FILE_BASED_OPTION_PAGE(WebViewOptionPage)
#else
DECLARE_OPTION_PAGE(WebViewOptionPage)
#endif

class SettingsDialog : public Dialogs::SettingsDialog
{
    Q_OBJECT
public:
    explicit SettingsDialog(Data::SyncthingConnection *connection, QWidget *parent = nullptr);
    ~SettingsDialog();
};

}

DECLARE_EXTERN_UI_FILE_BASED_OPTION_PAGE_NS(QtGui, ConnectionOptionPage)
DECLARE_EXTERN_UI_FILE_BASED_OPTION_PAGE_NS(QtGui, NotificationsOptionPage)
DECLARE_EXTERN_UI_FILE_BASED_OPTION_PAGE_NS(QtGui, AppearanceOptionPage)
DECLARE_EXTERN_UI_FILE_BASED_OPTION_PAGE_NS(QtGui, AutostartOptionPage)
DECLARE_EXTERN_UI_FILE_BASED_OPTION_PAGE_NS(QtGui, LauncherOptionPage)
#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
DECLARE_EXTERN_UI_FILE_BASED_OPTION_PAGE_NS(QtGui, SystemdOptionPage)
#endif
#ifndef SYNCTHINGTRAY_NO_WEBVIEW
DECLARE_EXTERN_UI_FILE_BASED_OPTION_PAGE_NS(QtGui, WebViewOptionPage)
#endif

#endif // SETTINGS_DIALOG_H
