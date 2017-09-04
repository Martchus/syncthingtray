#include "./syncthingconnection.h"
#include "./syncthingconfig.h"
#include "./syncthingconnectionsettings.h"
#include "./utils.h"

#ifdef LIB_SYNCTHING_CONNECTOR_CONNECTION_MOCKED
#include "./syncthingconnectionmockhelpers.h"
#endif

#include <c++utilities/conversion/conversionexception.h>
#include <c++utilities/conversion/stringconversion.h>

#include <QAuthenticator>
#include <QHostAddress>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonValue>
#include <QNetworkAccessManager>
#include <QNetworkInterface>
#include <QNetworkReply>
#include <QStringBuilder>
#include <QTimer>
#include <QUrlQuery>

#include <iostream>
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
    static auto networkAccessManager = new QNetworkAccessManager;
    return *networkAccessManager;
}

/*!
 * \class SyncthingConnection
 * \brief The SyncthingConnection class allows Qt applications to access Syncthing.
 * \remarks All requests are performed asynchronously.
 */

/*!
 * \brief Constructs a new instance ready to connect. To establish the connection, call connect().
 */
SyncthingConnection::SyncthingConnection(const QString &syncthingUrl, const QByteArray &apiKey, QObject *parent)
    : QObject(parent)
    , m_syncthingUrl(syncthingUrl)
    , m_apiKey(apiKey)
    , m_status(SyncthingStatus::Disconnected)
    , m_keepPolling(false)
    , m_reconnecting(false)
    , m_lastEventId(0)
    , m_autoReconnectTries(0)
    , m_totalIncomingTraffic(unknownTraffic)
    , m_totalOutgoingTraffic(unknownTraffic)
    , m_totalIncomingRate(0.0)
    , m_totalOutgoingRate(0.0)
    , m_configReply(nullptr)
    , m_statusReply(nullptr)
    , m_connectionsReply(nullptr)
    , m_errorsReply(nullptr)
    , m_eventsReply(nullptr)
    , m_unreadNotifications(false)
    , m_hasConfig(false)
    , m_hasStatus(false)
    , m_lastFileDeleted(false)
{
    m_trafficPollTimer.setInterval(2000);
    m_trafficPollTimer.setTimerType(Qt::VeryCoarseTimer);
    m_trafficPollTimer.setSingleShot(true);
    QObject::connect(&m_trafficPollTimer, &QTimer::timeout, this, &SyncthingConnection::requestConnections);
    m_devStatsPollTimer.setInterval(60000);
    m_devStatsPollTimer.setTimerType(Qt::VeryCoarseTimer);
    m_devStatsPollTimer.setSingleShot(true);
    QObject::connect(&m_devStatsPollTimer, &QTimer::timeout, this, &SyncthingConnection::requestDeviceStatistics);
    m_errorsPollTimer.setInterval(30000);
    m_errorsPollTimer.setTimerType(Qt::VeryCoarseTimer);
    m_errorsPollTimer.setSingleShot(true);
    QObject::connect(&m_errorsPollTimer, &QTimer::timeout, this, &SyncthingConnection::requestErrors);
    m_autoReconnectTimer.setTimerType(Qt::VeryCoarseTimer);
    QObject::connect(&m_autoReconnectTimer, &QTimer::timeout, this, &SyncthingConnection::autoReconnect);
#ifdef LIB_SYNCTHING_CONNECTOR_CONNECTION_MOCKED
    setupTestData();
#endif
}

/*!
 * \brief Destroys the instance. Ongoing requests are aborted.
 */
SyncthingConnection::~SyncthingConnection()
{
    m_status = SyncthingStatus::BeingDestroyed;
    disconnect();
}

/*!
 * \brief Returns the string representation of the current status().
 */
QString SyncthingConnection::statusText() const
{
    switch (m_status) {
    case SyncthingStatus::Disconnected:
        return tr("disconnected");
    case SyncthingStatus::Reconnecting:
        return tr("reconnecting");
    case SyncthingStatus::Idle:
        return tr("connected");
    case SyncthingStatus::Scanning:
        return tr("connected, scanning");
    case SyncthingStatus::Paused:
        return tr("connected, paused");
    case SyncthingStatus::Synchronizing:
        return tr("connected, synchronizing");
    case SyncthingStatus::OutOfSync:
        return tr("connected, out of sync");
    default:
        return tr("unknown");
    }
}

/*!
 * \brief Returns whether there is at least one directory out-of-sync.
 */
bool SyncthingConnection::hasOutOfSyncDirs() const
{
    for (const SyncthingDir &dir : m_dirs) {
        if (dir.status == SyncthingDirStatus::OutOfSync) {
            return true;
        }
    }
    return false;
}

/*!
 * \brief Connects asynchronously to Syncthing. Does nothing if already connected.
 */
void SyncthingConnection::connect()
{
    m_autoReconnectTimer.stop();
    m_autoReconnectTries = 0;
    if (isConnected()) {
        return;
    }

    m_reconnecting = m_hasConfig = m_hasStatus = false;
    if (m_apiKey.isEmpty() || m_syncthingUrl.isEmpty()) {
        emit error(tr("Connection configuration is insufficient."), SyncthingErrorCategory::OverallConnection, QNetworkReply::NoError);
        return;
    }
    requestConfig();
    requestStatus();
    m_keepPolling = true;
}

/*!
 * \brief Applies the specified configuration and tries to reconnect via reconnect() if properties requiring reconnect
 *        to take effect have changed.
 * \remarks The expected SSL errors of the specified configuration are updated accordingly.
 */
void SyncthingConnection::connect(SyncthingConnectionSettings &connectionSettings)
{
    if (applySettings(connectionSettings) || !isConnected()) {
        reconnect();
    }
}

/*!
 * \brief Disconnects. Does nothing if not connected.
 */
void SyncthingConnection::disconnect()
{
    m_reconnecting = m_hasConfig = m_hasStatus = false;
    m_autoReconnectTries = 0;
    abortAllRequests();
}

/*!
 * \brief Disconnects if connected, then (re-)connects asynchronously.
 * \remarks
 * - Clears the currently cached configuration.
 * - This explicit request to reconnect will reset the autoReconnectTries().
 */
void SyncthingConnection::reconnect()
{
    m_autoReconnectTimer.stop();
    m_autoReconnectTries = 0;
    if (isConnected()) {
        m_reconnecting = true;
        m_hasConfig = m_hasStatus = false;
        abortAllRequests();
    } else {
        continueReconnecting();
    }
}

/*!
 * \brief Applies the specified configuration and tries to reconnect via reconnect().
 * \remarks The expected SSL errors of the specified configuration are updated accordingly.
 */
void SyncthingConnection::reconnect(SyncthingConnectionSettings &connectionSettings)
{
    applySettings(connectionSettings);
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
    m_totalIncomingTraffic = unknownTraffic;
    m_totalOutgoingTraffic = unknownTraffic;
    m_totalIncomingRate = 0.0;
    m_totalOutgoingRate = 0.0;
    emit trafficChanged(unknownTraffic, unknownTraffic);
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
    if (m_apiKey.isEmpty() || m_syncthingUrl.isEmpty()) {
        emit error(tr("Connection configuration is insufficient."), SyncthingErrorCategory::OverallConnection, QNetworkReply::NoError);
        return;
    }
    requestConfig();
    requestStatus();
}

void SyncthingConnection::autoReconnect()
{
    const auto tmp = m_autoReconnectTries;
    connect();
    m_autoReconnectTries = tmp + 1;
}

/*!
 * \brief Requests pausing the devices with the specified IDs.
 *
 * The signal error() is emitted when the request was not successful.
 */
bool SyncthingConnection::pauseDevice(const QStringList &devIds)
{
    return pauseResumeDevice(devIds, true);
}

/*!
 * \brief Requests pausing all devices.
 *
 * The signal error() is emitted when the request was not successful.
 */
bool SyncthingConnection::pauseAllDevs()
{
    return pauseResumeDevice(deviceIds(), true);
}

/*!
 * \brief Requests resuming the devices with the specified IDs.
 *
 * The signal error() is emitted when the request was not successful.
 */
bool SyncthingConnection::resumeDevice(const QStringList &devIds)
{
    return pauseResumeDevice(devIds, false);
}

/*!
 * \brief Requests resuming all devices.
 *
 * The signal error() is emitted when the request was not successful.
 */
bool SyncthingConnection::resumeAllDevs()
{
    return pauseResumeDevice(deviceIds(), false);
}

/*!
 * \brief Pauses the directories with the specified IDs.
 * \remarks Calling this method when not connected results in an error because the *current* Syncthing config must
 *          be available for this call.
 * \returns Returns whether a request has been made.
 * The signal error() is emitted when the request was not successful.
 */
bool SyncthingConnection::pauseDirectories(const QStringList &dirIds)
{
    return pauseResumeDirectory(dirIds, true);
}

/*!
 * \brief Pauses all directories.
 * \remarks Calling this method when not connected results in an error because the *current* Syncthing config must
 *          be available for this call.
 *
 * The signal error() is emitted when the request was not successful.
 */
