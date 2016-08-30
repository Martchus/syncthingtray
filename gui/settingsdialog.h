#ifndef SETTINGS_DIALOG_H
#define SETTINGS_DIALOG_H

#include <qtutilities/settingsdialog/settingsdialog.h>
#include <qtutilities/settingsdialog/optionpage.h>
#include <qtutilities/settingsdialog/qtsettings.h>

#include <QWidget>

namespace Settings {
class KnownFieldModel;
class TargetLevelModel;
}

namespace Data {
class SyncthingConnection;
}

namespace QtGui {

BEGIN_DECLARE_UI_FILE_BASED_OPTION_PAGE_CUSTOM_CTOR(ConnectionOptionPage)
public:
    ConnectionOptionPage(Data::SyncthingConnection *connection, QWidget *parentWidget = nullptr);
private:
    DECLARE_SETUP_WIDGETS
    void updateConnectionStatus();
    void applyAndReconnect();
    Data::SyncthingConnection *m_connection;
END_DECLARE_OPTION_PAGE

DECLARE_UI_FILE_BASED_OPTION_PAGE(NotificationsOptionPage)

DECLARE_UI_FILE_BASED_OPTION_PAGE(AppearanceOptionPage)

DECLARE_UI_FILE_BASED_OPTION_PAGE(AutostartOptionPage)

#if defined(SYNCTHINGTRAY_USE_WEBENGINE) || defined(SYNCTHINGTRAY_USE_WEBKIT)
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

#endif // SETTINGS_DIALOG_H
