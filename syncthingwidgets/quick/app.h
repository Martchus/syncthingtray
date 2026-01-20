#ifndef SYNCTHING_TRAY_APP_H
#define SYNCTHING_TRAY_APP_H

#include "./appbase.h"
#include "./quickicon.h"
#include "./quickui.h"

#ifdef Q_OS_ANDROID
#include "./android.h"
#endif

#include "../misc/diffhighlighter.h"
#include "../misc/internalerror.h"
#include "../misc/otherdialogs.h"
#include "../misc/statusinfo.h"
#include "../misc/syncthinglauncher.h"
#include "../misc/syncthingmodels.h"

#include <qtutilities/settingsdialog/qtsettings.h>

#include <QJSValue>
#include <QJsonObject>
#include <QQmlApplicationEngine>

#include <array>
#include <atomic>
#include <optional>

QT_FORWARD_DECLARE_CLASS(QTextDocument)
QT_FORWARD_DECLARE_CLASS(QUrl)

namespace QtGui {

class SYNCTHINGWIDGETS_EXPORT App : public AppBase {
    Q_OBJECT
    Q_PROPERTY(QJsonObject settings READ settings WRITE setSettings NOTIFY settingsChanged)
    Q_PROPERTY(bool hasInternalErrors READ hasInternalErrors NOTIFY hasInternalErrorsChanged)
    Q_PROPERTY(QVariantMap statistics READ statistics)
    Q_PROPERTY(bool syncthingStarting READ isSyncthingStarting NOTIFY syncthingStartingChanged)
    Q_PROPERTY(bool syncthingRunning READ isSyncthingRunning NOTIFY syncthingRunningChanged)
    Q_PROPERTY(QUrl syncthingGuiUrl READ syncthingGuiUrl NOTIFY syncthingGuiUrlChanged)
    Q_PROPERTY(bool usingUnixDomainSocket READ isUsingUnixDomainSocket NOTIFY syncthingGuiUrlChanged)
    Q_PROPERTY(QString syncthingRunningStatus READ syncthingRunningStatus NOTIFY syncthingRunningStatusChanged)
    Q_PROPERTY(QString meteredStatus READ meteredStatus NOTIFY meteredStatusChanged)
    Q_PROPERTY(bool importExportOngoing READ isImportExportOngoing NOTIFY importExportOngoingChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusInfoChanged)
    Q_PROPERTY(QIcon statusIcon READ statusIcon NOTIFY statusInfoChanged)
    Q_PROPERTY(QString additionalStatusText READ additionalStatusText NOTIFY statusInfoChanged)
    Q_PROPERTY(bool manualServiceShutdown READ isServiceShutdownManual CONSTANT)
    Q_PROPERTY(bool storagePermissionGranted READ storagePermissionGranted NOTIFY storagePermissionGrantedChanged)
    Q_PROPERTY(bool notificationPermissionGranted READ notificationPermissionGranted NOTIFY notificationPermissionGrantedChanged)
    Q_PROPERTY(QString currentSyncthingHomeDir READ currentSyncthingHomeDir)
    Q_PROPERTY(QString closePreference READ closePreference)
    QML_ELEMENT
    QML_SINGLETON

    enum class ImportExportStatus { None, Checking, Importing, Exporting, CheckingMove, Moving, Cleaning, SavingSupportBundle };

public:
    explicit App(bool insecure, QQmlEngine *engine = nullptr, QObject *parent = nullptr);
    ~App();
    static App *create(QQmlEngine *, QJSEngine *engine);

