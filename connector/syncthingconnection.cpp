#include "./syncthingconnection.h"
#include "./syncthingconfig.h"
#include "./syncthingconnectionsettings.h"

#include <c++utilities/conversion/conversionexception.h>
#include <c++utilities/conversion/stringconversion.h>

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QAuthenticator>
#include <QStringBuilder>
#include <QTimer>
#include <QHostAddress>

#include <utility>

using namespace std;
using namespace ChronoUtilities;
using namespace ConversionUtilities;

namespace Data {

/*!
 * \brief Returns the QNetworkAccessManager instance used by SyncthingConnection instances.
 */
QNetworkAccessManager &networkAccessManager()
{
    static QNetworkAccessManager networkAccessManager;
    return networkAccessManager;
}

/*!
 * \brief Assigns the status from the specified status string.
 * \returns Returns whether the status has actually changed.
 */
bool SyncthingDir::assignStatus(const QString &statusStr, ChronoUtilities::DateTime time)
{
    if(lastStatusUpdate > time) {
        return false;
    } else {
        lastStatusUpdate = time;
    }
    DirStatus newStatus;
    if(statusStr == QLatin1String("idle")) {
        progressPercentage = 0;
        newStatus = DirStatus::Idle;
    } else if(statusStr == QLatin1String("scanning")) {
        newStatus = DirStatus::Scanning;
    } else if(statusStr == QLatin1String("syncing")) {
        if(!errors.empty()) {
            errors.clear(); // errors become obsolete
            status = DirStatus::Unknown; // ensure status changed signal is emitted
        }
        newStatus = DirStatus::Synchronizing;
    } else if(statusStr == QLatin1String("error")) {
        progressPercentage = 0;
        newStatus = DirStatus::OutOfSync;
    } else {
        newStatus = DirStatus::Unknown;
    }
    if(newStatus != status) {
        switch(status) {
        case DirStatus::Scanning:
            lastScanTime = DateTime::now();
            break;
        default:
            ;
        }
        status = newStatus;
        return true;
    }
    return false;
}

bool SyncthingDir::assignStatus(DirStatus newStatus, DateTime time)
{
    if(lastStatusUpdate > time) {
        return false;
    } else {
        lastStatusUpdate = time;
    }
    if(newStatus != status) {
        switch(status) {
        case DirStatus::Scanning:
            lastScanTime = DateTime::now();
            break;
        default:
            ;
        }
        status = newStatus;
        return true;
    }
    return false;
}

SyncthingItemDownloadProgress::SyncthingItemDownloadProgress(const QString &containingDirPath, const QString &relativeItemPath, const QJsonObject &values) :
    relativePath(relativeItemPath),
    fileInfo(containingDirPath % QChar('/') % QString(relativeItemPath).replace(QChar('\\'), QChar('/'))),
    blocksCurrentlyDownloading(values.value(QStringLiteral("Pulling")).toInt()),
    blocksAlreadyDownloaded(values.value(QStringLiteral("Pulled")).toInt()),
    totalNumberOfBlocks(values.value(QStringLiteral("Total")).toInt()),
    downloadPercentage((blocksAlreadyDownloaded > 0 && totalNumberOfBlocks > 0)
                       ? (static_cast<unsigned int>(blocksAlreadyDownloaded) * 100 / static_cast<unsigned int>(totalNumberOfBlocks))
                       : 0),
    blocksCopiedFromOrigin(values.value(QStringLiteral("CopiedFromOrigin")).toInt()),
    blocksCopiedFromElsewhere(values.value(QStringLiteral("CopiedFromElsewhere")).toInt()),
    blocksReused(values.value(QStringLiteral("Reused")).toInt()),
    bytesAlreadyHandled(values.value(QStringLiteral("BytesDone")).toInt()),
    totalNumberOfBytes(values.value(QStringLiteral("BytesTotal")).toInt()),
    label(QStringLiteral("%1 / %2 - %3 %").arg(
              QString::fromLatin1(dataSizeToString(blocksAlreadyDownloaded > 0 ? static_cast<uint64>(blocksAlreadyDownloaded) * syncthingBlockSize : 0).data()),
              QString::fromLatin1(dataSizeToString(totalNumberOfBlocks > 0 ? static_cast<uint64>(totalNumberOfBlocks) * syncthingBlockSize : 0).data()),
              QString::number(downloadPercentage))
          )
{}

/*!
 * \class SyncthingConnection
 * \brief The SyncthingConnection class allows Qt applications to access Syncthing.
 * \remarks All requests are performed asynchronously.
 */

/*!
 * \brief Constructs a new instance ready to connect. To establish the connection, call connect().
 */
SyncthingConnection::SyncthingConnection(const QString &syncthingUrl, const QByteArray &apiKey, QObject *parent) :
    QObject(parent),
    m_syncthingUrl(syncthingUrl),
    m_apiKey(apiKey),
    m_status(SyncthingStatus::Disconnected),
    m_keepPolling(false),
    m_reconnecting(false),
    m_lastEventId(0),
    m_trafficPollInterval(2000),
    m_devStatsPollInterval(60000),
    m_totalIncomingTraffic(0),
    m_totalOutgoingTraffic(0),
    m_totalIncomingRate(0),
    m_totalOutgoingRate(0),
    m_configReply(nullptr),
    m_statusReply(nullptr),
    m_connectionsReply(nullptr),
    m_errorsReply(nullptr),
    m_eventsReply(nullptr),
    m_unreadNotifications(false),
    m_hasConfig(false),
    m_hasStatus(false),
    m_lastFileDeleted(false)
{}

/*!
 * \brief Destroys the instance. Ongoing requests are aborted.
 */
SyncthingConnection::~SyncthingConnection()
{
    disconnect();
}

/*!
 * \brief Returns the string representation of the current status().
 */
QString SyncthingConnection::statusText() const
{
    switch(m_status) {
    case SyncthingStatus::Disconnected:
        return tr("disconnected");
    case SyncthingStatus::Reconnecting:
        return tr("reconnecting");
    case SyncthingStatus::Idle:
        return tr("connected");
    case SyncthingStatus::NotificationsAvailable:
        return tr("connected, notifications available");
    case SyncthingStatus::Paused:
        return tr("connected, paused");
    case SyncthingStatus::Synchronizing:
        return tr("connected, synchronizing");
    default:
        return tr("unknown");
    }
}

/*!
 * \brief Connects asynchronously to Syncthing. Does nothing if already connected.
 */
void SyncthingConnection::connect()
{
    if(!isConnected()) {
        m_reconnecting = m_hasConfig = m_hasStatus = false;
        if(m_apiKey.isEmpty() || m_syncthingUrl.isEmpty()) {
            emit error(tr("Connection configuration is insufficient."));
            return;
        }
        requestConfig();
        requestStatus();
        m_keepPolling = true;
    }
}

/*!
 * \brief Disconnects. Does nothing if not connected.
 */
void SyncthingConnection::disconnect()
{
    m_reconnecting = m_hasConfig = m_hasStatus = false;
    abortAllRequests();
}

/*!
 * \brief Disconnects if connected, then (re-)connects asynchronously.
 * \remarks Clears the currently cached configuration.
 */
void SyncthingConnection::reconnect()
{
    if(isConnected()) {
        m_reconnecting = true;
        m_hasConfig = m_hasStatus = false;
        abortAllRequests();
    } else {
        continueReconnecting();
    }
}

/*!
 * \brief Applies the specifies configuration and tries to reconnect via reconnect().
 * \remarks The expected SSL errors of the specified configuration are updated accordingly.
 */
void SyncthingConnection::reconnect(SyncthingConnectionSettings &connectionSettings)
{
    setSyncthingUrl(connectionSettings.syncthingUrl);
    setApiKey(connectionSettings.apiKey);
    if(connectionSettings.authEnabled) {
        setCredentials(connectionSettings.userName, connectionSettings.password);
    } else {
        setCredentials(QString(), QString());
    }
    setTrafficPollInterval(connectionSettings.trafficPollInterval);
    setDevStatsPollInterval(connectionSettings.devStatsPollInterval);
    loadSelfSignedCertificate();
    if(connectionSettings.expectedSslErrors.isEmpty()) {
        connectionSettings.expectedSslErrors = expectedSslErrors();
    }
    reconnect();
}

/*!
 * \brief Internally called to reconnect; ensures currently cached config is cleared.
 */
void SyncthingConnection::continueReconnecting()
{
    emit newConfig(QJsonObject()); // configuration will be invalidated
    setStatus(SyncthingStatus::Reconnecting);
    m_keepPolling = true;
    m_reconnecting = false;
    m_lastEventId = 0;
    m_configDir.clear();
    m_myId.clear();
    m_totalIncomingTraffic = 0;
    m_totalOutgoingTraffic = 0;
    m_totalIncomingRate = 0.0;
    m_totalOutgoingRate = 0.0;
    m_unreadNotifications = false;
    m_hasConfig = false;
    m_hasStatus = false;
    m_dirs.clear();
    m_devs.clear();
    m_lastConnectionsUpdate = DateTime();
    m_lastFileTime = DateTime();
    m_lastErrorTime = DateTime();
    m_lastFileName.clear();
    m_lastFileDeleted = false;
    if(m_apiKey.isEmpty() || m_syncthingUrl.isEmpty()) {
        emit error(tr("Connection configuration is insufficient."));
        return;
    }
    requestConfig();
    requestStatus();
}

/*!
 * \brief Requests pausing the device with the specified ID.
 *
 * The signal error() is emitted when the request was not successful.
 */
void SyncthingConnection::pause(const QString &devId)
{
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("device"), devId);
    QObject::connect(postData(QStringLiteral("system/pause"), query), &QNetworkReply::finished, this, &SyncthingConnection::readPauseResume);
}

