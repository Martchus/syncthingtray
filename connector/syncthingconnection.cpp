#include "./syncthingconnection.h"
#include "./syncthingconfig.h"
#include "./syncthingconnectionsettings.h"
#include "./utils.h"

#ifdef LIB_SYNCTHING_CONNECTOR_CONNECTION_MOCKED
#include "./syncthingconnectionmockhelpers.h"
#endif

#include "resources/config.h"

#include <c++utilities/conversion/stringconversion.h>
#include <c++utilities/io/ansiescapecodes.h>

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

#include <iostream>
#include <utility>

using namespace std;
using namespace CppUtilities;
using namespace CppUtilities::EscapeCodes;

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
 * \brief The SyncthingConnection class allows Qt applications to access Syncthing via its REST API.
 * \remarks All requests are performed asynchronously.
 *
 * The first thing to do when working with that class is setting the URL to connect to and the API key
 * via the constructor or setSyncthingUrl() and setApiKey(). Credentials for the HTTP authentication
 * can be set via setCredentials() if not included in the URL.
 *
 * Requests can then be done via the request...() methods, eg. requestConfig(). This would emit the
 * newConfig() signal on success and the error() signal when an error occurred. The other request...()
 * methods work in a similar way.
 *
 * However, usually it is best to simply call the connect() method. It will do all required requests
 * to populate dirInfo(), devInfo(), myId(), totalIncomingTraffic(), totalOutgoingTraffic() and all
 * the other variables. It will also use Syncthing's event API to listen for changes. The signals
 * newDirs() and newDevs() are can be used to know when dirInfo() and devInfo() become available.
 * Note that in this case the previous dirInfo()/devInfo() is invalidated.
 */

/*!
 * \brief Constructs a new instance ready to connect. To establish the connection, call connect().
 */
