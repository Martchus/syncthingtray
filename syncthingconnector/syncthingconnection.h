#ifndef SYNCTHINGCONNECTION_H
#define SYNCTHINGCONNECTION_H

#include "./syncthingconnectionenums.h"
#include "./syncthingconnectionstatus.h"
#include "./syncthingdev.h"
#include "./syncthingdir.h"
#include "./utils.h"

#include <c++utilities/misc/flagenumclass.h>

#include <QByteArray>
#include <QDateTime>
#include <QJsonObject>
#include <QList>
#include <QNetworkRequest>
#include <QObject>
#include <QSslError>
#include <QTimer>

#include <cstdint>
#include <functional>
#include <limits>
#include <optional>
#include <vector>

QT_FORWARD_DECLARE_CLASS(QNetworkAccessManager)
QT_FORWARD_DECLARE_CLASS(QNetworkReply)
QT_FORWARD_DECLARE_CLASS(QUrlQuery)
QT_FORWARD_DECLARE_CLASS(QJsonObject)
QT_FORWARD_DECLARE_CLASS(QJsonArray)
QT_FORWARD_DECLARE_CLASS(QJsonParseError)

class ConnectionTests;
class MiscTests;

namespace Data {

struct SyncthingConnectionSettings;

LIB_SYNCTHING_CONNECTOR_EXPORT QNetworkAccessManager &networkAccessManager();

struct LIB_SYNCTHING_CONNECTOR_EXPORT SyncthingLogEntry {
    SyncthingLogEntry(const QString &when = QString(), const QString &message = QString())
        : when(when)
        , message(message)
    {
    }
    QString when;
    QString message;
};

enum class SyncthingItemType {
    Unknown, /**< the type is unknown */
    File, /**< the item is a regular file */
    Directory, /**< the item is a directory */
    Symlink, /**< the item is a symlink (pointing to a file or directory) */
    Error, /**< the item represents an error message (e.g. the API query ran into an error); used by SyncthingFileModel */
    Loading, /**< the item represents a loading indication; used by SyncthingFileModel */
};

struct LIB_SYNCTHING_CONNECTOR_EXPORT SyncthingItem {
    /// \brief The matching ignore pattern was not initialized for this item.
    static constexpr auto ignorePatternNotInitialized = std::numeric_limits<std::size_t>::max();
    /// \brief The item did not match any of the current ignore patterns.
    static constexpr auto ignorePatternNoMatch = ignorePatternNotInitialized - 1;

    /// \brief The name of the filesystem item or error/loading message in case of those item types.
    QString name;
    /// \brief The modification time. Only populated with a meaningful value for files and directories.
    CppUtilities::DateTime modificationTime = CppUtilities::DateTime();
    /// \brief The file size. Only populated with a meaningful value for files.
    std::size_t size = std::size_t();
    /// \brief The type of the item.
    SyncthingItemType type = SyncthingItemType::Unknown;
    /// \brief The child items, if populated as indicated by childrenPopulated.
    std::vector<std::unique_ptr<SyncthingItem>> children;
    /// \brief The parent item; not populated by default but might be set as needed (take care in case the pointer gets invalidated).
    SyncthingItem *parent = nullptr;
    /// \brief The path of the item; not populated by default but might be set as needed.
    QString path;
    /// \brief The index of the item within its parent.
    std::size_t index = std::size_t();
    /// \brief The index of the ignore pattern (in the current list of ignore patterns) this item matches.
    /// \remarks Not populated by default.
    std::size_t ignorePattern = ignorePatternNotInitialized;
    /// \brief The level of nesting, does *not* include levels of the prefix.
    int level = 0;
    /// \brief Whether children are populated (depends on the requested level).
    bool childrenPopulated = false;
    /// \brief Whether the item is "checked"; not set by default but might be set to flag an item for some mass-action.
    Qt::CheckState checked = Qt::Unchecked;
    /// \brief Whether the item is present in the Syncthing database.
    std::optional<bool> existsInDb;
    /// \brief Whether the item is present in the local file system.
    std::optional<bool> existsLocally;

    bool isFilesystemItem() const;
};

/// \brief Returns whether the item is actually a filesystem item.
inline bool SyncthingItem::isFilesystemItem() const
{
    switch (type) {
    case Data::SyncthingItemType::File:
    case Data::SyncthingItemType::Directory:
    case Data::SyncthingItemType::Symlink:
        return true;
    default:
        return false;
    }
}

struct LIB_SYNCTHING_CONNECTOR_EXPORT SyncthingIgnores {
    QStringList ignore;
    QStringList expanded;
};

struct LIB_SYNCTHING_CONNECTOR_EXPORT SyncthingError {
    CppUtilities::DateTime when;
    QString message;
};

class LIB_SYNCTHING_CONNECTOR_EXPORT SyncthingConnection : public QObject {
    friend ConnectionTests;
    friend MiscTests;