bool SyncthingConnection::pauseAllDirs()
{
    return pauseResumeDirectory(directoryIds(), true);
}

/*!
 * \brief Resumes the directories with the specified IDs.
 * \remarks Calling this method when not connected results in an error because the *current* Syncthing config must
 *          be available for this call.
 *
 * The signal error() is emitted when the request was not successful.
 */
bool SyncthingConnection::resumeDirectories(const QStringList &dirIds)
{
    return pauseResumeDirectory(dirIds, false);
}

/*!
 * \brief Resumes all directories.
 * \remarks Calling this method when not connected results in an error because the *current* Syncthing config must
 *          be available for this call.
 *
 * The signal error() is emitted when the request was not successful.
 */
bool SyncthingConnection::resumeAllDirs()
{
    return pauseResumeDirectory(directoryIds(), false);
}

/*!
 * \brief Requests rescanning the directory with the specified ID.
 *
 * The signal error() is emitted when the request was not successful.
 */
void SyncthingConnection::rescan(const QString &dirId, const QString &relpath)
{
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("folder"), dirId);
    if (!relpath.isEmpty()) {
        query.addQueryItem(QStringLiteral("sub"), relpath);
    }
    QNetworkReply *reply = postData(QStringLiteral("db/scan"), query);
    reply->setProperty("dirId", dirId);
    QObject::connect(reply, &QNetworkReply::finished, this, &SyncthingConnection::readRescan);
}

/*!
 * \brief Requests rescanning all directories.
 *
 * Note that rescan is only requested for unpaused directories because requesting rescan for
 * paused directories only leads to an error.
 *
 * The signal error() is emitted when the request was not successful.
 */
