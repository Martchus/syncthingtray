#ifndef TRAY_WIDGET_H
#define TRAY_WIDGET_H

#include <syncthingwidgets/misc/syncthingmodels.h>
#include <syncthingwidgets/settings/settings.h>
#include <syncthingwidgets/webview/webviewdefs.h>

#include <syncthingmodel/syncthingdownloadmodel.h>

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

#ifdef SYNCTHINGWIDGETS_NO_WEBVIEW
using WebViewDialog = void;
#else
class WebViewDialog;
#endif
class SettingsDialog;
class Wizard;
class TrayMenu;

namespace Ui {
class TrayWidget;
}

class TrayWidget : public QWidget {
    Q_OBJECT

public:
    explicit TrayWidget(TrayMenu *parent = nullptr);
    ~TrayWidget() override;

    SyncthingData &data();
    QMenu *connectionsMenu();
    static const std::vector<TrayWidget *> &instances();
    Data::SyncthingConnectionSettings *selectedConnection();
    SettingsDialog *settingsDialog();

public Q_SLOTS:
    void showSettingsDialog();
    void showLauncherSettings();
    void showUpdateSettings();
    void showWizard();
    void showAboutDialog();
    void showWebUI();
    void showOwnDeviceId();
    void showLog();
    void showNotifications();
    void showUsingPositioningSettings();
    void showInternalError(const QString &errorMessage, Data::SyncthingErrorCategory category, int networkError, const QNetworkRequest &request,
        const QByteArray &response);
    void showInternalErrorsButton();
    void showInternalErrorsDialog();
    void restartSyncthing();
    void quitTray();
    void applySettings(const QString &connectionConfig = QString());
    void applySettingsChangesFromWizard();
    void saveSettings();

protected:
    bool event(QEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;

private Q_SLOTS:
    void handleStatusChanged(Data::SyncthingStatus status);
    static void applySettingsOnAllInstances();
    void openDir(const Data::SyncthingDir &dir);
    void openItemDir(const Data::SyncthingItemDownloadProgress &item);
    void scanDir(const Data::SyncthingDir &dir);
    void pauseResumeDev(const Data::SyncthingDev &dev);
    void pauseResumeDir(const Data::SyncthingDir &dir);
    void browseRemoteFiles(const Data::SyncthingDir &dir);
    void showIgnorePatterns(const Data::SyncthingDir &dir);
    void showRecentChangesContextMenu(const QPoint &position);
    void handleCurrentTabChanged(int index);
    void changeStatus();
    void updateTraffic();
    bool updateTrafficText();
    void updateOverallStatistics();
    void updateIconAndTooltip();
    void toggleRunning();
    Settings::Launcher::LauncherStatus handleLauncherStatusChanged();
    Settings::Launcher::LauncherStatus handleLauncherGuiAddressChanged(const QUrl &guiAddress);
    Settings::Launcher::LauncherStatus applyLauncherSettings(
        bool reconnectRequired = false, bool skipApplyingToConnection = false, bool skipStartStopButton = false);
#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
    Settings::Systemd::ServiceStatus handleSystemdStatusChanged();
    Settings::Systemd::ServiceStatus applySystemdSettings(bool reconnectRequired = false);
#endif
    void handleWebViewDeleted();
    void handleNotificationsDialogDeleted();
    void handleConnectionSelected(QAction *connectionAction);
    void handleNewErrors();
    void concludeWizard(const QString &errorMessage = QString());
    void showDialog(QWidget *dlg, bool maximized = false);
    void showCenteredDialog(QWidget *dlg, const QSize &size);
    void setBrightColorsOfModelsAccordingToPalette();
    void setLabelPixmaps();
    void setTrafficPixmaps(bool recompute = false);
    void connectWithUpdateNotifier();

private:
    TrayMenu *m_menu;
    std::unique_ptr<Ui::TrayWidget> m_ui;
    static SettingsDialog *s_settingsDlg;
    static Wizard *s_wizard;
    static QtUtilities::AboutDialog *s_aboutDlg;
    WebViewDialog *m_webViewDlg;
    QDialog *m_notificationsDlg;
    QFrame *m_cornerFrame;
    QPushButton *m_internalErrorsButton;
    SyncthingData m_data;
    SyncthingModels m_models;
    Data::SyncthingDownloadModel m_dlModel;
    QMenu *m_connectionsMenu;
    QActionGroup *m_connectionsActionGroup;
    Data::SyncthingConnectionSettings *m_selectedConnection;
    QMenu *m_notificationsMenu;
    enum class StartStopButtonTarget { None, Service, Launcher } m_startStopButtonTarget;
    QStringList m_tabTexts;
    struct {
        QPixmap uploadIconActive;
        QPixmap uploadIconInactive;
        QPixmap downloadIconActive;
        QPixmap downloadIconInactive;
    } m_trafficIcons;
    bool m_tabTextsShown;
    bool m_applyingSettingsForWizard;
    static std::vector<TrayWidget *> s_instances;
};

inline SyncthingData &TrayWidget::data()
{
    return m_data;
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
