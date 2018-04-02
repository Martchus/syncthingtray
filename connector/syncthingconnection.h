#ifndef SYNCTHINGCONNECTION_H
#define SYNCTHINGCONNECTION_H

#include "./syncthingdev.h"
#include "./syncthingdir.h"

#include <QJsonObject>
#include <QList>
#include <QNetworkRequest>
#include <QObject>
#include <QSslError>
#include <QTimer>

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

namespace Data {
#undef Q_NAMESPACE
#define Q_NAMESPACE
Q_NAMESPACE
extern LIB_SYNCTHING_CONNECTOR_EXPORT const QMetaObject staticMetaObject;
QT_ANNOTATE_CLASS(qt_qnamespace, "") /*end*/

struct SyncthingConnectionSettings;

QNetworkAccessManager LIB_SYNCTHING_CONNECTOR_EXPORT &networkAccessManager();

enum class SyncthingStatus { Disconnected, Reconnecting, Idle, Scanning, Paused, Synchronizing, OutOfSync, BeingDestroyed };
#if (QT_VERSION >= QT_VERSION_CHECK(5, 8, 0))
Q_ENUM_NS(SyncthingStatus)
#endif

enum class SyncthingErrorCategory { OverallConnection, SpecificRequest, Parsing };
#if (QT_VERSION >= QT_VERSION_CHECK(5, 8, 0))
Q_ENUM_NS(SyncthingErrorCategory)
#endif

struct LIB_SYNCTHING_CONNECTOR_EXPORT SyncthingLogEntry {
    SyncthingLogEntry(const QString &when = QString(), const QString &message = QString())
        : when(when)
        , message(message)
    {
    }
    QString when;
    QString message;
};

class LIB_SYNCTHING_CONNECTOR_EXPORT SyncthingConnection : public QObject {
    friend ConnectionTests;
    friend MiscTests;

    Q_OBJECT
    Q_PROPERTY(QString syncthingUrl READ syncthingUrl WRITE setSyncthingUrl)
    Q_PROPERTY(QByteArray apiKey READ apiKey WRITE setApiKey)
    Q_PROPERTY(bool isLocal READ isLocal)
    Q_PROPERTY(Data::SyncthingStatus status READ status NOTIFY statusChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusChanged)
    Q_PROPERTY(bool connected READ isConnected NOTIFY statusChanged)
    Q_PROPERTY(bool hasUnreadNotifications READ hasUnreadNotifications)
    Q_PROPERTY(bool hasOutOfSyncDirs READ hasOutOfSyncDirs)
    Q_PROPERTY(bool requestingCompletionEnabled READ isRequestingCompletionEnabled WRITE setRequestingCompletionEnabled)
    Q_PROPERTY(int trafficPollInterval READ trafficPollInterval WRITE setTrafficPollInterval)
    Q_PROPERTY(int devStatsPollInterval READ devStatsPollInterval WRITE setDevStatsPollInterval)
    Q_PROPERTY(QString configDir READ configDir NOTIFY configDirChanged)
    Q_PROPERTY(QString myId READ myId NOTIFY myIdChanged)
    Q_PROPERTY(int totalIncomingTraffic READ totalIncomingTraffic NOTIFY trafficChanged)
    Q_PROPERTY(int totalOutgoingTraffic READ totalOutgoingTraffic NOTIFY trafficChanged)
    Q_PROPERTY(double totalIncomingRate READ totalIncomingRate NOTIFY trafficChanged)
    Q_PROPERTY(double totalOutgoingRate READ totalOutgoingRate NOTIFY trafficChanged)
    Q_PROPERTY(std::size_t connectedDevices READ connectedDevices)
    Q_PROPERTY(QJsonObject rawConfig READ rawConfig NOTIFY newConfig)

public:
    explicit SyncthingConnection(
        const QString &syncthingUrl = QStringLiteral("http://localhost:8080"), const QByteArray &apiKey = QByteArray(), QObject *parent = nullptr);
    ~SyncthingConnection();