SyncthingConnection::SyncthingConnection(
    const QString &syncthingUrl, const QByteArray &apiKey, SyncthingConnectionLoggingFlags loggingFlags, QObject *parent)
    : QObject(parent)
    , m_syncthingUrl(syncthingUrl)
    , m_apiKey(apiKey)
    , m_status(SyncthingStatus::Disconnected)
    , m_statusComputionFlags(SyncthingStatusComputionFlags::Default)
    , m_loggingFlags(SyncthingConnectionLoggingFlags::None)
    , m_loggingFlagsHandler(SyncthingConnectionLoggingFlags::None)
    , m_keepPolling(false)
    , m_abortingAllRequests(false)
    , m_connectionAborted(false)
    , m_abortingToReconnect(false)
    , m_requestCompletion(true)
    , m_lastEventId(0)
    , m_lastDiskEventId(0)
    , m_autoReconnectTries(0)
    , m_totalIncomingTraffic(unknownTraffic)
    , m_totalOutgoingTraffic(unknownTraffic)
    , m_totalIncomingRate(0.0)
    , m_totalOutgoingRate(0.0)
    , m_configReply(nullptr)
    , m_statusReply(nullptr)
    , m_connectionsReply(nullptr)
    , m_errorsReply(nullptr)
    , m_dirStatsReply(nullptr)
    , m_devStatsReply(nullptr)
    , m_eventsReply(nullptr)
    , m_versionReply(nullptr)
    , m_diskEventsReply(nullptr)
    , m_logReply(nullptr)
    , m_unreadNotifications(false)
    , m_hasConfig(false)
    , m_hasStatus(false)
    , m_hasEvents(false)
    , m_hasDiskEvents(false)
    , m_lastFileDeleted(false)
    , m_dirStatsAltered(false)
    , m_recordFileChanges(false)
{
    m_trafficPollTimer.setInterval(SyncthingConnectionSettings::defaultTrafficPollInterval);
    m_trafficPollTimer.setTimerType(Qt::VeryCoarseTimer);
    m_trafficPollTimer.setSingleShot(true);
    QObject::connect(&m_trafficPollTimer, &QTimer::timeout, this, &SyncthingConnection::requestConnections);
    m_devStatsPollTimer.setInterval(SyncthingConnectionSettings::defaultDevStatusPollInterval);
    m_devStatsPollTimer.setTimerType(Qt::VeryCoarseTimer);
    m_devStatsPollTimer.setSingleShot(true);
    QObject::connect(&m_devStatsPollTimer, &QTimer::timeout, this, &SyncthingConnection::requestDeviceStatistics);
    m_errorsPollTimer.setInterval(SyncthingConnectionSettings::defaultErrorsPollInterval);
    m_errorsPollTimer.setTimerType(Qt::VeryCoarseTimer);
    m_errorsPollTimer.setSingleShot(true);
    QObject::connect(&m_errorsPollTimer, &QTimer::timeout, this, &SyncthingConnection::requestErrors);
    m_autoReconnectTimer.setTimerType(Qt::VeryCoarseTimer);
    m_autoReconnectTimer.setInterval(SyncthingConnectionSettings::defaultReconnectInterval);
    QObject::connect(&m_autoReconnectTimer, &QTimer::timeout, this, &SyncthingConnection::autoReconnect);

#ifdef LIB_SYNCTHING_CONNECTOR_CONNECTION_MOCKED
    setupTestData();
#endif

    setLoggingFlags(loggingFlags);
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
 * \brief Returns whether the currently assigned syncthingUrl() refers to the Syncthing instance on the local machine.
 */
bool SyncthingConnection::isLocal() const
{
    return ::Data::isLocal(QUrl(m_syncthingUrl));
}

/*!
 * \brief Returns the string representation of the specified \a status.
 */
QString SyncthingConnection::statusText(SyncthingStatus status)
{
    switch (status) {
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
    case SyncthingStatus::RemoteNotInSync:
        return tr("connected, remote not in sync");
    default:
        return tr("unknown");
    }
}

/*!
 * \brief Sets the specified logging \a flags.
 */
void SyncthingConnection::setLoggingFlags(SyncthingConnectionLoggingFlags flags)
{
    m_loggingFlags = flags;
    if (flags & SyncthingConnectionLoggingFlags::FromEnvironment) {
        if (!(flags & SyncthingConnectionLoggingFlags::All) && qEnvironmentVariableIntValue(PROJECT_VARNAME_UPPER "_LOG_ALL")) {
            m_loggingFlags |= SyncthingConnectionLoggingFlags::All;
        } else {
            if (!(flags & SyncthingConnectionLoggingFlags::ApiCalls) && qEnvironmentVariableIntValue(PROJECT_VARNAME_UPPER "_LOG_API_CALLS")) {
                m_loggingFlags |= SyncthingConnectionLoggingFlags::ApiCalls;
            }
            if (!(flags & SyncthingConnectionLoggingFlags::ApiCalls) && qEnvironmentVariableIntValue(PROJECT_VARNAME_UPPER "_LOG_API_REPLIES")) {
                m_loggingFlags |= SyncthingConnectionLoggingFlags::ApiCalls;
            }
            if (!(flags & SyncthingConnectionLoggingFlags::Events) && qEnvironmentVariableIntValue(PROJECT_VARNAME_UPPER "_LOG_EVENTS")) {
                m_loggingFlags |= SyncthingConnectionLoggingFlags::Events;
            }
            if (!(flags & SyncthingConnectionLoggingFlags::DirsOrDevsResetted)
                && qEnvironmentVariableIntValue(PROJECT_VARNAME_UPPER "_LOG_DIRS_OR_DEVS_RESETTED")) {
                m_loggingFlags |= SyncthingConnectionLoggingFlags::Events;
            }
        }
    }
    if ((m_loggingFlags & SyncthingConnectionLoggingFlags::DirsOrDevsResetted)
        && !(m_loggingFlagsHandler & SyncthingConnectionLoggingFlags::DirsOrDevsResetted)) {
        QObject::connect(this, &SyncthingConnection::newDirs, [this](const auto &dirs) {
            if (m_loggingFlags & SyncthingConnectionLoggingFlags::DirsOrDevsResetted) {
                std::cerr << Phrases::Info << "Directory list renewed:" << Phrases::End;
                std::cerr << displayNames(dirs).join(QStringLiteral(", ")).toStdString() << endl;
            }
        });
        QObject::connect(this, &SyncthingConnection::newDevices, [this](const auto &devs) {
            if (m_loggingFlags & SyncthingConnectionLoggingFlags::DirsOrDevsResetted) {
                std::cerr << Phrases::Info << "Device list renewed:" << Phrases::End;
                std::cerr << displayNames(devs).join(QStringLiteral(", ")).toStdString() << endl;
            }
        });
        m_loggingFlagsHandler |= SyncthingConnectionLoggingFlags::DirsOrDevsResetted;
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
 * \brief Sets all polling intervals (traffic, device statistics, errors) to 0 so polling is disabled.
 * \remarks Does not affect the auto-reconnect and does not affect the *long* polling for the event API.
 */
void SyncthingConnection::disablePolling()
{
    setTrafficPollInterval(0);
    setDevStatsPollInterval(0);
    setErrorsPollInterval(0);
}

/*!
 * \brief Connects asynchronously to Syncthing. Does nothing if already connected.
 *
 * Use this to connect the first time or to connect to the same Syncthing instance again or to ensure
 * the connection to the currently configured instance is established. Use reconnect() to connect to
 * a different instance.
 *
 * \remarks Does not clear data from a previous connection (except error items). Use reconnect() if that
 *          is required.
 */
void SyncthingConnection::connect()
{
    // reset auto-reconnect
    m_autoReconnectTimer.stop();
    m_autoReconnectTries = 0;

    // skip if already connected (see reconnect() to force reconnecting)
    if (isConnected()) {
        return;
    }

    // reset status
    m_connectionAborted = m_abortingToReconnect = m_hasConfig = m_hasStatus = m_hasEvents = m_hasDiskEvents = false;

    // check configuration
    if (m_apiKey.isEmpty() || m_syncthingUrl.isEmpty()) {
        emit error(tr("Connection configuration is insufficient."), SyncthingErrorCategory::OverallConnection, QNetworkReply::NoError);
        setStatus(SyncthingStatus::Disconnected);
        return;
    }

    // start by requesting config and status; if both are available request further info and events
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
 * \brief Connects in \a milliSeconds. Useful to "schedule" another attempt in case of a failure.
 * \remarks Does nothing if the connection attempt would happen anyways though auto-reconnect.
 */
void SyncthingConnection::connectLater(int milliSeconds)
{
    // skip if conneting via auto-reconnect anyways
    if (autoReconnectInterval() > 0 && milliSeconds > autoReconnectInterval()) {
        return;
    }
    QTimer::singleShot(milliSeconds, this, static_cast<void (SyncthingConnection::*)(void)>(&SyncthingConnection::connect));
}

/*!
 * \brief Disconnects. That means all (long) polling is stopped and ongoing requests are aborted via abortAllRequests().
 */
void SyncthingConnection::disconnect()
{
    m_abortingToReconnect = m_keepPolling = false;
    m_trafficPollTimer.stop();
    m_devStatsPollTimer.stop();
    m_errorsPollTimer.stop();
    m_autoReconnectTimer.stop();
    m_autoReconnectTries = 0;
    abortAllRequests();
}

/*!
 * \brief Aborts status-relevant, pending requests.
 * \remarks Status-relevant means that requests for triggering actions like rescan() or restart() are excluded. requestQrCode() does not
 *          contribute to the status as well and is excluded as well.
 */
void SyncthingConnection::abortAllRequests()
{
    m_connectionAborted = m_abortingAllRequests = true;
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
    if (m_dirStatsReply) {
        m_dirStatsReply->abort();
    }
    if (m_devStatsReply) {
        m_devStatsReply->abort();
    }
    if (m_eventsReply) {
        m_eventsReply->abort();
    }
    if (m_versionReply) {
        m_versionReply->abort();
    }
    if (m_diskEventsReply) {
        m_diskEventsReply->abort();
    }
    if (m_logReply) {
        m_logReply->abort();
    }
    for (auto *const reply : std::as_const(m_otherReplies)) {
        reply->abort();
    }

    m_abortingAllRequests = false;
    handleAdditionalRequestCanceled();
}

/*!
 * \brief Disconnects if connected, then (re-)connects asynchronously.
 * \remarks
 * - Clears the currently cached configuration.
 * - This explicit request to reconnect will reset the autoReconnectTries().
 */
void SyncthingConnection::reconnect()
{
    // reset reconnect timer
    m_autoReconnectTimer.stop();
    m_autoReconnectTries = 0;

    // stop other timers
    m_trafficPollTimer.stop();
    m_devStatsPollTimer.stop();
    m_errorsPollTimer.stop();

    // reset variables to track connection progress
    // note: especially resetting events is important as it influences the subsequent hasPendingRequests() call
    m_hasConfig = m_hasStatus = m_hasEvents = m_hasDiskEvents = false;

    // reconnect right now if no pending requests to be aborted
    if (!hasPendingRequests()) {
        continueReconnecting();
        return;
    }

    // abort pending requests before connecting again
    m_keepPolling = m_abortingToReconnect = true;
    abortAllRequests();
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
 * \brief Reconnects after the specified number of \a milliSeconds.
 */
void SyncthingConnection::reconnectLater(int milliSeconds)
{
    QTimer::singleShot(milliSeconds, this, static_cast<void (SyncthingConnection::*)(void)>(&SyncthingConnection::reconnect));
}

/*!
 * \brief Internally called to reconnect; ensures currently cached config is cleared.
 */
void SyncthingConnection::continueReconnecting()
{
    // notify that we're about to invalidate the configuration if not already invalidated anyways
    const auto isConfigInvalidated = m_rawConfig.isEmpty();
    if (!isConfigInvalidated) {
        emit newConfig(m_rawConfig = QJsonObject());
    }

    // cleanup information from previous connection
    m_keepPolling = true;
    m_connectionAborted = false;
    m_abortingToReconnect = false;
    m_lastEventId = 0;
    m_lastDiskEventId = 0;
    m_configDir.clear();
    m_myId.clear();
    m_tilde.clear();
    m_pathSeparator.clear();
    m_totalIncomingTraffic = unknownTraffic;
    m_totalOutgoingTraffic = unknownTraffic;
    m_totalIncomingRate = 0.0;
    m_totalOutgoingRate = 0.0;
    emit trafficChanged(unknownTraffic, unknownTraffic);
    m_unreadNotifications = false;
    m_hasConfig = false;
    m_hasStatus = false;
    m_hasEvents = false;
    m_hasDiskEvents = false;
    m_dirs.clear();
    m_devs.clear();
    m_lastConnectionsUpdate = DateTime();
    m_lastFileTime = DateTime();
    m_lastErrorTime = DateTime();
    m_startTime = DateTime();
    m_lastFileName.clear();
    m_lastFileDeleted = false;
    m_syncthingVersion.clear();
    m_dirStatsAltered = false;
    emit dirStatisticsChanged();

    // notify that the configuration has been invalidated
    if (!isConfigInvalidated) {
        emit newConfigApplied();
    }

    if (m_apiKey.isEmpty() || m_syncthingUrl.isEmpty()) {
        emit error(tr("Connection configuration is insufficient."), SyncthingErrorCategory::OverallConnection, QNetworkReply::NoError);
        setStatus(SyncthingStatus::Disconnected);
        return;
    }

    requestConfig();
    requestStatus();

    setStatus(SyncthingStatus::Reconnecting);
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

    readDevs(m_rawConfig.value(QLatin1String("devices")).toArray());
    readDirs(m_rawConfig.value(QLatin1String("folders")).toArray());
    emit newConfigApplied();

    continueConnecting();
}

/*!
 * \brief Sets the state from (re)connecting to Syncthing's actual state if polling but there are no more pending requests.
 * \remarks Called by read...() handlers for requests started in continueConnecting().
 * \sa hasPendingRequests()
 */
void SyncthingConnection::concludeConnection()
{
    if (!m_keepPolling || hasPendingRequests()) {
        return;
    }
    setStatus(SyncthingStatus::Idle);
    emitDirStatisticsChanged();
}

/*!
 * \brief Connects increasing the auto-reconnect tries.
 * \remarks Called via m_autoReconnectTimer.
 */
void SyncthingConnection::autoReconnect()
{
    const auto tmp = m_autoReconnectTries;
    connect();
    m_autoReconnectTries = tmp + 1;
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
    return nullptr;
}

/*!
 * \brief Returns the directory info object for the directory with the ID stored in the specified \a object with the specified \a key.
 */
SyncthingDir *SyncthingConnection::findDirInfo(QLatin1String key, const QJsonObject &object, int *row)
{
    const auto dirId(object.value(key).toString());
    if (dirId.isEmpty()) {
        return nullptr;
    }
    int dummyRow;
    auto &rowRef(row ? *row : dummyRow);
    return findDirInfo(dirId, rowRef);
}

/*!
 * \brief Returns the directory info object for the directory with the specified \a path.
 *
 * If a corresponding Syncthing directory could be found, \a relativePath is set to the path of the item relative
 * to the location of the corresponding Syncthing directory.
 *
 * \returns Returns a pointer to the object or nullptr if not found.
 * \remarks The returned object becomes invalid when the newDirs() signal is emitted or the connection is destroyed.
 */
SyncthingDir *SyncthingConnection::findDirInfoByPath(const QString &path, QString &relativePath, int &row)
{
    row = 0;
    for (SyncthingDir &dir : m_dirs) {
        if (path == dir.pathWithoutTrailingSlash()) {
            relativePath.clear();
            return &dir;
        } else if (path.startsWith(dir.path)) {
            relativePath = path.mid(dir.path.size());
            return &dir;
        }
        ++row;
    }
    return nullptr;
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
    if (auto *const existingDirInfo = findDirInfo(dirId, row)) {
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
    return nullptr;
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

/*!
 * \brief Returns all directory IDs for the current configuration.
 * \remarks Computed by looping dirInfo().
 */
QStringList SyncthingConnection::directoryIds() const
{
    return ids(m_dirs);
}

/*!
 * \brief Returns all device IDs for the current configuration.
 * \remarks Computed by looping devInfo().
 */
QStringList SyncthingConnection::deviceIds() const
{
    return ids(m_devs);
}

/*!
 * \brief Returns the device name for the specified \a deviceId if known; otherwise returns just the \a deviceId itself.
 */
QString SyncthingConnection::deviceNameOrId(const QString &deviceId) const
{
    for (const auto &dev : devInfo()) {
        if (dev.id == deviceId) {
            return dev.name;
        }
    }
    return deviceId;
}

/*!
 * \brief Returns the number of devices Syncthing is currently connected to.
 * \remarks Computed by looping devInfo().
 */
std::vector<const SyncthingDev *> SyncthingConnection::connectedDevices() const
{
    std::vector<const SyncthingDev *> connectedDevs;
    connectedDevs.reserve(devInfo().size());
    for (const SyncthingDev &dev : devInfo()) {
        if (dev.isConnected()) {
            connectedDevs.emplace_back(&dev);
        }
    }
    return connectedDevs;
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
    if (SyncthingDev *const existingDevInfo = findDevInfo(devId, row)) {
        devs.emplace_back(move(*existingDevInfo));
    } else {
        devs.emplace_back(devId);
    }
    return &devs.back();
}

/*!
 * \brief Internally called to parse a time stamp.
 */
DateTime SyncthingConnection::parseTimeStamp(const QJsonValue &jsonValue, const QString &context, DateTime defaultValue, bool greaterThanEpoch)
{
    const auto utf16 = jsonValue.toString();
    const auto utf8 = utf16.toUtf8();
    try {
        const auto [localTime, utcOffset] = DateTime::fromIsoString(utf8.data());
        return !greaterThanEpoch || (localTime - utcOffset) > DateTime::unixEpochStart() ? localTime : defaultValue;
    } catch (const ConversionException &e) {
        emit error(tr("Unable to parse timestamp \"%1\" (%2): %3").arg(utf16, context, QString::fromUtf8(e.what())), SyncthingErrorCategory::Parsing,
            QNetworkReply::NoError);
    }
    return defaultValue;
}

/*!
 * \brief Continues connecting if both - config and status - have been parsed yet and continuous polling is enabled.
 */
void SyncthingConnection::continueConnecting()
{
    // skip if config and status are missing or we're not supposed to actually connect
    if (!m_keepPolling || !m_hasConfig || !m_hasStatus) {
        return;
    }

    // read additional information (beside config and status)
    // FIXME: make those requests configurable (eg. flag enum)
    requestConnections();
    requestDirStatistics();
    requestDeviceStatistics();
    requestErrors();
    requestVersion();
    for (const SyncthingDir &dir : m_dirs) {
        requestDirStatus(dir.id);
        if (!m_requestCompletion || dir.paused) {
            continue;
        }
        for (const QString &devId : dir.deviceIds) {
            requestCompletion(devId, dir.id);
        }
    }

    // poll for events
    m_lastEventId = m_lastDiskEventId = 0;
    requestEvents();
    requestDiskEvents();
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
    if (!::Data::isLocal(syncthingUrl)) {
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
    setStatusComputionFlags(connectionSettings.statusComputionFlags);

    return reconnectRequired;
}

/*!
 * \brief Sets the connection status. Ensures statusChanged() is emitted if the status has actually changed.
 * \param status Specifies the status; should be either SyncthingStatus::Disconnected, SyncthingStatus::Reconnecting, or
 *        SyncthingStatus::Idle. There is no use in specifying other values such as SyncthingStatus::Synchronizing as
 *        these are determined automatically within the method according to SyncthingConnection::statusComputionFlags().
 *
 * The precedence of the "connected" states from highest to lowest is:
 * 1. SyncthingStatus::Synchronizing
 * 2. SyncthingStatus::RemoteSynchronizing
 * 3. SyncthingStatus::Scanning
 * 4. SyncthingStatus::Paused
 * 5. SyncthingStatus::Idle
 *
 * \remarks
 * - The "out-of-sync" status is (currently) *not* handled by this function. One needs to query this via
 *   the SyncthingConnection::hasOutOfSyncDirs() function.
 * - Whether notifications are available is *not* handled by this function. One needs to query this via
 *   SyncthingConnection::hasUnreadNotifications().
 */
void SyncthingConnection::setStatus(SyncthingStatus status)
{
    if (m_status == SyncthingStatus::BeingDestroyed) {
        return;
    }
    switch (status) {
    case SyncthingStatus::Disconnected:
        // disable (long) polling
        m_keepPolling = false;
        m_connectionAborted = true;
        [[fallthrough]];
    case SyncthingStatus::Reconnecting:
        m_devStatsPollTimer.stop();
        m_trafficPollTimer.stop();
        m_errorsPollTimer.stop();
        break;
    default:
        // reset reconnect tries
        m_autoReconnectTries = 0;

        // skip if no further status information should be gathered
        status = SyncthingStatus::Idle;
        if (m_statusComputionFlags == SyncthingStatusComputionFlags::None) {
            break;
        }

        // check whether at least one directory is scanning, preparing to synchronize or synchronizing
        // note: We don't distinguish between "preparing to sync" and "synchronizing" for computing the overall
        //       status at the moment.
        auto scanning = false, synchronizing = false, remoteSynchronizing = false;
        if (m_statusComputionFlags & SyncthingStatusComputionFlags::Synchronizing
            || m_statusComputionFlags & SyncthingStatusComputionFlags::Scanning) {
            for (const SyncthingDir &dir : m_dirs) {
                switch (dir.status) {
                case SyncthingDirStatus::WaitingToSync:
                case SyncthingDirStatus::PreparingToSync:
                case SyncthingDirStatus::Synchronizing:
                    synchronizing = m_statusComputionFlags & SyncthingStatusComputionFlags::Synchronizing;
                    break;
                case SyncthingDirStatus::WaitingToScan:
                case SyncthingDirStatus::Scanning:
                    scanning = m_statusComputionFlags & SyncthingStatusComputionFlags::Scanning;
                    break;
                default:;
                }
                if (synchronizing) {
                    break; // skip remaining dirs, "synchronizing" overrides "scanning" anyways
                }
            }
        }

        // set the status to "remote synchronizing" if at least one remote device is still in progress
        if (!synchronizing && (m_statusComputionFlags & SyncthingStatusComputionFlags::RemoteSynchronizing)) {
            for (const SyncthingDev &dev : m_devs) {
                if (dev.status == SyncthingDevStatus::Synchronizing) {
                    remoteSynchronizing = true;
                    break;
                }
            }
        }

        if (synchronizing) {
            status = SyncthingStatus::Synchronizing;
        } else if (remoteSynchronizing) {
            status = SyncthingStatus::RemoteNotInSync;
        } else if (scanning) {
            status = SyncthingStatus::Scanning;
        } else if (m_statusComputionFlags & SyncthingStatusComputionFlags::DevicePaused) {
            // check whether at least one device is paused
            for (const SyncthingDev &dev : m_devs) {
                if (dev.paused) {
                    status = SyncthingStatus::Paused;
                    break;
                }
            }
        }
    }
    if (m_status != status || status == SyncthingStatus::Disconnected) {
        // emit event if status changed and always for disconnects so isConnecting() is re-evaluated
        emit statusChanged(m_status = status);
    }
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
    emit error(message + reply->errorString(), category, reply->error(), reply->request(), reply->bytesAvailable() ? reply->readAll() : QByteArray());
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
                dev.paused = false;
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
 * \brief Internally called to emit tildeChanged() signal.
 */
void SyncthingConnection::emitTildeChanged(const QString &newTilde, const QString &newPathSeparator)
{
    if ((newTilde.isEmpty() || m_tilde == newTilde) && (newPathSeparator.isEmpty() && m_pathSeparator == newPathSeparator)) {
        return;
    }
    m_tilde = newTilde;
    m_pathSeparator = newPathSeparator;
    emit tildeChanged(m_tilde);
}

/*!
 * \brief Internally called to emit dirStatisticsChanged() event.
 */
void SyncthingConnection::emitDirStatisticsChanged()
{
    if (m_dirStatsAltered) {
        m_dirStatsAltered = false;
        emit dirStatisticsChanged();
    }
}

/*!
 * \brief Internally called to handle a fatal error when reading config (dirs/devs), status and events.
 */
void SyncthingConnection::handleFatalConnectionError()
{
    setStatus(SyncthingStatus::Disconnected);
    abortAllRequests();
    if (m_autoReconnectTimer.interval()) {
        m_autoReconnectTimer.start();
    }
}

/*!
 * \brief Handles cancellation of additional requests done via continueConnecting() method.
 */
void SyncthingConnection::handleAdditionalRequestCanceled()
{
    // postpone handling if there are still other requests pending
    if (hasPendingRequests()) {
        return;
    }
    if (m_abortingToReconnect) {
        // if reconnection flag is set, instantly etstablish a new connection ...
        continueReconnecting();
    } else {
        // ... otherwise declare we're disconnected if that was the last pending request
        setStatus(SyncthingStatus::Disconnected);
    }
}

/*!
 * \brief Internally called to recalculate the overall connection status, e.g. after the status of a directory
 *        changed.
 * \remarks
 * - This is achieved by simply setting the status to idle. setStatus() will calculate the specific status.
 * - If not connected, this method does nothing. This is important, because when this method is called when
 *   establishing a connection (and the status is hence still disconnected) timers for polling would be killed.
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
 * \fn SyncthingConnection::newDevAvailable()
 * \brief Indicates another device wants to talk to us.
 */

/*!
 * \fn SyncthingConnection::newDirAvailable()
 * \brief Indicates a device wants to share an so far unknown directory with us.
 * \remarks \a dev might be nullptr. In this case the device which wants to share the directory is unknown as well.
 */

/*!
 * \fn SyncthingConnection::error()
 * \brief Indicates a request (for configuration, events, ...) failed.
 */

/*!
 * \fn SyncthingConnection::statusChanged()
 * \brief Indicates the status of the connection changed (status(), hasOutOfSyncDirs()).
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
 * \fn SyncthingConnection::tildeChanged()
 * \brief Indicates the tilde or path separator of the connected Syncthing instance changed.
 */

/*!
 * \fn SyncthingConnection::trafficChanged()
 * \brief Indicates totalIncomingTraffic() or totalOutgoingTraffic() has changed.
 */

/*!
 * \fn SyncthingConnection::newConfigTriggered()
 * \brief Indicates a new configuration has posted successfully via postConfig().
 * \remarks In contrast to newConfig(), this signal is only emitted for configuration changes internally posted via postConfig().
 */

/*!
 * \fn SyncthingConnection::rescanTriggered()
 * \brief Indicates a rescan has been triggered successfully.
 * \remarks Only emitted for rescans triggered internally via rescan() or rescanAll().
 */

/*!
 * \fn SyncthingConnection::pauseTriggered()
 * \brief Indicates a device has been paused successfully.
 * \remarks Only emitted for pausing triggered internally via pause() or pauseAll().
 */

/*!
 * \fn SyncthingConnection::resumeTriggered()
 * \brief Indicates a device has been resumed successfully.
 * \remarks Only emitted for resuming triggered internally via resume() or resumeAll().
 */

/*!
 * \fn SyncthingConnection::restartTriggered()
 * \brief Indicates a restart has been successfully triggered via restart().
 */
} // namespace Data