    Q_OBJECT
    Q_PROPERTY(QString syncthingUrl READ syncthingUrl WRITE setSyncthingUrl NOTIFY syncthingUrlChanged)
    Q_PROPERTY(QUrl syncthingUrlWithCredentials READ makeUrlWithCredentials NOTIFY syncthingUrlChanged)
    Q_PROPERTY(QString localPath READ localPath WRITE setLocalPath)
    Q_PROPERTY(QByteArray apiKey READ apiKey WRITE setApiKey)
    Q_PROPERTY(bool isLocal READ isLocal)
    Q_PROPERTY(QString user READ user)
    Q_PROPERTY(QString password READ password)
    Q_PROPERTY(Data::SyncthingStatus status READ status NOTIFY statusChanged)
    Q_PROPERTY(Data::SyncthingStatusComputionFlags statusComputionFlags READ statusComputionFlags WRITE setStatusComputionFlags)
    Q_PROPERTY(Data::SyncthingConnectionLoggingFlags loggingFlags READ loggingFlags WRITE setLoggingFlags)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusChanged)
    Q_PROPERTY(bool connected READ isConnected NOTIFY statusChanged)
    Q_PROPERTY(bool connecting READ isConnecting NOTIFY statusChanged)
    Q_PROPERTY(bool aborted READ isAborted NOTIFY statusChanged)
    Q_PROPERTY(bool hasErrors READ hasErrors NOTIFY newErrors)
    Q_PROPERTY(bool hasOutOfSyncDirs READ hasOutOfSyncDirs NOTIFY hasOutOfSyncDirsChanged)
    Q_PROPERTY(bool hasState READ hasState NOTIFY hasStateChanged)
    Q_PROPERTY(bool requestingCompletionEnabled READ isRequestingCompletionEnabled WRITE setRequestingCompletionEnabled)
    Q_PROPERTY(int autoReconnectInterval READ autoReconnectInterval WRITE setAutoReconnectInterval NOTIFY autoReconnectIntervalChanged)
    Q_PROPERTY(unsigned int autoReconnectTries READ autoReconnectTries)
    Q_PROPERTY(int trafficPollInterval READ trafficPollInterval WRITE setTrafficPollInterval)
    Q_PROPERTY(int devStatsPollInterval READ devStatsPollInterval WRITE setDevStatsPollInterval)
    Q_PROPERTY(bool recordFileChanges READ recordFileChanges WRITE setRecordFileChanges)
    Q_PROPERTY(int requestTimeout READ requestTimeout WRITE setRequestTimeout)
    Q_PROPERTY(int longPollingTimeout READ longPollingTimeout WRITE setLongPollingTimeout)
    Q_PROPERTY(int diskEventLimit READ diskEventLimit WRITE setDiskEventLimit)
    Q_PROPERTY(QString myId READ myId NOTIFY myIdChanged)
    Q_PROPERTY(QString tilde READ tilde NOTIFY tildeChanged)
    Q_PROPERTY(QString pathSeparator READ pathSeparator NOTIFY tildeChanged)
    Q_PROPERTY(QString configDir READ configDir NOTIFY configDirChanged)
    Q_PROPERTY(quint64 totalIncomingTraffic READ totalIncomingTraffic NOTIFY trafficChanged)
    Q_PROPERTY(quint64 totalOutgoingTraffic READ totalOutgoingTraffic NOTIFY trafficChanged)
    Q_PROPERTY(double totalIncomingRate READ totalIncomingRate NOTIFY trafficChanged)
    Q_PROPERTY(double totalOutgoingRate READ totalOutgoingRate NOTIFY trafficChanged)
    Q_PROPERTY(Data::SyncthingOverallDirStatistics overallDirStatistics READ computeOverallDirStatistics NOTIFY dirStatisticsChanged)
    Q_PROPERTY(Data::SyncthingCompletion overallRemoteCompletion READ computeOverallRemoteCompletion NOTIFY devCompletionChanged)
    Q_PROPERTY(QString lastSyncedFile READ lastSyncedFile)
    Q_PROPERTY(QString syncthingVersion READ syncthingVersion)
    Q_PROPERTY(CppUtilities::DateTime lastSyncTime READ lastSyncTime)
#ifndef QT_NO_SSL
    Q_PROPERTY(QList<QSslError> expectedSslErrors READ expectedSslErrors)
#endif
    Q_PROPERTY(std::vector<const SyncthingDev *> connectedDevices READ connectedDevices)
    Q_PROPERTY(QStringList directoryIds READ directoryIds NOTIFY newDirs)
    Q_PROPERTY(QStringList deviceIds READ deviceIds NOTIFY newDevices)
    Q_PROPERTY(bool guiRequiringAuth READ isGuiRequiringAuth NOTIFY newConfig)
    Q_PROPERTY(QJsonObject rawConfig READ rawConfig NOTIFY newConfig)
    Q_PROPERTY(bool useDeprecatedRoutes READ isUsingDeprecatedRoutes WRITE setUseDeprecatedRoutes)
    Q_PROPERTY(bool pausingOnMeteredConnection READ isPausingOnMeteredConnection WRITE setPausingOnMeteredConnection)
    Q_PROPERTY(bool insecure READ isInsecure WRITE setInsecure)

public:
    explicit SyncthingConnection(const QString &syncthingUrl = QStringLiteral("http://localhost:8080"), const QByteArray &apiKey = QByteArray(),
        SyncthingConnectionLoggingFlags loggingFlags = SyncthingConnectionLoggingFlags::FromEnvironment, QObject *parent = nullptr);
    ~SyncthingConnection() override;

    /// \brief The QueryResult struct is used to return the reply and associated signal/slot-connection of certain requests.
    struct QueryResult {
        QNetworkReply *reply = nullptr;
        QMetaObject::Connection connection;
    };

    /// \brief The PollingFlags enum class specifies what information the connection is supposed to request/process.
    enum class PollingFlags {
        None, /**< only initial state is queried, no events are consumed to keep it up to date */
        MainEvents = (1 << 0), /**< most important events are requested/processed to keep folder and device information up to date */
        DiskEvents = (1 << 1), /**< events to emit the fileChanged() signal are requested/processed (used to show recent changed in the UI) */
        DownloadProgress
        = (1 << 2), /**< events to emit the downloadProgressChanged() signal are requested/processed (used to show downloads in the UI) */
        RemoteIndexUpdated = (1
            << 3), /**< requests the completion for the relevant folder/device again on `RemoteIndexUpdated` events (normally not required as `FolderCompletion` events contain this information as well) */
        ItemFinished = (1
            << 4), /**< processes `ItemFinished` events to update errors and list file information of folders (normally not required as calls to requestDirStatistics() on `StateChanged` events cover this) */
        TrafficStatistics = (1 << 5), /**< polls for traffic statistics according to trafficPollInterval() */
        DeviceStatistics = (1 << 6), /**< polls for device statistics according to devStatsPollInterval() */
        Errors = (1 << 7), /**< polls for errors according to errorsPollInterval() */
        All = MainEvents | DiskEvents | DownloadProgress | TrafficStatistics | DeviceStatistics
            | Errors, /**< all events the SyncthingConnection class can make use of are requested/processed, this excludes redundant events */
        NormalEvents
        = MainEvents | DownloadProgress | RemoteIndexUpdated | ItemFinished, /**< events requested via the long-poling API by requestEvents() */
    };

    // getter/setter for various properties
    const QString &syncthingUrl() const;
    void setSyncthingUrl(const QString &url);
    const QString &localPath() const;
    void setLocalPath(const QString &localPath);
    bool isLocal() const;
    const QByteArray &apiKey() const;
    void setApiKey(const QByteArray &apiKey);
    const QString &user() const;
    const QString &password() const;
    void setCredentials(const QString &user, const QString &password);
    Q_INVOKABLE QUrl makeUrlWithCredentials() const;
    bool isUsingDeprecatedRoutes() const;
    void setUseDeprecatedRoutes(bool useLegacyRoutes);
    PollingFlags pollingFlags() const;
    void setPollingFlags(PollingFlags flags);

