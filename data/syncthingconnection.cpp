#include "./syncthingconnection.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QPixmap>
#include <QAuthenticator>
#include <QStringBuilder>

#include <utility>

using namespace std;

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
 * \class SyncthingConnection
 * \brief The SyncthingConnection class allows Qt applications to access Syncthing.
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
    m_configReply(nullptr),
    m_statusReply(nullptr),
    m_eventsReply(nullptr),
    m_unreadNotifications(false)
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
    case SyncthingStatus::Default:
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
        m_reconnecting = false;
        requestConfig();
        requestStatus();
        m_lastEventId = 0;
        requestEvents();
        m_keepPolling = true;
    }
}

/*!
 * \brief Disconnects. Does nothing if not connected.
 */
void SyncthingConnection::disconnect()
{
    m_reconnecting = false;
    if(m_configReply) {
        m_configReply->abort();
    }
    if(m_statusReply) {
        m_statusReply->abort();
    }
    if(m_eventsReply) {
        m_eventsReply->abort();
    }
}

/*!
 * \brief Disconnects if connected, then (re-)connects asynchronously.
 */
void SyncthingConnection::reconnect()
{
    if(isConnected()) {
        m_reconnecting = true;
        if(m_configReply) {
            m_configReply->abort();
        }
        if(m_statusReply) {
            m_statusReply->abort();
        }
        if(m_eventsReply) {
            m_eventsReply->abort();
        }
    } else {
        connect();
    }
}

void SyncthingConnection::pause(const QString &dev)
{
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("device"), dev);
    QObject::connect(postData(QStringLiteral("system/pause"), query), &QNetworkReply::finished, this, &SyncthingConnection::readPauseResume);
}

void SyncthingConnection::pauseAllDevs()
{
    for(const SyncthingDev &dev : m_devs) {
        pause(dev.id);
    }
}

void SyncthingConnection::resume(const QString &dev)
{
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("device"), dev);
    QObject::connect(postData(QStringLiteral("system/resume"), query), &QNetworkReply::finished, this, &SyncthingConnection::readPauseResume);
}

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
void SyncthingConnection::rescan(const QString &dir)
{
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("folder"), dir);
    QObject::connect(postData(QStringLiteral("db/scan"), query), &QNetworkReply::finished, this, &SyncthingConnection::readRescan);
}

void SyncthingConnection::rescanAllDirs()
{
    for(const SyncthingDir &dir : m_dirs) {
        rescan(dir.id);
    }
}

void SyncthingConnection::notificationsRead()
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
inline QNetworkReply *SyncthingConnection::requestData(const QString &path, const QUrlQuery &query, bool rest)
{
    return networkAccessManager().get(prepareRequest(path, query, rest));
}

/*!
 * \brief Posts asynchronously data using the rest API.
 */
inline QNetworkReply *SyncthingConnection::postData(const QString &path, const QUrlQuery &query, const QByteArray &data)
{
    return networkAccessManager().post(prepareRequest(path, query), data);
}

SyncthingDir *SyncthingConnection::findDirInfo(const QString &dir, int &row)
{
    for(SyncthingDir &d : m_dirs) {
        if(d.id == dir) {
            return &d;
        }
        ++row;
    }
    return nullptr; // TODO: dir is unknown, trigger refreshing the config
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

void SyncthingConnection::requestStatus()
{
    QObject::connect(m_statusReply = requestData(QStringLiteral("system/status"), QUrlQuery()), &QNetworkReply::finished, this, &SyncthingConnection::readStatus);
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
void SyncthingConnection::requestQrCode(const QString &text, std::function<void(const QPixmap &)> callback)
{
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("text"), text);
    QNetworkReply *reply = requestData(QStringLiteral("/qr/"), query, false);
    QObject::connect(reply, &QNetworkReply::finished, [this, reply, callback] {
        reply->deleteLater();
        QPixmap pixmap;
        switch(reply->error()) {
        case QNetworkReply::NoError:
            pixmap.loadFromData(reply->readAll());
            callback(pixmap);
            break;
        default:
            emit error(tr("Unable to request QR-Code: ") + reply->errorString());
        }
    });
}

void SyncthingConnection::requestLog(std::function<void (const std::vector<SyncthingLogEntry> &)> callback)
{
    QNetworkReply *reply = requestData(QStringLiteral("system/log"), QUrlQuery());
    QObject::connect(reply, &QNetworkReply::finished, [this, reply, callback] {
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
                    SyncthingLogEntry entry;
                    entry.when = logObj.value(QStringLiteral("when")).toString();
                    entry.message = logObj.value(QStringLiteral("message")).toString();
                    logEntries.emplace_back(move(entry));
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
            const QJsonObject replyObj = replyDoc.object();
            emit newConfig(replyObj);
            readDirs(replyObj.value(QStringLiteral("folders")).toArray());
            readDevs(replyObj.value(QStringLiteral("devices")).toArray());
        } else {
            emit error(tr("Unable to parse Syncthing config: ") + jsonError.errorString());
        }
    } case QNetworkReply::OperationCanceledError:
        return; // intended, not an error
    default:
        emit error(tr("Unable to request Syncthing config: ") + reply->errorString());
    }
}