void SyncthingConnection::rescanAllDirs()
{
    for (const SyncthingDir &dir : m_dirs) {
        if (!dir.paused) {
            rescan(dir.id);
        }
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
 * \brief Requests Syncthing to exit and not restart.
 *
 * The signal error() is emitted when the request was not successful.
 */
void SyncthingConnection::shutdown()
{
    QObject::connect(postData(QStringLiteral("system/shutdown"), QUrlQuery()), &QNetworkReply::finished, this, &SyncthingConnection::readShutdown);
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
    request.setHeader(QNetworkRequest::ContentTypeHeader, QByteArray("application/x-www-form-urlencoded"));
    request.setRawHeader("X-API-Key", m_apiKey);
    return request;
}

/*!
 * \brief Requests asynchronously data using the rest API.
 */
QNetworkReply *SyncthingConnection::requestData(const QString &path, const QUrlQuery &query, bool rest)
{
#ifndef LIB_SYNCTHING_CONNECTOR_CONNECTION_MOCKED
    auto *reply = networkAccessManager().get(prepareRequest(path, query, rest));
    reply->ignoreSslErrors(m_expectedSslErrors);
    return reply;
#else
    return MockedReply::forRequest(QStringLiteral("GET"), path, query, rest);
#endif
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
 * \brief Internally used to pause/resume directories.
 * \returns Returns whether a request has been made.
 * \remarks This might currently result in errors caused by Syncthing not
 *          handling E notation correctly when using Qt < 5.9:
 *          https://github.com/syncthing/syncthing/issues/4001
 */
bool SyncthingConnection::pauseResumeDevice(const QStringList &devIds, bool paused)
{
    if (devIds.isEmpty()) {
        return false;
    }
    if (!isConnected()) {
        emit error(tr("Unable to pause/resume a devices when not connected"), SyncthingErrorCategory::SpecificRequest, QNetworkReply::NoError);
        return false;
    }

    QJsonObject config = m_rawConfig;
    if (!setDevicesPaused(config, devIds, paused)) {
        return false;
    }

    QJsonDocument doc;
    doc.setObject(config);
    QNetworkReply *reply = postData(QStringLiteral("system/config"), QUrlQuery(), doc.toJson(QJsonDocument::Compact));
    reply->setProperty("devIds", devIds);
    reply->setProperty("resume", !paused);
    QObject::connect(reply, &QNetworkReply::finished, this, &SyncthingConnection::readDevPauseResume);
    return true;
}

/*!
 * \brief Internally used to pause/resume directories.
 * \returns Returns whether a request has been made.
 * \remarks This might currently result in errors caused by Syncthing not
 *          handling E notation correctly when using Qt < 5.9:
 *          https://github.com/syncthing/syncthing/issues/4001
 */
bool SyncthingConnection::pauseResumeDirectory(const QStringList &dirIds, bool paused)
{
    if (dirIds.isEmpty()) {
        return false;
    }
    if (!isConnected()) {
        emit error(tr("Unable to pause/resume a directories when not connected"), SyncthingErrorCategory::SpecificRequest, QNetworkReply::NoError);
        return false;
    }

    QJsonObject config = m_rawConfig;
    if (setDirectoriesPaused(config, dirIds, paused)) {
        QJsonDocument doc;
        doc.setObject(config);
        QNetworkReply *reply = postData(QStringLiteral("system/config"), QUrlQuery(), doc.toJson(QJsonDocument::Compact));
        reply->setProperty("dirIds", dirIds);
        reply->setProperty("resume", !paused);
        QObject::connect(reply, &QNetworkReply::finished, this, &SyncthingConnection::readDirPauseResume);
        return true;
    }
    return false;
}

/*!
 * \brief Returns the directory info object for the directory with the specified ID.
 * \returns Returns a pointer to the object or nullptr if not found.
 * \remarks The returned object becomes invalid when the newDirs() signal is emitted or the connection is destroyed.
 */
SyncthingDir *SyncthingConnection::findDirInfo(const QString &dirId, int &row)
{
    row = 0;
    for (SyncthingDir &d : m_dirs) {
        if (d.id == dirId) {
            return &d;
        }
        ++row;
    }
    return nullptr; // TODO: dir is unknown, trigger refreshing the config
}

/*!
 * \brief Appends a directory info object with the specified \a dirId to \a dirs.
 *
 * If such an object already exists, it is recycled by moving it to \a dirs.
 * Otherwise a new, empty object is created.
 *
 * \returns Returns the directory info object or nullptr if \a dirId is invalid.
 */
SyncthingDir *SyncthingConnection::addDirInfo(std::vector<SyncthingDir> &dirs, const QString &dirId)
{
    if (dirId.isEmpty()) {
        return nullptr;
    }
    int row;
    if (SyncthingDir *existingDirInfo = findDirInfo(dirId, row)) {
        dirs.emplace_back(move(*existingDirInfo));
    } else {
        dirs.emplace_back(dirId);
    }
    return &dirs.back();
}

/*!
 * \brief Returns the device info object for the device with the specified ID.
 * \returns Returns a pointer to the object or nullptr if not found.
 * \remarks The returned object becomes invalid when the newConfig() signal is emitted or the connection is destroyed.
 */
SyncthingDev *SyncthingConnection::findDevInfo(const QString &devId, int &row)
{
    row = 0;
    for (SyncthingDev &d : m_devs) {
        if (d.id == devId) {
            return &d;
        }
        ++row;
    }
    return nullptr; // TODO: dev is unknown, trigger refreshing the config
}

/*!
 * \brief Returns the device info object for the first device with the specified name.
 * \returns Returns a pointer to the object or nullptr if not found.
 * \remarks The returned object becomes invalid when the newConfig() signal is emitted or the connection is destroyed.
 */
SyncthingDev *SyncthingConnection::findDevInfoByName(const QString &devName, int &row)
{
    row = 0;
    for (SyncthingDev &d : m_devs) {
        if (d.name == devName) {
            return &d;
        }
        ++row;
    }
    return nullptr;
}

QStringList SyncthingConnection::directoryIds() const
{
    return ids(m_dirs);
}

QStringList SyncthingConnection::deviceIds() const
{
    return ids(m_devs);
}

/*!
 * \brief Appends a device info object with the specified \a devId to \a devs.
 *
 * If such an object already exists, it is recycled by moving it to \a devs.
 * Otherwise a new, empty object is created.
 *
 * \returns Returns the device info object or nullptr if \a devId is invalid.
 */
SyncthingDev *SyncthingConnection::addDevInfo(std::vector<SyncthingDev> &devs, const QString &devId)
{
    if (devId.isEmpty()) {
        return nullptr;
    }
    int row;
    if (SyncthingDev *existingDevInfo = findDevInfo(devId, row)) {
        devs.emplace_back(move(*existingDevInfo));
    } else {
        devs.emplace_back(devId);
    }
    return &devs.back();
}

/*!
 * \brief Continues connecting if both - config and status - have been parsed yet and continuous polling is enabled.
 */
void SyncthingConnection::continueConnecting()
{
    if (m_keepPolling && m_hasConfig && m_hasStatus) {
        requestConnections();
        requestDirStatistics();
        requestDeviceStatistics();
        requestErrors();
        for (const SyncthingDir &dir : m_dirs) {
            requestDirStatus(dir.id);
        }
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
    if (m_configReply) {
        m_configReply->abort();
    }
    if (m_statusReply) {
        m_statusReply->abort();
    }
    if (m_connectionsReply) {
        m_connectionsReply->abort();
    }
    if (m_errorsReply) {
        m_errorsReply->abort();
    }
    if (m_eventsReply) {
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
    QObject::connect(
        m_configReply = requestData(QStringLiteral("system/config"), QUrlQuery()), &QNetworkReply::finished, this, &SyncthingConnection::readConfig);
}

/*!
 * \brief Requests the Syncthing status asynchronously.
 *
 * The signal configDirChanged() and myIdChanged() emitted when those values have changed; error() is emitted in the error case.
 */
void SyncthingConnection::requestStatus()
{
    QObject::connect(
        m_statusReply = requestData(QStringLiteral("system/status"), QUrlQuery()), &QNetworkReply::finished, this, &SyncthingConnection::readStatus);
}

/*!
 * \brief Requests current connections asynchronously.
 *
 * The signal devStatusChanged() is emitted for each device where the connection status has changed; error() is emitted in the error case.
 */
void SyncthingConnection::requestConnections()
{
    QObject::connect(m_connectionsReply = requestData(QStringLiteral("system/connections"), QUrlQuery()), &QNetworkReply::finished, this,
        &SyncthingConnection::readConnections);
}

/*!
 * \brief Requests errors asynchronously.
 *
 * The signal newNotification() is emitted on success; error() is emitted in the error case.
 */
void SyncthingConnection::requestErrors()
{
    QObject::connect(
        m_errorsReply = requestData(QStringLiteral("system/error"), QUrlQuery()), &QNetworkReply::finished, this, &SyncthingConnection::readErrors);
}

/*!
 * \brief Requests clearing errors asynchronously.
 *
 * The signal error() is emitted in the error case.
 */
void SyncthingConnection::requestClearingErrors()
{
    QObject::connect(
        postData(QStringLiteral("system/error/clear"), QUrlQuery()), &QNetworkReply::finished, this, &SyncthingConnection::readClearingErrors);
}

/*!
 * \brief Requests statistics (last file, last scan) for all directories asynchronously.
 */
void SyncthingConnection::requestDirStatistics()
{
    QObject::connect(
        requestData(QStringLiteral("stats/folder"), QUrlQuery()), &QNetworkReply::finished, this, &SyncthingConnection::readDirStatistics);
}

/*!
 * \brief Requests statistics (global and local status) for \a dirId asynchronously.
 */
void SyncthingConnection::requestDirStatus(const QString &dirId)
{
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("folder"), dirId);
    QNetworkReply *reply = requestData(QStringLiteral("db/status"), query);
    reply->setProperty("dirId", dirId);
    QObject::connect(reply, &QNetworkReply::finished, this, &SyncthingConnection::readDirStatus);
}

/*!
 * \brief Requests device statistics asynchronously.
 */
void SyncthingConnection::requestDeviceStatistics()
{
    QObject::connect(
        requestData(QStringLiteral("stats/device"), QUrlQuery()), &QNetworkReply::finished, this, &SyncthingConnection::readDeviceStatistics);
}

/*!
 * \brief Requests the Syncthing events (since the last successful call) asynchronously.
 *
 * The signal newEvents() is emitted on success; otherwise error() is emitted.
 */
void SyncthingConnection::requestEvents()
{
    QUrlQuery query;
    if (m_lastEventId) {
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
        switch (reply->error()) {
        case QNetworkReply::NoError:
            callback(reply->readAll());
            break;
        default:
            emit error(tr("Unable to request QR-Code: ") + reply->errorString(), SyncthingErrorCategory::SpecificRequest, reply->error());
        }
    });
}

/*!
 * \brief Requests the Syncthing log.
 *
 * The specified \a callback is called on success; otherwise error() is emitted.
 */
QMetaObject::Connection SyncthingConnection::requestLog(std::function<void(const std::vector<SyncthingLogEntry> &)> callback)
{
    QNetworkReply *reply = requestData(QStringLiteral("system/log"), QUrlQuery());
    return QObject::connect(reply, &QNetworkReply::finished, [this, reply, callback] {
        reply->deleteLater();
        switch (reply->error()) {
        case QNetworkReply::NoError: {
            QJsonParseError jsonError;
            const QJsonDocument replyDoc = QJsonDocument::fromJson(reply->readAll(), &jsonError);
            if (jsonError.error != QJsonParseError::NoError) {
                emit error(tr("Unable to parse Syncthing log: ") + jsonError.errorString(), SyncthingErrorCategory::Parsing, QNetworkReply::NoError);
                return;
            }

            const QJsonArray log(replyDoc.object().value(QStringLiteral("messages")).toArray());
            vector<SyncthingLogEntry> logEntries;
            logEntries.reserve(static_cast<size_t>(log.size()));
            for (const QJsonValue &logVal : log) {
                const QJsonObject logObj(logVal.toObject());
                logEntries.emplace_back(logObj.value(QStringLiteral("when")).toString(), logObj.value(QStringLiteral("message")).toString());
            }
            callback(logEntries);
            break;
        }
        default:
            emit error(tr("Unable to request Syncthing log: ") + reply->errorString(), SyncthingErrorCategory::SpecificRequest, reply->error());
        }
    });
}

/*!
 * \brief Locates and loads the (self-signed) certificate used by the Syncthing GUI.
 * \remarks
 *  - Ensures any previous certificates are cleared in any case.
 *  - Emits error() when an error occurs.
 *  - Loading the certificate is only possible if the connection object is configured
 *    to connect to the locally running Syncthing instance. Otherwise this method will
 *    only do the cleanup of previous certificates but not emit any errors.
 * \returns Returns whether a certificate could be loaded.
 */
bool SyncthingConnection::loadSelfSignedCertificate()
{
    // ensure current exceptions for self-signed certificates are cleared
    m_expectedSslErrors.clear();

    // not required when not using secure connection
    const QUrl syncthingUrl(m_syncthingUrl);
    if (!syncthingUrl.scheme().endsWith(QChar('s'))) {
        return false;
    }

    // only possible if the Syncthing instance is running on the local machine
    if (!isLocal(syncthingUrl)) {
        return false;
    }

    // find cert
    const QString certPath = !m_configDir.isEmpty() ? (m_configDir + QStringLiteral("/https-cert.pem")) : SyncthingConfig::locateHttpsCertificate();
    if (certPath.isEmpty()) {
        emit error(tr("Unable to locate certificate used by Syncthing."), SyncthingErrorCategory::OverallConnection, QNetworkReply::NoError);
        return false;
    }
    // add exception
    const QList<QSslCertificate> certs = QSslCertificate::fromPath(certPath);
    if (certs.isEmpty()) {
        emit error(tr("Unable to load certificate used by Syncthing."), SyncthingErrorCategory::OverallConnection, QNetworkReply::NoError);
        return false;
    }
    const QSslCertificate &cert = certs.at(0);
    m_expectedSslErrors.reserve(4);
    m_expectedSslErrors << QSslError(QSslError::UnableToGetLocalIssuerCertificate, cert) << QSslError(QSslError::UnableToVerifyFirstCertificate, cert)
                        << QSslError(QSslError::SelfSignedCertificate, cert) << QSslError(QSslError::HostNameMismatch, cert);
    return true;
}

/*!
 * \brief Applies the specified configuration.
 * \remarks
 * - The expected SSL errors of the specified configuration are updated accordingly.
 * - The configuration is not used instantly. It will be used on the next reconnect.
 * \returns Returns whether at least one property requiring a reconnect to take effect has changed.
 * \sa reconnect()
 */
bool SyncthingConnection::applySettings(SyncthingConnectionSettings &connectionSettings)
{
    bool reconnectRequired = false;
    if (syncthingUrl() != connectionSettings.syncthingUrl) {
        setSyncthingUrl(connectionSettings.syncthingUrl);
        reconnectRequired = true;
    }
    if (apiKey() != connectionSettings.apiKey) {
        setApiKey(connectionSettings.apiKey);
        reconnectRequired = true;
    }
    if ((connectionSettings.authEnabled && (user() != connectionSettings.userName || password() != connectionSettings.password))
        || (!connectionSettings.authEnabled && (!user().isEmpty() || !password().isEmpty()))) {
        if (connectionSettings.authEnabled) {
            setCredentials(connectionSettings.userName, connectionSettings.password);
        } else {
            setCredentials(QString(), QString());
        }
        reconnectRequired = true;
    }
    if (connectionSettings.expectedSslErrors.isEmpty()) {
        const bool previouslyHadExpectedSslErrors = !expectedSslErrors().isEmpty();
        const bool ok = loadSelfSignedCertificate();
        connectionSettings.expectedSslErrors = expectedSslErrors();
        if (ok || (previouslyHadExpectedSslErrors && !ok)) {
            reconnectRequired = true;
        }
    } else if (expectedSslErrors() != connectionSettings.expectedSslErrors) {
        m_expectedSslErrors = connectionSettings.expectedSslErrors;
        reconnectRequired = true;
    }

    setTrafficPollInterval(connectionSettings.trafficPollInterval);
    setDevStatsPollInterval(connectionSettings.devStatsPollInterval);
    setErrorsPollInterval(connectionSettings.errorsPollInterval);
    setAutoReconnectInterval(connectionSettings.reconnectInterval);

    return reconnectRequired;
}

/*!
 * \brief Reads results of requestConfig().
 */
void SyncthingConnection::readConfig()
{
    auto *const reply = static_cast<QNetworkReply *>(sender());
    reply->deleteLater();
    if (reply == m_configReply) {
        m_configReply = nullptr;
    }

    switch (reply->error()) {
    case QNetworkReply::NoError: {
        const QByteArray response(reply->readAll());
        QJsonParseError jsonError;
        const QJsonDocument replyDoc = QJsonDocument::fromJson(response, &jsonError);
        if (jsonError.error != QJsonParseError::NoError) {
            emitError(tr("Unable to parse Syncthing config: "), jsonError, reply, response);
            handleFatalConnectionError();
            return;
        }

        m_rawConfig = replyDoc.object();
        emit newConfig(m_rawConfig);
        m_hasConfig = true;
        concludeReadingConfigAndStatus();
        break;
    }
    case QNetworkReply::OperationCanceledError:
        return; // intended, not an error
    default:
        emitError(tr("Unable to request Syncthing config: "), SyncthingErrorCategory::OverallConnection, reply);
        handleFatalConnectionError();
    }
}

/*!
 * \brief Reads directory results of requestConfig(); called by readConfig().
 * \remarks
 * - The devs are required to resolve the names of the devices a directory is shared with.
 *   So when parsing the config, readDevs() should be called first.
 * - The own device ID is required to filter it from the devices a directory is shared with.
 *   So the readStatus() should have been called first.
 */
void SyncthingConnection::readDirs(const QJsonArray &dirs)
{
    std::vector<SyncthingDir> newDirs;
    newDirs.reserve(static_cast<size_t>(dirs.size()));
    int dummy;
    for (const QJsonValue &dirVal : dirs) {
        const QJsonObject dirObj(dirVal.toObject());
        SyncthingDir *const dirItem = addDirInfo(newDirs, dirObj.value(QStringLiteral("id")).toString());
        if (!dirItem) {
            continue;
        }

        dirItem->label = dirObj.value(QStringLiteral("label")).toString();
        dirItem->path = dirObj.value(QStringLiteral("path")).toString();
        dirItem->deviceIds.clear();
        dirItem->deviceNames.clear();
        for (const QJsonValue &dev : dirObj.value(QStringLiteral("devices")).toArray()) {
            const QString devId = dev.toObject().value(QStringLiteral("deviceID")).toString();
            if (!devId.isEmpty() && devId != m_myId) {
                dirItem->deviceIds << devId;
                if (const SyncthingDev *const dev = findDevInfo(devId, dummy)) {
                    dirItem->deviceNames << dev->name;
                }
            }
        }
        dirItem->readOnly = dirObj.value(QStringLiteral("readOnly")).toBool(false);
        dirItem->rescanInterval = dirObj.value(QStringLiteral("rescanIntervalS")).toInt(-1);
        dirItem->ignorePermissions = dirObj.value(QStringLiteral("ignorePerms")).toBool(false);
        dirItem->autoNormalize = dirObj.value(QStringLiteral("autoNormalize")).toBool(false);
        dirItem->minDiskFreePercentage = dirObj.value(QStringLiteral("minDiskFreePct")).toInt(-1);
        dirItem->paused = dirObj.value(QStringLiteral("paused")).toBool(dirItem->paused);
    }
    m_dirs.swap(newDirs);
    m_syncedDirs.reserve(m_dirs.size());
    emit this->newDirs(m_dirs);
}

/*!
 * \brief Reads device results of requestConfig(); called by readConfig().
 */
void SyncthingConnection::readDevs(const QJsonArray &devs)
{
    vector<SyncthingDev> newDevs;
    newDevs.reserve(static_cast<size_t>(devs.size()));
    for (const QJsonValue &devVal : devs) {
        const QJsonObject devObj(devVal.toObject());
        SyncthingDev *const devItem = addDevInfo(newDevs, devObj.value(QStringLiteral("deviceID")).toString());
        if (!devItem) {
            continue;
        }

        devItem->name = devObj.value(QStringLiteral("name")).toString();
        devItem->addresses.clear();
        for (const QJsonValue &addrVal : devObj.value(QStringLiteral("addresses")).toArray()) {
            devItem->addresses << addrVal.toString();
        }
        devItem->compression = devObj.value(QStringLiteral("compression")).toString();
        devItem->certName = devObj.value(QStringLiteral("certName")).toString();
        devItem->introducer = devObj.value(QStringLiteral("introducer")).toBool(false);
        devItem->status = devItem->id == m_myId ? SyncthingDevStatus::OwnDevice : SyncthingDevStatus::Unknown;
        devItem->paused = devObj.value(QStringLiteral("paused")).toBool(devItem->paused);
    }
    m_devs.swap(newDevs);
    emit this->newDevices(m_devs);
}

/*!
 * \brief Reads results of requestStatus().
 */
void SyncthingConnection::readStatus()
{
    auto *const reply = static_cast<QNetworkReply *>(sender());
    reply->deleteLater();
    if (reply == m_statusReply) {
        m_statusReply = nullptr;
    }

    switch (reply->error()) {
    case QNetworkReply::NoError: {
        const QByteArray response(reply->readAll());
        QJsonParseError jsonError;
        const QJsonDocument replyDoc = QJsonDocument::fromJson(response, &jsonError);
        if (jsonError.error != QJsonParseError::NoError) {
            emitError(tr("Unable to parse Syncthing status: "), jsonError, reply, response);
            handleFatalConnectionError();
            return;
        }

        emitMyIdChanged(replyDoc.object().value(QStringLiteral("myID")).toString());
        // other values are currently not interesting
        m_hasStatus = true;
        concludeReadingConfigAndStatus();
        break;
    }
    case QNetworkReply::OperationCanceledError:
        return; // intended, not an error
    default:
        emitError(tr("Unable to request Syncthing status: "), SyncthingErrorCategory::OverallConnection, reply);
        handleFatalConnectionError();
    }
}

/*!
 * \brief Reads results of requestConfig() and requestStatus().
 * \remarks Called in readConfig() or readStatus() to conclude reading parts requiring config *and* status
 *          being available. Does nothing if this is not the case (yet).
 */
void SyncthingConnection::concludeReadingConfigAndStatus()
{
    if (!m_hasConfig || !m_hasStatus) {
        return;
    }

    readDevs(m_rawConfig.value(QStringLiteral("devices")).toArray());
    readDirs(m_rawConfig.value(QStringLiteral("folders")).toArray());

    if (!isConnected()) {
        continueConnecting();
    }
}

/*!
 * \brief Reads results of requestConnections().
 */
void SyncthingConnection::readConnections()
{
    auto *const reply = static_cast<QNetworkReply *>(sender());
    reply->deleteLater();
    if (reply == m_connectionsReply) {
        m_connectionsReply = nullptr;
    }

    switch (reply->error()) {
    case QNetworkReply::NoError: {
        const QByteArray response(reply->readAll());
        QJsonParseError jsonError;
        const QJsonDocument replyDoc = QJsonDocument::fromJson(response, &jsonError);
        if (jsonError.error != QJsonParseError::NoError) {
            emitError(tr("Unable to parse connections: "), jsonError, reply, response);
            return;
        }

        const QJsonObject replyObj(replyDoc.object());
        const QJsonObject totalObj(replyObj.value(QStringLiteral("total")).toObject());

        // read traffic, the conversion to double is neccassary because toInt() doesn't work for high values
        const QJsonValue totalIncomingTrafficValue(totalObj.value(QStringLiteral("inBytesTotal")));
        const QJsonValue totalOutgoingTrafficValue(totalObj.value(QStringLiteral("outBytesTotal")));
        const uint64 totalIncomingTraffic
            = totalIncomingTrafficValue.isDouble() ? static_cast<uint64>(totalIncomingTrafficValue.toDouble(0.0)) : unknownTraffic;
        const uint64 totalOutgoingTraffic
            = totalOutgoingTrafficValue.isDouble() ? static_cast<uint64>(totalOutgoingTrafficValue.toDouble(0.0)) : unknownTraffic;
        double transferTime;
        const bool hasDelta
            = !m_lastConnectionsUpdate.isNull() && ((transferTime = (DateTime::gmtNow() - m_lastConnectionsUpdate).totalSeconds()) != 0.0);
        m_totalIncomingRate = (hasDelta && totalIncomingTraffic != unknownTraffic && m_totalIncomingTraffic != unknownTraffic)
            ? (totalIncomingTraffic - m_totalIncomingTraffic) * 0.008 / transferTime
            : 0.0;
        m_totalOutgoingRate = (hasDelta && totalOutgoingTraffic != unknownTraffic && m_totalOutgoingRate != unknownTraffic)
            ? (totalOutgoingTraffic - m_totalOutgoingTraffic) * 0.008 / transferTime
            : 0.0;
        emit trafficChanged(m_totalIncomingTraffic = totalIncomingTraffic, m_totalOutgoingTraffic = totalOutgoingTraffic);

        // read connection status
        const QJsonObject connectionsObj(replyObj.value(QStringLiteral("connections")).toObject());
        int index = 0;
        for (SyncthingDev &dev : m_devs) {
            const QJsonObject connectionObj(connectionsObj.value(dev.id).toObject());
            if (connectionObj.isEmpty()) {
                ++index;
                continue;
            }

            switch (dev.status) {
            case SyncthingDevStatus::OwnDevice:
                break;
            case SyncthingDevStatus::Disconnected:
            case SyncthingDevStatus::Unknown:
                if (connectionObj.value(QStringLiteral("connected")).toBool(false)) {
                    dev.status = SyncthingDevStatus::Idle;
                } else {
                    dev.status = SyncthingDevStatus::Disconnected;
                }
                break;
            default:
                if (!connectionObj.value(QStringLiteral("connected")).toBool(false)) {
                    dev.status = SyncthingDevStatus::Disconnected;
                }
            }
            dev.paused = connectionObj.value(QStringLiteral("paused")).toBool(false);
            dev.totalIncomingTraffic = static_cast<uint64>(connectionObj.value(QStringLiteral("inBytesTotal")).toDouble(0));
            dev.totalOutgoingTraffic = static_cast<uint64>(connectionObj.value(QStringLiteral("outBytesTotal")).toDouble(0));
            dev.connectionAddress = connectionObj.value(QStringLiteral("address")).toString();
            dev.connectionType = connectionObj.value(QStringLiteral("type")).toString();
            dev.clientVersion = connectionObj.value(QStringLiteral("clientVersion")).toString();
            emit devStatusChanged(dev, index);
            ++index;
        }

        m_lastConnectionsUpdate = DateTime::gmtNow();

        // since there seems no event for this data, keep polling
        if (m_keepPolling && m_trafficPollTimer.interval()) {
            m_trafficPollTimer.start();
        }

        break;
    }
    case QNetworkReply::OperationCanceledError:
        return; // intended, not an error
    default:
        emitError(tr("Unable to request connections: "), SyncthingErrorCategory::OverallConnection, reply);
    }
}

/*!
 * \brief Reads results of requestDirStatistics().
 */
void SyncthingConnection::readDirStatistics()
{
    auto *const reply = static_cast<QNetworkReply *>(sender());
    reply->deleteLater();

    switch (reply->error()) {
    case QNetworkReply::NoError: {
        const QByteArray response(reply->readAll());
        QJsonParseError jsonError;
        const QJsonDocument replyDoc = QJsonDocument::fromJson(response, &jsonError);
        if (jsonError.error != QJsonParseError::NoError) {
            emitError(tr("Unable to parse directory statistics: "), jsonError, reply, response);
            return;
        }

        const QJsonObject replyObj(replyDoc.object());
        int index = 0;
        for (SyncthingDir &dirInfo : m_dirs) {
            const QJsonObject dirObj(replyObj.value(dirInfo.id).toObject());
            if (dirObj.isEmpty()) {
                ++index;
                continue;
            }

            bool dirModified = false;
            try {
                dirInfo.lastScanTime = DateTime::fromIsoStringLocal(dirObj.value(QStringLiteral("lastScan")).toString().toUtf8().data());
                dirModified = true;
            } catch (const ConversionException &) {
                dirInfo.lastScanTime = DateTime();
            }
            const QJsonObject lastFileObj(dirObj.value(QStringLiteral("lastFile")).toObject());
            if (!lastFileObj.isEmpty()) {
                dirInfo.lastFileName = lastFileObj.value(QStringLiteral("filename")).toString();
                dirModified = true;
                if (!dirInfo.lastFileName.isEmpty()) {
                    dirInfo.lastFileDeleted = lastFileObj.value(QStringLiteral("deleted")).toBool(false);
                    try {
                        dirInfo.lastFileTime = DateTime::fromIsoStringLocal(lastFileObj.value(QStringLiteral("at")).toString().toUtf8().data());
                        if (dirInfo.lastFileTime > m_lastFileTime) {
                            m_lastFileTime = dirInfo.lastFileTime, m_lastFileName = dirInfo.lastFileName, m_lastFileDeleted = dirInfo.lastFileDeleted;
                        }
                    } catch (const ConversionException &) {
                        dirInfo.lastFileTime = DateTime();
                    }
                }
            }
            if (dirModified) {
                emit dirStatusChanged(dirInfo, index);
            }
            ++index;
        }
        break;
    }
    case QNetworkReply::OperationCanceledError:
        return; // intended, not an error
    default:
        emitError(tr("Unable to request directory statistics: "), SyncthingErrorCategory::OverallConnection, reply);
    }
}

/*!
 * \brief Reads results of requestDeviceStatistics().
 */
void SyncthingConnection::readDeviceStatistics()
{
    auto *const reply = static_cast<QNetworkReply *>(sender());
    reply->deleteLater();

    switch (reply->error()) {
    case QNetworkReply::NoError: {
        const QByteArray response(reply->readAll());
        QJsonParseError jsonError;
        const QJsonDocument replyDoc = QJsonDocument::fromJson(response, &jsonError);
        if (jsonError.error != QJsonParseError::NoError) {
            emitError(tr("Unable to parse device statistics: "), jsonError, reply, response);
            return;
        }

        const QJsonObject replyObj(replyDoc.object());
        int index = 0;
        for (SyncthingDev &devInfo : m_devs) {
            const QJsonObject devObj(replyObj.value(devInfo.id).toObject());
            if (!devObj.isEmpty()) {
                try {
                    devInfo.lastSeen = DateTime::fromIsoStringLocal(devObj.value(QStringLiteral("lastSeen")).toString().toUtf8().data());
                    emit devStatusChanged(devInfo, index);
                } catch (const ConversionException &) {
                    devInfo.lastSeen = DateTime();
                }
            }
            ++index;
        }
        // since there seems no event for this data, keep polling
        if (m_keepPolling && m_devStatsPollTimer.interval()) {
            m_devStatsPollTimer.start();
        }
        break;
    }
    case QNetworkReply::OperationCanceledError:
        return; // intended, not an error
    default:
        emitError(tr("Unable to request device statistics: "), SyncthingErrorCategory::OverallConnection, reply);
    }
}

/*!
 * \brief Reads results of requestErrors().
 */
void SyncthingConnection::readErrors()
{
    auto *const reply = static_cast<QNetworkReply *>(sender());
    reply->deleteLater();
    if (reply == m_errorsReply) {
        m_errorsReply = nullptr;
    }

    // ignore any errors occured before connecting
    if (m_lastErrorTime.isNull()) {
        m_lastErrorTime = DateTime::now();
    }

    switch (reply->error()) {
    case QNetworkReply::NoError: {
        const QByteArray response(reply->readAll());
        QJsonParseError jsonError;
        const QJsonDocument replyDoc = QJsonDocument::fromJson(response, &jsonError);
        if (jsonError.error != QJsonParseError::NoError) {
            emitError(tr("Unable to parse errors: "), jsonError, reply, response);
            return;
        }

        for (const QJsonValue &errorVal : replyDoc.object().value(QStringLiteral("errors")).toArray()) {
            const QJsonObject errorObj(errorVal.toObject());
            if (errorObj.isEmpty()) {
                continue;
            }
            try {
                const DateTime when = DateTime::fromIsoStringLocal(errorObj.value(QStringLiteral("when")).toString().toLocal8Bit().data());
                if (m_lastErrorTime < when) {
                    emitNotification(m_lastErrorTime = when, errorObj.value(QStringLiteral("message")).toString());
                }
            } catch (const ConversionException &) {
            }
        }

        // since there seems no event for this data, keep polling
        if (m_keepPolling && m_errorsPollTimer.interval()) {
            m_errorsPollTimer.start();
        }
        break;
    }
    case QNetworkReply::OperationCanceledError:
        return; // intended, not an error
    default:
        emitError(tr("Unable to request errors: "), SyncthingErrorCategory::SpecificRequest, reply);
    }
}

/*!
 * \brief Reads results of requestClearingErrors().
 */
void SyncthingConnection::readClearingErrors()
{
    auto *const reply = static_cast<QNetworkReply *>(sender());
    reply->deleteLater();

    switch (reply->error()) {
    case QNetworkReply::NoError:
        break;
    default:
        emitError(tr("Unable to request clearing errors: "), SyncthingErrorCategory::SpecificRequest, reply);
    }
}

/*!
 * \brief Reads results of requestEvents().
 */
void SyncthingConnection::readEvents()
{
    auto *const reply = static_cast<QNetworkReply *>(sender());
    reply->deleteLater();
    if (reply == m_eventsReply) {
        m_eventsReply = nullptr;
    }

    switch (reply->error()) {
    case QNetworkReply::NoError: {
        const QByteArray response(reply->readAll());
        QJsonParseError jsonError;
        const QJsonDocument replyDoc = QJsonDocument::fromJson(response, &jsonError);
        if (jsonError.error != QJsonParseError::NoError) {
            emitError(tr("Unable to parse Syncthing events: "), jsonError, reply, response);
            handleFatalConnectionError();
            return;
        }

        const QJsonArray replyArray = replyDoc.array();
        emit newEvents(replyArray);
        // search the array for interesting events
        for (const QJsonValue &eventVal : replyArray) {
            const QJsonObject event = eventVal.toObject();
            m_lastEventId = event.value(QStringLiteral("id")).toInt(m_lastEventId);
            DateTime eventTime;
            try {
                eventTime = DateTime::fromIsoStringGmt(event.value(QStringLiteral("time")).toString().toLocal8Bit().data());
            } catch (const ConversionException &) {
                // ignore conversion error
            }
            const QString eventType(event.value(QStringLiteral("type")).toString());
            const QJsonObject eventData(event.value(QStringLiteral("data")).toObject());
            if (eventType == QLatin1String("Starting")) {
                readStartingEvent(eventData);
            } else if (eventType == QLatin1String("StateChanged")) {
                readStatusChangedEvent(eventTime, eventData);
            } else if (eventType == QLatin1String("DownloadProgress")) {
                readDownloadProgressEvent(eventTime, eventData);
            } else if (eventType.startsWith(QLatin1String("Folder"))) {
                readDirEvent(eventTime, eventType, eventData);
            } else if (eventType.startsWith(QLatin1String("Device"))) {
                readDeviceEvent(eventTime, eventType, eventData);
            } else if (eventType == QLatin1String("ItemStarted")) {
                readItemStarted(eventTime, eventData);
            } else if (eventType == QLatin1String("ItemFinished")) {
                readItemFinished(eventTime, eventData);
            } else if (eventType == QLatin1String("ConfigSaved")) {
                requestConfig(); // just consider current config as invalidated
            }
        }
        break;
    }
    case QNetworkReply::TimeoutError:
        // no new events available, keep polling
        break;
    case QNetworkReply::OperationCanceledError:
        // intended disconnect, not an error
        if (m_reconnecting) {
            // if reconnection flag is set, instantly etstablish a new connection ...
            continueReconnecting();
        } else {
            // ... otherwise keep disconnected
            setStatus(SyncthingStatus::Disconnected);
        }
        return;
    default:
        emitError(tr("Unable to request Syncthing events: "), SyncthingErrorCategory::OverallConnection, reply);
        handleFatalConnectionError();
        return;
    }

    if (m_keepPolling) {
        requestEvents();
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
    const QString configDir(eventData.value(QStringLiteral("home")).toString());
    if (configDir != m_configDir) {
        emit configDirChanged(m_configDir = configDir);
    }
    emitMyIdChanged(eventData.value(QStringLiteral("myID")).toString());
}

/*!
 * \brief Reads results of requestEvents().
 */
void SyncthingConnection::readStatusChangedEvent(DateTime eventTime, const QJsonObject &eventData)
{
    const QString dir(eventData.value(QStringLiteral("folder")).toString());
    if (dir.isEmpty()) {
        return;
    }

    // find the directory
    int index;
    SyncthingDir *dirInfo = findDirInfo(dir, index);

    // add a new directory if the dir is not present yet
    const bool dirAlreadyPresent = dirInfo;
    if (!dirAlreadyPresent) {
        m_dirs.emplace_back(dir);
        dirInfo = &m_dirs.back();
    }

    // assign new status
    bool statusChanged = dirInfo->assignStatus(eventData.value(QStringLiteral("to")).toString(), eventTime);
    if (dirInfo->status == SyncthingDirStatus::OutOfSync) {
        const QString errorMessage(eventData.value(QStringLiteral("error")).toString());
        if (!errorMessage.isEmpty()) {
            dirInfo->globalError = errorMessage;
            statusChanged = true;
        }
    }
    if (dirAlreadyPresent) {
        // emit status changed when dir already present
        if (statusChanged) {
            emit dirStatusChanged(*dirInfo, index);
        }
    } else {
        // request config for complete meta data of new directory
        requestConfig();
    }
}

/*!
 * \brief Reads results of requestEvents().
 */
void SyncthingConnection::readDownloadProgressEvent(DateTime eventTime, const QJsonObject &eventData)
{
    VAR_UNUSED(eventTime)
    for (SyncthingDir &dirInfo : m_dirs) {
        // disappearing implies that the download has been finished so just wipe old entries
        dirInfo.downloadingItems.clear();
        dirInfo.blocksAlreadyDownloaded = dirInfo.blocksToBeDownloaded = 0;

        // read progress of currently downloading items
        const QJsonObject dirObj(eventData.value(dirInfo.id).toObject());
        if (!dirObj.isEmpty()) {
            dirInfo.downloadingItems.reserve(static_cast<size_t>(dirObj.size()));
            for (auto filePair = dirObj.constBegin(), end = dirObj.constEnd(); filePair != end; ++filePair) {
                dirInfo.downloadingItems.emplace_back(dirInfo.path, filePair.key(), filePair.value().toObject());
                const SyncthingItemDownloadProgress &itemProgress = dirInfo.downloadingItems.back();
                dirInfo.blocksAlreadyDownloaded += itemProgress.blocksAlreadyDownloaded;
                dirInfo.blocksToBeDownloaded += itemProgress.totalNumberOfBlocks;
            }
        }
        dirInfo.downloadPercentage = (dirInfo.blocksAlreadyDownloaded > 0 && dirInfo.blocksToBeDownloaded > 0)
            ? (static_cast<unsigned int>(dirInfo.blocksAlreadyDownloaded) * 100 / static_cast<unsigned int>(dirInfo.blocksToBeDownloaded))
            : 0;
        dirInfo.downloadLabel
            = QStringLiteral("%1 / %2 - %3 %")
                  .arg(QString::fromLatin1(dataSizeToString(dirInfo.blocksAlreadyDownloaded > 0
                               ? static_cast<uint64>(dirInfo.blocksAlreadyDownloaded) * SyncthingItemDownloadProgress::syncthingBlockSize
                               : 0).data()),
                      QString::fromLatin1(dataSizeToString(dirInfo.blocksToBeDownloaded > 0
                              ? static_cast<uint64>(dirInfo.blocksToBeDownloaded) * SyncthingItemDownloadProgress::syncthingBlockSize
                              : 0).data()),
                      QString::number(dirInfo.downloadPercentage));
    }
    emit downloadProgressChanged();
}

/*!
 * \brief Reads results of requestEvents().
 */
void SyncthingConnection::readDirEvent(DateTime eventTime, const QString &eventType, const QJsonObject &eventData)
{
    QString dir(eventData.value(QStringLiteral("folder")).toString());
    if (dir.isEmpty()) {
        dir = eventData.value(QStringLiteral("id")).toString();
    }
    if (dir.isEmpty()) {
        return;
    }

    int index;
    if (SyncthingDir *dirInfo = findDirInfo(dir, index)) {
        if (eventType == QLatin1String("FolderErrors")) {
            const QJsonArray errors(eventData.value(QStringLiteral("errors")).toArray());
            if (errors.isEmpty()) {
                return;
            }

            for (const QJsonValue &errorVal : errors) {
                const QJsonObject error(errorVal.toObject());
                if (error.isEmpty()) {
                    continue;
                }
                auto &errors = dirInfo->itemErrors;
                SyncthingItemError dirError(error.value(QStringLiteral("error")).toString(), error.value(QStringLiteral("path")).toString());
                if (find(errors.cbegin(), errors.cend(), dirError) != errors.cend()) {
                    continue;
                }
                errors.emplace_back(move(dirError));
                dirInfo->assignStatus(SyncthingDirStatus::OutOfSync, eventTime);

                // emit newNotification() for new errors
                const auto &previousErrors = dirInfo->previousItemErrors;
                if (find(previousErrors.cbegin(), previousErrors.cend(), dirInfo->itemErrors.back()) == previousErrors.cend()) {
                    emitNotification(eventTime, dirInfo->itemErrors.back().message);
                }
            }
            emit dirStatusChanged(*dirInfo, index);
        } else if (eventType == QLatin1String("FolderSummary")) {
            readDirSummary(eventTime, eventData.value(QStringLiteral("summary")).toObject(), *dirInfo, index);
        } else if (eventType == QLatin1String("FolderCompletion")) {
            //const QString device(eventData.value(QStringLiteral("device")).toString());
            int percentage = eventData.value(QStringLiteral("completion")).toInt();
            if (percentage > 0 && percentage < 100 && (dirInfo->progressPercentage <= 0 || percentage < dirInfo->progressPercentage)) {
                // Syncthing provides progress percentage for each device
                // just show the smallest percentage for now
                dirInfo->progressPercentage = percentage;
            }
        } else if (eventType == QLatin1String("FolderScanProgress")) {
            // FIXME: for some reason this is always 0
            const int current = eventData.value(QStringLiteral("current")).toInt(0), total = eventData.value(QStringLiteral("total")).toInt(0),
                      rate = eventData.value(QStringLiteral("rate")).toInt(0);
            if (current > 0 && total > 0) {
                dirInfo->progressPercentage = current * 100 / total;
                dirInfo->progressRate = rate;
                dirInfo->assignStatus(SyncthingDirStatus::Scanning, eventTime); // ensure state is scanning
                emit dirStatusChanged(*dirInfo, index);
            }
        } else if (eventType == QLatin1String("FolderPaused")) {
            if (!dirInfo->paused) {
                dirInfo->paused = true;
                emit dirStatusChanged(*dirInfo, index);
            }
        } else if (eventType == QLatin1String("FolderResumed")) {
            if (dirInfo->paused) {
                dirInfo->paused = false;
                emit dirStatusChanged(*dirInfo, index);
            }
        }
    }
}

/*!
 * \brief Reads results of requestEvents().
 */
void SyncthingConnection::readDeviceEvent(DateTime eventTime, const QString &eventType, const QJsonObject &eventData)
{
    if (eventTime.isNull() && m_lastConnectionsUpdate.isNull() && eventTime < m_lastConnectionsUpdate) {
        return; // ignore device events happened before the last connections update
    }
    const QString dev(eventData.value(QStringLiteral("device")).toString());
    if (dev.isEmpty()) {
        return;
    }

    // dev status changed, depending on event type
    int index;
    if (SyncthingDev *devInfo = findDevInfo(dev, index)) {
        SyncthingDevStatus status = devInfo->status;
        bool paused = devInfo->paused;
        if (eventType == QLatin1String("DeviceConnected")) {
            status = SyncthingDevStatus::Idle; // TODO: figure out when dev is actually syncing
        } else if (eventType == QLatin1String("DeviceDisconnected")) {
            status = SyncthingDevStatus::Disconnected;
        } else if (eventType == QLatin1String("DevicePaused")) {
            paused = true;
        } else if (eventType == QLatin1String("DeviceRejected")) {
            status = SyncthingDevStatus::Rejected;
        } else if (eventType == QLatin1String("DeviceResumed")) {
            paused = false;
            // FIXME: correct to assume device which has just been resumed is still disconnected?
            status = SyncthingDevStatus::Disconnected;
        } else if (eventType == QLatin1String("DeviceDiscovered")) {
            // we know about this device already, set status anyways because it might still be unknown
            if (status == SyncthingDevStatus::Unknown) {
                status = SyncthingDevStatus::Disconnected;
            }
        } else {
            return; // can't handle other event types currently
        }
        if (devInfo->status != status || devInfo->paused != paused) {
            if (devInfo->status != SyncthingDevStatus::OwnDevice) { // don't mess with the status of the own device
                devInfo->status = status;
            }
            devInfo->paused = paused;
            emit devStatusChanged(*devInfo, index);
        }
    }
}

/*!
 * \brief Reads results of requestEvents().
 * \todo Implement this.
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
    if (dir.isEmpty()) {
        return;
    }

    int index;
    if (SyncthingDir *dirInfo = findDirInfo(dir, index)) {
        const QString error(eventData.value(QStringLiteral("error")).toString()), item(eventData.value(QStringLiteral("item")).toString());
        if (error.isEmpty()) {
            if (dirInfo->lastFileTime.isNull() || eventTime < dirInfo->lastFileTime) {
                dirInfo->lastFileTime = eventTime, dirInfo->lastFileName = item,
                dirInfo->lastFileDeleted = (eventData.value(QStringLiteral("action")) != QLatin1String("delete"));
                if (eventTime > m_lastFileTime) {
                    m_lastFileTime = dirInfo->lastFileTime, m_lastFileName = dirInfo->lastFileName, m_lastFileDeleted = dirInfo->lastFileDeleted;
                }
                emit dirStatusChanged(*dirInfo, index);
            }
        } else if (dirInfo->status == SyncthingDirStatus::OutOfSync) {
            // FIXME: find better way to check whether the event is still relevant
            dirInfo->itemErrors.emplace_back(error, item);
            dirInfo->status = SyncthingDirStatus::OutOfSync;
            // emitNotification will trigger status update, so no need to call setStatus(status())
            emit dirStatusChanged(*dirInfo, index);
            emitNotification(eventTime, error);
        }
    }
}

/*!
 * \brief Reads results of rescan().
 */
void SyncthingConnection::readRescan()
{
    auto *const reply = static_cast<QNetworkReply *>(sender());
    reply->deleteLater();
    switch (reply->error()) {
    case QNetworkReply::NoError:
        emit rescanTriggered(reply->property("dirId").toString());
        break;
    default:
        emitError(tr("Unable to request rescan: "), SyncthingErrorCategory::SpecificRequest, reply);
    }
}

/*!
 * \brief Reads results of pauseDevice() and resumeDevice().
 */
void SyncthingConnection::readDevPauseResume()
{
    auto *const reply = static_cast<QNetworkReply *>(sender());
    reply->deleteLater();
    switch (reply->error()) {
    case QNetworkReply::NoError: {
        const QStringList devIds(reply->property("devIds").toStringList());
        const bool resume = reply->property("resume").toBool();
        setDevicesPaused(m_rawConfig, devIds, !resume);
        if (reply->property("resume").toBool()) {
            emit deviceResumeTriggered(devIds);
        } else {
            emit devicePauseTriggered(devIds);
        }
        break;
    }
    default:
        emitError(tr("Unable to request device pause/resume: "), SyncthingErrorCategory::SpecificRequest, reply);
    }
}

void SyncthingConnection::readDirPauseResume()
{
    auto *const reply = static_cast<QNetworkReply *>(sender());
    reply->deleteLater();
    switch (reply->error()) {
    case QNetworkReply::NoError: {
        const QStringList dirIds(reply->property("dirIds").toStringList());
        const bool resume = reply->property("resume").toBool();
        setDirectoriesPaused(m_rawConfig, dirIds, !resume);
        if (resume) {
            emit directoryResumeTriggered(dirIds);
        } else {
            emit directoryPauseTriggered(dirIds);
        }
        break;
    }
    default:
        emitError(tr("Unable to request directory pause/resume: "), SyncthingErrorCategory::SpecificRequest, reply);
    }
}

/*!
 * \brief Reads results of restart().
 */
void SyncthingConnection::readRestart()
{
    auto *const reply = static_cast<QNetworkReply *>(sender());
    reply->deleteLater();
    switch (reply->error()) {
    case QNetworkReply::NoError:
        emit restartTriggered();
        break;
    default:
        emitError(tr("Unable to request restart: "), SyncthingErrorCategory::SpecificRequest, reply);
    }
}

/*!
 * \brief Reads results of shutdown().
 */
void SyncthingConnection::readShutdown()
{
    auto *const reply = static_cast<QNetworkReply *>(sender());
    reply->deleteLater();
    switch (reply->error()) {
    case QNetworkReply::NoError:
        emit shutdownTriggered();
        break;
    default:
        emitError(tr("Unable to request shutdown: "), SyncthingErrorCategory::SpecificRequest, reply);
    }
}

/*!
 * \brief Reads data from requestDirStatus().
 */
void SyncthingConnection::readDirStatus()
{
    auto *const reply = static_cast<QNetworkReply *>(sender());
    reply->deleteLater();
    switch (reply->error()) {
    case QNetworkReply::NoError: {
        // determine relevant dir
        int index;
        const QString dirId(reply->property("dirId").toString());
        SyncthingDir *const dir = findDirInfo(dirId, index);
        if (!dir) {
            // discard status for unknown dirs
            return;
        }

        // parse JSON
        const QByteArray response(reply->readAll());
        QJsonParseError jsonError;
        const QJsonDocument replyDoc = QJsonDocument::fromJson(response, &jsonError);
        if (jsonError.error != QJsonParseError::NoError) {
            emitError(tr("Unable to parse status for directory %1: ").arg(dirId), jsonError, reply, response);
            return;
        }

        if (readDirSummary(DateTime::gmtNow(), replyDoc.object(), *dir, index)) {
            recalculateStatus();
        }
        break;
    }
    default:
        emitError(tr("Unable to request directory statistics: "), SyncthingErrorCategory::SpecificRequest, reply);
    }
}

/*!
 * \brief Reads data from requestDirStatus() and FolderSummary-event and stores them to \a dir.
 */
bool SyncthingConnection::readDirSummary(DateTime eventTime, const QJsonObject &summary, SyncthingDir &dir, int index)
{
    if (summary.isEmpty() || dir.lastStatisticsUpdate > eventTime) {
        return false;
    }

    // update statistics
    dir.globalBytes = toUInt64(summary.value(QStringLiteral("globalBytes")));
    dir.globalDeleted = toUInt64(summary.value(QStringLiteral("globalDeleted")));
    dir.globalFiles = toUInt64(summary.value(QStringLiteral("globalFiles")));
    dir.globalDirs = toUInt64(summary.value(QStringLiteral("globalDirectories")));
    dir.localBytes = toUInt64(summary.value(QStringLiteral("localBytes")));
    dir.localDeleted = toUInt64(summary.value(QStringLiteral("localDeleted")));
    dir.localFiles = toUInt64(summary.value(QStringLiteral("localFiles")));
    dir.localDirs = toUInt64(summary.value(QStringLiteral("localDirectories")));
    dir.neededByted = toUInt64(summary.value(QStringLiteral("needByted")));
    dir.neededFiles = toUInt64(summary.value(QStringLiteral("needFiles")));
    dir.ignorePatterns = summary.value(QStringLiteral("ignorePatterns")).toBool();
    dir.lastStatisticsUpdate = eventTime;

    // update status
    bool stateChanged = false;
    const QString state(summary.value(QStringLiteral("state")).toString());
    if (!state.isEmpty()) {
        try {
            dir.assignStatus(state, DateTime::fromIsoStringGmt(summary.value(QStringLiteral("stateChanged")).toString().toUtf8().data()));
        } catch (const ConversionException &) {
            // FIXME: warning about invalid stateChanged
        }
    }

    emit dirStatusChanged(dir, index);
    return stateChanged;
}

/*!
 * \brief Sets the connection status. Ensures statusChanged() is emitted.
 * \param status Specifies the status; should be either SyncthingStatus::Disconnected, SyncthingStatus::Reconnecting, or
 *        SyncthingStatus::Idle. There is no use in specifying other values such as SyncthingStatus::Synchronizing as
 *        these are determined automatically within the method.
 */
void SyncthingConnection::setStatus(SyncthingStatus status)
{
    if (m_status == SyncthingStatus::BeingDestroyed) {
        return;
    }
    switch (status) {
    case SyncthingStatus::Disconnected:
    case SyncthingStatus::Reconnecting:
        // don't consider synchronization finished in this this case
        m_devStatsPollTimer.stop();
        m_trafficPollTimer.stop();
        m_errorsPollTimer.stop();
        m_syncedDirs.clear();
        break;
    default:
        // reset reconnect tries
        m_autoReconnectTries = 0;

        // check whether at least one directory is scanning or synchronizing
        bool scanning = false;
        bool synchronizing = false;
        for (SyncthingDir &dir : m_dirs) {
            if (dir.status == SyncthingDirStatus::Synchronizing) {
                if (find(m_syncedDirs.cbegin(), m_syncedDirs.cend(), &dir) == m_syncedDirs.cend()) {
                    m_syncedDirs.push_back(&dir);
                }
                synchronizing = true;
            } else if (dir.status == SyncthingDirStatus::Scanning) {
                scanning = true;
            }
        }
        if (synchronizing) {
            status = SyncthingStatus::Synchronizing;
        } else if (scanning) {
            status = SyncthingStatus::Scanning;
        } else {
            // check whether at least one device is paused
            bool paused = false;
            for (const SyncthingDev &dev : m_devs) {
                if (dev.paused) {
                    paused = true;
                    break;
                }
            }
            if (paused) {
                status = SyncthingStatus::Paused;
                // don't consider synchronization finished in this this case
                m_syncedDirs.clear();
            } else {
                status = SyncthingStatus::Idle;
            }
        }
        if (status != SyncthingStatus::Synchronizing) {
            m_completedDirs.clear();
            m_completedDirs.swap(m_syncedDirs);
        }
    }
    if (m_status != status) {
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
    emit newNotification(when, message);
    recalculateStatus();
}

/*!
 * \brief Internally called to emit a JSON parsing error.
 * \remarks Since in this case the reply has already been read, its response must be passed as extra argument.
 */
void SyncthingConnection::emitError(const QString &message, const QJsonParseError &jsonError, QNetworkReply *reply, const QByteArray &response)
{
    emit error(message % jsonError.errorString() % QChar(' ') % QChar('(') % tr("at offset %1").arg(jsonError.offset) % QChar(')'),
        SyncthingErrorCategory::Parsing, QNetworkReply::NoError, reply->request(), response);
}

/*!
 * \brief Internally called to emit a network error (server replied error code are connection or server could not be reached at all).
 */
void SyncthingConnection::emitError(const QString &message, SyncthingErrorCategory category, QNetworkReply *reply)
{
    emit error(message + reply->errorString(), category, reply->error(), reply->request(), reply->readAll());
}

/*!
 * \brief Internally called to emit myIdChanged() signal.
 */
void SyncthingConnection::emitMyIdChanged(const QString &newId)
{
    if (newId.isEmpty() || m_myId == newId) {
        return;
    }

    // adjust device status
    int row = 0;
    for (SyncthingDev &dev : m_devs) {
        if (dev.id == newId) {
            if (dev.status != SyncthingDevStatus::OwnDevice) {
                dev.status = SyncthingDevStatus::OwnDevice;
                emit devStatusChanged(dev, row);
            }
        } else if (dev.status == SyncthingDevStatus::OwnDevice) {
            dev.status = SyncthingDevStatus::Unknown;
            emit devStatusChanged(dev, row);
        }
        ++row;
    }

    emit myIdChanged(m_myId = newId);
}

/*!
 * \brief Internally called to handle a fatal error when reading config (dirs/devs), status and events.
 */
void SyncthingConnection::handleFatalConnectionError()
{
    setStatus(SyncthingStatus::Disconnected);
    if (m_autoReconnectTimer.interval()) {
        m_autoReconnectTimer.start();
    }
}

/*!
 * \brief Internally called to recalculate the overall connection status, eg. after the status of a directory
 *        changed.
 * \remarks
 * - This is achieved by simply setting the status to idle. setStatus() will calculate the specific status.
 * - If not connected, this method does nothing. This is important, because when this method is called when
 *   establishing a connection (and the status is hence still disconnected) timers for polling would be
 *   killed.
 */
void SyncthingConnection::recalculateStatus()
{
    if (isConnected()) {
        setStatus(SyncthingStatus::Idle);
    }
}

/*!
 * \fn SyncthingConnection::newConfig()
 * \brief Indicates new configuration (dirs, devs, ...) is available.
 * \remarks
 * - Configuration is requested automatically when connecting.
 * - Previous directories (and directory info objects!) are invalidated.
 * - Previous devices (and device info objects!) are invalidated.
 */

/*!
 * \fn SyncthingConnection::newDirs()
 * \brief Indicates new directories are available.
 * \remarks Always emitted after newConfig() as soon as new directory info objects become available.
 */

/*!
 * \fn SyncthingConnection::newDevices()
 * \brief Indicates new devices are available.
 * \remarks Always emitted after newConfig() as soon as new device info objects become available.
 */

/*!
 * \fn SyncthingConnection::newEvents()
 * \brief Indicates new events (dir status changed, ...) are available.
 * \remarks New events are automatically polled when connected.
 */

/*!
 * \fn SyncthingConnection::dirStatusChanged()
 * \brief Indicates the status of the specified \a dir changed.
 */

/*!
 * \fn SyncthingConnection::devStatusChanged()
 * \brief Indicates the status of the specified \a dev changed.
 */

/*!
 * \fn SyncthingConnection::downloadProgressChanged()
 * \brief Indicates the download progress changed.
 */

/*!
 * \fn SyncthingConnection::newNotification()
 * \brief Indicates a new Syncthing notification is available.
 */

/*!
 * \fn SyncthingConnection::error()
 * \brief Indicates a request (for configuration, events, ...) failed.
 */

/*!
 * \fn SyncthingConnection::statusChanged()
 * \brief Indicates the status of the connection changed.
 */

/*!
 * \fn SyncthingConnection::configDirChanged()
 * \brief Indicates the Syncthing home/configuration directory changed.
 */

/*!
 * \fn SyncthingConnection::myIdChanged()
 * \brief Indicates ID of the own Syncthing device changed.
 */

/*!
 * \fn SyncthingConnection::trafficChanged()
 * \brief Indicates totalIncomingTraffic() or totalOutgoingTraffic() has changed.
 */

/*!
 * \fn SyncthingConnection::rescanTriggered()
 * \brief Indicates a rescan has been triggered sucessfully.
 * \remarks Only emitted for rescans triggered internally via rescan() or rescanAll().
 */

/*!
 * \fn SyncthingConnection::pauseTriggered()
 * \brief Indicates a device has been paused sucessfully.
 * \remarks Only emitted for pausing triggered internally via pause() or pauseAll().
 */

/*!
 * \fn SyncthingConnection::resumeTriggered()
 * \brief Indicates a device has been resumed sucessfully.
 * \remarks Only emitted for resuming triggered internally via resume() or resumeAll().
 */

/*!
 * \fn SyncthingConnection::restartTriggered()
 * \brief Indicates a restart has been successfully triggered via restart().
 */
}