/*!
 * \brief Requests pausing all devices.
 *
 * The signal error() is emitted when the request was not successful.
 */
void SyncthingConnection::pauseAllDevs()
{
    for(const SyncthingDev &dev : m_devs) {
        pause(dev.id);
    }
}

/*!
 * \brief Requests resuming the device with the specified ID.
 *
 * The signal error() is emitted when the request was not successful.
 */
void SyncthingConnection::resume(const QString &devId)
{
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("device"), devId);
    QObject::connect(postData(QStringLiteral("system/resume"), query), &QNetworkReply::finished, this, &SyncthingConnection::readPauseResume);
}

/*!
 * \brief Requests resuming all devices.
 *
 * The signal error() is emitted when the request was not successful.
 */
void SyncthingConnection::resumeAllDevs()
{
    for(const SyncthingDev &dev : m_devs) {
        resume(dev.id);
    }
}

/*!
 * \brief Requests rescanning the directory with the specified ID.
 *
 * The signal error() is emitted when the request was not successful.
 */
void SyncthingConnection::rescan(const QString &dirId)
{
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("folder"), dirId);
    QObject::connect(postData(QStringLiteral("db/scan"), query), &QNetworkReply::finished, this, &SyncthingConnection::readRescan);
}

/*!
 * \brief Requests rescanning all directories.
 *
 * The signal error() is emitted when the request was not successful.
 */
void SyncthingConnection::rescanAllDirs()
{
    for(const SyncthingDir &dir : m_dirs) {
        rescan(dir.id);
    }
}

/*!
 * \brief Requests Syncthing to restart.
 *
 * The signal error() is emitted when the request was not successful.
 */
void SyncthingConnection::restart()
{
    QObject::connect(postData(QStringLiteral("system/restart"), QUrlQuery()), &QNetworkReply::finished, this, &SyncthingConnection::readRestart);
}

/*!
 * \brief Considers all notifications as read; hence might trigger a status update.
 */
void SyncthingConnection::considerAllNotificationsRead()
{
    m_unreadNotifications = false;
    setStatus(status());
}

/*!
 * \brief Prepares a request for the specified \a path and \a query.
 */
QNetworkRequest SyncthingConnection::prepareRequest(const QString &path, const QUrlQuery &query, bool rest)
{
    QUrl url(m_syncthingUrl);
    url.setPath(rest ? (url.path() % QStringLiteral("/rest/") % path) : (url.path() + path));
    url.setUserName(user());
    url.setPassword(password());
    url.setQuery(query);
    QNetworkRequest request(url);
    request.setRawHeader("X-API-Key", m_apiKey);
    return request;
}

/*!
 * \brief Requests asynchronously data using the rest API.
 */
QNetworkReply *SyncthingConnection::requestData(const QString &path, const QUrlQuery &query, bool rest)
{
    auto *reply = networkAccessManager().get(prepareRequest(path, query, rest));
    reply->ignoreSslErrors(m_expectedSslErrors);
    return reply;
}

/*!
 * \brief Posts asynchronously data using the rest API.
 */
QNetworkReply *SyncthingConnection::postData(const QString &path, const QUrlQuery &query, const QByteArray &data)
{
    auto *reply = networkAccessManager().post(prepareRequest(path, query), data);
    reply->ignoreSslErrors(m_expectedSslErrors);
    return reply;
}

/*!
 * \brief Returns the directory info object for the directory with the specified ID.
 * \returns Returns a pointer to the object or nullptr if not found.
 * \remarks The returned object becomes invalid when the newDirs() signal is emitted or the connection is destroyed.
 */