    // properties
    QJsonObject settings() const
    {
        return m_settings;
    }
    void setSettings(const QJsonObject &settings)
    {
        m_settings = settings;
        applySettings();
        storeSettings();
        emit settingsChanged(m_settings);
    }
    const QString &status() override final;
    bool hasInternalErrors() const
    {
        return !m_internalErrors.isEmpty();
    }
    bool isImportExportOngoing() const
    {
        return m_importExportStatus != ImportExportStatus::None;
    }
    bool syncthingStarting() const
    {
        return m_isSyncthingStarting;
    }
    bool isSyncthingStarting() const
    {
        return m_isSyncthingStarting;
    }
    bool isSyncthingRunning() const override final
    {
        return m_isSyncthingRunning;
    }
    bool mayPauseDevicesOnMeteredNetworkConnection() const override final
    {
        return false; // handle pausing of devices on metered network connection only in the service
    }
    const QUrl &syncthingGuiUrl() const
    {
        return m_syncthingGuiUrl;
    }
    bool isUsingUnixDomainSocket() const
    {
        return m_syncthingGuiUrl.scheme() == QLatin1String("unix") && !m_syncthingUnixSocketPath.isEmpty();
    }
    const QString &syncthingRunningStatus() const
    {
        return m_syncthingRunningStatus;
    }
    const QString &meteredStatus() const
    {
        return m_meteredStatus;
    }
    const QString &statusText() const
    {
        return m_statusInfo.statusText();
    }
    const QIcon &statusIcon() const
    {
        return m_statusInfo.statusIcon();
    }
    const QString &additionalStatusText() const
    {
        return m_statusInfo.additionalStatusText();
    }
    /*!
     * \brief Whether the shutdown of the accomanying AppService is manual.
     * \remarks
     * Under Android, the service is supposed to keep running in the background even when the app UI is closed.
     * Hence the service has to be shutdown manually.
     */
    static constexpr bool isServiceShutdownManual()
    {
#ifdef Q_OS_ANDROID
        return true;
#else
        return false;
#endif
    }
    bool storagePermissionGranted() const;
    bool notificationPermissionGranted() const;
    QString currentSyncthingHomeDir() const;
    QObject *currentDialog();
    const QString &closePreference();
    qint64 databaseSize(const QString &path, const QString &extension) const;
    QVariant formattedDatabaseSize(const QString &path, const QString &extension) const;
    QVariantMap statistics() const;
    void statistics(QVariantMap &res) const;

