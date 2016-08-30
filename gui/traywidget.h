#ifndef TRAY_WIDGET_H
#define TRAY_WIDGET_H

#include "./webviewprovider.h"

#include "../data/syncthingconnection.h"
#include "../data/syncthingdirectorymodel.h"
#include "../data/syncthingdevicemodel.h"

#include <QWidget>

#include <memory>

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
    void changeStatus();
    void updateTraffic();
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

}

#endif // TRAY_WIDGET_H
