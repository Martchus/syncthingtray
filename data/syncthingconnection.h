#ifndef SYNCTHINGCONNECTION_H
#define SYNCTHINGCONNECTION_H

#include <c++utilities/chrono/datetime.h>

#include <QObject>
#include <QList>
#include <QSslError>

#include <functional>
#include <vector>

QT_FORWARD_DECLARE_CLASS(QNetworkAccessManager)
QT_FORWARD_DECLARE_CLASS(QNetworkReply)
QT_FORWARD_DECLARE_CLASS(QNetworkRequest)
QT_FORWARD_DECLARE_CLASS(QUrlQuery)
QT_FORWARD_DECLARE_CLASS(QJsonObject)
QT_FORWARD_DECLARE_CLASS(QJsonArray)

namespace Settings {
struct ConnectionSettings;
}

namespace Data {

QNetworkAccessManager &networkAccessManager();

enum class SyncthingStatus
{
    Disconnected,
    Reconnecting,
    Idle,
    Scanning,
    NotificationsAvailable,
    Paused,
    Synchronizing
};

enum class DirStatus
{
    Unknown,
    Idle,
    Scanning,
    Synchronizing,
    Paused,
    OutOfSync
};

struct DirErrors
{
    DirErrors(const QString &message, const QString &path) :
        message(message),
        path(path)
    {}
    QString message;
    QString path;
};

struct SyncthingDir
{    
    QString id;
    QString label;
    QString path;
    QStringList devices;
    bool readOnly = false;
    bool ignorePermissions = false;
    bool autoNormalize = false;
    int rescanInterval = 0;
    int minDiskFreePercentage = 0;
    DirStatus status = DirStatus::Idle;
    ChronoUtilities::DateTime lastStatusUpdate;
    int progressPercentage = 0;
    int progressRate = 0;
    std::vector<DirErrors> errors;
    int globalBytes = 0, globalDeleted = 0, globalFiles = 0;
    int localBytes = 0, localDeleted = 0, localFiles = 0;
    int neededByted = 0, neededFiles = 0;
    ChronoUtilities::DateTime lastScanTime;
    ChronoUtilities::DateTime lastFileTime;
    QString lastFileName;
    bool lastFileDeleted = false;

    bool assignStatus(const QString &statusStr, ChronoUtilities::DateTime time);
    bool assignStatus(DirStatus newStatus, ChronoUtilities::DateTime time);
};

enum class DevStatus
{
    Unknown,
    Disconnected,
    OwnDevice,
    Idle,
    Synchronizing,
    OutOfSync,
    Rejected
};

struct SyncthingDev
{
    QString id;
    QString name;
    QStringList addresses;
    QString compression;
    QString certName;
    DevStatus status;
    int progressPercentage = 0;
    int progressRate = 0;
    bool introducer = false;
    bool paused = false;
    int totalIncomingTraffic = 0;
    int totalOutgoingTraffic = 0;
    QString connectionAddress;
    QString connectionType;
    QString clientVersion;
    ChronoUtilities::DateTime lastSeen;
};

struct SyncthingLogEntry
{
    QString when;
    QString message;
};

class SyncthingConnection : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString syncthingUrl READ syncthingUrl WRITE setSyncthingUrl)
    Q_PROPERTY(QByteArray apiKey READ apiKey WRITE setApiKey)
    Q_PROPERTY(SyncthingStatus status READ status NOTIFY statusChanged)
    Q_PROPERTY(QString configDir READ configDir NOTIFY configDirChanged)
    Q_PROPERTY(QString myId READ myId NOTIFY myIdChanged)
    Q_PROPERTY(int totalIncomingTraffic READ totalIncomingTraffic NOTIFY trafficChanged)
    Q_PROPERTY(int totalOutgoingTraffic READ totalOutgoingTraffic NOTIFY trafficChanged)

public:
    explicit SyncthingConnection(const QString &syncthingUrl = QStringLiteral("http://localhost:8080"), const QByteArray &apiKey = QByteArray(), QObject *parent = nullptr);
    ~SyncthingConnection();

    const QString &syncthingUrl() const;
    void setSyncthingUrl(const QString &url);
    const QByteArray &apiKey() const;
    void setApiKey(const QByteArray &apiKey);
    const QString &user() const;
    const QString &password() const;
    void setCredentials(const QString &user, const QString &password);
    SyncthingStatus status() const;
    QString statusText() const;
    bool isConnected() const;
    const QString &configDir() const;
    const QString &myId() const;
    int totalIncomingTraffic() const;
    int totalOutgoingTraffic() const;
    double totalIncomingRate() const;
    double totalOutgoingRate() const;
    const std::vector<SyncthingDir> &dirInfo() const;
    const std::vector<SyncthingDev> &devInfo() const;
    QMetaObject::Connection requestQrCode(const QString &text, std::function<void (const QPixmap &)> callback);
    QMetaObject::Connection requestLog(std::function<void (const std::vector<SyncthingLogEntry> &)> callback);
    const QList<QSslError> &expectedSslErrors();

public Q_SLOTS:
    void loadSelfSignedCertificate();
    void connect();
    void disconnect();
    void reconnect();
    void reconnect(Settings::ConnectionSettings &connectionSettings);
    void pause(const QString &dev);
    void pauseAllDevs();
    void resume(const QString &dev);
    void resumeAllDevs();
    void rescan(const QString &dir);
    void rescanAllDirs();
    void restart();
    void notificationsRead();