    // helper functions
    Q_INVOKABLE bool initEngine();
#ifdef Q_OS_ANDROID
    Q_INVOKABLE bool destroyEngine();
#endif
    Q_INVOKABLE bool loadMain();
    Q_INVOKABLE bool reloadMain();
    Q_INVOKABLE bool unloadMain();
    Q_INVOKABLE void shutdownService();
    Q_INVOKABLE bool storeSettings();
    Q_INVOKABLE bool applyLauncherSettings();
    Q_INVOKABLE bool applySettings();
    Q_INVOKABLE bool reloadSettings();
    Q_INVOKABLE bool clearLogfile();
    Q_INVOKABLE bool openSyncthingConfigFile();
    Q_INVOKABLE bool openSyncthingLogFile();
    Q_INVOKABLE QVariantList internalErrors() const;
    Q_INVOKABLE void clearInternalErrors();
    Q_INVOKABLE bool cleanSyncthingHomeDirectory(const QJSValue &callback = QJSValue());
    Q_INVOKABLE bool checkOngoingImportExport();
    Q_INVOKABLE bool checkSettings(QUrl url, const QJSValue &callback = QJSValue());
    Q_INVOKABLE bool importSettings(QVariantMap availableSettings, QVariantMap selectedSettings, const QJSValue &callback = QJSValue());
    Q_INVOKABLE bool exportSettings(QUrl url, const QJSValue &callback = QJSValue());
    Q_INVOKABLE bool checkSyncthingHome(const QJSValue &callback = QJSValue());
    Q_INVOKABLE bool moveSyncthingHome(QString newHomeDir, const QJSValue &callback = QJSValue());
    Q_INVOKABLE bool saveSupportBundle(QUrl url, const QJSValue &callback = QJSValue());
    Q_INVOKABLE void setCurrentControls(bool visible, int tabIndex = -1);
    Q_INVOKABLE bool loadStatistics(const QJSValue &callback);
    Q_INVOKABLE bool minimize();
    Q_INVOKABLE void quit();
    Q_INVOKABLE bool requestStoragePermission();
    Q_INVOKABLE bool requestNotificationPermission();
    Q_INVOKABLE bool showLog(QObject *textArea);
    Q_INVOKABLE void clearLog();
    Q_INVOKABLE bool loadDirErrors(const QString &dirId, QObject *view);
    Q_INVOKABLE void terminateSyncthing();
    Q_INVOKABLE void restartSyncthing();
    Q_INVOKABLE void shutdownSyncthing();
    Q_INVOKABLE void connectToSyncthing();
    Q_INVOKABLE void reconnectToSyncthing();
#ifdef Q_OS_ANDROID
    Q_INVOKABLE void sendMessageToService(ServiceAction action, int arg1 = 0, int arg2 = 0, const QString &str = QString());
    Q_INVOKABLE void handleMessageFromService(ActivityAction action, int arg1, int arg2, const QString &str, const QByteArray &variant);
#endif
    Q_INVOKABLE void handleLauncherStatusBroadcast(const QVariant &status);

Q_SIGNALS:
    void internalError(const QtGui::InternalError &error);
    void info(const QString &infoMessage, const QString &details = QString());
    void logsAvailable(const QString &newLogMessages);
    void hasInternalErrorsChanged();
    void internalErrorsRequested();
    void connectionErrorsRequested();
    void syncthingStartingChanged(bool syncthingStarting);
    void syncthingRunningChanged(bool syncthingRunning);
    void syncthingGuiUrlChanged(const QUrl &syncthingGuiUrl);
    void syncthingRunningStatusChanged(const QString &syncthingRunningStatus);
    void meteredStatusChanged(const QString &meteredStatus);
    void importExportOngoingChanged(bool importExportOngoing);
    void statusInfoChanged();
    void textShared(const QString &text);
    void newDeviceTriggered(const QString &devId);
    void newDirTriggered(const QString &devId, const QString &dirId, const QString &dirLabel);
    void storagePermissionGrantedChanged(bool storagePermissionGranted);
    void notificationPermissionGrantedChanged(bool notificationPermissionGranted);
#ifndef Q_OS_ANDROID
    void syncthingTerminationRequested();
    void syncthingRestartRequested();
    void syncthingShutdownRequested();
    void syncthingConnectRequested();
    void syncthingReconnectRequested();
    void settingsReloadRequested();
    void launcherStatusRequested();
    void stoppingLibSyncthingRequested();
    void clearLogRequested();
    void replayLogRequested();
#endif

protected:
    bool eventFilter(QObject *object, QEvent *event) override;

private Q_SLOTS:
    void handleConnectionError(const QString &errorMessage, Data::SyncthingErrorCategory category, int networkError, const QNetworkRequest &request,
        const QByteArray &response);
    void invalidateStatus() override;
    void handleRunningChanged(bool isRunning);
    void handleChangedDevices();
    void handleNewErrors(const std::vector<Data::SyncthingError> &errors);
    void handleStateChanged(Qt::ApplicationState state);
    void handleConnectionStatusChanged(Data::SyncthingStatus newStatus);
#ifdef Q_OS_ANDROID
    void handleAndroidIntent(const QString &page, bool fromNotification);
    void handleStoragePermissionChanged(bool storagePermissionGranted);
    void handleNotificationPermissionChanged(bool notificationPermissionGranted);
#endif

private:
    void setImportExportStatus(ImportExportStatus importExportStatus);
    QString locateSettingsExportDir();

    std::optional<QQmlApplicationEngine> m_appEngine;
    QQmlEngine *m_engine;
    QString m_syncthingRunningStatus;
    QUrl m_syncthingGuiUrl;
    QString m_meteredStatus;
    QGuiApplication *m_app;
    QtUtilities::QtSettings m_qtSettings;
    QuickUI m_ui;
    SyncthingModels m_models;
#ifdef Q_OS_ANDROID
    mutable std::optional<bool> m_storagePermissionGranted;
    mutable std::optional<bool> m_notificationPermissionGranted;
#endif
    struct {
        QVariantMap availableSettings;
        QVariantMap selectedSettings;
        QJSValue callback;
    } m_settingsImport;
    struct {
        std::optional<QUrl> url;
        QJSValue callback;
    } m_settingsExport;
    struct {
        std::optional<QFile> outputFile;
    } m_download;
    struct {
        std::optional<QString> newHomeDir;
        QJSValue callback;
    } m_homeDirMove;
    std::optional<QString> m_closePreference;
    std::array<QObject *, 5> m_uiObjects;
    ImportExportStatus m_importExportStatus;
    int m_tabIndex;
    bool m_clearingLogfile;
    bool m_isGuiLoaded;
    bool m_unloadGuiWhenHidden;
    bool m_isSyncthingStarting;
    bool m_isSyncthingRunning;
    std::atomic_bool m_isManuallyStopped;
};

inline QVariantList App::internalErrors() const
{
    return m_internalErrors;
}

} // namespace QtGui

#endif // SYNCTHING_TRAY_APP_H