    const QString &syncthingUrl() const;
    void setSyncthingUrl(const QString &url);
    bool isLocal() const;
    const QByteArray &apiKey() const;
    void setApiKey(const QByteArray &apiKey);
    const QString &user() const;
    const QString &password() const;
    void setCredentials(const QString &user, const QString &password);
    SyncthingStatus status() const;
    QString statusText() const;
    bool isConnected() const;
    bool hasUnreadNotifications() const;
    bool hasOutOfSyncDirs() const;
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
    const QString &configDir() const;
    const QString &myId() const;
    uint64 totalIncomingTraffic() const;
    uint64 totalOutgoingTraffic() const;
    double totalIncomingRate() const;
    double totalOutgoingRate() const;
    static constexpr uint64 unknownTraffic = std::numeric_limits<uint64>::max();
    const std::vector<SyncthingDir> &dirInfo() const;
    const std::vector<SyncthingDev> &devInfo() const;
    QMetaObject::Connection requestQrCode(const QString &text, std::function<void(const QByteArray &)> callback);
    QMetaObject::Connection requestLog(std::function<void(const std::vector<SyncthingLogEntry> &)> callback);
    const QList<QSslError> &expectedSslErrors();
    SyncthingDir *findDirInfo(const QString &dirId, int &row);
    SyncthingDir *findDirInfoByPath(const QString &path, QString &relativePath, int &row);
    SyncthingDev *findDevInfo(const QString &devId, int &row);
    SyncthingDev *findDevInfoByName(const QString &devName, int &row);
    QStringList directoryIds() const;
    QStringList deviceIds() const;
    QString deviceNameOrId(const QString &deviceId) const;
    std::size_t connectedDevices() const;
    const QJsonObject &rawConfig() const;

public Q_SLOTS:
    bool loadSelfSignedCertificate();
    bool applySettings(SyncthingConnectionSettings &connectionSettings);
    void connect();
    void connect(SyncthingConnectionSettings &connectionSettings);
    void connectLater(int milliSeconds);
    void disconnect();
    void reconnect();
    void reconnect(SyncthingConnectionSettings &connectionSettings);
    void reconnectLater(int milliSeconds);
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

    void requestConfig();
    void requestStatus();
    void requestErrors();
    void requestConnections();
    void requestClearingErrors();
    void requestDirStatistics();
    void requestDirStatus(const QString &dirId);
    void requestCompletion(const QString &devId, const QString &dirId);
    void requestDeviceStatistics();
    void postConfig(const QJsonObject &rawConfig);

Q_SIGNALS:
    void newConfig(const QJsonObject &rawConfig);
    void newDirs(const std::vector<SyncthingDir> &dirs);
    void newDevices(const std::vector<SyncthingDev> &devs);
    void newEvents(const QJsonArray &events);
    void dirStatusChanged(const SyncthingDir &dir, int index);
    void devStatusChanged(const SyncthingDev &dev, int index);
    void downloadProgressChanged();
    void dirCompleted(ChronoUtilities::DateTime when, const SyncthingDir &dir, int index, const SyncthingDev *remoteDev = nullptr);
    void newNotification(ChronoUtilities::DateTime when, const QString &message);
    void error(const QString &errorMessage, SyncthingErrorCategory category, int networkError, const QNetworkRequest &request = QNetworkRequest(),
        const QByteArray &response = QByteArray());
    void statusChanged(SyncthingStatus newStatus);
    void configDirChanged(const QString &newConfigDir);
    void myIdChanged(const QString &myNewId);
    void trafficChanged(uint64 totalIncomingTraffic, uint64 totalOutgoingTraffic);
    void newConfigTriggered();
    void rescanTriggered(const QString &dirId);
    void devicePauseTriggered(const QStringList &devIds);
    void deviceResumeTriggered(const QStringList &devIds);
    void directoryPauseTriggered(const QStringList &dirIds);
    void directoryResumeTriggered(const QStringList &dirIds);
    void restartTriggered();
    void shutdownTriggered();

private Q_SLOTS:
    void requestEvents();
    void abortAllRequests();

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
    void readStartingEvent(const QJsonObject &eventData);
    void readStatusChangedEvent(ChronoUtilities::DateTime eventTime, const QJsonObject &eventData);
    void readDownloadProgressEvent(ChronoUtilities::DateTime eventTime, const QJsonObject &eventData);
    void readDirEvent(ChronoUtilities::DateTime eventTime, const QString &eventType, const QJsonObject &eventData);
    void readDeviceEvent(ChronoUtilities::DateTime eventTime, const QString &eventType, const QJsonObject &eventData);
    void readItemStarted(ChronoUtilities::DateTime eventTime, const QJsonObject &eventData);
    void readItemFinished(ChronoUtilities::DateTime eventTime, const QJsonObject &eventData);
    void readFolderErrors(ChronoUtilities::DateTime eventTime, const QJsonObject &eventData, SyncthingDir &dirInfo, int index);
    void readFolderCompletion(ChronoUtilities::DateTime eventTime, const QJsonObject &eventData, SyncthingDir &dirInfo, int index);
    void readFolderCompletion(
        ChronoUtilities::DateTime eventTime, const QJsonObject &eventData, SyncthingDir &dirInfo, int index, const QString &devId);
    void readLocalFolderCompletion(ChronoUtilities::DateTime eventTime, const QJsonObject &eventData, SyncthingDir &dirInfo, int index);
    void readRemoteFolderCompletion(
        ChronoUtilities::DateTime eventTime, const QJsonObject &eventData, SyncthingDir &dirInfo, int index, const QString &devId);
    void readRemoteIndexUpdated(ChronoUtilities::DateTime eventTime, const QJsonObject &eventData);
    void readPostConfig();
    void readRescan();
    void readDevPauseResume();
    void readDirPauseResume();
    void readRestart();
    void readShutdown();
    void readDirStatus();
    bool readDirSummary(ChronoUtilities::DateTime eventTime, const QJsonObject &summary, SyncthingDir &dirInfo, int index);
    void readCompletion();

