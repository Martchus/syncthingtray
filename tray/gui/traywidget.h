#ifndef TRAY_WIDGET_H
#define TRAY_WIDGET_H

#include <syncthingwidgets/settings/settings.h>
#include <syncthingwidgets/webview/webviewdefs.h>

#include <syncthingmodel/syncthingdevicemodel.h>
#include <syncthingmodel/syncthingdirectorymodel.h>
#include <syncthingmodel/syncthingdownloadmodel.h>
#include <syncthingmodel/syncthingrecentchangesmodel.h>
#include <syncthingmodel/syncthingsortfiltermodel.h>

#include <syncthingconnector/syncthingconnection.h>
#include <syncthingconnector/syncthingnotifier.h>
#include <syncthingconnector/syncthingprocess.h>

#include <QWidget>

#include <memory>

QT_FORWARD_DECLARE_CLASS(QFrame)
QT_FORWARD_DECLARE_CLASS(QMenu)
QT_FORWARD_DECLARE_CLASS(QActionGroup)
QT_FORWARD_DECLARE_CLASS(QPushButton)
QT_FORWARD_DECLARE_CLASS(QTreeView)

namespace CppUtilities {
class QtConfigArguments;
}

namespace QtUtilities {
class AboutDialog;
}

namespace QtGui {

class WebViewDialog;
class SettingsDialog;
class TrayMenu;

namespace Ui {
class TrayWidget;
}

class TrayWidget : public QWidget {
    Q_OBJECT

public:
    TrayWidget(TrayMenu *parent = nullptr);
    ~TrayWidget() override;

    Data::SyncthingConnection &connection();
    const Data::SyncthingConnection &connection() const;
    Data::SyncthingNotifier &notifier();
    const Data::SyncthingNotifier &notifier() const;
    QMenu *connectionsMenu();
    static const std::vector<TrayWidget *> &instances();
    Data::SyncthingConnectionSettings *selectedConnection();
    SettingsDialog *settingsDialog();

public Q_SLOTS:
    void showSettingsDialog();
    void showAboutDialog();
    void showWebUi();
    void showOwnDeviceId();
    void showLog();
    void showNotifications();
    void showUsingPositioningSettings();
    void showInternalError(const QString &errorMessage, Data::SyncthingErrorCategory category, int networkError, const QNetworkRequest &request,
        const QByteArray &response);
    void showInternalErrorsButton();
    void showInternalErrorsDialog();
    void dismissNotifications();
    void restartSyncthing();
    void quitTray();
    void applySettings(const QString &connectionConfig = QString());

protected:
    bool event(QEvent *event) override;

private Q_SLOTS:
    void handleStatusChanged(Data::SyncthingStatus status);
    static void applySettingsOnAllInstances();
    void openDir(const Data::SyncthingDir &dir);
    void openItemDir(const Data::SyncthingItemDownloadProgress &item);
    void scanDir(const Data::SyncthingDir &dir);
    void pauseResumeDev(const Data::SyncthingDev &dev);
    void pauseResumeDir(const Data::SyncthingDir &dir);
    void showRecentChangesContextMenu(const QPoint &position);
    void changeStatus();
    void updateTraffic();
    void updateOverallStatistics();
    void updateIconAndTooltip();
    void toggleRunning();
    Settings::Launcher::LauncherStatus handleLauncherStatusChanged();
    Settings::Launcher::LauncherStatus applyLauncherSettings(
        bool reconnectRequired = false, bool skipApplyingToConnection = false, bool skipStartStopButton = false);
#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
    Settings::Systemd::ServiceStatus handleSystemdStatusChanged();
    Settings::Systemd::ServiceStatus applySystemdSettings(bool reconnectRequired = false);
#endif
#ifndef SYNCTHINGWIDGETS_NO_WEBVIEW
    void handleWebViewDeleted();
#endif
    void handleNewNotification(CppUtilities::DateTime when, const QString &msg);
    void handleConnectionSelected(QAction *connectionAction);
    void showDialog(QWidget *dlg, bool maximized = false);
    void setBrightColorsOfModelsAccordingToPalette();

private:
    TrayMenu *m_menu;
    std::unique_ptr<Ui::TrayWidget> m_ui;
    static QWidget *s_dialogParent;
    static SettingsDialog *s_settingsDlg;
    static QtUtilities::AboutDialog *s_aboutDlg;
#ifndef SYNCTHINGWIDGETS_NO_WEBVIEW
    WebViewDialog *m_webViewDlg;
#endif
    QFrame *m_cornerFrame;
    QPushButton *m_internalErrorsButton;
    Data::SyncthingConnection m_connection;
    Data::SyncthingNotifier m_notifier;
    Data::SyncthingDirectoryModel m_dirModel;
    Data::SyncthingSortFilterModel m_sortFilterDirModel;
    Data::SyncthingDeviceModel m_devModel;
    Data::SyncthingSortFilterModel m_sortFilterDevModel;
    Data::SyncthingDownloadModel m_dlModel;
    Data::SyncthingRecentChangesModel m_recentChangesModel;
    QMenu *m_connectionsMenu;
    QActionGroup *m_connectionsActionGroup;
    Data::SyncthingConnectionSettings *m_selectedConnection;
    QMenu *m_notificationsMenu;
    std::vector<Data::SyncthingLogEntry> m_notifications;
    enum class StartStopButtonTarget { None, Service, Launcher } m_startStopButtonTarget;
    static std::vector<TrayWidget *> s_instances;
};

inline Data::SyncthingConnection &TrayWidget::connection()
{
    return m_connection;
}

inline const Data::SyncthingConnection &TrayWidget::connection() const
{
    return m_connection;
}

inline Data::SyncthingNotifier &TrayWidget::notifier()
{
    return m_notifier;
}

inline const Data::SyncthingNotifier &TrayWidget::notifier() const
{
    return m_notifier;
}

inline QMenu *TrayWidget::connectionsMenu()
{
    return m_connectionsMenu;
}

inline const std::vector<TrayWidget *> &TrayWidget::instances()
{
    return s_instances;
}

inline Data::SyncthingConnectionSettings *TrayWidget::selectedConnection()
{
    return m_selectedConnection;
}

} // namespace QtGui

#endif // TRAY_WIDGET_H