SyncthingDir *SyncthingConnection::findDirInfo(const QString &dirId, int &row)
{
    row = 0;
    for(SyncthingDir &d : m_dirs) {
        if(d.id == dirId) {
            return &d;
        }
        ++row;
    }
    return nullptr; // TODO: dir is unknown, trigger refreshing the config
}

/*!
 * \brief Returns the device info object for the device with the specified ID.
 * \returns Returns a pointer to the object or nullptr if not found.
 * \remarks The returned object becomes invalid when the newConfig() signal is emitted or the connection is destroyed.
 */
SyncthingDev *SyncthingConnection::findDevInfo(const QString &devId, int &row)
{
    row = 0;
    for(SyncthingDev &d : m_devs) {
        if(d.id == devId) {
            return &d;
        }
        ++row;
    }
    return nullptr; // TODO: dev is unknown, trigger refreshing the config
}

/*!
 * \brief Continues connecting if both - config and status - have been parsed yet and continuous polling is enabled.
 */
void SyncthingConnection::continueConnecting()
{
    if(m_keepPolling && m_hasConfig && m_hasStatus) {
        requestConnections();
        requestDirStatistics();
        requestDeviceStatistics();
        requestErrors();
        // since config and status could be read successfully, let's poll for events
        m_lastEventId = 0;
        requestEvents();
    }
}

/*!
 * \brief Aborts all pending requests.
 */
void SyncthingConnection::abortAllRequests()
{
    if(m_configReply) {
        m_configReply->abort();
    }
    if(m_statusReply) {
        m_statusReply->abort();
    }
    if(m_connectionsReply) {
        m_connectionsReply->abort();
    }
    if(m_errorsReply) {
        m_errorsReply->abort();
    }
    if(m_eventsReply) {
        m_eventsReply->abort();
    }
}

/*!
 * \brief Requests the Syncthing configuration asynchronously.
 *
 * The signal newConfig() is emitted on success; otherwise error() is emitted.
 */
void SyncthingConnection::requestConfig()
{
    QObject::connect(m_configReply = requestData(QStringLiteral("system/config"), QUrlQuery()), &QNetworkReply::finished, this, &SyncthingConnection::readConfig);
}

/*!
 * \brief Requests the Syncthing status asynchronously.
 *
 * The signal configDirChanged() and myIdChanged() emitted when those values have changed; error() is emitted in the error case.
 */
void SyncthingConnection::requestStatus()
{
    QObject::connect(m_statusReply = requestData(QStringLiteral("system/status"), QUrlQuery()), &QNetworkReply::finished, this, &SyncthingConnection::readStatus);
}

/*!
 * \brief Requests current connections asynchronously.
 *
 * The signal devStatusChanged() is emitted for each device where the connection status has changed; error() is emitted in the error case.
 */
void SyncthingConnection::requestConnections()
{
    QObject::connect(m_connectionsReply = requestData(QStringLiteral("system/connections"), QUrlQuery()), &QNetworkReply::finished, this, &SyncthingConnection::readConnections);
}

/*!
 * \brief Requests errors asynchronously.
 *
 * The signal newNotification() is emitted on success; error() is emitted in the error case.
 */
void SyncthingConnection::requestErrors()
{
    QObject::connect(m_errorsReply = requestData(QStringLiteral("system/error"), QUrlQuery()), &QNetworkReply::finished, this, &SyncthingConnection::readErrors);
}

/*!
 * \brief Requests directory statistics asynchronously.
 */
void SyncthingConnection::requestDirStatistics()
{
    QObject::connect(requestData(QStringLiteral("stats/folder"), QUrlQuery()), &QNetworkReply::finished, this, &SyncthingConnection::readDirStatistics);
}

/*!
 * \brief Requests device statistics asynchronously.
 */
void SyncthingConnection::requestDeviceStatistics()
{
    QObject::connect(requestData(QStringLiteral("stats/device"), QUrlQuery()), &QNetworkReply::finished, this, &SyncthingConnection::readDeviceStatistics);
}

/*!
 * \brief Requests the Syncthing events (since the last successful call) asynchronously.
 *
 * The signal newEvents() is emitted on success; otherwise error() is emitted.
 */
void SyncthingConnection::requestEvents()
{
    QUrlQuery query;
    if(m_lastEventId) {
        query.addQueryItem(QStringLiteral("since"), QString::number(m_lastEventId));
    }
    QObject::connect(m_eventsReply = requestData(QStringLiteral("events"), query), &QNetworkReply::finished, this, &SyncthingConnection::readEvents);
}

/*!
 * \brief Requests a QR code for the specified \a text.
 *
 * The specified \a callback is called on success; otherwise error() is emitted.
 */
QMetaObject::Connection SyncthingConnection::requestQrCode(const QString &text, std::function<void(const QByteArray &)> callback)
{
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("text"), text);
    QNetworkReply *reply = requestData(QStringLiteral("/qr/"), query, false);
    return QObject::connect(reply, &QNetworkReply::finished, [this, reply, callback] {
        reply->deleteLater();
        switch(reply->error()) {
        case QNetworkReply::NoError:
            callback(reply->readAll());
            break;
        default:
            emit error(tr("Unable to request QR-Code: ") + reply->errorString());
        }
    });
}

/*!
 * \brief Requests the Syncthing log.
 *
 * The specified \a callback is called on success; otherwise error() is emitted.
 */
QMetaObject::Connection SyncthingConnection::requestLog(std::function<void (const std::vector<SyncthingLogEntry> &)> callback)
{
    QNetworkReply *reply = requestData(QStringLiteral("system/log"), QUrlQuery());
    return QObject::connect(reply, &QNetworkReply::finished, [this, reply, callback] {
        reply->deleteLater();
        switch(reply->error()) {
        case QNetworkReply::NoError: {
            QJsonParseError jsonError;
            const QJsonDocument replyDoc = QJsonDocument::fromJson(reply->readAll(), &jsonError);
            if(jsonError.error == QJsonParseError::NoError) {
                const QJsonArray log(replyDoc.object().value(QStringLiteral("messages")).toArray());
                vector<SyncthingLogEntry> logEntries;
                logEntries.reserve(log.size());
                for(const QJsonValue &logVal : log) {
                    const QJsonObject logObj(logVal.toObject());
                    logEntries.emplace_back(logObj.value(QStringLiteral("when")).toString(), logObj.value(QStringLiteral("message")).toString());
                }
                callback(logEntries);
            } else {
                emit error(tr("Unable to parse Syncthing log: ") + jsonError.errorString());
            }
            break;
        } default:
            emit error(tr("Unable to request system log: ") + reply->errorString());
        }
    });
}