    // getter for the status of the connection to Syncthing and of Syncthing itself
    SyncthingStatus status() const;
    QString statusText() const;
    static QString statusText(SyncthingStatus status);
    SyncthingStatusComputionFlags statusComputionFlags() const;
    void setStatusComputionFlags(SyncthingStatusComputionFlags flags);
    SyncthingConnectionLoggingFlags loggingFlags() const;
    void setLoggingFlags(SyncthingConnectionLoggingFlags flags);
    bool isConnected() const;
    bool isAborted() const;
    bool isConnecting() const;
    bool hasPendingRequests() const;
    bool hasPendingRequestsIncludingEvents() const;
    bool hasErrors() const;
    bool hasOutOfSyncDirs() const;
    bool hasState() const;

    // getter/setter to configure connection behavior
    bool isRequestingCompletionEnabled() const;
    void setRequestingCompletionEnabled(bool requestingCompletionEnabled);
    int trafficPollInterval() const;
    void setTrafficPollInterval(int trafficPollInterval);
    int devStatsPollInterval() const;
    void setDevStatsPollInterval(int devStatsPollInterval);
    int errorsPollInterval() const;
    void setErrorsPollInterval(int errorsPollInterval);
    int autoReconnectInterval() const;
    unsigned int autoReconnectTries() const;
    void setAutoReconnectInterval(int interval);
    void disablePolling();
    bool recordFileChanges() const;
    void setRecordFileChanges(bool recordFileChanges);
    int requestTimeout() const;
    void setRequestTimeout(int requestTimeout);
    int longPollingTimeout() const;
    void setLongPollingTimeout(int longPollingTimeout);
    int diskEventLimit() const;
    void setDiskEventLimit(int diskEventLimit);
    bool isPausingOnMeteredConnection() const;
    void setPausingOnMeteredConnection(bool pausingOnMeteredConnection);
    bool isInsecure() const;
    void setInsecure(bool insecure);

    // getter for information retrieved from Syncthing
    const QString &configDir() const;
    const QString &myId() const;
    const QString &tilde() const;
    const QString &pathSeparator() const;
    Q_INVOKABLE QString substituteTilde(const QString &path) const;
    std::uint64_t totalIncomingTraffic() const;
    std::uint64_t totalOutgoingTraffic() const;
    double totalIncomingRate() const;
    double totalOutgoingRate() const;
    static constexpr std::uint64_t unknownTraffic = std::numeric_limits<std::uint64_t>::max();
    const std::vector<SyncthingDir> &dirInfo() const;
    const std::vector<SyncthingDev> &devInfo() const;
    const std::vector<SyncthingError> &errors() const;
    SyncthingOverallDirStatistics computeOverallDirStatistics() const;
    SyncthingCompletion computeOverallRemoteCompletion() const;
    const QString &lastSyncedFile() const;
    CppUtilities::DateTime lastSyncTime() const;
    CppUtilities::DateTime startTime() const;
    CppUtilities::TimeSpan uptime() const;
    const QString &syncthingVersion() const;
    QStringList directoryIds() const;
    QStringList deviceIds() const;
    Q_INVOKABLE QString deviceNameOrId(const QString &deviceId) const;
    std::vector<const SyncthingDev *> connectedDevices() const;
    bool isGuiRequiringAuth() const;
    const QJsonObject &rawConfig() const;
    SyncthingDir *findDirInfo(const QString &dirId, int &row);
    SyncthingDir *findDirInfoConsideringLabels(const QString &dirIdOrLabel, int &row);
    const SyncthingDir *findDirInfo(const QString &dirId, int &row) const;
    SyncthingDir *findDirInfo(QLatin1String key, const QJsonObject &object, int *row = nullptr);
    SyncthingDir *findDirInfoByPath(const QString &path, QString &relativePath, int &row);
    SyncthingDev *findDevInfo(const QString &devId, int &row);
    const SyncthingDev *findDevInfo(const QString &devId, int &row) const;
    SyncthingDev *findDevInfoByName(const QString &devName, int &row);
    Q_INVOKABLE QString fullPath(const QString &dirId, const QString &relativePath) const;

#ifndef QT_NO_SSL
    const QList<QSslError> &expectedSslErrors() const;
#endif

public Q_SLOTS:
#ifndef QT_NO_SSL
    bool loadSelfSignedCertificate(const QUrl &url = QUrl());
    void clearSelfSignedCertificate();
#endif
    bool applySettings(Data::SyncthingConnectionSettings &connectionSettings);
    void applyRawConfig();
    void postRawConfig(const QByteArray &rawConfig);

    // methods to initiate/close connection
    void connect();
    void connect(Data::SyncthingConnectionSettings &connectionSettings);
    void connectLater(int milliSeconds);
    void disconnect();
    void reconnect();
    void reconnect(Data::SyncthingConnectionSettings &connectionSettings);
    void reconnectLater(int milliSeconds);
    void abortAllRequests();

    // methods to trigger certain actions (resume, rescan, restart, ...)
    bool pauseDevice(const QStringList &devIds);
    bool pauseAllDevs();
    bool resumeDevice(const QStringList &devIds);
    bool resumeAllDevs();
    bool pauseDirectories(const QStringList &dirIds);
    bool pauseAllDirs();
    bool resumeDirectories(const QStringList &dirIds);
    bool resumeAllDirs();
    void rescan(const QString &dirId, const QString &relpath = QString());
    void rescanAllDirs();
    void restart();
    void shutdown();