void SyncthingConnection::readDirs(const QJsonArray &dirs)
{
    m_dirs.clear();
    m_dirs.reserve(dirs.size());
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
            dirItem.status = DirStatus::Unknown;
            dirItem.progressPercentage = 0;
            m_dirs.emplace_back(move(dirItem));
        }
    }
    emit newDirs(m_dirs);
}

void SyncthingConnection::readDevs(const QJsonArray &devs)
{
    m_devs.clear();
    m_devs.reserve(devs.size());
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
            devItem.status = DevStatus::Unknown;
            devItem.progressPercentage = 0;
            m_devs.push_back(move(devItem));
        }
    }
    emit newDevices(m_devs);
}

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
            const QJsonObject replyObj = replyDoc.object();
            const QString myId(replyObj.value(QStringLiteral("myID")).toString());
            if(myId != m_myId) {
                emit configDirChanged(m_myId = myId);
            }
            // other values are currently not interesting
        } else {
            emit error(tr("Unable to parse Syncthing config: ") + jsonError.errorString());
        }
    } case QNetworkReply::OperationCanceledError:
        return; // intended, not an error
    default:
        emit error(tr("Unable to request Syncthing config: ") + reply->errorString());
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
                const QString eventType = event.value(QStringLiteral("type")).toString();
                const QJsonObject eventData = event.value(QStringLiteral("data")).toObject();
                if(eventType == QLatin1String("Starting")) {
                    readStartingEvent(eventData);
                } else if(eventType == QLatin1String("StateChanged")) {
                    readStatusChangedEvent(eventData);
                }
            }
        } else {
            emit error(tr("Unable to parse Syncthing events: ") + jsonError.errorString());
            setStatus(SyncthingStatus::Disconnected);
            return;
        }
    } case QNetworkReply::TimeoutError:
        // no new events available, keep polling
        break;
    case QNetworkReply::OperationCanceledError:
        // intended disconnect, not an error
        if(m_reconnecting) {
            // if reconnection flag is set, instantly etstablish a new connection ...
            m_reconnecting = false;
            requestConfig();
            requestStatus();
            m_lastEventId = 0;
            requestEvents();
            m_keepPolling = true;
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
        setStatus(SyncthingStatus::Default);
    } else {
        setStatus(SyncthingStatus::Disconnected);
    }
}

/*!
 * \brief Reads results of requestEvents().
 */
void SyncthingConnection::readStartingEvent(const QJsonObject &event)
{
    QString strValue = event.value(QStringLiteral("home")).toString();
    if(strValue != m_configDir) {
        emit configDirChanged(m_configDir = strValue);
    }
    strValue = event.value(QStringLiteral("myID")).toString();
    if(strValue != m_myId) {
        emit configDirChanged(m_myId = strValue);
    }
}

/*!
 * \brief Reads results of requestEvents().
 */
void SyncthingConnection::readStatusChangedEvent(const QJsonObject &event)
{
    const QString dir(event.value(QStringLiteral("folder")).toString());
    if(!dir.isEmpty()) {
        // dir status changed
        int row;
        if(SyncthingDir *dirInfo = findDirInfo(dir, row)) {
            const QString statusStr(event.value(QStringLiteral("to")).toString());
            DirStatus status;
            if(statusStr == QLatin1String("idle")) {
                status = DirStatus::Idle;
            } else if(statusStr == QLatin1String("scanning")) {
                status = DirStatus::Scanning;
            } else if(statusStr == QLatin1String("syncing")) {
                status = DirStatus::Synchronizing;
            } else if(statusStr == QLatin1String("error")) {
                status = DirStatus::OutOfSync;
            } else {
                status = DirStatus::Unknown;
            }
            if(dirInfo->status != status) {
                dirInfo->status = status;
                emit dirStatusChanged(*dirInfo, row);
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
 * \brief Sets the connection status. Ensures statusChanged() is emitted.
 * \param status Specifies the status; should be either SyncthingStatus::Disconnected or SyncthingStatus::Default.
 * \remarks If \a status is not SyncthingStatus::Disconnected the best status for the current connection is determined automatically.
 */
void SyncthingConnection::setStatus(SyncthingStatus status)
{
    if(m_status != status) {
        switch(m_status = status) {
        case SyncthingStatus::Disconnected:
            break;
        default:
            if(m_unreadNotifications) {
                m_status = SyncthingStatus::NotificationsAvailable;
            } else {
                // check whether at least one directory is synchronizing
                bool synchronizing = false;
                for(const SyncthingDir &dir : m_dirs) {
                    if(dir.status == DirStatus::Synchronizing) {
                        synchronizing = true;
                    }
                }
                if(synchronizing) {
                    m_status = SyncthingStatus::Synchronizing;
                } else {
                    // check whether at least one device is paused
                    bool paused = false;
                    for(const SyncthingDev &dev : m_devs) {
                        if(dev.status == DevStatus::Paused) {
                            paused = true;
                        }
                    }
                    if(paused) {
                        m_status = SyncthingStatus::Paused;
                    } else {
                        m_status = SyncthingStatus::Default;
                    }
                }
            }
        }
        emit statusChanged(m_status);
    }
}

}
