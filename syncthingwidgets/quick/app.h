#ifndef SYNCTHING_TRAY_APP_H
#define SYNCTHING_TRAY_APP_H

#include "./appbase.h"

#ifdef Q_OS_ANDROID
#include "./android.h"
#endif

#include <syncthingwidgets/misc/diffhighlighter.h>
#include <syncthingwidgets/misc/internalerror.h>
#include <syncthingwidgets/misc/otherdialogs.h>
#include <syncthingwidgets/misc/statusinfo.h>
#include <syncthingwidgets/misc/syncthinglauncher.h>
#include <syncthingwidgets/misc/utils.h>
#include <syncthingwidgets/quick/quickicon.h>

#include <syncthingmodel/syncthingdevicemodel.h>
#include <syncthingmodel/syncthingdirectorymodel.h>
#include <syncthingmodel/syncthingfilemodel.h>
#include <syncthingmodel/syncthingrecentchangesmodel.h>
#include <syncthingmodel/syncthingsortfiltermodel.h>

#include <qtutilities/settingsdialog/qtsettings.h>

#include <QJsonObject>
#include <QQmlApplicationEngine>
#include <QtVersion>

#include <array>
#include <atomic>
#include <optional>

QT_FORWARD_DECLARE_CLASS(QTextDocument)
QT_FORWARD_DECLARE_CLASS(QUrl)

namespace QtForkAwesome {
class QuickImageProvider;
}

namespace QtGui {

class SYNCTHINGWIDGETS_EXPORT App : public AppBase {
    Q_OBJECT
    Q_PROPERTY(Data::SyncthingDirectoryModel *dirModel READ dirModel CONSTANT)
    Q_PROPERTY(Data::SyncthingSortFilterModel *sortFilterDirModel READ sortFilterDirModel CONSTANT)
    Q_PROPERTY(Data::SyncthingDeviceModel *devModel READ devModel CONSTANT)
    Q_PROPERTY(Data::SyncthingSortFilterModel *sortFilterDevModel READ sortFilterDevModel CONSTANT)
    Q_PROPERTY(Data::SyncthingRecentChangesModel *changesModel READ changesModel CONSTANT)
    Q_PROPERTY(QJsonObject settings READ settings WRITE setSettings NOTIFY settingsChanged)
    Q_PROPERTY(QString faUrlBase READ faUrlBase CONSTANT)
    Q_PROPERTY(bool darkmodeEnabled READ isDarkmodeEnabled NOTIFY darkmodeEnabledChanged)
    Q_PROPERTY(int iconSize READ iconSize CONSTANT)
    Q_PROPERTY(bool nativePopups READ nativePopups CONSTANT)
    Q_PROPERTY(bool extendedClientArea READ extendedClientArea CONSTANT)
    Q_PROPERTY(QString syncthingVersion READ syncthingVersion CONSTANT)
    Q_PROPERTY(QString qtVersion READ qtVersion CONSTANT)
    Q_PROPERTY(QString readmeUrl READ readmeUrl CONSTANT)
    Q_PROPERTY(QString website READ website CONSTANT)
    Q_PROPERTY(bool hasInternalErrors READ hasInternalErrors NOTIFY hasInternalErrorsChanged)
    Q_PROPERTY(QVariantMap statistics READ statistics)
    Q_PROPERTY(bool savingConfig READ isSavingConfig NOTIFY savingConfigChanged)
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
    Q_PROPERTY(bool scanSupported READ isScanSupported CONSTANT)
    Q_PROPERTY(bool manualServiceShutdown READ isServiceShutdownManual CONSTANT)
    Q_PROPERTY(QString fontFamily READ fontFamily CONSTANT)
    Q_PROPERTY(qreal fontScale READ fontScale CONSTANT)
    Q_PROPERTY(int fontWeightAdjustment READ fontWeightAdjustment CONSTANT)
    Q_PROPERTY(QFont font READ font CONSTANT)
    Q_PROPERTY(bool storagePermissionGranted READ storagePermissionGranted NOTIFY storagePermissionGrantedChanged)
    Q_PROPERTY(bool notificationPermissionGranted READ notificationPermissionGranted NOTIFY notificationPermissionGrantedChanged)
    Q_PROPERTY(QObject *currentDialog READ currentDialog)
    QML_ELEMENT
    QML_SINGLETON

    enum class ImportExportStatus { None, Checking, Importing, Exporting, CheckingMove, Moving, Cleaning, SavingSupportBundle };

public:
    explicit App(bool insecure, QObject *parent = nullptr);
    ~App();
    static App *create(QQmlEngine *, QJSEngine *engine);