/*!
 * \brief Locates and loads the (self-signed) certificate used by the Syncthing GUI.
 */
void SyncthingConnection::loadSelfSignedCertificate()
{
    // ensure current exceptions for self-signed certificates are cleared
    m_expectedSslErrors.clear();

    // only possible if the Syncthing instance is running on the local machine
    const QString host(QUrl(syncthingUrl()).host());
    if(host.compare(QLatin1String("localhost"), Qt::CaseInsensitive) != 0 && !QHostAddress(host).isLoopback()) {
        return;
    }

    // find cert
    const QString certPath = !m_configDir.isEmpty() ? (m_configDir + QStringLiteral("/https-cert.pem")) : SyncthingConfig::locateHttpsCertificate();
    if(certPath.isEmpty()) {
        emit error(tr("Unable to locate certificate used by Syncthing GUI."));
        return;
    }
    // add exception
    const QList<QSslCertificate> cert = QSslCertificate::fromPath(certPath);
    if(cert.isEmpty()) {
        emit error(tr("Unable to load certificate used by Syncthing GUI."));
        return;
    }
    m_expectedSslErrors.reserve(4);
    m_expectedSslErrors << QSslError(QSslError::UnableToGetLocalIssuerCertificate, cert.at(0));
    m_expectedSslErrors << QSslError(QSslError::UnableToVerifyFirstCertificate, cert.at(0));
    m_expectedSslErrors << QSslError(QSslError::SelfSignedCertificate, cert.at(0));
    m_expectedSslErrors << QSslError(QSslError::HostNameMismatch, cert.at(0));
}

/*!
 * \brief Reads results of requestConfig().
 */
void SyncthingConnection::readConfig()
{
    auto *reply = static_cast<QNetworkReply *>(sender());
    reply->deleteLater();
    if(reply == m_configReply) {
        m_configReply = nullptr;
    }

    switch(reply->error()) {
    case QNetworkReply::NoError: {
        QJsonParseError jsonError;
        const QJsonDocument replyDoc = QJsonDocument::fromJson(reply->readAll(), &jsonError);
        if(jsonError.error == QJsonParseError::NoError) {
            const QJsonObject replyObj(replyDoc.object());
            emit newConfig(replyObj);
            readDirs(replyObj.value(QStringLiteral("folders")).toArray());
            readDevs(replyObj.value(QStringLiteral("devices")).toArray());
            m_hasConfig = true;
            continueConnecting();
        } else {
            emit error(tr("Unable to parse Syncthing config: ") + jsonError.errorString());
        }
        break;
    } case QNetworkReply::OperationCanceledError:
        return; // intended, not an error
    default:
        emit error(tr("Unable to request Syncthing config: ") + reply->errorString());
        setStatus(SyncthingStatus::Disconnected);
    }
}

/*!
 * \brief Reads directory results of requestConfig(); called by readConfig().
 */
void SyncthingConnection::readDirs(const QJsonArray &dirs)
{
    m_dirs.clear();
    m_dirs.reserve(static_cast<size_t>(dirs.size()));
    for(const QJsonValue &dirVal : dirs) {
        const QJsonObject dirObj(dirVal.toObject());
        SyncthingDir dirItem;
        dirItem.id = dirObj.value(QStringLiteral("id")).toString();
        if(!dirItem.id.isEmpty()) { // ignore dirs with empty id
            dirItem.label = dirObj.value(QStringLiteral("label")).toString();
            dirItem.path = dirObj.value(QStringLiteral("path")).toString();
            for(const QJsonValue &dev : dirObj.value(QStringLiteral("devices")).toArray()) {
                const QString devId = dev.toObject().value(QStringLiteral("deviceID")).toString();
                if(!devId.isEmpty()) {
                    dirItem.devices << devId;
                }
            }
            dirItem.readOnly = dirObj.value(QStringLiteral("readOnly")).toBool(false);
            dirItem.rescanInterval = dirObj.value(QStringLiteral("rescanIntervalS")).toInt(-1);
            dirItem.ignorePermissions = dirObj.value(QStringLiteral("ignorePerms")).toBool(false);
            dirItem.autoNormalize = dirObj.value(QStringLiteral("autoNormalize")).toBool(false);
            dirItem.minDiskFreePercentage = dirObj.value(QStringLiteral("minDiskFreePct")).toInt(-1);
            m_dirs.emplace_back(move(dirItem));
        }
    }
    emit newDirs(m_dirs);
}

/*!
 * \brief Reads device results of requestConfig(); called by readConfig().
 */
void SyncthingConnection::readDevs(const QJsonArray &devs)
{
    m_devs.clear();
    m_devs.reserve(static_cast<size_t>(devs.size()));
    for(const QJsonValue &devVal: devs) {
        const QJsonObject devObj(devVal.toObject());
        SyncthingDev devItem;
        devItem.id = devObj.value(QStringLiteral("deviceID")).toString();
        if(!devItem.id.isEmpty()) { // ignore dirs with empty id
            devItem.name = devObj.value(QStringLiteral("name")).toString();
            for(const QJsonValue &addrVal : devObj.value(QStringLiteral("addresses")).toArray()) {
                devItem.addresses << addrVal.toString();
            }
            devItem.compression = devObj.value(QStringLiteral("compression")).toString();
            devItem.certName = devObj.value(QStringLiteral("certName")).toString();
            devItem.introducer = devObj.value(QStringLiteral("introducer")).toBool(false);
            devItem.status = devItem.id == m_myId ? DevStatus::OwnDevice : DevStatus::Unknown;
            m_devs.push_back(move(devItem));
        }
    }
    emit newDevices(m_devs);
}

/*!
 * \brief Reads results of requestStatus().
 */
void SyncthingConnection::readStatus()
{
    auto *reply = static_cast<QNetworkReply *>(sender());
    reply->deleteLater();
    if(reply == m_statusReply) {
        m_statusReply = nullptr;
    }

    switch(reply->error()) {
    case QNetworkReply::NoError: {
        QJsonParseError jsonError;
        const QJsonDocument replyDoc = QJsonDocument::fromJson(reply->readAll(), &jsonError);
        if(jsonError.error == QJsonParseError::NoError) {
            const QJsonObject replyObj(replyDoc.object());
            const QString myId(replyObj.value(QStringLiteral("myID")).toString());
            if(myId != m_myId) {
                emit myIdChanged(m_myId = myId);
                int index = 0;
                for(SyncthingDev &dev : m_devs) {
                    if(dev.id == m_myId) {
                        dev.status = DevStatus::OwnDevice;
                        emit devStatusChanged(dev, index);
                        break;
                    }
                    ++index;
                }
            }
            // other values are currently not interesting
            m_hasStatus = true;
            continueConnecting();
        } else {
            emit error(tr("Unable to parse Syncthing config: ") + jsonError.errorString());
        }
        break;
    } case QNetworkReply::OperationCanceledError:
        return; // intended, not an error
    default:
        emit error(tr("Unable to request Syncthing config: ") + reply->errorString());
    }
}

