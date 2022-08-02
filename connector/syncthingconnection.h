#ifndef SYNCTHINGCONNECTION_H
#define SYNCTHINGCONNECTION_H

#include "./syncthingconnectionstatus.h"
#include "./syncthingdev.h"
#include "./syncthingdir.h"

#include <c++utilities/misc/flagenumclass.h>

#include <QByteArray>
#include <QJsonObject>
#include <QList>
#include <QNetworkRequest>
#include <QObject>
#include <QSslError>
#include <QTimer>

#include <cstdint>
#include <functional>
#include <limits>
#include <vector>

QT_FORWARD_DECLARE_CLASS(QNetworkAccessManager)
QT_FORWARD_DECLARE_CLASS(QNetworkReply)
QT_FORWARD_DECLARE_CLASS(QUrlQuery)
QT_FORWARD_DECLARE_CLASS(QJsonObject)
QT_FORWARD_DECLARE_CLASS(QJsonArray)
QT_FORWARD_DECLARE_CLASS(QJsonParseError)

class ConnectionTests;
class MiscTests;

#define SYNCTHING_CONNECTOR_ENUM_CLASS enum class
namespace Data {
SYNCTHING_CONNECTOR_ENUM_CLASS SyncthingStatusComputionFlags : quint64;
}
#undef SYNCTHING_CONNECTOR_ENUM_CLASS

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

enum class SyncthingConnectionLoggingFlags : quint64 {
    None, /**< loggingn is disabled */
    FromEnvironment = (1 << 0), /**< environment variables are checked to pull in any of the other flags dynamically */
    ApiCalls = (1 << 1), /**< log calls to Syncthing's REST-API and responses */
    ApiReplies = (1 << 2), /**< log replies fromm Syncthing's REST-API */
    Events = (1 << 3), /**< log events received via Syncthing's event API */
    DirsOrDevsResetted = (1 << 4), /**< log list of directories/devices when list is reset */
    All = ApiCalls | ApiReplies | Events | DirsOrDevsResetted, /** log as much as possible */
};

} // namespace Data

CPP_UTILITIES_MARK_FLAG_ENUM_CLASS(Data, Data::SyncthingConnectionLoggingFlags)

namespace Data {

class LIB_SYNCTHING_CONNECTOR_EXPORT SyncthingConnection : public QObject {
    friend ConnectionTests;
    friend MiscTests;

    Q_OBJECT
    Q_PROPERTY(QString syncthingUrl READ syncthingUrl WRITE setSyncthingUrl)
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
    Q_PROPERTY(bool hasUnreadNotifications READ hasUnreadNotifications)
    Q_PROPERTY(bool hasOutOfSyncDirs READ hasOutOfSyncDirs)
    Q_PROPERTY(bool requestingCompletionEnabled READ isRequestingCompletionEnabled WRITE setRequestingCompletionEnabled)
    Q_PROPERTY(int autoReconnectInterval READ autoReconnectInterval WRITE setAutoReconnectInterval)
    Q_PROPERTY(unsigned int autoReconnectTries READ autoReconnectTries)
    Q_PROPERTY(int trafficPollInterval READ trafficPollInterval WRITE setTrafficPollInterval)
    Q_PROPERTY(int devStatsPollInterval READ devStatsPollInterval WRITE setDevStatsPollInterval)
    Q_PROPERTY(bool recordFileChanges READ recordFileChanges WRITE setRecordFileChanges)
    Q_PROPERTY(QString myId READ myId NOTIFY myIdChanged)
    Q_PROPERTY(QString tilde READ tilde NOTIFY tildeChanged)
    Q_PROPERTY(QString pathSeparator READ pathSeparator NOTIFY tildeChanged)
    Q_PROPERTY(QString configDir READ configDir NOTIFY configDirChanged)
    Q_PROPERTY(quint64 totalIncomingTraffic READ totalIncomingTraffic NOTIFY trafficChanged)
    Q_PROPERTY(quint64 totalOutgoingTraffic READ totalOutgoingTraffic NOTIFY trafficChanged)
    Q_PROPERTY(double totalIncomingRate READ totalIncomingRate NOTIFY trafficChanged)
    Q_PROPERTY(double totalOutgoingRate READ totalOutgoingRate NOTIFY trafficChanged)
    Q_PROPERTY(QString lastSyncedFile READ lastSyncedFile)
    Q_PROPERTY(QString syncthingVersion READ syncthingVersion)
    Q_PROPERTY(CppUtilities::DateTime lastSyncTime READ lastSyncTime)
    Q_PROPERTY(QList<QSslError> expectedSslErrors READ expectedSslErrors)
    Q_PROPERTY(std::vector<const SyncthingDev *> connectedDevices READ connectedDevices)
    Q_PROPERTY(QStringList directoryIds READ directoryIds)
    Q_PROPERTY(QStringList deviceIds READ deviceIds)
    Q_PROPERTY(QJsonObject rawConfig READ rawConfig NOTIFY newConfig)

public:
    explicit SyncthingConnection(const QString &syncthingUrl = QStringLiteral("http://localhost:8080"), const QByteArray &apiKey = QByteArray(),
        SyncthingConnectionLoggingFlags loggingFlags = SyncthingConnectionLoggingFlags::FromEnvironment, QObject *parent = nullptr);
    ~SyncthingConnection() override;