    // methods to GET or POST information from/to Syncthing
    void requestConfig();
    void requestStatus();
    void requestConfigAndStatus();
    void requestEvents();
    void requestErrors();
    void requestConnections();
    void requestClearingErrors();
    void requestDirStatistics();
    void requestDirStatus(const QString &dirId);
    void requestDirPullErrors(const QString &dirId, int page = 0, int perPage = 0);
    void requestCompletion(const QString &devId, const QString &dirId);
    void requestDeviceStatistics();
    void requestVersion();
    void requestDiskEvents(int limit = 25);
    void requestQrCode(const QString &text);
    void requestLog();
    void requestOverride(const QString &dirId);
    void requestRevert(const QString &dirId);

public:
    // methods to GET or POST information from/to Syncthing (non-slots)
    QueryResult requestJsonData(const QByteArray &verb, const QString &path, const QUrlQuery &query, const QByteArray &data,
        std::function<void(QJsonDocument &&, QString &&)> &&callback = std::function<void(QJsonDocument &&, QString &&)>(), bool rest = true,
        bool longPolling = false);
    QueryResult browse(const QString &dirId, const QString &prefix, int level,
        std::function<void(std::vector<std::unique_ptr<SyncthingItem>> &&, QString &&)> &&callback);
    QueryResult ignores(const QString &dirId, std::function<void(SyncthingIgnores &&, QString &&)> &&callback);
    QueryResult setIgnores(const QString &dirId, const SyncthingIgnores &ignores, std::function<void(QString &&)> &&callback);
    QueryResult postConfigFromJsonObject(
        const QJsonObject &rawConfig, std::function<void(QString &&)> &&callback = std::function<void(QString &&)>());
    QueryResult postConfigFromByteArray(const QByteArray &rawConfig, std::function<void(QString &&)> &&callback = std::function<void(QString &&)>());
    QueryResult sendCustomRequest(const QByteArray &verb, const QUrl &url,
        const QMap<QByteArray, QByteArray> &headers = QMap<QByteArray, QByteArray>(), QIODevice *data = nullptr);
    QueryResult downloadSupportBundle();

Q_SIGNALS:
    void syncthingUrlChanged(const QString &newUrl);
    void autoReconnectIntervalChanged(int autoReconnectInterval);
    void newConfig(const QJsonObject &rawConfig);
    void newDirs(const std::vector<Data::SyncthingDir> &dirs);
    void newDevices(const std::vector<Data::SyncthingDev> &devs);
    void newErrors(const std::vector<Data::SyncthingError> &errors);
    void beforeNewErrors(const std::vector<Data::SyncthingError> &oldErrors, const std::vector<Data::SyncthingError> &newErrors);
    void newConfigApplied();
    void newEvents(const QJsonArray &events);
    void allEventsProcessed();
    void dirStatusChanged(const Data::SyncthingDir &dir, int index);
    void devStatusChanged(const Data::SyncthingDev &dev, int index);
    void fileChanged(const Data::SyncthingDir &dir, int index, const Data::SyncthingFileChange &fileChange);
    void downloadProgressChanged();
    void dirStatisticsChanged();
    void devCompletionChanged();
    void dirCompleted(CppUtilities::DateTime when, const Data::SyncthingDir &dir, int index, const Data::SyncthingDev *remoteDev = nullptr);
    void newNotification(CppUtilities::DateTime when, const QString &message);
    void newDevAvailable(CppUtilities::DateTime when, const QString &devId, const QString &address);
    void newDirAvailable(
        CppUtilities::DateTime when, const QString &devId, const Data::SyncthingDev *dev, const QString &dirId, const QString &dirLabel);
    void error(const QString &errorMessage, Data::SyncthingErrorCategory category, int networkError,
        const QNetworkRequest &request = QNetworkRequest(), const QByteArray &response = QByteArray());
    void statusChanged(Data::SyncthingStatus newStatus);
    void configDirChanged(const QString &newConfigDir);
    void myIdChanged(const QString &myNewId);
    void tildeChanged(const QString &tilde);
    void trafficChanged(std::uint64_t totalIncomingTraffic, std::uint64_t totalOutgoingTraffic);
    void newConfigTriggered();
    void rescanTriggered(const QString &dirId);
    void devicePauseTriggered(const QStringList &devIds);
    void deviceResumeTriggered(const QStringList &devIds);
    void directoryPauseTriggered(const QStringList &dirIds);
    void directoryResumeTriggered(const QStringList &dirIds);
    void restartTriggered();
    void shutdownTriggered();
    void errorsCleared();
    void logAvailable(const std::vector<Data::SyncthingLogEntry> &logEntries);
    void qrCodeAvailable(const QString &text, const QByteArray &qrCodeData);
    void overrideTriggered(const QString &dirId);
    void revertTriggered(const QString &dirId);
    void hasOutOfSyncDirsChanged();
    void hasStateChanged();

private Q_SLOTS:
    // handler to evaluate results from request...() methods
    void readConfig();
    void readDirs(const QJsonArray &dirs);
    void readDevs(const QJsonArray &devs);
    void readStatus();
    void concludeReadingConfigAndStatus();
    void readConnections();
    void readDirStatistics();
    void readDeviceStatistics();
    void readErrors();
    void readClearingErrors();
    void readEvents();
    bool readEventsFromJsonArray(const QJsonArray &events, quint64 &idVariable);
    void readStartingEvent(const QJsonObject &eventData);
    void readStatusChangedEvent(SyncthingEventId eventId, CppUtilities::DateTime eventTime, const QJsonObject &eventData);
    void readDownloadProgressEvent(const QJsonObject &eventData);
    void readDirEvent(SyncthingEventId eventId, CppUtilities::DateTime eventTime, const QString &eventType, const QJsonObject &eventData);
    void readDeviceEvent(SyncthingEventId eventId, CppUtilities::DateTime eventTime, const QString &eventType, const QJsonObject &eventData);
    void readItemFinished(SyncthingEventId eventId, CppUtilities::DateTime eventTime, const QJsonObject &eventData);
    void readFolderErrors(
        SyncthingEventId eventId, CppUtilities::DateTime eventTime, const QJsonObject &eventData, Data::SyncthingDir &dirInfo, int index);
    void readFolderCompletion(SyncthingEventId eventId, CppUtilities::DateTime eventTime, const QJsonObject &eventData, const QString &dirId,
        Data::SyncthingDir *dirInfo, int dirIndex);
    void readFolderCompletion(SyncthingEventId eventId, CppUtilities::DateTime eventTime, const QJsonObject &eventData, const QString &devId,
        Data::SyncthingDev *devInfo, int devIndex, const QString &dirId, Data::SyncthingDir *dirInfo, int dirIndex);
    void readLocalFolderCompletion(
        SyncthingEventId eventId, CppUtilities::DateTime eventTime, const QJsonObject &eventData, Data::SyncthingDir &dirInfo, int index);
    void readRemoteFolderCompletion(CppUtilities::DateTime eventTime, const QJsonObject &eventData, const QString &devId, Data::SyncthingDev *devInfo,
        int devIndex, const QString &dirId, Data::SyncthingDir *dirInfo, int dirIndex);
    void readRemoteFolderCompletion(const Data::SyncthingCompletion &completion, const QString &devId, Data::SyncthingDev *devInfo, int devIndex,
        const QString &dirId, Data::SyncthingDir *dirInfo, int dirIndex);
    void readRemoteIndexUpdated(SyncthingEventId eventId, const QJsonObject &eventData);
    void readRescan();
    void readDevPauseResume();
    void readDirPauseResume();
    void readRestart();
    void readShutdown();
    void readDirStatus();
    void readDirPullErrors();
    void readDirSummary(
        SyncthingEventId eventId, CppUtilities::DateTime eventTime, const QJsonObject &summary, Data::SyncthingDir &dirInfo, int index);
    void readDirRejected(CppUtilities::DateTime eventTime, const QString &dirId, const QJsonObject &eventData);
    void readDevRejected(CppUtilities::DateTime eventTime, const QString &devId, const QJsonObject &eventData);
    void readCompletion();
    void readVersion();
    void readDiskEvents();
    void readChangeEvent(CppUtilities::DateTime eventTime, const QString &eventType, const QJsonObject &eventData);
    void readLog();
    void readQrCode();
    void readOverride();
    void readRevert();

