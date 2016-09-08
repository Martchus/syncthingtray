#ifndef TRAY_WIDGET_H
#define TRAY_WIDGET_H

#include "./webviewprovider.h"

#include "../data/syncthingconnection.h"
#include "../data/syncthingdirectorymodel.h"
#include "../data/syncthingdevicemodel.h"
#include "../data/syncthingprocess.h"
#include "../application/settings.h"

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

public slots:
    void showSettingsDialog();
    void showAboutDialog();
    void showWebUi();
    void showOwnDeviceId();
    void showLog();
    void showNotifications();

private slots:
    void handleStatusChanged(Data::SyncthingStatus status);
    void applySettings();
    void openDir(const QModelIndex &dirIndex);
    void scanDir(const QModelIndex &dirIndex);
    void pauseResumeDev(const QModelIndex &devIndex);
    void changeStatus();
    void updateTraffic();
#ifndef SYNCTHINGTRAY_NO_WEBVIEW
    void handleWebViewDeleted();
#endif
    void handleNewNotification(ChronoUtilities::DateTime when, const QString &msg);
    void handleConnectionSelected(QAction *connectionAction);
    void showConnectionsMenu();
    void showDialog(QWidget *dlg);

private:
    TrayMenu *m_menu;
    std::unique_ptr<Ui::TrayWidget> m_ui;
    SettingsDialog *m_settingsDlg;
    Dialogs::AboutDialog *m_aboutDlg;
#ifndef SYNCTHINGTRAY_NO_WEBVIEW
    WebViewDialog *m_webViewDlg;
#endif
    QFrame *m_cornerFrame;
    Data::SyncthingConnection m_connection;
    Data::SyncthingDirectoryModel m_dirModel;
    Data::SyncthingDeviceModel m_devModel;
    QMenu *m_connectionsMenu;
    QActionGroup *m_connectionsActionGroup;
    Settings::ConnectionSettings *m_selectedConnection;
    std::vector<Data::SyncthingLogEntry> m_notifications;
};

inline Data::SyncthingConnection &TrayWidget::connection()
{
    return m_connection;
}

inline QMenu *TrayWidget::connectionsMenu()
{
    return m_connectionsMenu;
}

}

#endif // TRAY_WIDGET_H