/*!
 * \brief Reads results of requestConnections().
 */
void SyncthingConnection::readConnections()
{
    auto *reply = static_cast<QNetworkReply *>(sender());
    reply->deleteLater();
    if(reply == m_connectionsReply) {
        m_connectionsReply = nullptr;
    }

    switch(reply->error()) {
    case QNetworkReply::NoError: {
        QJsonParseError jsonError;
        const QJsonDocument replyDoc = QJsonDocument::fromJson(reply->readAll(), &jsonError);
        if(jsonError.error == QJsonParseError::NoError) {
            const QJsonObject replyObj(replyDoc.object());
            const QJsonObject totalObj(replyObj.value(QStringLiteral("total")).toObject());

            // read traffic
            const int totalIncomingTraffic = totalObj.value(QStringLiteral("inBytesTotal")).toInt(0);
            const int totalOutgoingTraffic = totalObj.value(QStringLiteral("outBytesTotal")).toInt(0);
            double transferTime;
            if(!m_lastConnectionsUpdate.isNull() && ((transferTime = (DateTime::gmtNow() - m_lastConnectionsUpdate).totalSeconds()) != 0.0)) {
                m_totalIncomingRate = (totalIncomingTraffic - m_totalIncomingTraffic) * 0.008 / transferTime,
                        m_totalOutgoingRate = (totalOutgoingTraffic - m_totalOutgoingTraffic) * 0.008 / transferTime;
            } else {
                m_totalIncomingRate = m_totalOutgoingRate = 0.0;
            }
            emit trafficChanged(m_totalIncomingTraffic = totalIncomingTraffic, m_totalOutgoingTraffic = totalOutgoingTraffic);

            // read connection status
            const QJsonObject connectionsObj(replyObj.value(QStringLiteral("connections")).toObject());
            int index = 0;
            for(SyncthingDev &dev : m_devs) {
                const QJsonObject connectionObj(connectionsObj.value(dev.id).toObject());
                if(!connectionObj.isEmpty()) {
                    switch(dev.status) {
                    case DevStatus::OwnDevice:
                        break;
                    case DevStatus::Disconnected:
                    case DevStatus::Unknown:
                        if(connectionObj.value(QStringLiteral("connected")).toBool(false)) {
                            dev.status = DevStatus::Idle;
                        } else {
                            dev.status = DevStatus::Disconnected;
                        }
                        break;
                    default:
                        if(!connectionObj.value(QStringLiteral("connected")).toBool(false)) {
                            dev.status = DevStatus::Disconnected;
                        }
                    }
                    dev.paused = connectionObj.value(QStringLiteral("paused")).toBool(false);
                    dev.totalIncomingTraffic = connectionObj.value(QStringLiteral("inBytesTotal")).toInt(0);
                    dev.totalOutgoingTraffic = connectionObj.value(QStringLiteral("outBytesTotal")).toInt(0);
                    dev.connectionAddress = connectionObj.value(QStringLiteral("address")).toString();
                    dev.connectionType = connectionObj.value(QStringLiteral("type")).toString();
                    dev.clientVersion = connectionObj.value(QStringLiteral("clientVersion")).toString();
                    emit devStatusChanged(dev, index);
                }
                ++index;
            }

            m_lastConnectionsUpdate = DateTime::gmtNow();

            // since there seems no event for this data, just request every 2 seconds
            if(m_keepPolling) {
                QTimer::singleShot(m_trafficPollInterval, Qt::VeryCoarseTimer, this, &SyncthingConnection::requestConnections);
            }
        } else {
            emit error(tr("Unable to parse connections: ") + jsonError.errorString());
        }
        break;
    } case QNetworkReply::OperationCanceledError:
        return; // intended, not an error
    default:
        emit error(tr("Unable to request connections: ") + reply->errorString());
    }
}

/*!
 * \brief Reads results of requestDirStatistics().
 * \remarks TODO
 */
void SyncthingConnection::readDirStatistics()
{
    auto *reply = static_cast<QNetworkReply *>(sender());
    reply->deleteLater();

    switch(reply->error()) {
    case QNetworkReply::NoError: {
        QJsonParseError jsonError;
        const QJsonDocument replyDoc = QJsonDocument::fromJson(reply->readAll(), &jsonError);
        if(jsonError.error == QJsonParseError::NoError) {
            const QJsonObject replyObj(replyDoc.object());
            int index = 0;
            for(SyncthingDir &dirInfo : m_dirs) {
                const QJsonObject dirObj(replyObj.value(dirInfo.id).toObject());
                if(!dirObj.isEmpty()) {
                    bool mod = false;
                    try {
                        dirInfo.lastScanTime = DateTime::fromIsoStringLocal(dirObj.value(QStringLiteral("lastScan")).toString().toUtf8().data());
                        mod = true;
                    } catch(const ConversionException &) {
                        dirInfo.lastScanTime = DateTime();
                    }
                    const QJsonObject lastFileObj(dirObj.value(QStringLiteral("lastFile")).toObject());
                    if(!lastFileObj.isEmpty()) {
                        dirInfo.lastFileName = lastFileObj.value(QStringLiteral("filename")).toString();
                        mod = true;
                        if(!dirInfo.lastFileName.isEmpty()) {
                            dirInfo.lastFileDeleted = lastFileObj.value(QStringLiteral("deleted")).toBool(false);
                            try {
                                dirInfo.lastFileTime = DateTime::fromIsoStringLocal(lastFileObj.value(QStringLiteral("at")).toString().toUtf8().data());
                                if(dirInfo.lastFileTime > m_lastFileTime) {
                                    m_lastFileTime = dirInfo.lastFileTime,
                                    m_lastFileName = dirInfo.lastFileName,
                                    m_lastFileDeleted = dirInfo.lastFileDeleted;
                                }
                            } catch(const ConversionException &) {
                                dirInfo.lastFileTime = DateTime();
                            }
                        }
                    }
                    if(mod) {
                        emit dirStatusChanged(dirInfo, index);
                    }
                }
                ++index;
            }
        } else {
            emit error(tr("Unable to parse directory statistics: ") + jsonError.errorString());
        }
        break;
    } case QNetworkReply::OperationCanceledError:
        return; // intended, not an error
    default:
        emit error(tr("Unable to request directory statistics: ") + reply->errorString());
    }
}