    // internal helper methods
    void continueConnecting();
    void continueReconnecting();
    void autoReconnect();
    bool setStatus(Data::SyncthingStatus status);
    void emitError(const QString &message, const QJsonParseError &jsonError, QNetworkReply *reply, const QByteArray &response = QByteArray());
    void emitError(const QString &message, Data::SyncthingErrorCategory category, QNetworkReply *reply);
    void emitError(const QString &message, QNetworkReply *reply);
    void emitMyIdChanged(const QString &newId);
    void emitTildeChanged(const QString &newTilde, const QString &newPathSeparator);
    void handleFatalConnectionError();
    void handleAdditionalRequestCanceled();
#ifndef QT_NO_SSL
    void handleSslErrors(const QList<QSslError> &errors);
#endif
    void handleRedirection(const QUrl &url);
    void handleMeteredConnection();
    void recalculateStatus();
    void invalidateHasOutOfSyncDirs();

private:
    // handler to evaluate results from request...() methods
    void readJsonData(std::function<void(QJsonDocument &&, QString &&)> &&callback);
    void readBrowse(const QString &dirId, int levels, std::function<void(std::vector<std::unique_ptr<SyncthingItem>> &&, QString &&)> &&callback);
    void readIgnores(const QString &dirId, std::function<void(SyncthingIgnores &&, QString &&)> &&callback);
    void readSetIgnores(const QString &dirId, std::function<void(QString &&)> &&callback);
    void readPostConfig(std::function<void(QString &&)> &&callback);

    // internal helper methods
    enum class StatusRecomputation {
        None,
        Status = (1 << 0),
        OutOfSyncDirs = (1 << 1),
        DirStats = (1 << 2),
        RemoteCompletion = (1 << 3),
        StatusAndOutOfSyncDirs = Status | OutOfSyncDirs
    };
    void concludeConnection(StatusRecomputation flags);
    struct Reply {
        QNetworkReply *reply;
        QByteArray response;
    };
    QNetworkRequest prepareRequest(const QString &path, const QUrlQuery &query, bool rest = true, bool longPolling = false);
    QNetworkRequest prepareRequest(const QUrl &url, bool longPolling = false);
    QNetworkReply *requestData(const QString &path, const QUrlQuery &query, bool rest = true, bool longPolling = false);
    QNetworkReply *postData(const QString &path, const QUrlQuery &query, const QByteArray &data = QByteArray());
    QNetworkReply *sendData(const QByteArray &verb, const QString &path, const QUrlQuery &query, const QByteArray &data = QByteArray());
    Reply prepareReply(bool readData = true, bool handleAborting = true);
    Reply prepareReply(QNetworkReply *&expectedReply, bool readData = true, bool handleAborting = true);
    Reply prepareReply(QList<QNetworkReply *> &expectedReplies, bool readData = true, bool handleAborting = true);
    Reply handleReply(QNetworkReply *reply, bool readData, bool handleAborting);
    bool pauseResumeDevice(const QStringList &devIds, bool paused, bool dueToMetered = false);
    bool pauseResumeDirectory(const QStringList &dirIds, bool paused);
    SyncthingDir *addDirInfo(std::vector<SyncthingDir> &dirs, const QString &dirId);
    SyncthingDev *addDevInfo(std::vector<SyncthingDev> &devs, const QString &devId);
    CppUtilities::DateTime parseTimeStamp(const QJsonValue &jsonValue, const QString &context,
        CppUtilities::DateTime defaultValue = CppUtilities::DateTime(), bool greaterThanEpoch = false);
    QString configPath() const;
    QByteArray changeConfigVerb() const;
    QString folderErrorsPath() const;
    bool checkConnectionConfiguration();

    QString m_syncthingUrl;
    QString m_localPath;
    QByteArray m_apiKey;
    QString m_user;
    QString m_password;
    SyncthingStatus m_status;
    SyncthingStatusComputionFlags m_statusComputionFlags;
    SyncthingConnectionLoggingFlags m_loggingFlags;
    SyncthingConnectionLoggingFlags m_loggingFlagsHandler;

    bool m_keepPolling;
    bool m_abortingAllRequests;
    bool m_connectionAborted;
    bool m_abortingToConnect;
    bool m_abortingToReconnect;
    bool m_requestCompletion;
    QString m_eventMask;
    PollingFlags m_pollingFlags;
    StatusRecomputation m_statusRecomputationFlags;
    SyncthingEventId m_lastEventId;
    SyncthingEventId m_lastDiskEventId;
    QHash<QString, SyncthingEventId> m_lastEventIdByMask;
    QTimer m_trafficPollTimer;
    QTimer m_devStatsPollTimer;
    QTimer m_errorsPollTimer;
    QTimer m_autoReconnectTimer;
    unsigned int m_autoReconnectTries;
    int m_requestTimeout;
    int m_longPollingTimeout;
    int m_diskEventLimit;
    QString m_configDir;
    QString m_myId;
    QString m_tilde;
    QString m_pathSeparator;
    std::uint64_t m_totalIncomingTraffic;
    std::uint64_t m_totalOutgoingTraffic;
    double m_totalIncomingRate;
    double m_totalOutgoingRate;
    QNetworkReply *m_configReply;
    QNetworkReply *m_statusReply;
    QNetworkReply *m_connectionsReply;
    QNetworkReply *m_errorsReply;
    QNetworkReply *m_dirStatsReply;
    QNetworkReply *m_devStatsReply;
    QNetworkReply *m_eventsReply;
    QNetworkReply *m_versionReply;
    QNetworkReply *m_diskEventsReply;
    QNetworkReply *m_logReply;
    QList<QNetworkReply *> m_otherReplies;
    mutable std::optional<bool> m_hasOutOfSyncDirs;
    bool m_hasConfig;
    bool m_hasStatus;
    bool m_hasEvents;
    bool m_hasDiskEvents;
    bool m_statsRequested;
    std::vector<SyncthingDir> m_dirs;
    std::vector<SyncthingDev> m_devs;
    std::vector<SyncthingError> m_errors;
    QStringList m_devsPausedDueToMeteredConnection;
    SyncthingEventId m_lastConnectionsUpdateEvent;
    CppUtilities::DateTime m_lastConnectionsUpdateTime;
    SyncthingEventId m_lastFileEvent = 0;
    CppUtilities::DateTime m_lastFileTime;
    CppUtilities::DateTime m_lastErrorTime;
    CppUtilities::DateTime m_startTime;
    QString m_lastFileName;
    QString m_syncthingVersion;
    bool m_lastFileDeleted;
#ifndef QT_NO_SSL
    QString m_certificatePath;
    QString m_dynamicallyDeterminedCertificatePath;
    QDateTime m_certificateLastModified;
    QList<QSslError> m_expectedSslErrors;
    QSslCertificate m_certFromLastSslError;
#endif
    QJsonObject m_rawConfig;
    bool m_recordFileChanges;
    bool m_useDeprecatedRoutes;
    bool m_pausingOnMeteredConnection;
#ifdef SYNCTHINGCONNECTION_SUPPORT_METERED
    bool m_handlingMeteredConnectionInitialized;
#endif
    bool m_insecure;
};