    // getter/setter for
    const QString &syncthingUrl() const;
    void setSyncthingUrl(const QString &url);
    bool isLocal() const;
    const QByteArray &apiKey() const;
    void setApiKey(const QByteArray &apiKey);
    const QString &user() const;
    const QString &password() const;
    void setCredentials(const QString &user, const QString &password);

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
    bool hasUnreadNotifications() const;
    bool hasOutOfSyncDirs() const;

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

    // getter for information retrieved from Syncthing
    const QString &configDir() const;
    const QString &myId() const;
    const QString &tilde() const;
    const QString &pathSeparator() const;
    std::uint64_t totalIncomingTraffic() const;
    std::uint64_t totalOutgoingTraffic() const;
    double totalIncomingRate() const;
    double totalOutgoingRate() const;
    static constexpr std::uint64_t unknownTraffic = std::numeric_limits<std::uint64_t>::max();
    const std::vector<SyncthingDir> &dirInfo() const;
    const std::vector<SyncthingDev> &devInfo() const;
    SyncthingOverallDirStatistics computeOverallDirStatistics() const;
    const QString &lastSyncedFile() const;
    CppUtilities::DateTime lastSyncTime() const;
    CppUtilities::DateTime startTime() const;
    CppUtilities::TimeSpan uptime() const;
    const QString &syncthingVersion() const;
    QStringList directoryIds() const;
    QStringList deviceIds() const;
    QString deviceNameOrId(const QString &deviceId) const;
    std::vector<const SyncthingDev *> connectedDevices() const;
    const QJsonObject &rawConfig() const;
    SyncthingDir *findDirInfo(const QString &dirId, int &row);
    const SyncthingDir *findDirInfo(const QString &dirId, int &row) const;
    SyncthingDir *findDirInfo(QLatin1String key, const QJsonObject &object, int *row = nullptr);
    SyncthingDir *findDirInfoByPath(const QString &path, QString &relativePath, int &row);
    SyncthingDev *findDevInfo(const QString &devId, int &row);
    const SyncthingDev *findDevInfo(const QString &devId, int &row) const;
    SyncthingDev *findDevInfoByName(const QString &devName, int &row);

    const QList<QSslError> &expectedSslErrors() const;

public Q_SLOTS:
    bool loadSelfSignedCertificate();
    bool applySettings(SyncthingConnectionSettings &connectionSettings);