/*!
 * \brief Reads results of requestDeviceStatistics().
 * \remarks TODO
 */
void SyncthingConnection::readDeviceStatistics()
{
    auto *reply = static_cast<QNetworkReply *>(sender());
    reply->deleteLater();

    switch(reply->error()) {
    case QNetworkReply::NoError: {
        QJsonParseError jsonError;
        const QJsonDocument replyDoc = QJsonDocument::fromJson(reply->readAll(), &jsonError);
        if(jsonError.error == QJsonParseError::NoError) {
            const QJsonObject replyObj(replyDoc.object());
            int index = 0;
            for(SyncthingDev &devInfo : m_devs) {
                const QJsonObject devObj(replyObj.value(devInfo.id).toObject());
                if(!devObj.isEmpty()) {
                    try {
                        devInfo.lastSeen = DateTime::fromIsoStringLocal(devObj.value(QStringLiteral("lastSeen")).toString().toUtf8().data());
                        emit devStatusChanged(devInfo, index);
                    } catch(const ConversionException &) {
                        devInfo.lastSeen = DateTime();
                    }
                }
                ++index;
            }
            // since there seems no event for this data, just request every minute, FIXME: make interval configurable
            if(m_keepPolling) {
                QTimer::singleShot(m_devStatsPollInterval, Qt::VeryCoarseTimer, this, SLOT(requestConnections()));
            }
        } else {
            emit error(tr("Unable to parse device statistics: ") + jsonError.errorString());
        }
        break;
    } case QNetworkReply::OperationCanceledError:
        return; // intended, not an error
    default:
        emit error(tr("Unable to request device statistics: ") + reply->errorString());
    }
}

void SyncthingConnection::readErrors()
{
    auto *reply = static_cast<QNetworkReply *>(sender());
    reply->deleteLater();
    if(reply == m_errorsReply) {
        m_errorsReply = nullptr;
    }

    // ignore any errors occured before connecting
    if(m_lastErrorTime.isNull()) {
        m_lastErrorTime = DateTime::now();
    }

    switch(reply->error()) {
    case QNetworkReply::NoError: {
        QJsonParseError jsonError;
        const QJsonDocument replyDoc = QJsonDocument::fromJson(reply->readAll(), &jsonError);
        if(jsonError.error == QJsonParseError::NoError) {
            for(const QJsonValue &errorVal : replyDoc.object().value(QStringLiteral("errors")).toArray()) {
                const QJsonObject errorObj(errorVal.toObject());
                if(!errorObj.isEmpty()) {
                    try {
                        const DateTime when = DateTime::fromIsoStringLocal(errorObj.value(QStringLiteral("when")).toString().toLocal8Bit().data());
                        if(m_lastErrorTime < when) {
                            emitNotification(m_lastErrorTime = when, errorObj.value(QStringLiteral("message")).toString());
                        }
                    } catch(const ConversionException &) {
                    }
                }
            }
        } else {
            emit error(tr("Unable to parse errors: ") + jsonError.errorString());
        }

        // since there seems no event for this data, just request every thirty seconds, FIXME: make interval configurable
        if(m_keepPolling) {
            QTimer::singleShot(30000, Qt::VeryCoarseTimer, this, SLOT(requestErrors()));
        }
        break;
    } case QNetworkReply::OperationCanceledError:
        return; // intended, not an error
    default:
        emit error(tr("Unable to request errors: ") + reply->errorString());
    }
}

/*!
 * \brief Reads results of requestEvents().
 */
void SyncthingConnection::readEvents()
{
    auto *reply = static_cast<QNetworkReply *>(sender());
    reply->deleteLater();
    if(reply == m_eventsReply) {
        m_eventsReply = nullptr;
    }

    switch(reply->error()) {
    case QNetworkReply::NoError: {
        QJsonParseError jsonError;
        const QJsonDocument replyDoc = QJsonDocument::fromJson(reply->readAll(), &jsonError);
        if(jsonError.error == QJsonParseError::NoError) {
            const QJsonArray replyArray = replyDoc.array();
            emit newEvents(replyArray);
            // search the array for interesting events
            for(const QJsonValue &eventVal : replyArray) {
                const QJsonObject event = eventVal.toObject();
                m_lastEventId = event.value(QStringLiteral("id")).toInt(m_lastEventId);
                DateTime eventTime;
                try {
                    eventTime = DateTime::fromIsoStringGmt(event.value(QStringLiteral("time")).toString().toLocal8Bit().data());
                } catch(const ConversionException &) {
                    // ignore conversion error
                }
                const QString eventType(event.value(QStringLiteral("type")).toString());
                const QJsonObject eventData(event.value(QStringLiteral("data")).toObject());
                if(eventType == QLatin1String("Starting")) {
                    readStartingEvent(eventData);
                } else if(eventType == QLatin1String("StateChanged")) {
                    readStatusChangedEvent(eventTime, eventData);
                } else if(eventType == QLatin1String("DownloadProgress")) {
                    readDownloadProgressEvent(eventTime, eventData);
                } else if(eventType.startsWith(QLatin1String("Folder"))) {
                    readDirEvent(eventTime, eventType, eventData);
                } else if(eventType.startsWith(QLatin1String("Device"))) {
                    readDeviceEvent(eventTime, eventType, eventData);
                } else if(eventType == QLatin1String("ItemStarted")) {
                    readItemStarted(eventTime, eventData);
                } else if(eventType == QLatin1String("ItemFinished")) {
                    readItemFinished(eventTime, eventData);
                }
            }
        } else {
            emit error(tr("Unable to parse Syncthing events: ") + jsonError.errorString());
            setStatus(SyncthingStatus::Disconnected);
            return;
        }
        break;
    } case QNetworkReply::TimeoutError:
        // no new events available, keep polling
        break;
    case QNetworkReply::OperationCanceledError:
        // intended disconnect, not an error
        if(m_reconnecting) {
            // if reconnection flag is set, instantly etstablish a new connection ...
            continueReconnecting();
        } else {
            // ... otherwise keep disconnected
            setStatus(SyncthingStatus::Disconnected);
        }
        return;
    default:
        emit error(tr("Unable to request Syncthing events: ") + reply->errorString());
        setStatus(SyncthingStatus::Disconnected);
        return;
    }

    if(m_keepPolling) {
        requestEvents();
        // TODO: need to change the status somewhere else
        setStatus(SyncthingStatus::Idle);
    } else {
        setStatus(SyncthingStatus::Disconnected);
    }
}

/*!
 * \brief Reads results of requestEvents().
 */