/*!
 * \brief Returns the URL used to connect to Syncthing.
 */
inline const QString &SyncthingConnection::syncthingUrl() const
{
    return m_syncthingUrl;
}

/*!
 * \brief Sets the URL used to connect to Syncthing.
 */
inline void SyncthingConnection::setSyncthingUrl(const QString &url)
{
    if (m_syncthingUrl != url) {
        emit syncthingUrlChanged(m_syncthingUrl = url);
    }
}

/*!
 * \brief Returns the path of the Unix domain socket to connect to Syncthing.
 */
inline const QString &SyncthingConnection::localPath() const
{
    return m_localPath;
}

/*!
 * \brief Sets the path of the Unix domain socket to connect to Syncthing.
 * \remarks This path is only used when specifying a "unix+http://" URL via setSyncthingUrl().
 */
inline void SyncthingConnection::setLocalPath(const QString &localPath)
{
    m_localPath = localPath;
}

/*!
 * \brief Returns the API key used to connect to Syncthing.
 */
inline const QByteArray &SyncthingConnection::apiKey() const
{
    return m_apiKey;
}

/*!
 * \brief Sets the API key used to connect to Syncthing.
 */
inline void SyncthingConnection::setApiKey(const QByteArray &apiKey)
{
    m_apiKey = apiKey;
}

/*!
 * \brief Returns the user name which has been set using setCredentials().
 */
inline const QString &SyncthingConnection::user() const
{
    return m_user;
}

/*!
 * \brief Returns the password which has been set using setCredentials().
 */
inline const QString &SyncthingConnection::password() const
{
    return m_password;
}

/*!
 * \brief Provides credentials used for HTTP authentication.
 */
inline void SyncthingConnection::setCredentials(const QString &user, const QString &password)
{
    m_user = user;
    m_password = password;
}

/*!
 * \brief Returns whether deprecated routes should still be used in order to support older versions of Syncthing.
 * \remarks
 * - This is still enabled by default but may cease to work once those deprecated routes have been removed by
 *   Syncthing itself.
 * - Disabling this will require Syncthing 1.12.0 or newer.
 */
inline bool SyncthingConnection::isUsingDeprecatedRoutes() const
{
    return m_useDeprecatedRoutes;
}

/*!
 * \brief Returns whether deprecated routes should still be used in order to support older versions of Syncthing.
 * \sa See isUsingLegacyRoutes() for details.
 */
inline void SyncthingConnection::setUseDeprecatedRoutes(bool useDeprecatedRoutes)
{
    m_useDeprecatedRoutes = useDeprecatedRoutes;
}

/*!
 * \brief Returns what kind of events are polled for.
 */
inline SyncthingConnection::PollingFlags SyncthingConnection::pollingFlags() const
{
    return m_pollingFlags;
}

/*!
 * \brief Returns the string representation of the current status().
 */
inline QString SyncthingConnection::statusText() const
{
    auto text = m_status == SyncthingStatus::Disconnected && !isAborted() && hasPendingRequests() ? tr("connecting") : statusText(m_status);
    if (m_autoReconnectTimer.isActive() && m_autoReconnectTimer.interval()) {
        text += tr(", re-connect attempt every %1 ms").arg(m_autoReconnectTimer.interval());
    }
    return text;
}

/*!
 * \brief Returns the (pre-computed) connection status.
 * \sa See SyncthingConnection::setStatus() and SyncthingStatus for details how it is computed.
 */
inline SyncthingStatus SyncthingConnection::status() const
{
    return m_status;
}

/*!
 * \brief Returns whether the connection to Syncthing has been established.
 *
 * If true, all information like dirInfo() and devInfo() has been populated and will be updated if it changes.
 */
inline bool SyncthingConnection::isConnected() const
{
    return m_status != SyncthingStatus::Disconnected && m_status != SyncthingStatus::Reconnecting;
}

/*!
 * \brief Returns whether the connection has been aborted, either by invoking disconnect(), abortAllRequests() or
 *        by an error.
 */
inline bool SyncthingConnection::isAborted() const
{
    return m_connectionAborted;
}

/*!
 * \brief Returns whether the connection is currently being established (and has not been established yet).
 */
inline bool SyncthingConnection::isConnecting() const
{
    return !isConnected() && !isAborted() && hasPendingRequests();
}

/*!
 * \brief Returns whether the SyncthingConnector instance is waiting for Syncthing to respond to a request.
 * \remarks
 * - Requests for (disk) events are excluded because those are long polling requests and therefore always pending.
 *   Instead, we take only into account whether those requests have been at least concluded once (since the last
 *   reconnect).
 * - Only requests which contribute to the overall state and population of myId(), tilde(), dirInfo(), devInfo(),
 *   traffic statistics, ... are considered. So requests for QR code, logs, clearing errors, rescan, ... are not
 *   taken into account.
 * - This function will also return true as long as the method abortAllRequests() is executed.
 */
inline bool SyncthingConnection::hasPendingRequests() const
{
    return m_abortingAllRequests || m_configReply || m_statusReply || (m_eventsReply && !m_hasEvents) || (m_diskEventsReply && !m_hasDiskEvents)
        || m_connectionsReply || m_dirStatsReply || m_devStatsReply || m_errorsReply || m_versionReply || !m_otherReplies.isEmpty();
}

/*!
 * \brief Returns whether there are errors (notifications) available.
 */
inline bool SyncthingConnection::hasErrors() const
{
    return !m_errors.empty();
}

/*!
 * \brief Returns whether we have a known "state" (up-to-date or stale).
 * \remarks So this function will return true if a connection was established - also if the connection to Syncthing is currently lost.
 */