    // methods to initiate/close connection
    void connect();
    void connect(SyncthingConnectionSettings &connectionSettings);
    void connectLater(int milliSeconds);
    void disconnect();
    void reconnect();
    void reconnect(SyncthingConnectionSettings &connectionSettings);
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
    void considerAllNotificationsRead();

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
    void postConfigFromJsonObject(const QJsonObject &rawConfig);
    void postConfigFromByteArray(const QByteArray &rawConfig);

Q_SIGNALS:
    void newConfig(const QJsonObject &rawConfig);
    void newDirs(const std::vector<SyncthingDir> &dirs);
    void newDevices(const std::vector<SyncthingDev> &devs);
    void newConfigApplied();
    void newEvents(const QJsonArray &events);
    void dirStatusChanged(const SyncthingDir &dir, int index);
    void devStatusChanged(const SyncthingDev &dev, int index);
    void fileChanged(const SyncthingDir &dir, int index, const SyncthingFileChange &fileChange);
    void downloadProgressChanged();
    void dirStatisticsChanged();
    void dirCompleted(CppUtilities::DateTime when, const SyncthingDir &dir, int index, const SyncthingDev *remoteDev = nullptr);
    void newNotification(CppUtilities::DateTime when, const QString &message);
    void newDevAvailable(CppUtilities::DateTime when, const QString &devId, const QString &address);
    void newDirAvailable(CppUtilities::DateTime when, const QString &devId, const SyncthingDev *dev, const QString &dirId, const QString &dirLabel);
    void error(const QString &errorMessage, SyncthingErrorCategory category, int networkError, const QNetworkRequest &request = QNetworkRequest(),
        const QByteArray &response = QByteArray());
    void statusChanged(SyncthingStatus newStatus);
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
    void logAvailable(const std::vector<SyncthingLogEntry> &logEntries);
    void qrCodeAvailable(const QString &text, const QByteArray &qrCodeData);

private Q_SLOTS:
    // handler to evaluate results from request...() methods
    void readConfig();
    void readDirs(const QJsonArray &dirs);
    void readDevs(const QJsonArray &devs);
    void readStatus();
    void concludeReadingConfigAndStatus();
    void concludeConnection();
    void readConnections();
    void readDirStatistics();
    void readDeviceStatistics();
    void readErrors();
    void readClearingErrors();
    void readEvents();
    void readEventsFromJsonArray(const QJsonArray &events, int &idVariable);
    void readStartingEvent(const QJsonObject &eventData);
    void readStatusChangedEvent(CppUtilities::DateTime eventTime, const QJsonObject &eventData);
    void readDownloadProgressEvent(CppUtilities::DateTime eventTime, const QJsonObject &eventData);
    void readDirEvent(CppUtilities::DateTime eventTime, const QString &eventType, const QJsonObject &eventData);
    void readDeviceEvent(CppUtilities::DateTime eventTime, const QString &eventType, const QJsonObject &eventData);
    void readItemStarted(CppUtilities::DateTime eventTime, const QJsonObject &eventData);
    void readItemFinished(CppUtilities::DateTime eventTime, const QJsonObject &eventData);
    void readFolderErrors(CppUtilities::DateTime eventTime, const QJsonObject &eventData, SyncthingDir &dirInfo, int index);
    void readFolderCompletion(
        CppUtilities::DateTime eventTime, const QJsonObject &eventData, const QString &dirId, SyncthingDir *dirInfo, int dirIndex);
    void readFolderCompletion(CppUtilities::DateTime eventTime, const QJsonObject &eventData, const QString &devId, SyncthingDev *devInfo,
        int devIndex, const QString &dirId, SyncthingDir *dirInfo, int dirIndex);
    void readLocalFolderCompletion(CppUtilities::DateTime eventTime, const QJsonObject &eventData, SyncthingDir &dirInfo, int index);
    void readRemoteFolderCompletion(CppUtilities::DateTime eventTime, const QJsonObject &eventData, const QString &devId, SyncthingDev *devInfo,
        int devIndex, const QString &dirId, SyncthingDir *dirInfo, int dirIndex);
    void readRemoteFolderCompletion(const SyncthingCompletion &completion, const QString &devId, SyncthingDev *devInfo, int devIndex,
        const QString &dirId, SyncthingDir *dirInfo, int dirIndex);
    void readRemoteIndexUpdated(CppUtilities::DateTime eventTime, const QJsonObject &eventData);
    void readPostConfig();
    void readRescan();
    void readDevPauseResume();
    void readDirPauseResume();
    void readRestart();
    void readShutdown();
    void readDirStatus();
    void readDirPullErrors();
    void readDirSummary(CppUtilities::DateTime eventTime, const QJsonObject &summary, SyncthingDir &dirInfo, int index);
    void readDirRejected(CppUtilities::DateTime eventTime, const QString &dirId, const QJsonObject &eventData);
    void readDevRejected(CppUtilities::DateTime eventTime, const QString &devId, const QJsonObject &eventData);
    void readCompletion();
    void readVersion();
    void readDiskEvents();
    void readChangeEvent(CppUtilities::DateTime eventTime, const QString &eventType, const QJsonObject &eventData);
    void readLog();
    void readQrCode();