void SyncthingConnection::readStartingEvent(const QJsonObject &eventData)
{
    QString strValue = eventData.value(QStringLiteral("home")).toString();
    if(strValue != m_configDir) {
        emit configDirChanged(m_configDir = strValue);
    }
    strValue = eventData.value(QStringLiteral("myID")).toString();
    if(strValue != m_myId) {
        emit configDirChanged(m_myId = strValue);
    }
}

/*!
 * \brief Reads results of requestEvents().
 */
void SyncthingConnection::readStatusChangedEvent(DateTime eventTime, const QJsonObject &eventData)
{
    const QString dir(eventData.value(QStringLiteral("folder")).toString());
    if(!dir.isEmpty()) {
        // dir status changed
        int index;
        if(SyncthingDir *dirInfo = findDirInfo(dir, index)) {
            if(dirInfo->assignStatus(eventData.value(QStringLiteral("to")).toString(), eventTime)) {
                emit dirStatusChanged(*dirInfo, index);
            }
        }
    }
}

/*!
 * \brief Reads results of requestEvents().
 */
void SyncthingConnection::readDownloadProgressEvent(DateTime eventTime, const QJsonObject &eventData)
{
    VAR_UNUSED(eventTime)
    for(SyncthingDir &dirInfo : m_dirs) {
        // disappearing implies that the download has been finished so just wipe old entries
        dirInfo.downloadingItems.clear();
        dirInfo.blocksAlreadyDownloaded = dirInfo.blocksToBeDownloaded = 0;

        // read progress of currently downloading items
        const QJsonObject dirObj(eventData.value(dirInfo.id).toObject());
        if(!dirObj.isEmpty()) {
            dirInfo.downloadingItems.reserve(static_cast<size_t>(dirObj.size()));
            for(auto filePair = dirObj.constBegin(), end = dirObj.constEnd(); filePair != end; ++filePair) {
                dirInfo.downloadingItems.emplace_back(dirInfo.path, filePair.key(), filePair.value().toObject());
                const SyncthingItemDownloadProgress &itemProgress = dirInfo.downloadingItems.back();
                dirInfo.blocksAlreadyDownloaded += itemProgress.blocksAlreadyDownloaded;
                dirInfo.blocksToBeDownloaded += itemProgress.totalNumberOfBlocks;
            }
        }
        dirInfo.downloadPercentage = (dirInfo.blocksAlreadyDownloaded > 0 && dirInfo.blocksToBeDownloaded > 0)
                ? (static_cast<unsigned int>(dirInfo.blocksAlreadyDownloaded) * 100 / static_cast<unsigned int>(dirInfo.blocksToBeDownloaded))
                : 0;
        dirInfo.downloadLabel = QStringLiteral("%1 / %2 - %3 %").arg(
                    QString::fromLatin1(dataSizeToString(dirInfo.blocksAlreadyDownloaded > 0 ? static_cast<uint64>(dirInfo.blocksAlreadyDownloaded) * SyncthingItemDownloadProgress::syncthingBlockSize : 0).data()),
                    QString::fromLatin1(dataSizeToString(dirInfo.blocksToBeDownloaded > 0 ? static_cast<uint64>(dirInfo.blocksToBeDownloaded) * SyncthingItemDownloadProgress::syncthingBlockSize : 0).data()),
                    QString::number(dirInfo.downloadPercentage));
    }
    emit downloadProgressChanged();
}

/*!
 * \brief Reads results of requestEvents().
 */
void SyncthingConnection::readDirEvent(DateTime eventTime, const QString &eventType, const QJsonObject &eventData)
{
    const QString dir(eventData.value(QStringLiteral("folder")).toString());
    if(!dir.isEmpty()) {
        int index;
        if(SyncthingDir *dirInfo = findDirInfo(dir, index)) {
            if(eventType == QLatin1String("FolderErrors")) {
                // check for errors
                const QJsonArray errors(eventData.value(QStringLiteral("errors")).toArray());
                if(!errors.isEmpty()) {
                    for(const QJsonValue &errorVal : errors) {
                        const QJsonObject error(errorVal.toObject());
                        if(!error.isEmpty()) {
                            dirInfo->errors.emplace_back(error.value(QStringLiteral("error")).toString(), error.value(QStringLiteral("path")).toString());
                            dirInfo->assignStatus(DirStatus::OutOfSync, eventTime);
                            emitNotification(eventTime, dirInfo->errors.back().message);
                        }
                    }
                    emit dirStatusChanged(*dirInfo, index);
                }
            } else if(eventType == QLatin1String("FolderSummary")) {
                // check for summary
                const QJsonObject summary(eventData.value(QStringLiteral("summary")).toObject());
                if(!summary.isEmpty()) {
                    dirInfo->globalBytes = summary.value(QStringLiteral("globalBytes")).toInt();
                    dirInfo->globalDeleted = summary.value(QStringLiteral("globalDeleted")).toInt();
                    dirInfo->globalFiles = summary.value(QStringLiteral("globalFiles")).toInt();
                    dirInfo->localBytes = summary.value(QStringLiteral("localBytes")).toInt();
                    dirInfo->localDeleted = summary.value(QStringLiteral("localDeleted")).toInt();
                    dirInfo->localFiles = summary.value(QStringLiteral("localFiles")).toInt();
                    dirInfo->neededByted = summary.value(QStringLiteral("needByted")).toInt();
                    dirInfo->neededFiles = summary.value(QStringLiteral("needFiles")).toInt();
                    // FIXME: dirInfo->assignStatus(summary.value(QStringLiteral("state")).toString());
                    emit dirStatusChanged(*dirInfo, index);
                }
            } else if(eventType == QLatin1String("FolderCompletion")) {
                // check for progress percentage
                //const QString device(eventData.value(QStringLiteral("device")).toString());
                int percentage = eventData.value(QStringLiteral("completion")).toInt();
                if(percentage > 0 && percentage < 100 && (dirInfo->progressPercentage <= 0 || percentage < dirInfo->progressPercentage)) {
                    // Syncthing provides progress percentage for each device
                    // just show the smallest percentage for now
                    dirInfo->progressPercentage = percentage;
                }
            } else if(eventType == QLatin1String("FolderScanProgress")) {
                // FIXME: for some reason this is always 0
                int current = eventData.value(QStringLiteral("current")).toInt(0),
                    total = eventData.value(QStringLiteral("total")).toInt(0),
                    rate = eventData.value(QStringLiteral("rate")).toInt(0);
                if(current > 0 && total > 0) {
                    dirInfo->progressPercentage = current * 100 / total;
                    dirInfo->progressRate = rate;
                    dirInfo->assignStatus(DirStatus::Scanning, eventTime); // ensure state is scanning
                    emit dirStatusChanged(*dirInfo, index);
                }
            }
        }
    }
}

