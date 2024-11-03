#ifndef SYNCTHING_TRAY_APP_H
#define SYNCTHING_TRAY_APP_H

#include <syncthingwidgets/misc/statusinfo.h>
#include <syncthingwidgets/misc/syncthinglauncher.h>
#include <syncthingwidgets/misc/utils.h>

#include <syncthingmodel/syncthingdevicemodel.h>
#include <syncthingmodel/syncthingdirectorymodel.h>
#include <syncthingmodel/syncthingrecentchangesmodel.h>

#include <syncthingconnector/syncthingconfig.h>
#include <syncthingconnector/syncthingconnection.h>
#include <syncthingconnector/syncthingconnectionsettings.h>
#include <syncthingconnector/syncthingnotifier.h>

#include <qtutilities/settingsdialog/qtsettings.h>

#include <QDir>
#include <QFile>
#include <QJsonObject>
#include <QQmlApplicationEngine>
#include <QUrl>

#ifdef Q_OS_ANDROID
#include <QHash>
#include <QJniObject>
#endif

#include <optional>

QT_FORWARD_DECLARE_CLASS(QTextDocument)

namespace Data {
class SyncthingFileModel;
}

namespace QtGui {
class DiffHighlighter;

class App : public QObject {
    Q_OBJECT
    Q_PROPERTY(Data::SyncthingConnection *connection READ connection CONSTANT)
    Q_PROPERTY(Data::SyncthingNotifier *notifier READ notifier CONSTANT)
    Q_PROPERTY(Data::SyncthingDirectoryModel *dirModel READ dirModel CONSTANT)
    Q_PROPERTY(Data::SyncthingDeviceModel *devModel READ devModel CONSTANT)
    Q_PROPERTY(Data::SyncthingRecentChangesModel *changesModel READ changesModel CONSTANT)
    Q_PROPERTY(Data::SyncthingLauncher *launcher READ launcher CONSTANT)
    Q_PROPERTY(QJsonObject settings READ settings WRITE setSettings NOTIFY settingsChanged)
    Q_PROPERTY(QString faUrlBase READ faUrlBase CONSTANT)
    Q_PROPERTY(bool darkmodeEnabled READ isDarkmodeEnabled NOTIFY darkmodeEnabledChanged)
    Q_PROPERTY(int iconSize READ iconSize CONSTANT)
    Q_PROPERTY(bool nativePopups READ nativePopups CONSTANT)
    Q_PROPERTY(QString status READ status NOTIFY statusChanged)
    Q_PROPERTY(QString syncthingVersion READ syncthingVersion CONSTANT)
    Q_PROPERTY(QString readmeUrl READ readmeUrl CONSTANT)
    Q_PROPERTY(QString website READ website CONSTANT)

public:
    explicit App(bool insecure = false, QObject *parent = nullptr);