Q_SIGNALS:
    /*!
     * \brief Indicates new configuration (dirs, devs, ...) is available.
     * \remarks
     * - Configuration is requested automatically when connecting.
     * - Previous directories (and directory info objects!) are invalidated.
     * - Previous devices (and device info objects!) are invalidated.
     */
    void newConfig(const QJsonObject &config);

    /*!
     * \brief Indicates new directories are available.
     * \remarks Always emitted after newConfig() as soon as new directory info objects become available.
     */
    void newDirs(const std::vector<SyncthingDir> &dirs);

    /*!
     * \brief Indicates new devices are available.
     * \remarks Always emitted after newConfig() as soon as new device info objects become available.
     */
    void newDevices(const std::vector<SyncthingDev> &devs);

    /*!
     * \brief Indicates new events (dir status changed, ...) are available.
     * \remarks New events are automatically polled when connected.
     */
    void newEvents(const QJsonArray &events);

    /*!
     * \brief Indicates the status of the specified \a dir changed.
     */
    void dirStatusChanged(const SyncthingDir &dir, int index);

    /*!
     * \brief Indicates the status of the specified \a dev changed.
     */
    void devStatusChanged(const SyncthingDev &dev, int index);

    /*!
     * \brief Indicates a new Syncthing notification is available.
     */
    void newNotification(const QString &message);

    /*!
     * \brief Indicates a request (for configuration, events, ...) failed.
     */
    void error(const QString &errorMessage);

    /*!
     * \brief Indicates the status of the connection changed.
     */
    void statusChanged(SyncthingStatus newStatus);

    /*!
     * \brief Indicates the Syncthing home/configuration directory changed.
     */
    void configDirChanged(const QString &newConfigDir);

    /*!
     * \brief Indicates ID of the own Syncthing device changed.
     */
    void myIdChanged(const QString &myNewId);

    /*!
     * \brief Indicates totalIncomingTraffic() or totalOutgoingTraffic() has changed.
     */
    void trafficChanged(int totalIncomingTraffic, int totalOutgoingTraffic);

private Q_SLOTS:
    void requestConfig();
    void requestStatus();
    void requestConnections();
    void requestDirStatistics();
    void requestDeviceStatistics();
    void requestEvents();
    void abortAllRequests();

    void readConfig();
    void readDirs(const QJsonArray &dirs);
    void readDevs(const QJsonArray &devs);
    void readStatus();
    void readConnections();
    void readDirStatistics();
    void readDeviceStatistics();
    void readEvents();
    void readStartingEvent(const QJsonObject &eventData);
    void readStatusChangedEvent(ChronoUtilities::DateTime eventTime, const QJsonObject &eventData);
    void readDownloadProgressEvent(const QJsonObject &eventData);
    void readDirEvent(ChronoUtilities::DateTime eventTime, const QString &eventType, const QJsonObject &eventData);
    void readDeviceEvent(ChronoUtilities::DateTime eventTime, const QString &eventType, const QJsonObject &eventData);
    void readItemStarted(ChronoUtilities::DateTime eventTime, const QJsonObject &eventData);
    void readItemFinished(ChronoUtilities::DateTime eventTime, const QJsonObject &eventData);
    void readRescan();
    void readPauseResume();
    void readRestart();

    void continueReconnect();
    void setStatus(SyncthingStatus status);

private:
    QNetworkRequest prepareRequest(const QString &path, const QUrlQuery &query, bool rest = true);
    QNetworkReply *requestData(const QString &path, const QUrlQuery &query, bool rest = true);
    QNetworkReply *postData(const QString &path, const QUrlQuery &query, const QByteArray &data = QByteArray());
    SyncthingDir *findDirInfo(const QString &dir, int &row);
    SyncthingDev *findDevInfo(const QString &dev, int &row);
    void continueConnecting();

    QString m_syncthingUrl;
    QByteArray m_apiKey;
    QString m_user;
    QString m_password;
    SyncthingStatus m_status;
    bool m_keepPolling;
    bool m_reconnecting;
    int m_lastEventId;
    QString m_configDir;
    QString m_myId;
    int m_totalIncomingTraffic;
    int m_totalOutgoingTraffic;
    double m_totalIncomingRate;
    double m_totalOutgoingRate;
    QNetworkReply *m_configReply;
    QNetworkReply *m_statusReply;
    QNetworkReply *m_connectionsReply;
    QNetworkReply *m_eventsReply;
    bool m_unreadNotifications;
    bool m_hasConfig;
    bool m_hasStatus;
    std::vector<SyncthingDir> m_dirs;
    std::vector<SyncthingDev> m_devs;
    ChronoUtilities::DateTime m_lastConnectionsUpdate;
    ChronoUtilities::DateTime m_lastFileTime;
    QString m_lastFileName;
    bool m_lastFileDeleted;
    QList<QSslError> m_expectedSslErrors;
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
inline int SyncthingConnection::totalIncomingTraffic() const
{
    return m_totalIncomingTraffic;
}

/*!
 * \brief Returns the total outgoing traffic in byte.
 */
inline int SyncthingConnection::totalOutgoingTraffic() const
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
 * \brief Returns all available directory info.
 * \remarks The returned object container object is persistent. However, the contained
 *          info objects are invalidated when the newConfig() signal is emitted.
 */
inline const std::vector<SyncthingDir> &SyncthingConnection::dirInfo() const
{
    return m_dirs;
}

/*!
 * \brief Returns all available device info.
 * \remarks The returned object container object is persistent. However, the contained
 *          info objects are invalidated when the newConfig() signal is emitted.
 */
inline const std::vector<SyncthingDev> &SyncthingConnection::devInfo() const
{
    return m_devs;
}

/*!
 * \brief Returns a list of all expected certificate errors.
 * \remarks This list is shared by all instances and updated via loadSelfSignedCertificate().
 */
inline const QList<QSslError> &SyncthingConnection::expectedSslErrors()
{
    return m_expectedSslErrors;
}

}

#endif // SYNCTHINGCONNECTION_H