/*!
 * \brief Reads results of requestEvents().
 */
void SyncthingConnection::readDeviceEvent(DateTime eventTime, const QString &eventType, const QJsonObject &eventData)
{
    if(eventTime.isNull() && m_lastConnectionsUpdate.isNull() && eventTime < m_lastConnectionsUpdate) {
        return; // ignore device events happened before the last connections update
    }
    const QString dev(eventData.value(QStringLiteral("device")).toString());
    if(!dev.isEmpty()) {
        // dev status changed, depending on event type
        int index;
        if(SyncthingDev *devInfo = findDevInfo(dev, index)) {
            DevStatus status = devInfo->status;
            bool paused = devInfo->paused;
            if(eventType == QLatin1String("DeviceConnected")) {
                status = DevStatus::Idle; // TODO: figure out when dev is actually syncing
            } else if(eventType == QLatin1String("DeviceDisconnected")) {
                status = DevStatus::Disconnected;
            } else if(eventType == QLatin1String("DevicePaused")) {
                paused = true;
            } else if(eventType == QLatin1String("DeviceRejected")) {
                status = DevStatus::Rejected;
            } else if(eventType == QLatin1String("DeviceResumed")) {
                paused = false;
                // FIXME: correct to assume device which has just been resumed is still disconnected?
                status = DevStatus::Disconnected;
            } else if(eventType == QLatin1String("DeviceDiscovered")) {
                // we know about this device already, set status anyways because it might still be unknown
                status = DevStatus::Disconnected;
            } else {
                return; // can't handle other event types currently
            }
            if(devInfo->status != status || devInfo->paused != paused) {
                if(devInfo->status != DevStatus::OwnDevice) { // don't mess with the status of the own device
                    devInfo->status = status;
                }
                devInfo->paused = paused;
                emit devStatusChanged(*devInfo, index);
            }
        }
    }
}

/*!
 * \brief Reads results of requestEvents().
 */
void SyncthingConnection::readItemStarted(DateTime eventTime, const QJsonObject &eventData)
{
    VAR_UNUSED(eventTime)
    VAR_UNUSED(eventData)
}

/*!
 * \brief Reads results of requestEvents().
 */
void SyncthingConnection::readItemFinished(DateTime eventTime, const QJsonObject &eventData)
{
    const QString dir(eventData.value(QStringLiteral("folder")).toString());
    if(!dir.isEmpty()) {
        int index;
        if(SyncthingDir *dirInfo = findDirInfo(dir, index)) {
            const QString error(eventData.value(QStringLiteral("error")).toString()),
                          item(eventData.value(QStringLiteral("item")).toString());
            if(error.isEmpty()) {
                if(dirInfo->lastFileTime.isNull() || eventTime < dirInfo->lastFileTime) {
                    dirInfo->lastFileTime = eventTime,
                    dirInfo->lastFileName = item,
                    dirInfo->lastFileDeleted = (eventData.value(QStringLiteral("action")) != QLatin1String("delete"));
                    if(eventTime > m_lastFileTime) {
                        m_lastFileTime = dirInfo->lastFileTime,
                        m_lastFileName = dirInfo->lastFileName,
                        m_lastFileDeleted = dirInfo->lastFileDeleted;
                    }
                    emit dirStatusChanged(*dirInfo, index);
                }
            } else if(dirInfo->status == DirStatus::OutOfSync) {
                // FIXME: find better way to check whether the event is still relevant
                dirInfo->errors.emplace_back(error, item);
                emitNotification(eventTime, error);
            }
        }
    }
}

/*!
 * \brief Reads results of rescan().
 */
void SyncthingConnection::readRescan()
{
    auto *reply = static_cast<QNetworkReply *>(sender());
    reply->deleteLater();
    switch(reply->error()) {
    case QNetworkReply::NoError:
        break;
    default:
        emit error(tr("Unable to request rescan: ") + reply->errorString());
    }
}

/*!
 * \brief Reads results of pause().
 */
void SyncthingConnection::readPauseResume()
{
    auto *reply = static_cast<QNetworkReply *>(sender());
    reply->deleteLater();
    switch(reply->error()) {
    case QNetworkReply::NoError:
        break;
    default:
        emit error(tr("Unable to request pause/resume: ") + reply->errorString());
    }
}

/*!
 * \brief Reads results of restart().
 */
void SyncthingConnection::readRestart()
{
    auto *reply = static_cast<QNetworkReply *>(sender());
    reply->deleteLater();
    switch(reply->error()) {
    case QNetworkReply::NoError:
        break;
    default:
        emit error(tr("Unable to request restart: ") + reply->errorString());
    }
}

/*!
 * \brief Sets the connection status. Ensures statusChanged() is emitted.
 * \param status Specifies the status; should be either SyncthingStatus::Disconnected or SyncthingStatus::Default. There is no use
 *               in specifying other values such as SyncthingStatus::Synchronizing as these are determined automatically within the method.
 */
void SyncthingConnection::setStatus(SyncthingStatus status)
{
    switch(status) {
    case SyncthingStatus::Disconnected:
    case SyncthingStatus::Reconnecting:
        break;
    default:
        if(m_unreadNotifications) {
            status = SyncthingStatus::NotificationsAvailable;
        } else {
            // check whether at least one directory is scanning or synchronizing
            bool scanning = false;
            bool synchronizing = false;
            for(const SyncthingDir &dir : m_dirs) {
                if(dir.status == DirStatus::Synchronizing) {
                    synchronizing = true;
                    break;
                } else if(dir.status == DirStatus::Scanning) {
                    scanning = true;
                }
            }
            if(synchronizing) {
                status = SyncthingStatus::Synchronizing;
            } else if(scanning) {
                status = SyncthingStatus::Scanning;
            } else {
                // check whether at least one device is paused
                bool paused = false;
                for(const SyncthingDev &dev : m_devs) {
                    if(dev.paused) {
                        paused = true;
                        break;
                    }
                }
                if(paused) {
                    status = SyncthingStatus::Paused;
                } else {
                    status = SyncthingStatus::Idle;
                }
            }
        }
    }
    if(m_status != status) {
        emit statusChanged(m_status = status);
    }
}

/*!
 * \brief Interanlly called to emit the notification with the specified \a message.
 * \remarks Ensures the status is updated and the unread notifications flag is set.
 */
void SyncthingConnection::emitNotification(DateTime when, const QString &message)
{
    m_unreadNotifications = true;
    setStatus(status());
    emit newNotification(when, message);
}

}