inline bool SyncthingConnection::hasState() const
{
    return m_hasConfig && m_hasStatus && m_hasEvents;
}

/*!
 * \brief Returns whether completion for all directories of all devices should be requested automatically.
 * \remarks Completion can be requested manually using requestCompletion().
 */
inline bool SyncthingConnection::isRequestingCompletionEnabled() const
{
    return m_requestCompletion;
}

/*!
 * \brief Sets whether completion for all directories of all devices should be requested automatically.
 * \remarks Completion can be requested manually using requestCompletion().
 */
inline void SyncthingConnection::setRequestingCompletionEnabled(bool requestingCompletionEnabled)
{
    m_requestCompletion = requestingCompletionEnabled;
}

/*!
 * \brief Returns the interval for polling traffic status (which can not be received via event API) in milliseconds.
 * \remarks For default value see SyncthingConnectionSettings. Zero means polling is disabled.
 */
inline int SyncthingConnection::trafficPollInterval() const
{
    return m_trafficPollTimer.interval();
}

/*!
 * \brief Sets the interval for polling traffic status (which can not be received via event API) in milliseconds.
 * \remarks For default value see SyncthingConnectionSettings. Zero means polling is disabled.
 */
inline void SyncthingConnection::setTrafficPollInterval(int trafficPollInterval)
{
    if (!trafficPollInterval) {
        m_trafficPollTimer.stop();
    }
    m_trafficPollTimer.setInterval(trafficPollInterval);
}

/*!
 * \brief Returns the interval for polling device statistics (which can not be received via event API) in milliseconds.
 * \remarks For default value see SyncthingConnectionSettings. Zero means polling is disabled.
 */
inline int SyncthingConnection::devStatsPollInterval() const
{
    return m_devStatsPollTimer.interval();
}

/*!
 * \brief Sets the interval for polling device statistics (which can not be received via event API) in milliseconds.
 * \remarks For default value see SyncthingConnectionSettings. Zero means polling is disabled.
 */
inline void SyncthingConnection::setDevStatsPollInterval(int devStatsPollInterval)
{
    if (!devStatsPollInterval) {
        m_devStatsPollTimer.stop();
    }
    m_devStatsPollTimer.setInterval(devStatsPollInterval);
}

/*!
 * \brief Returns the interval for polling Syncthing errors (which can not be received via event API) in milliseconds.
 * \remarks For default value see SyncthingConnectionSettings. Zero means polling is disabled.
 */
inline int SyncthingConnection::errorsPollInterval() const
{
    return m_errorsPollTimer.interval();
}

/*!
 * \brief Sets the interval for polling Syncthing errors (which can not be received via event API) in milliseconds.
 * \remarks For default value see SyncthingConnectionSettings. Zero means polling is disabled.
 */
inline void SyncthingConnection::setErrorsPollInterval(int errorPollInterval)
{
    if (!errorPollInterval) {
        m_errorsPollTimer.stop();
    }
    m_errorsPollTimer.setInterval(errorPollInterval);
}

/*!
 * \brief Returns the reconnect interval in milliseconds.
 * \remarks For default value see SyncthingConnectionSettings. A value of 0 indicates that auto-reconnect is disabled.
 */
inline int SyncthingConnection::autoReconnectInterval() const
{
    return m_autoReconnectTimer.interval();
}

/*!
 * \brief Returns the current number of auto-reconnect tries.
 */
inline unsigned int SyncthingConnection::autoReconnectTries() const
{
    return m_autoReconnectTries;
}

/*!
 * \brief Sets the reconnect interval in milliseconds.
 * \remarks For default value see SyncthingConnectionSettings. A value of 0 indicates that auto-reconnect is disabled.
 */
inline void SyncthingConnection::setAutoReconnectInterval(int interval)
{
    if (!interval) {
        m_autoReconnectTimer.stop();
    }
    m_autoReconnectTimer.setInterval(interval);
    emit autoReconnectIntervalChanged(interval);
}

/*!
 * \brief Returns whether file changes are recorded for each directory so SyncthingDir::recentChanges is being populated.
 * \remarks The fileChanged() signal is unaffected.
 */
inline bool SyncthingConnection::recordFileChanges() const
{
    return m_recordFileChanges;
}

/*!
 * \brief Returns whether file changes are recorded for each directory so SyncthingDir::recentChanges is being populated.
 * \remarks The fileChanged() signal is unaffected.
 */
inline void SyncthingConnection::setRecordFileChanges(bool recordFileChanges)
{
    m_recordFileChanges = recordFileChanges;
}

/*!
 * \brief Returns the transfer timeout for requests in milliseconds.
 * \sa QNetworkRequest::transferTimeout()
 */
inline int SyncthingConnection::requestTimeout() const
{
    return m_requestTimeout;
}

/*!
 * \brief Sets the transfer timeout for requests in milliseconds.
 * \remarks Existing requests are not affected. Only effective when compiled against Qt 5.15 or higher.
 * \sa QNetworkRequest::setTransferTimeout()
 */
inline void SyncthingConnection::setRequestTimeout(int requestTimeout)
{
    m_requestTimeout = requestTimeout;
}

/*!
 * \brief Returns the transfer timeout for long polling requests (event APIs) in milliseconds.
 * \sa QNetworkRequest::transferTimeout()
 */
inline int SyncthingConnection::longPollingTimeout() const
{
    return m_longPollingTimeout;
}

/*!
 * \brief Sets the transfer timeout for long polling requests (event APIs) in milliseconds.
 * \remarks Existing requests are not affected. Only effective when compiled against Qt 5.15 or higher.
 * \sa QNetworkRequest::setTransferTimeout()
 */
inline void SyncthingConnection::setLongPollingTimeout(int longPollingTimeout)
{
    m_longPollingTimeout = longPollingTimeout;
}

/*!
 * \brief Returns the limit for requestDiskEvents().
 * \remarks
 * The limit is used when requestDiskEvents() is invoked to establish a connection. When invoking
 * requestDiskEvents() manually the limit is passed as first argument.
 */
inline int SyncthingConnection::diskEventLimit() const
{
    return m_diskEventLimit;
}

/*!
 * \brief Sets the limit for requestDiskEvents().
 * \remarks
 * The limit is used when requestDiskEvents() is invoked to establish a connection. When invoking
 * requestDiskEvents() manually the limit is passed as first argument.
 */
inline void SyncthingConnection::setDiskEventLimit(int diskEventLimit)
{
    m_diskEventLimit = diskEventLimit;
}

/*!
 * \brief Returns whether to pause all devices on metered connections.
 */
