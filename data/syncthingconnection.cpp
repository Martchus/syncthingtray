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
 * \brief Assigns the status from the specified status string.
 * \returns Returns whether the status has actually changed.
 */
bool SyncthingDir::assignStatus(const QString &statusStr)
{
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
        status = newStatus;
        return true;
    }
    return false;
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
    m_lastEventId(0),
    m_totalIncomingTraffic(0),
    m_totalOutgoingTraffic(0),
    m_configReply(nullptr),
    m_statusReply(nullptr),
    m_eventsReply(nullptr),
    m_unreadNotifications(false),
    m_hasConfig(false),
    m_hasStatus(false)
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
        m_reconnecting = m_hasConfig = m_hasStatus = false;
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
 */
void SyncthingConnection::reconnect()
{
    if(isConnected()) {
        m_reconnecting = true;
        m_hasConfig = m_hasStatus = false;
        abortAllRequests();
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
    row = 0;
    for(SyncthingDir &d : m_dirs) {
        if(d.id == dir) {
            return &d;
        }
        ++row;
    }
    return nullptr; // TODO: dir is unknown, trigger refreshing the config
}

SyncthingDev *SyncthingConnection::findDevInfo(const QString &dev, int &row)
{
    row = 0;
    for(SyncthingDev &d : m_devs) {
        if(d.id == dev) {
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
 * The signal devStatusChanged() is emitted for each device where the connection status has changed updated; error() is emitted in the error case.
 */
void SyncthingConnection::requestConnections()
{
    QObject::connect(m_connectionsReply = requestData(QStringLiteral("system/connections"), QUrlQuery()), &QNetworkReply::finished, this, &SyncthingConnection::readConnections);
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

/*!
 * \brief Requests the Syncthing log.
 *
 * The specified \a callback is called on success; otherwise error() is emitted.
 */
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
            const QJsonObject replyObj(replyDoc.object());
            emit newConfig(replyObj);
            readDirs(replyObj.value(QStringLiteral("folders")).toArray());
            readDevs(replyObj.value(QStringLiteral("devices")).toArray());
            m_hasConfig = true;
            continueConnecting();
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
 * \brief Reads directory results of requestConfig(); called by readConfig().
 */
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
            m_totalIncomingTraffic = totalObj.value(QStringLiteral("inBytesTotal")).toInt(0);
            m_totalOutgoingTraffic = totalObj.value(QStringLiteral("outBytesTotal")).toInt(0);
            emit trafficChanged(m_totalIncomingTraffic, m_totalOutgoingTraffic);
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
        } else {
            emit error(tr("Unable to parse connections: ") + jsonError.errorString());
        }
    } case QNetworkReply::OperationCanceledError:
        return; // intended, not an error
    default:
        emit error(tr("Unable to request connections: ") + reply->errorString());
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
                const QString eventType(event.value(QStringLiteral("type")).toString());
                const QJsonObject eventData(event.value(QStringLiteral("data")).toObject());
                if(eventType == QLatin1String("Starting")) {
                    readStartingEvent(eventData);
                } else if(eventType == QLatin1String("StateChanged")) {
                    readStatusChangedEvent(eventData);
                } else if(eventType == QLatin1String("DownloadProgress")) {
                    readDownloadProgressEvent(eventData);
                } else if(eventType.startsWith(QLatin1String("Folder"))) {
                    readDirEvent(eventType, eventData);
                } else if(eventType.startsWith(QLatin1String("Device"))) {
                    readDeviceEvent(eventType, eventData);
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
void SyncthingConnection::readStatusChangedEvent(const QJsonObject &eventData)
{
    const QString dir(eventData.value(QStringLiteral("folder")).toString());
    if(!dir.isEmpty()) {
        // dir status changed
        int index;
        if(SyncthingDir *dirInfo = findDirInfo(dir, index)) {
            if(dirInfo->assignStatus(eventData.value(QStringLiteral("to")).toString())) {
                emit dirStatusChanged(*dirInfo, index);
            }
        }
    }
}

/*!
 * \brief Reads results of requestEvents().
 * \remarks TODO
 */
void SyncthingConnection::readDownloadProgressEvent(const QJsonObject &eventData)
{}

void SyncthingConnection::readDirEvent(const QString &eventType, const QJsonObject &eventData)
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
                            emit newNotification(dirInfo->errors.back().message);
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
                    //dirInfo->assignStatus(summary.value(QStringLiteral("state")).toString());
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
            }
        }
    }
}

/*!
 * \brief Reads results of requestEvents().
 */
void SyncthingConnection::readDeviceEvent(const QString &eventType, const QJsonObject &eventData)
{
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
 * \param status Specifies the status; should be either SyncthingStatus::Disconnected or SyncthingStatus::Default. There is no use
 *               in specifying other values such as SyncthingStatus::Synchronizing as these are determined automatically within the method.
 */
void SyncthingConnection::setStatus(SyncthingStatus status)
{
    switch(status) {
    case SyncthingStatus::Disconnected:
        break;
    default:
        if(m_unreadNotifications) {
            status = SyncthingStatus::NotificationsAvailable;
        } else {
            // check whether at least one directory is synchronizing
            bool synchronizing = false;
            for(const SyncthingDir &dir : m_dirs) {
                if(dir.status == DirStatus::Synchronizing) {
                    synchronizing = true;
                    break;
                }
            }
            if(synchronizing) {
                status = SyncthingStatus::Synchronizing;
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
                    status = SyncthingStatus::Default;
                }
            }
        }
    }
    if(m_status != status) {
        emit statusChanged(m_status = status);
    }
}

}
