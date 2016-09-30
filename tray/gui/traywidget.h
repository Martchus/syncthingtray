#ifndef TRAY_WIDGET_H
#define TRAY_WIDGET_H

#include "./webviewprovider.h"

#include "../application/settings.h"

#include "../../connector/syncthingconnection.h"
#include "../../connector/syncthingprocess.h"

#include "../../model/syncthingdirectorymodel.h"
#include "../../model/syncthingdevicemodel.h"
#include "../../model/syncthingdownloadmodel.h"

#include <QWidget>

#include <memory>

QT_FORWARD_DECLARE_CLASS(QFrame)
QT_FORWARD_DECLARE_CLASS(QMenu)
QT_FORWARD_DECLARE_CLASS(QActionGroup)

namespace ApplicationUtilities {
class QtConfigArguments;
}

namespace Dialogs {
class AboutDialog;
}

namespace QtGui {

class WebViewDialog;
class SettingsDialog;
class TrayMenu;

namespace Ui {
class TrayWidget;
}

class TrayWidget : public QWidget
{
    Q_OBJECT

public:
    TrayWidget(TrayMenu *parent = nullptr);
    ~TrayWidget();

    Data::SyncthingConnection &connection();
    QMenu *connectionsMenu();
    static const std::vector<TrayWidget *> &instances();

public slots:
    void showSettingsDialog();
    void showAboutDialog();
    void showWebUi();
    void showOwnDeviceId();
    void showLog();
    void showNotifications();
    void quitTray();

private slots:
    void handleStatusChanged(Data::SyncthingStatus status);
    static void applySettings();
    void openDir(const Data::SyncthingDir &dir);
    void openItemDir(const Data::SyncthingItemDownloadProgress &item);
    void scanDir(const Data::SyncthingDir &dir);
    void pauseResumeDev(const Data::SyncthingDev &dev);
    void changeStatus();
    void updateTraffic();
#ifndef SYNCTHINGTRAY_NO_WEBVIEW
    void handleWebViewDeleted();
#endif
    void handleNewNotification(ChronoUtilities::DateTime when, const QString &msg);
    void handleConnectionSelected(QAction *connectionAction);
    void showDialog(QWidget *dlg);

private:
    TrayMenu *m_menu;
    std::unique_ptr<Ui::TrayWidget> m_ui;
    static SettingsDialog *m_settingsDlg;
    static Dialogs::AboutDialog *m_aboutDlg;
#ifndef SYNCTHINGTRAY_NO_WEBVIEW
    WebViewDialog *m_webViewDlg;
#endif
    QFrame *m_cornerFrame;
    Data::SyncthingConnection m_connection;
    Data::SyncthingDirectoryModel m_dirModel;
    Data::SyncthingDeviceModel m_devModel;
    Data::SyncthingDownloadModel m_dlModel;
    QMenu *m_connectionsMenu;
    QActionGroup *m_connectionsActionGroup;
    Data::SyncthingConnectionSettings *m_selectedConnection;
    std::vector<Data::SyncthingLogEntry> m_notifications;
    static std::vector<TrayWidget *> m_instances;
};

inline Data::SyncthingConnection &TrayWidget::connection()
{
    return m_connection;
}

inline QMenu *TrayWidget::connectionsMenu()
{
    return m_connectionsMenu;
}

inline const std::vector<TrayWidget *> &TrayWidget::instances()
{
    return m_instances;
}

}

#endif // TRAY_WIDGET_H