    // internal helper methods
    void continueConnecting();
    void continueReconnecting();
    void autoReconnect();
    void setStatus(SyncthingStatus status);
    void emitNotification(CppUtilities::DateTime when, const QString &message);
    void emitError(const QString &message, const QJsonParseError &jsonError, QNetworkReply *reply, const QByteArray &response = QByteArray());
    void emitError(const QString &message, SyncthingErrorCategory category, QNetworkReply *reply);
    void emitMyIdChanged(const QString &newId);
    void emitTildeChanged(const QString &newTilde, const QString &newPathSeparator);
    void emitDirStatisticsChanged();
    void handleFatalConnectionError();
    void handleAdditionalRequestCanceled();
    void handleSslErrors(const QList<QSslError> &errors);
    void recalculateStatus();

private:
    // internal helper methods
    struct Reply {
        QNetworkReply *reply;
        QByteArray response;
    };
    QNetworkRequest prepareRequest(const QString &path, const QUrlQuery &query, bool rest = true);
    QNetworkReply *requestData(const QString &path, const QUrlQuery &query, bool rest = true);
    QNetworkReply *postData(const QString &path, const QUrlQuery &query, const QByteArray &data = QByteArray());
    Reply prepareReply(bool readData = true, bool handleAborting = true);
    Reply prepareReply(QNetworkReply *&expectedReply, bool readData = true, bool handleAborting = true);
    Reply prepareReply(QList<QNetworkReply *> &expectedReplies, bool readData = true, bool handleAborting = true);
    Reply handleReply(QNetworkReply *reply, bool readData, bool handleAborting);
    bool pauseResumeDevice(const QStringList &devIds, bool paused);
    bool pauseResumeDirectory(const QStringList &dirIds, bool paused);
    SyncthingDir *addDirInfo(std::vector<SyncthingDir> &dirs, const QString &dirId);
    SyncthingDev *addDevInfo(std::vector<SyncthingDev> &devs, const QString &devId);
    CppUtilities::DateTime parseTimeStamp(const QJsonValue &jsonValue, const QString &context,
        CppUtilities::DateTime defaultValue = CppUtilities::DateTime(), bool greaterThanEpoch = false);

    QString m_syncthingUrl;
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
    bool m_abortingToReconnect;
    bool m_requestCompletion;
    int m_lastEventId;
    int m_lastDiskEventId;
    QTimer m_trafficPollTimer;
    QTimer m_devStatsPollTimer;
    QTimer m_errorsPollTimer;
    QTimer m_autoReconnectTimer;
    unsigned int m_autoReconnectTries;
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
    bool m_unreadNotifications;
    bool m_hasConfig;
    bool m_hasStatus;
    bool m_hasEvents;
    bool m_hasDiskEvents;
    std::vector<SyncthingDir> m_dirs;
    std::vector<SyncthingDev> m_devs;
    CppUtilities::DateTime m_lastConnectionsUpdate;
    CppUtilities::DateTime m_lastFileTime;
    CppUtilities::DateTime m_lastErrorTime;
    CppUtilities::DateTime m_startTime;
    QString m_lastFileName;
    QString m_syncthingVersion;
    bool m_lastFileDeleted;
    QList<QSslError> m_expectedSslErrors;
    QSslCertificate m_certFromLastSslError;
    QJsonObject m_rawConfig;
    bool m_dirStatsAltered;
    bool m_recordFileChanges;
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
    m_syncthingUrl = url;
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
 * \brief Returns the string representation of the current status().
 */
inline QString SyncthingConnection::statusText() const
{
    return statusText(m_status);
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
 * \brief Returns whether there are unread notifications available.
 * \remarks This flag is set to true when new notifications become available. It can be unset again by calling considerAllNotificationsRead().
 */
inline bool SyncthingConnection::hasUnreadNotifications() const
{
    return m_unreadNotifications;
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
 * \brief Internally called to emit the notification with the specified \a message.
 * \remarks Ensures the unread notifications flag is set.
 */
inline void SyncthingConnection::emitNotification(CppUtilities::DateTime when, const QString &message)
{
    m_unreadNotifications = true;
    emit newNotification(when, message);
}

/*!
 * \brief Considers all notifications as read; hence might trigger a status update.
 */
inline void SyncthingConnection::considerAllNotificationsRead()
{
    m_unreadNotifications = false;
    requestClearingErrors();
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

/*!
 * \brief Returns a list of all expected certificate errors. This is meant to allow self-signed certificates.
 * \remarks This list is updated via loadSelfSignedCertificate().
 */
inline const QList<QSslError> &SyncthingConnection::expectedSslErrors() const
{
    return m_expectedSslErrors;
}

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

#endif // SYNCTHINGCONNECTION_H