    // properties
    Data::SyncthingDirectoryModel *dirModel()
    {
        return &m_dirModel;
    }
    Data::SyncthingSortFilterModel *sortFilterDirModel()
    {
        return &m_sortFilterDirModel;
    }
    Data::SyncthingDeviceModel *devModel()
    {
        return &m_devModel;
    }
    Data::SyncthingSortFilterModel *sortFilterDevModel()
    {
        return &m_sortFilterDevModel;
    }
    Data::SyncthingRecentChangesModel *changesModel()
    {
        return &m_changesModel;
    }
    const QString &faUrlBase()
    {
        return m_faUrlBase;
    }
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
#if defined(Q_OS_ANDROID) || defined(Q_OS_WINDOWS) // it leads to crashes on those platforms
    static constexpr bool nativePopups()
    {
        return false;
    }
#else
    bool nativePopups() const;
#endif
    static constexpr bool extendedClientArea()
    {
        // disable extended client areas by default as further tweaking is needed
        // note: Safe areas are not working as expected with Qt 6.9.2 at all but this is fixed with Qt 6.9.3.
#ifdef SYNCTHINGWIDGETS_EXTENDED_CLIENT_AREA
        return true;
#else
        return false;
#endif
    }
    QString syncthingVersion() const
    {
#ifdef SYNCTHINGWIDGETS_USE_LIBSYNCTHING
        return Data::SyncthingLauncher::libSyncthingVersionInfo().remove(0, 11);
#else
        return tr("not available");
#endif
    }
    QString qtVersion() const
    {
        return QString::fromUtf8(qVersion());
    }
    QString readmeUrl() const
    {
        return QtGui::readmeUrl();
    }
    QString website() const;