    // properties
    Data::SyncthingConnection *connection()
    {
        return &m_connection;
    }
    Data::SyncthingNotifier *notifier()
    {
        return &m_notifier;
    }
    Data::SyncthingDirectoryModel *dirModel()
    {
        return &m_dirModel;
    }
    Data::SyncthingDeviceModel *devModel()
    {
        return &m_devModel;
    }
    Data::SyncthingRecentChangesModel *changesModel()
    {
        return &m_changesModel;
    }
    Data::SyncthingLauncher *launcher()
    {
        return &m_launcher;
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
    bool nativePopups() const
    {
#if defined(Q_OS_ANDROID) || defined(Q_OS_WINDOWS)  // it leads to crashes on those platforms
        return false;
#else
        return true;
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
    const QString &status();

    // helper functions invoked from QML
    Q_INVOKABLE bool loadMain();
    Q_INVOKABLE bool reloadMain();
    Q_INVOKABLE bool unloadMain();
    Q_INVOKABLE void shutdown();
    Q_INVOKABLE bool loadSettings();
    Q_INVOKABLE bool storeSettings();
    Q_INVOKABLE bool applySettings();
    Q_INVOKABLE void applyLauncherSettings();
    Q_INVOKABLE bool importSettings(const QUrl &url);
    Q_INVOKABLE bool exportSettings(const QUrl &url);
    Q_INVOKABLE bool openPath(const QString &path);
    Q_INVOKABLE bool openPath(const QString &dirId, const QString &relativePath);
    Q_INVOKABLE bool copyText(const QString &text);
    Q_INVOKABLE bool copyPath(const QString &dirId, const QString &relativePath);
    Q_INVOKABLE QString getClipboardText() const;
    Q_INVOKABLE bool loadIgnorePatterns(const QString &dirId, QObject *textArea);
    Q_INVOKABLE bool saveIgnorePatterns(const QString &dirId, QObject *textArea);
    Q_INVOKABLE bool showLog(QObject *textArea);
    Q_INVOKABLE void clearLog();
    Q_INVOKABLE bool loadDirErrors(const QString &dirId, QObject *view);
    Q_INVOKABLE bool showError(const QString &errorMessage);
    Q_INVOKABLE void setCurrentControls(bool visible, int tabIndex);
    Q_INVOKABLE bool performHapticFeedback();
    Q_INVOKABLE bool showToast(const QString &message);
    Q_INVOKABLE QString resolveUrl(const QUrl &url);
    Q_INVOKABLE Data::SyncthingFileModel *createFileModel(const QString &dirId, QObject *parent);
    Q_INVOKABLE QtGui::DiffHighlighter *createDiffHighlighter(QTextDocument *parent);

Q_SIGNALS:
    void darkmodeEnabledChanged(bool darkmodeEnabled);
    void settingsChanged(const QJsonObject &settingsChanged);
    void error(const QString &errorMessage, const QString &details = QString());
    void info(const QString &infoMessage, const QString &details = QString());
    void statusChanged();
    void logsAvailable(const QString &newLogMessages);

protected:
    bool eventFilter(QObject *object, QEvent *event) override;

private Q_SLOTS:
    void handleConnectionError(const QString &errorMessage, Data::SyncthingErrorCategory category, int networkError, const QNetworkRequest &request,
        const QByteArray &response);
    void invalidateStatus();
    void gatherLogs(const QByteArray &newOutput);
    void handleRunningChanged(bool isRunning);
    void handleGuiAddressChanged(const QUrl &newUrl);
    void handleNewDevices(const std::vector<Data::SyncthingDev> &newDevices);
    void handleStateChanged(Qt::ApplicationState state);
#ifdef Q_OS_ANDROID
    void invalidateAndroidIconCache();
    void updateAndroidNotification();
#endif

private:
    void applyDarkmodeChange(bool isDarkColorSchemeEnabled, bool isDarkPaletteEnabled);
    bool openSettings();
    QString locateSettingsExportDir();

    QQmlApplicationEngine m_engine;
    Data::SyncthingConnection m_connection;
    Data::SyncthingNotifier m_notifier;
    Data::SyncthingDirectoryModel m_dirModel;
    Data::SyncthingDeviceModel m_devModel;
    Data::SyncthingRecentChangesModel m_changesModel;
    Data::SyncthingConnectionSettings m_connectionSettingsFromLauncher;
    Data::SyncthingConnectionSettings m_connectionSettingsFromConfig;
#ifdef Q_OS_ANDROID
    StatusInfo m_statusInfo;
    QHash<const QIcon *, QJniObject> m_androidIconCache;
#endif
    Data::SyncthingConfig m_syncthingConfig;
    QString m_syncthingConfigDir;
    QString m_syncthingDataDir;
    QUrl m_importingSettingsFrom;
    Data::SyncthingLauncher m_launcher;
    QtUtilities::QtSettings m_qtSettings;
    QFile m_settingsFile;
    std::optional<QDir> m_settingsDir;
    QJsonObject m_settings;
    QString m_faUrlBase;
    std::optional<QString> m_status;
    QString m_log;
    int m_iconSize;
    bool m_insecure;
    bool m_connectToLaunched;
    bool m_darkmodeEnabled;
    bool m_darkColorScheme;
    bool m_darkPalette;
    bool m_isGuiLoaded;
    bool m_unloadGuiWhenHidden;
};

inline void App::clearLog()
{
    m_log.clear();
}

} // namespace QtGui

#endif // SYNCTHING_TRAY_APP_H