    void continueConnecting();
    void continueReconnecting();
    void autoReconnect();
    void setStatus(SyncthingStatus status);
    void emitNotification(ChronoUtilities::DateTime when, const QString &message);
    void emitError(const QString &message, const QJsonParseError &jsonError, QNetworkReply *reply, const QByteArray &response = QByteArray());
    void emitError(const QString &message, SyncthingErrorCategory category, QNetworkReply *reply);
    void emitMyIdChanged(const QString &newId);
    void handleFatalConnectionError();
    void recalculateStatus();

private:
    QNetworkRequest prepareRequest(const QString &path, const QUrlQuery &query, bool rest = true);
    QNetworkReply *requestData(const QString &path, const QUrlQuery &query, bool rest = true);
    QNetworkReply *postData(const QString &path, const QUrlQuery &query, const QByteArray &data = QByteArray());
    bool pauseResumeDevice(const QStringList &devIds, bool paused);
    bool pauseResumeDirectory(const QStringList &dirIds, bool paused);
    SyncthingDir *addDirInfo(std::vector<SyncthingDir> &dirs, const QString &dirId);
    SyncthingDev *addDevInfo(std::vector<SyncthingDev> &devs, const QString &devId);

    QString m_syncthingUrl;
    QByteArray m_apiKey;
    QString m_user;
    QString m_password;
    SyncthingStatus m_status;
    bool m_keepPolling;
    bool m_reconnecting;
    bool m_requestCompletion;
    int m_lastEventId;
    QTimer m_trafficPollTimer;
    QTimer m_devStatsPollTimer;
    QTimer m_errorsPollTimer;
    QTimer m_autoReconnectTimer;
    unsigned int m_autoReconnectTries;
    QString m_configDir;
    QString m_myId;
    uint64 m_totalIncomingTraffic;
    uint64 m_totalOutgoingTraffic;
    double m_totalIncomingRate;
    double m_totalOutgoingRate;
    QNetworkReply *m_configReply;
    QNetworkReply *m_statusReply;
    QNetworkReply *m_connectionsReply;
    QNetworkReply *m_errorsReply;
    QNetworkReply *m_eventsReply;
    bool m_unreadNotifications;
    bool m_hasConfig;
    bool m_hasStatus;
    std::vector<SyncthingDir> m_dirs;
    std::vector<SyncthingDev> m_devs;
    ChronoUtilities::DateTime m_lastConnectionsUpdate;
    ChronoUtilities::DateTime m_lastFileTime;
    ChronoUtilities::DateTime m_lastErrorTime;
    QString m_lastFileName;
    bool m_lastFileDeleted;
    QList<QSslError> m_expectedSslErrors;
    QJsonObject m_rawConfig;
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
    m_user = user, m_password = password;
}

/*!
 * \brief Returns the connection status.
 */
inline SyncthingStatus SyncthingConnection::status() const
{
    return m_status;
}

/*!
 * \brief Returns whether the connection has been established.
 */
inline bool SyncthingConnection::isConnected() const
{
    return m_status != SyncthingStatus::Disconnected && m_status != SyncthingStatus::Reconnecting;
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
 * \brief Considers all notifications as read; hence might trigger a status update.
 */
inline void SyncthingConnection::considerAllNotificationsRead()
{
    m_unreadNotifications = false;
    requestClearingErrors();
}

/*!
 * \brief Returns the interval for polling traffic status (which currently can not be received via event API) in milliseconds.
 * \remarks Default value is 2000 milliseconds.
 */
inline int SyncthingConnection::trafficPollInterval() const
{
    return m_trafficPollTimer.interval();
}

/*!
 * \brief Sets the interval for polling traffic status (which currently can not be received via event API) in milliseconds.
 * \remarks Default value is 2000 milliseconds.
 */
inline void SyncthingConnection::setTrafficPollInterval(int trafficPollInterval)
{
    if (!trafficPollInterval) {
        m_trafficPollTimer.stop();
    }
    m_trafficPollTimer.setInterval(trafficPollInterval);
}

/*!
 * \brief Returns the interval for polling device statistics (which currently can not be received via event API) in milliseconds.
 * \remarks Default value is 60000 milliseconds.
 */
inline int SyncthingConnection::devStatsPollInterval() const
{
    return m_devStatsPollTimer.interval();
}

/*!
 * \brief Sets the interval for polling device statistics (which currently can not be received via event API) in milliseconds.
 * \remarks Default value is 60000 milliseconds.
 */
inline void SyncthingConnection::setDevStatsPollInterval(int devStatsPollInterval)
{
    if (!devStatsPollInterval) {
        m_devStatsPollTimer.stop();
    }
    m_devStatsPollTimer.setInterval(devStatsPollInterval);
}

/*!
 * \brief Returns the interval for polling Syncthing errors (which currently can not be received via event API) in milliseconds.
 * \remarks Default value is 30000 milliseconds.
 */
inline int SyncthingConnection::errorsPollInterval() const
{
    return m_errorsPollTimer.interval();
}

/*!
 * \brief Sets the interval for polling Syncthing errors (which currently can not be received via event API) in milliseconds.
 * \remarks Default value is 30000 milliseconds.
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
 * \remarks Default value is 0 which indicates disabled auto-reconnect.
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
 * \remarks Default value is 0 which indicates disabled auto-reconnect.
 */
inline void SyncthingConnection::setAutoReconnectInterval(int interval)
{
    if (!interval) {
        m_autoReconnectTimer.stop();
    }
    m_autoReconnectTimer.setInterval(interval);
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
 * \brief Returns the total incoming traffic in byte.
 */
inline uint64 SyncthingConnection::totalIncomingTraffic() const
{
    return m_totalIncomingTraffic;
}

/*!
 * \brief Returns the total outgoing traffic in byte.
 */
inline uint64 SyncthingConnection::totalOutgoingTraffic() const
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
 * \brief Returns a list of all expected certificate errors. This is meant to allow self-signed certificates.
 * \remarks This list is updated via loadSelfSignedCertificate().
 */
inline const QList<QSslError> &SyncthingConnection::expectedSslErrors()
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
} // namespace Data

Q_DECLARE_METATYPE(Data::SyncthingLogEntry)

#endif // SYNCTHINGCONNECTION_H