    /*!
     * \brief Returns whether darkmode is enabled.
     * \remarks
     * The QML code could just use "Qt.styleHints.colorScheme === Qt.Dark" but this would not be consistent with the approach in
     * QtSettings that also takes the palette into account for platforms without darkmode flag.
     */
    bool isDarkmodeEnabled() const
    {
        return m_darkmodeEnabled;
    }
    int iconSize() const
    {
        return m_iconSize;
    }
    const QString &status() override final;
    bool hasInternalErrors() const
    {
        return !m_internalErrors.isEmpty();
    }
    qint64 databaseSize(const QString &path, const QString &extension) const;
    QVariant formattedDatabaseSize(const QString &path, const QString &extension) const;
    QVariantMap statistics() const;
    void statistics(QVariantMap &res) const;
    bool isSavingConfig() const
    {
        return m_pendingConfigChange.reply != nullptr;
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
     * \brief Whether scanPath() is available.
     */
    static constexpr bool isScanSupported()
    {
#ifdef Q_OS_ANDROID
        return true;
#else
        return false;
#endif
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
    QString fontFamily() const;
    qreal fontScale() const;
    int fontWeightAdjustment() const;
    QFont font() const;
    bool storagePermissionGranted() const;
    bool notificationPermissionGranted() const;
    QString currentSyncthingHomeDir() const;
    QObject *currentDialog();

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
    Q_INVOKABLE bool cleanSyncthingHomeDirectory(const QJSValue &callback = QJSValue());
    Q_INVOKABLE bool checkOngoingImportExport();
    Q_INVOKABLE bool openSyncthingConfigFile();
    Q_INVOKABLE bool openSyncthingLogFile();
    Q_INVOKABLE bool checkSettings(const QUrl &url, const QJSValue &callback = QJSValue());
    Q_INVOKABLE bool importSettings(const QVariantMap &availableSettings, const QVariantMap &selectedSettings, const QJSValue &callback = QJSValue());
    Q_INVOKABLE bool exportSettings(const QUrl &url, const QJSValue &callback = QJSValue());
    Q_INVOKABLE bool checkSyncthingHome(const QJSValue &callback = QJSValue());
    Q_INVOKABLE bool moveSyncthingHome(const QString &newHomeDir, const QJSValue &callback = QJSValue());
    Q_INVOKABLE bool saveSupportBundle(const QUrl &url, const QJSValue &callback = QJSValue());
    Q_INVOKABLE bool openPath(const QString &path);
    Q_INVOKABLE bool openPath(const QString &dirId, const QString &relativePath);
    Q_INVOKABLE bool scanPath(const QString &path);
    Q_INVOKABLE bool copyText(const QString &text);
    Q_INVOKABLE bool copyPath(const QString &dirId, const QString &relativePath);
    Q_INVOKABLE QString getClipboardText() const;
    Q_INVOKABLE bool loadIgnorePatterns(const QString &dirId, QObject *textArea);
    Q_INVOKABLE bool saveIgnorePatterns(const QString &dirId, QObject *textArea);
    Q_INVOKABLE bool openIgnorePatterns(const QString &dirId);
    Q_INVOKABLE bool loadErrors(QObject *listView);
    Q_INVOKABLE bool showLog(QObject *textArea);
    Q_INVOKABLE void clearLog();
    Q_INVOKABLE bool showQrCode(Icon *icon);
    Q_INVOKABLE bool loadDirErrors(const QString &dirId, QObject *view);
    Q_INVOKABLE bool loadStatistics(const QJSValue &callback);
    Q_INVOKABLE bool showError(const QString &errorMessage);
    Q_INVOKABLE void setCurrentControls(bool visible, int tabIndex = -1);
    Q_INVOKABLE bool performHapticFeedback();
    Q_INVOKABLE bool showToast(const QString &message);
    Q_INVOKABLE QString resolveUrl(const QUrl &url);
    Q_INVOKABLE bool shouldIgnorePermissions(const QString &path);
    Q_INVOKABLE Data::SyncthingFileModel *createFileModel(const QString &dirId, QObject *parent);
    Q_INVOKABLE QtGui::DiffHighlighter *createDiffHighlighter(QTextDocument *parent);
    Q_INVOKABLE QVariantList internalErrors() const;
    Q_INVOKABLE void clearInternalErrors();
    Q_INVOKABLE bool postSyncthingConfig(const QJsonObject &rawConfig, const QJSValue &callback = QJSValue());
    Q_INVOKABLE bool invokeDirAction(const QString &dirId, const QString &action);
    Q_INVOKABLE bool requestFromSyncthing(
        const QString &verb, const QString &path, const QVariantMap &parameters, const QJSValue &callback = QJSValue());
    Q_INVOKABLE QString formatDataSize(quint64 size) const;
    Q_INVOKABLE QString formatTraffic(quint64 total, double rate) const;
    Q_INVOKABLE bool hasDevice(const QString &id);
    Q_INVOKABLE bool hasDir(const QString &id);
    Q_INVOKABLE QString deviceDisplayName(const QString &id) const;
    Q_INVOKABLE QString dirDisplayName(const QString &id) const;
    Q_INVOKABLE QVariantList computeDirsNeedingItems(const QModelIndex &devProxyModelIndex) const;
    Q_INVOKABLE QVariant isPopulated(const QString &path) const;
    Q_INVOKABLE bool minimize();
    Q_INVOKABLE void quit();
    Q_INVOKABLE void setPalette(const QColor &foreground, const QColor &background);
    Q_INVOKABLE bool requestStoragePermission();
    Q_INVOKABLE bool requestNotificationPermission();
    Q_INVOKABLE void addDialog(QObject *dialog);
    Q_INVOKABLE void removeDialog(QObject *dialog);
    Q_INVOKABLE void terminateSyncthing();
    Q_INVOKABLE void restartSyncthing();
    Q_INVOKABLE void shutdownSyncthing();
    Q_INVOKABLE void connectToSyncthing();
#ifdef Q_OS_ANDROID
    Q_INVOKABLE void sendMessageToService(ServiceAction action, int arg1 = 0, int arg2 = 0, const QString &str = QString());
    Q_INVOKABLE void handleMessageFromService(ActivityAction action, int arg1, int arg2, const QString &str, const QByteArray &variant);
#endif
    Q_INVOKABLE void handleLauncherStatusBroadcast(const QVariant &status);

Q_SIGNALS:
    void darkmodeEnabledChanged(bool darkmodeEnabled);
    void internalError(const QtGui::InternalError &error);
    void info(const QString &infoMessage, const QString &details = QString());
    void logsAvailable(const QString &newLogMessages);
    void hasInternalErrorsChanged();
    void internalErrorsRequested();
    void connectionErrorsRequested();
    void savingConfigChanged(bool isSavingConfig);
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
    QString externalFilesDir() const;
    QStringList externalStoragePaths() const;
    void setImportExportStatus(ImportExportStatus importExportStatus);
    void applyDarkmodeChange(bool isDarkColorSchemeEnabled, bool isDarkPaletteEnabled);
    QString locateSettingsExportDir();

    std::optional<QQmlApplicationEngine> m_engine;
    QString m_syncthingRunningStatus;
    QUrl m_syncthingGuiUrl;
    QString m_meteredStatus;
    QGuiApplication *m_app;
    QtForkAwesome::QuickImageProvider *m_imageProvider;
    Data::SyncthingDirectoryModel m_dirModel;
    Data::SyncthingSortFilterModel m_sortFilterDirModel;
    Data::SyncthingDeviceModel m_devModel;
    Data::SyncthingSortFilterModel m_sortFilterDevModel;
    Data::SyncthingRecentChangesModel m_changesModel;
    Data::SyncthingConnection::QueryResult m_pendingConfigChange;
#ifdef Q_OS_ANDROID
    mutable std::optional<bool> m_storagePermissionGranted;
    mutable std::optional<bool> m_notificationPermissionGranted;
#endif
    std::pair<QVariantMap, QVariantMap> m_settingsImport;
    std::optional<QUrl> m_settingsExport;
    std::optional<QFile> m_downloadFile;
    std::optional<QString> m_homeDirMove;
    QtUtilities::QtSettings m_qtSettings;
    QString m_faUrlBase;
    std::array<QObject *, 5> m_uiObjects;
    QObjectList m_dialogs;
    int m_iconSize;
    int m_tabIndex;
    ImportExportStatus m_importExportStatus;
    bool m_clearingLogfile;
    bool m_darkmodeEnabled;
    bool m_darkColorScheme;
    bool m_darkPalette;
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

inline QObject *App::currentDialog()
{
    return m_dialogs.isEmpty() ? nullptr : m_dialogs.back();
}

} // namespace QtGui

#endif // SYNCTHING_TRAY_APP_H