inline bool SyncthingConnection::isPausingOnMeteredConnection() const
{
    return m_pausingOnMeteredConnection;
}

/*!
 * \brief Returns whether any certificate errors will be ignored.
 * \remarks This will only ever be the case when this has been configured via setInsecure().
 */
inline bool SyncthingConnection::isInsecure() const
{
    return m_insecure;
}

/*!
 * \brief Sets whether any certificate errors will be ignored.
 */
inline void SyncthingConnection::setInsecure(bool insecure)
{
    m_insecure = insecure;
}

/*!
 * \brief Returns what information is considered to compute the overall status returned by status().
 */
inline SyncthingStatusComputionFlags SyncthingConnection::statusComputionFlags() const
{
    return m_statusComputionFlags;
}

/*!
 * \brief Sets what information should be used to compute the overall status returned by status().
 */
inline void SyncthingConnection::setStatusComputionFlags(SyncthingStatusComputionFlags flags)
{
    if (m_statusComputionFlags != flags) {
        m_statusComputionFlags = flags;
        recalculateStatus();
    }
}

/*!
 * \brief Returns the currently active logging flags.
 */
inline SyncthingConnectionLoggingFlags SyncthingConnection::loggingFlags() const
{
    return m_loggingFlags;
}

/*!
 * \brief Returns the Syncthing home/configuration directory.
 */
inline const QString &SyncthingConnection::configDir() const
{
    return m_configDir;
}

/*!
 * \brief Returns the ID of the own Syncthing device.
 */
inline const QString &SyncthingConnection::myId() const
{
    return m_myId;
}

/*!
 * \brief Returns the substitution for "~" of the Syncthing instance.
 */
inline const QString &SyncthingConnection::tilde() const
{
    return m_tilde;
}

/*!
 * \brief Returns the path separator of the Syncthing instance.
 */
inline const QString &SyncthingConnection::pathSeparator() const
{
    return m_pathSeparator;
}

/*!
 * \brief Returns the total incoming traffic in byte.
 */
inline std::uint64_t SyncthingConnection::totalIncomingTraffic() const
{
    return m_totalIncomingTraffic;
}

/*!
 * \brief Returns the total outgoing traffic in byte.
 */
inline std::uint64_t SyncthingConnection::totalOutgoingTraffic() const
{
    return m_totalOutgoingTraffic;
}

/*!
 * \brief Returns the total incoming transfer rate in kbit/s.
 */
inline double SyncthingConnection::totalIncomingRate() const
{
    return m_totalIncomingRate;
}

/*!
 * \brief Returns the total outgoing transfer rate in kbit/s.
 */
inline double SyncthingConnection::totalOutgoingRate() const
{
    return m_totalOutgoingRate;
}

/*!
 * \brief Returns all available directory information.
 * \remarks The returned object container object is persistent. However, the contained
 *          info objects are invalidated when the newConfig() signal is emitted.
 */
inline const std::vector<SyncthingDir> &SyncthingConnection::dirInfo() const
{
    return m_dirs;
}

/*!
 * \brief Returns all available device information.
 * \remarks The returned object container object is persistent. However, the contained
 *          info objects are invalidated when the newConfig() signal is emitted.
 */
inline const std::vector<SyncthingDev> &SyncthingConnection::devInfo() const
{
    return m_devs;
}

/*!
 * \brief Returns all currently present errors.
 */
inline const std::vector<SyncthingError> &Data::SyncthingConnection::errors() const
{
    return m_errors;
}

/*!
 * \brief Computes overall directory statistics based on the currently available directory information.
 */
inline SyncthingOverallDirStatistics SyncthingConnection::computeOverallDirStatistics() const
{
    return SyncthingOverallDirStatistics(dirInfo());
}

/*!
 * \brief Returns the name of the file which has been synced most recently.
 */
inline const QString &SyncthingConnection::lastSyncedFile() const
{
    return m_lastFileName;
}

/*!
 * \brief Returns the time of the most recent sync.
 */
inline CppUtilities::DateTime SyncthingConnection::lastSyncTime() const
{
    return m_lastFileTime;
}

/*!
 * \brief Returns when Syncthing has been started.
 */
inline CppUtilities::DateTime SyncthingConnection::startTime() const
{
    return m_startTime;
}

/*!
 * \brief Returns how long Syncthing has been running.
 */
inline CppUtilities::TimeSpan SyncthingConnection::uptime() const
{
    return m_startTime.isNull() ? CppUtilities::TimeSpan() : CppUtilities::DateTime::now() - m_startTime;
}

/*!
 * \brief Returns the Syncthing version.
 */
inline const QString &SyncthingConnection::syncthingVersion() const
{
    return m_syncthingVersion;
}

#ifndef QT_NO_SSL
/*!
 * \brief Returns a list of all expected certificate errors. This is meant to allow self-signed certificates.
 * \remarks This list is updated via loadSelfSignedCertificate().
 */
inline const QList<QSslError> &SyncthingConnection::expectedSslErrors() const
{
    return m_expectedSslErrors;
}
#endif

/*!
 * \brief Returns the raw Syncthing configuration.
 * \remarks The referenced object is updated when newConfig() is emitted.
 */
inline const QJsonObject &SyncthingConnection::rawConfig() const
{
    return m_rawConfig;
}

/*!
 * \brief Returns the directory info object for the directory with the specified ID.
 * \returns Returns a pointer to the object or nullptr if not found.
 * \remarks The returned object becomes invalid when the newDirs() signal is emitted or the connection is destroyed.
 */
inline const SyncthingDir *SyncthingConnection::findDirInfo(const QString &dirId, int &row) const
{
    return const_cast<SyncthingConnection *>(this)->findDirInfo(dirId, row);
}

/*!
 * \brief Returns the device info object for the device with the specified ID.
 * \returns Returns a pointer to the object or nullptr if not found.
 * \remarks The returned object becomes invalid when the newConfig() signal is emitted or the connection is destroyed.
 */
inline const SyncthingDev *SyncthingConnection::findDevInfo(const QString &devId, int &row) const
{
    return const_cast<SyncthingConnection *>(this)->findDevInfo(devId, row);
}

} // namespace Data

Q_DECLARE_METATYPE(Data::SyncthingLogEntry)

CPP_UTILITIES_MARK_FLAG_ENUM_CLASS(Data, Data::SyncthingConnection::StatusRecomputation)
CPP_UTILITIES_MARK_FLAG_ENUM_CLASS(Data, Data::SyncthingConnection::PollingFlags)

#endif // SYNCTHINGCONNECTION_H
