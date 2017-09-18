#ifndef SETTINGS_DIALOG_H
#define SETTINGS_DIALOG_H

#include "./settings.h"

#include <qtutilities/settingsdialog/optionpage.h>
#include <qtutilities/settingsdialog/qtsettings.h>
#include <qtutilities/settingsdialog/settingsdialog.h>

#include <QProcess>
#include <QWidget>

namespace ChronoUtilities {
class DateTime;
}

namespace Data {
class SyncthingConnection;
class SyncthingService;
class SyncthingProcess;
} // namespace Data

namespace QtGui {

/*!
 * \brief The GuiType enum specifies a GUI type.
 *
 * Such a value can be passed to some option pages to show only the options which are relevant
 * for the particular GUI type.
 */
enum class GuiType {
    TrayWidget,
    Plasmoid,
};

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
void saveCurrentConfigName(const QString &name);
void addNewConfig();
void removeSelectedConfig();
void moveSelectedConfigDown();
void moveSelectedConfigUp();
void setCurrentIndex(int currentIndex);
Data::SyncthingConnection *m_connection;
Data::SyncthingConnectionSettings m_primarySettings;
std::vector<Data::SyncthingConnectionSettings> m_secondarySettings;
int m_currentIndex;
END_DECLARE_OPTION_PAGE

BEGIN_DECLARE_UI_FILE_BASED_OPTION_PAGE_CUSTOM_CTOR(NotificationsOptionPage)
public:
NotificationsOptionPage(GuiType guiType = GuiType::TrayWidget, QWidget *parentWidget = nullptr);

private:
DECLARE_SETUP_WIDGETS
const GuiType m_guiType;
END_DECLARE_OPTION_PAGE

DECLARE_UI_FILE_BASED_OPTION_PAGE(AppearanceOptionPage)

DECLARE_UI_FILE_BASED_OPTION_PAGE_CUSTOM_SETUP(AutostartOptionPage)

BEGIN_DECLARE_UI_FILE_BASED_OPTION_PAGE_CUSTOM_CTOR(LauncherOptionPage)
public:
LauncherOptionPage(QWidget *parentWidget = nullptr);
LauncherOptionPage(const QString &tool, QWidget *parentWidget = nullptr);

private:
DECLARE_SETUP_WIDGETS
void handleSyncthingReadyRead();
void handleSyncthingExited(int exitCode, QProcess::ExitStatus exitStatus);
void launch();
void stop();
Data::SyncthingProcess &m_process;
QList<QMetaObject::Connection> m_connections;
bool m_kill;
QString m_tool;
END_DECLARE_OPTION_PAGE

#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
BEGIN_DECLARE_UI_FILE_BASED_OPTION_PAGE(SystemdOptionPage)
private:
DECLARE_SETUP_WIDGETS
void handleDescriptionChanged(const QString &description);
void handleStatusChanged(const QString &activeState, const QString &subState, ChronoUtilities::DateTime activeSince);
void handleEnabledChanged(const QString &unitFileState);
Data::SyncthingService &m_service;
END_DECLARE_OPTION_PAGE
#endif

#ifndef SYNCTHINGWIDGETS_NO_WEBVIEW
DECLARE_UI_FILE_BASED_OPTION_PAGE(WebViewOptionPage)
#else
DECLARE_OPTION_PAGE(WebViewOptionPage)
#endif

class SYNCTHINGWIDGETS_EXPORT SettingsDialog : public Dialogs::SettingsDialog {
    Q_OBJECT
public:
    explicit SettingsDialog(Data::SyncthingConnection *connection, QWidget *parent = nullptr);
    explicit SettingsDialog(const QList<Dialogs::OptionCategory *> &categories, QWidget *parent = nullptr);
    ~SettingsDialog();

private:
    void init();
};
} // namespace QtGui

DECLARE_EXTERN_UI_FILE_BASED_OPTION_PAGE_NS(QtGui, ConnectionOptionPage)
DECLARE_EXTERN_UI_FILE_BASED_OPTION_PAGE_NS(QtGui, NotificationsOptionPage)
DECLARE_EXTERN_UI_FILE_BASED_OPTION_PAGE_NS(QtGui, AppearanceOptionPage)
DECLARE_EXTERN_UI_FILE_BASED_OPTION_PAGE_NS(QtGui, AutostartOptionPage)
DECLARE_EXTERN_UI_FILE_BASED_OPTION_PAGE_NS(QtGui, LauncherOptionPage)
#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
DECLARE_EXTERN_UI_FILE_BASED_OPTION_PAGE_NS(QtGui, SystemdOptionPage)
#endif
#ifndef SYNCTHINGWIDGETS_NO_WEBVIEW
DECLARE_EXTERN_UI_FILE_BASED_OPTION_PAGE_NS(QtGui, WebViewOptionPage)
#endif

#endif // SETTINGS_DIALOG_H
