#ifndef TRAY_H
#define TRAY_H

#include "./webviewprovider.h"
#include "../data/syncthingconnection.h"
#include "../data/syncthingdirectorymodel.h"
#include "../data/syncthingdevicemodel.h"

#include <QWidget>
#include <QSystemTrayIcon>
#include <QMenu>

#include <memory>

QT_FORWARD_DECLARE_CLASS(QString)
QT_FORWARD_DECLARE_CLASS(QPixmap)

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

public slots:
    void showSettingsDialog();
    void showAboutDialog();
    void showWebUi();
    void showOwnDeviceId();
    void showLog();

private slots:
    void updateStatusButton(Data::SyncthingStatus status);
    void applySettings();
    void openDir(const QModelIndex &dirIndex);
    void scanDir(const QModelIndex &dirIndex);
    void pauseResumeDev(const QModelIndex &devIndex);
    void handleStatusButtonClicked();
    void handleWebViewDeleted();

private:
    TrayMenu *m_menu;
    std::unique_ptr<Ui::TrayWidget> m_ui;
    SettingsDialog *m_settingsDlg;
    Dialogs::AboutDialog *m_aboutDlg;
#ifndef SYNCTHINGTRAY_NO_WEBVIEW
    WebViewDialog *m_webViewDlg;
#endif
    Data::SyncthingConnection m_connection;
    Data::SyncthingDirectoryModel m_dirModel;
    Data::SyncthingDeviceModel m_devModel;
};

inline Data::SyncthingConnection &TrayWidget::connection()
{
    return m_connection;
}

class TrayMenu : public QMenu
{
    Q_OBJECT

public:
    TrayMenu(QWidget *parent = nullptr);

    QSize sizeHint() const;
    TrayWidget *widget();

private:
    TrayWidget *m_trayWidget;
};

inline TrayWidget *TrayMenu::widget()
{
    return m_trayWidget;
}

class TrayIcon : public QSystemTrayIcon
{
    Q_OBJECT

public:
    TrayIcon(QObject *parent = nullptr);

private slots:
    void handleActivated(QSystemTrayIcon::ActivationReason reason);
    void showSyncthingError(const QString &errorMsg);
    void showSyncthingNotification(const QString &message);
    void updateStatusIconAndText(Data::SyncthingStatus status);

private:
    QPixmap renderSvgImage(const QString &path);

    const QSize m_size;
    const QIcon m_statusIconDisconnected;
    const QIcon m_statusIconDefault;
    const QIcon m_statusIconNotify;
    const QIcon m_statusIconPause;
    const QIcon m_statusIconSync;
    TrayMenu m_trayMenu;
    QMenu m_contextMenu;
    Data::SyncthingStatus m_status;
};

}

#endif // TRAY_H
