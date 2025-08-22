#include "./syncthingconnection.h"
#include "./syncthingconfig.h"
#include "./syncthingconnectionsettings.h"
#include "./utils.h"

#if defined(LIB_SYNCTHING_CONNECTOR_CONNECTION_MOCKED) || defined(LIB_SYNCTHING_CONNECTOR_MOCKED)
#include "./syncthingconnectionmockhelpers.h"
#endif

#include "resources/config.h"

#include <c++utilities/conversion/stringconversion.h>
#include <c++utilities/io/ansiescapecodes.h>

#include <QAuthenticator>
#include <QDir>
#include <QFileInfo>
#include <QHostAddress>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonValue>
#include <QNetworkAccessManager>
#include <QNetworkInterface>
#include <QNetworkReply>
#include <QStringBuilder>
#include <QTimer>

#if (QT_VERSION >= QT_VERSION_CHECK(6, 4, 0))
#include <QNetworkInformation>
#define SYNCTHINGCONNECTION_SUPPORT_METERED
#endif

#include <iostream>
#include <utility>

using namespace std;
using namespace CppUtilities;
using namespace CppUtilities::EscapeCodes;

namespace Data {

#ifndef LIB_SYNCTHING_CONNECTOR_MOCKED
/*!
 * \brief Returns the QNetworkAccessManager instance used by SyncthingConnection instances.
 */
QNetworkAccessManager &networkAccessManager()
{
    static auto networkAccessManager = new QNetworkAccessManager;
    return *networkAccessManager;
}
#endif

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
    , m_abortingToConnect(false)
    , m_abortingToReconnect(false)
    , m_requestCompletion(true)
    , m_pollingFlags(PollingFlags::All)
    , m_statusRecomputationFlags(StatusRecomputation::None)
    , m_lastEventId(0)
    , m_lastDiskEventId(0)
    , m_autoReconnectTries(0)
    , m_requestTimeout(SyncthingConnectionSettings::defaultRequestTimeout)
    , m_longPollingTimeout(SyncthingConnectionSettings::defaultLongPollingTimeout)
    , m_diskEventLimit(SyncthingConnectionSettings::defaultDiskEventLimit)
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
    , m_hasConfig(false)
    , m_hasStatus(false)
    , m_hasEvents(false)
    , m_hasDiskEvents(false)
    , m_statsRequested(false)
    , m_lastFileDeleted(false)
    , m_recordFileChanges(false)
    , m_useDeprecatedRoutes(true)
    , m_pausingOnMeteredConnection(false)
#ifdef SYNCTHINGCONNECTION_SUPPORT_METERED
    , m_handlingMeteredConnectionInitialized(false)
#endif
    , m_insecure(false)
{
    m_trafficPollTimer.setInterval(SyncthingConnectionSettings::defaultTrafficPollInterval);
    m_trafficPollTimer.setTimerType(Qt::CoarseTimer);
    m_trafficPollTimer.setSingleShot(true);
    QObject::connect(&m_trafficPollTimer, &QTimer::timeout, this, &SyncthingConnection::requestConnections);
    m_devStatsPollTimer.setInterval(SyncthingConnectionSettings::defaultDevStatusPollInterval);
    m_devStatsPollTimer.setTimerType(Qt::CoarseTimer);
    m_devStatsPollTimer.setSingleShot(true);
    QObject::connect(&m_devStatsPollTimer, &QTimer::timeout, this, &SyncthingConnection::requestDeviceStatistics);
    m_errorsPollTimer.setInterval(SyncthingConnectionSettings::defaultErrorsPollInterval);
    m_errorsPollTimer.setTimerType(Qt::CoarseTimer);
    m_errorsPollTimer.setSingleShot(true);
    QObject::connect(&m_errorsPollTimer, &QTimer::timeout, this, &SyncthingConnection::requestErrors);
    m_autoReconnectTimer.setTimerType(Qt::CoarseTimer);
    m_autoReconnectTimer.setInterval(SyncthingConnectionSettings::defaultReconnectInterval);
    QObject::connect(&m_autoReconnectTimer, &QTimer::timeout, this, &SyncthingConnection::autoReconnect);

#if defined(LIB_SYNCTHING_CONNECTOR_CONNECTION_MOCKED) || defined(LIB_SYNCTHING_CONNECTOR_MOCKED)
    setupTestData();
#endif

    setLoggingFlags(loggingFlags);

    // allow initializing the default value for m_useDeprecatedRoutes via environment variable
    auto useDeprecatedRoutesIsInt = false;
    auto useDeprecatedRoutesInt = qEnvironmentVariableIntValue(PROJECT_VARNAME_UPPER "_USE_DEPRECATED_ROUTES", &useDeprecatedRoutesIsInt);
    if (useDeprecatedRoutesIsInt) {
        m_useDeprecatedRoutes = useDeprecatedRoutesInt;
    }
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
    case SyncthingStatus::NoRemoteConnected:
        return tr("connected, but all remote devices disconnected");
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
    if (flags && SyncthingConnectionLoggingFlags::FromEnvironment) {
        if (!(flags && SyncthingConnectionLoggingFlags::All) && qEnvironmentVariableIntValue(PROJECT_VARNAME_UPPER "_LOG_ALL")) {
            m_loggingFlags |= SyncthingConnectionLoggingFlags::All;
        } else {
            if (!(flags && SyncthingConnectionLoggingFlags::ApiCalls) && qEnvironmentVariableIntValue(PROJECT_VARNAME_UPPER "_LOG_API_CALLS")) {
                m_loggingFlags |= SyncthingConnectionLoggingFlags::ApiCalls;
            }
            if (!(flags && SyncthingConnectionLoggingFlags::ApiCalls) && qEnvironmentVariableIntValue(PROJECT_VARNAME_UPPER "_LOG_API_REPLIES")) {
                m_loggingFlags |= SyncthingConnectionLoggingFlags::ApiCalls;
            }
            if (!(flags && SyncthingConnectionLoggingFlags::Events) && qEnvironmentVariableIntValue(PROJECT_VARNAME_UPPER "_LOG_EVENTS")) {
                m_loggingFlags |= SyncthingConnectionLoggingFlags::Events;
            }
            if (!(flags && SyncthingConnectionLoggingFlags::DirsOrDevsResetted)
                && qEnvironmentVariableIntValue(PROJECT_VARNAME_UPPER "_LOG_DIRS_OR_DEVS_RESETTED")) {
                m_loggingFlags |= SyncthingConnectionLoggingFlags::Events;
            }
            if (!(flags && SyncthingConnectionLoggingFlags::CertLoading) && qEnvironmentVariableIntValue(PROJECT_VARNAME_UPPER "_LOG_CERT_LOADING")) {
                m_loggingFlags |= SyncthingConnectionLoggingFlags::CertLoading;
            }
        }
    }
    if ((m_loggingFlags && SyncthingConnectionLoggingFlags::DirsOrDevsResetted)
        && !(m_loggingFlagsHandler && SyncthingConnectionLoggingFlags::DirsOrDevsResetted)) {
        QObject::connect(this, &SyncthingConnection::newDirs, [this](const auto &dirs) {
            if (m_loggingFlags && SyncthingConnectionLoggingFlags::DirsOrDevsResetted) {
                std::cerr << Phrases::Info << "Folder list renewed:" << Phrases::End;
                std::cerr << displayNames(dirs).join(QStringLiteral(", ")).toStdString() << endl;
            }
        });
        QObject::connect(this, &SyncthingConnection::newDevices, [this](const auto &devs) {
            if (m_loggingFlags && SyncthingConnectionLoggingFlags::DirsOrDevsResetted) {
                std::cerr << Phrases::Info << "Device list renewed:" << Phrases::End;
                std::cerr << displayNames(devs).join(QStringLiteral(", ")).toStdString() << endl;
            }
        });
        m_loggingFlagsHandler |= SyncthingConnectionLoggingFlags::DirsOrDevsResetted;
    }
}

/*!
 * \brief Cancels \a reply without considering the connection aborted.
 * \remarks Setting \a reply back to nullptr before aborting it avoids the usual cancellation handler to be invoked.
 */
static inline void cancelReplyWithoutAbortingConnection(QNetworkReply *&reply)
{
    if (reply) {
        std::exchange(reply, nullptr)->abort();
    }
}

/*!
 * \brief Ensure the request with the specified \a timer, \a pendingReply and \a requestFunction is enabled or disabled depending on \a enable.
 * \remarks This function is only supposed to be called if \a enabled has actually changed.
 */
static inline void manageTimerBasedRequest(
    QTimer &timer, QNetworkReply *pendingReply, SyncthingConnection &connection, void (SyncthingConnection::*requestFunction)(void), bool enable)
{
    // stop any possibly active timer if the polling-flag has been disabled (stopping a pending request would be possible but not gain us anything)
    if (!enable) {
        timer.stop();
        return;
    }

    // make a request immediately (unless there's already a pending reply) if the polling-flag has been enabled and a non-zero polling interval is configured
    if (timer.interval() && !pendingReply) {
        timer.stop();
        std::invoke(requestFunction, connection);
    }
}

/*!
 * \brief Returns whether flags matching the specified \a mask have been changed between \a flags1 and \a flags2.
 */
static inline bool haveFlagsChanged(
    SyncthingConnection::PollingFlags flags1, SyncthingConnection::PollingFlags flags2, SyncthingConnection::PollingFlags mask)
{
    return (flags1 & mask) != (flags2 & mask);
}

/*!
 * \brief Sets what kind of events are polled for.
 * \remarks
 * - Call this function to reduce CPU usage in case not all events are needed right now, e.g. remove PollingFlags::DiskEvents
 *   if the fileChanged() signal is not used anyway.
 * - Restarts pending requests as necessary.
 */
void SyncthingConnection::setPollingFlags(PollingFlags flags)
{
    if (flags == m_pollingFlags) {
        return;
    }

    // track what flags have changed and set new flags
    const auto normalEventsChanged = haveFlagsChanged(flags, m_pollingFlags, PollingFlags::NormalEvents);
    const auto diskEventsChanged = haveFlagsChanged(flags, m_pollingFlags, PollingFlags::DiskEvents);
    const auto trafficStatsChanged = haveFlagsChanged(flags, m_pollingFlags, PollingFlags::TrafficStatistics);
    const auto devStatsChanged = haveFlagsChanged(flags, m_pollingFlags, PollingFlags::DeviceStatistics);
    const auto errorsChanged = haveFlagsChanged(flags, m_pollingFlags, PollingFlags::Errors);
    m_pollingFlags = flags;

    // restart events/disk-events request as necessary
    if (normalEventsChanged) {
        if (!m_eventMask.isEmpty()) {
            m_lastEventIdByMask.insert(m_eventMask, m_lastEventId);
            m_eventMask.clear();
        }
        cancelReplyWithoutAbortingConnection(m_eventsReply);
    }
    if (diskEventsChanged) {
        cancelReplyWithoutAbortingConnection(m_diskEventsReply);
    }
    if (m_hasConfig && m_hasStatus && m_keepPolling) {
        requestEvents();
        requestDiskEvents(m_diskEventLimit);
    }

    // manage timers/requests for timer-based requests
    if (trafficStatsChanged) {
        manageTimerBasedRequest(m_trafficPollTimer, m_connectionsReply, *this, &SyncthingConnection::requestConnections,
            m_keepPolling && (m_pollingFlags && PollingFlags::TrafficStatistics));
    }
    if (devStatsChanged) {
        manageTimerBasedRequest(m_devStatsPollTimer, m_devStatsReply, *this, &SyncthingConnection::requestDeviceStatistics,
            m_keepPolling && (m_pollingFlags && PollingFlags::DeviceStatistics));
    }
    if (errorsChanged) {
        manageTimerBasedRequest(
            m_errorsPollTimer, m_errorsReply, *this, &SyncthingConnection::requestErrors, m_keepPolling && (m_pollingFlags && PollingFlags::Errors));
    }
}

/*!
 * \brief Returns whether there is at least one directory out-of-sync.
 */
bool SyncthingConnection::hasOutOfSyncDirs() const
{
    if (m_hasOutOfSyncDirs.has_value()) {
        return m_hasOutOfSyncDirs.value();
    }
    for (const SyncthingDir &dir : m_dirs) {
        if (!dir.paused && dir.status == SyncthingDirStatus::OutOfSync) {
            return m_hasOutOfSyncDirs.emplace(true);
        }
    }
    return m_hasOutOfSyncDirs.emplace(false);
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
 * \brief Sets whether to pause all devices on metered connections.
 */
void SyncthingConnection::setPausingOnMeteredConnection(bool pausingOnMeteredConnection)
{
    if (m_pausingOnMeteredConnection != pausingOnMeteredConnection) {
        if ((m_pausingOnMeteredConnection = pausingOnMeteredConnection)) {
            // initialize handling of metered connections
#ifdef SYNCTHINGCONNECTION_SUPPORT_METERED
            if (!m_handlingMeteredConnectionInitialized) {
                if (const auto [networkInformation, isMetered] = loadNetworkInformationBackendForMetered(); networkInformation) {
                    QObject::connect(networkInformation, &QNetworkInformation::isMeteredChanged, this, &SyncthingConnection::handleMeteredConnection);
                }
            }
#endif
        }
        if (m_hasConfig) {
            handleMeteredConnection();
        }
    }
}

/*!
 * \brief Substitutes the tilde as first element in \a path using current values of tilde() and pathSeparator().
 */
QString SyncthingConnection::substituteTilde(const QString &path) const
{
    return Data::substituteTilde(path, m_tilde, m_pathSeparator);
}

/*!
 * \brief Ensures that devices are paused/resumed depending on whether the network connection is metered.
 */
void SyncthingConnection::handleMeteredConnection()
{
#ifdef SYNCTHINGCONNECTION_SUPPORT_METERED
    const auto *const networkInformation = QNetworkInformation::instance();
    if (!networkInformation || !networkInformation->supports(QNetworkInformation::Feature::Metered)) {
        return;
    }
    if (networkInformation->isMetered() && m_pausingOnMeteredConnection) {
        auto hasDevicesToPause = false;
        m_devsPausedDueToMeteredConnection.reserve(static_cast<qsizetype>(m_devs.size()));
        for (const auto &device : m_devs) {
            if (!device.paused && device.status != SyncthingDevStatus::ThisDevice) {
                if (!m_devsPausedDueToMeteredConnection.contains(device.id)) {
                    m_devsPausedDueToMeteredConnection << device.id;
                    hasDevicesToPause = true;
                }
            }
        }
        if (hasDevicesToPause) {
            pauseResumeDevice(m_devsPausedDueToMeteredConnection, true, true);
        }
    } else {
        pauseResumeDevice(m_devsPausedDueToMeteredConnection, false, true);
        m_devsPausedDueToMeteredConnection.clear();
    }
#endif
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

    // abort pending requests before connecting
    if (hasPendingRequests()) {
        m_keepPolling = m_abortingToConnect = true;
        abortAllRequests();
        return;
    }

    // reset status
    m_connectionAborted = m_abortingToConnect = m_abortingToReconnect = m_hasConfig = m_hasStatus = m_hasEvents = m_hasDiskEvents = m_statsRequested
        = false;

    if (!checkConnectionConfiguration()) {
        return;
    }
    requestConfig();
    requestStatus();
    m_keepPolling = true;
}

/*!
 * \brief Returns whether the connection configuration is sufficient and sets the connection into the disconnected state if not.
 */
bool SyncthingConnection::checkConnectionConfiguration()
{
    if (!m_apiKey.isEmpty() && !m_syncthingUrl.isEmpty()) {
        return true;
    }
    emit error(tr("Connection configuration is insufficient."), SyncthingErrorCategory::OverallConnection, QNetworkReply::NoError);
    setStatus(SyncthingStatus::Disconnected);
    return false;
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
 * \remarks Does nothing if the connection attempt would happen anyway though auto-reconnect.
 */
void SyncthingConnection::connectLater(int milliSeconds)
{
    // skip if connecting via auto-reconnect anyway
    if (m_autoReconnectTimer.isActive() && milliSeconds > m_autoReconnectTimer.interval()) {
        return;
    }
    QTimer::singleShot(milliSeconds, this, static_cast<void (SyncthingConnection::*)(void)>(&SyncthingConnection::connect));
}

/*!
 * \brief Disconnects. That means all (long) polling is stopped and ongoing requests are aborted via abortAllRequests().
 */
void SyncthingConnection::disconnect()
{
    m_abortingToConnect = m_abortingToReconnect = m_keepPolling = false;
    m_statusRecomputationFlags = StatusRecomputation::None;
    m_trafficPollTimer.stop();
    m_devStatsPollTimer.stop();
    m_errorsPollTimer.stop();
    m_autoReconnectTimer.stop();
    m_autoReconnectTries = 0;
    abortAllRequests();
}

/*!
 * \brief Aborts the specified \a reply if it is not nullptr.
 */
static inline void abortMaybe(QNetworkReply *reply)
{
    if (reply) {
        reply->abort();
    }
}

/*!
 * \brief Aborts status-relevant, pending requests.
 * \remarks Status-relevant means that requests for triggering actions like rescan() or restart() are excluded. requestQrCode() does not
 *          contribute to the status as well and is excluded as well.
 */
void SyncthingConnection::abortAllRequests()
{
    m_connectionAborted = m_abortingAllRequests = true;
    abortMaybe(m_configReply);
    abortMaybe(m_statusReply);
    abortMaybe(m_connectionsReply);
    abortMaybe(m_errorsReply);
    abortMaybe(m_dirStatsReply);
    abortMaybe(m_devStatsReply);
    abortMaybe(m_eventsReply);
    abortMaybe(m_versionReply);
    abortMaybe(m_diskEventsReply);
    abortMaybe(m_logReply);
    abortMaybe(m_configReply);
    abortMaybe(m_configReply);
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
    m_hasConfig = m_hasStatus = m_hasEvents = m_hasDiskEvents = m_statsRequested = false;

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
    // notify that we're about to invalidate the configuration if not already invalidated anyway
    const auto isConfigInvalidated = m_rawConfig.isEmpty();
    if (!isConfigInvalidated) {
        emit newConfig(m_rawConfig = QJsonObject());
    }

    // cleanup information from previous connection
    m_keepPolling = true;
    m_statusRecomputationFlags = StatusRecomputation::None;
    m_connectionAborted = false;
    m_abortingToConnect = m_abortingToReconnect = false;
    m_lastEventId = 0;
    m_lastDiskEventId = 0;
    m_lastEventIdByMask.clear();
    m_configDir.clear();
    m_myId.clear();
    m_tilde.clear();
    m_pathSeparator.clear();
    m_totalIncomingTraffic = unknownTraffic;
    m_totalOutgoingTraffic = unknownTraffic;
    m_totalIncomingRate = 0.0;
    m_totalOutgoingRate = 0.0;
    emit trafficChanged(unknownTraffic, unknownTraffic);
    m_hasOutOfSyncDirs.reset();
    m_hasConfig = false;
    m_hasStatus = false;
    m_hasEvents = false;
    m_hasDiskEvents = false;
    m_statsRequested = false;
    m_dirs.clear();
    m_devs.clear();
    m_errors.clear();
    m_devsPausedDueToMeteredConnection.clear();
    m_lastConnectionsUpdateEvent = 0;
    m_lastConnectionsUpdateTime = DateTime();
    m_lastFileEvent = 0;
    m_lastFileTime = DateTime();
    m_lastErrorTime = DateTime();
    m_startTime = DateTime();
    m_lastFileName.clear();
    m_lastFileDeleted = false;
    m_syncthingVersion.clear();
    emit dirStatisticsChanged();

    // notify that the state/configuration has been invalidated
    emit hasStateChanged();
    if (!isConfigInvalidated) {
        emit newConfigApplied();
    }

    if (!checkConnectionConfiguration()) {
        return;
    }
    requestConfig();
    requestStatus();
    setStatus(SyncthingStatus::Reconnecting);
}

/*!
 * \brief Reads devs and dirs from the raw config.
 */
void SyncthingConnection::applyRawConfig()
{
    readDevs(m_rawConfig.value(QLatin1String("devices")).toArray());
    readDirs(m_rawConfig.value(QLatin1String("folders")).toArray());
    emit newConfigApplied();
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

    applyRawConfig();
    continueConnecting();
}

/*!
 * \brief Sets the status from (re)connecting to Syncthing's actual state if polling but there are no more pending requests.
 * \remarks
 * - Called by read...() handlers for requests started in continueConnecting().
 * - The flags are used to decide whether the status should be recomputed (as not all read...() handlers require a recomputation).
 * \sa hasPendingRequests()
 */
void SyncthingConnection::concludeConnection(StatusRecomputation flags)
{
    if (!m_keepPolling) {
        return;
    }

    // take always record of the specified flags but return early if there are still pending requests or the status does not need to be recomputed
    m_statusRecomputationFlags += flags;
    if (hasPendingRequests() || (m_statusRecomputationFlags == StatusRecomputation::None && isConnected())) {
        return;
    }

    // recompute status and emit events according to flags
    if (!setStatus(SyncthingStatus::Idle) && (m_statusRecomputationFlags && StatusRecomputation::OutOfSyncDirs) && !m_hasOutOfSyncDirs.has_value()) {
        emit hasOutOfSyncDirsChanged();
    }
    if (m_statusRecomputationFlags && StatusRecomputation::DirStats) {
        emit dirStatisticsChanged();
    }
    if (m_statusRecomputationFlags && StatusRecomputation::RemoteCompletion) {
        emit devCompletionChanged();
    }

    m_statusRecomputationFlags = StatusRecomputation::None;
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
 * \brief Returns the directory info object for the directory with the specified ID or label.
 * \returns Returns a pointer to the object or nullptr if not found.
 * \remarks
 * - IDs have precedence, labels are checked as fallback.
 * - The returned object becomes invalid when the newDirs() signal is emitted or the connection is destroyed.
 */
SyncthingDir *SyncthingConnection::findDirInfoConsideringLabels(const QString &dirIdOrLabel, int &row)
{
    if (auto *const dir = findDirInfo(dirIdOrLabel, row)) {
        return dir;
    }
    row = 0;
    for (SyncthingDir &d : m_dirs) {
        if (d.label == dirIdOrLabel) {
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
    const auto cleanPath = QDir::cleanPath(path);
    for (SyncthingDir &dir : m_dirs) {
        const auto dirCleanPath = QDir::cleanPath(dir.path);
        if (cleanPath == dirCleanPath) {
            relativePath.clear();
            return &dir;
        } else if (cleanPath.startsWith(dirCleanPath)) {
            relativePath = cleanPath.mid(dirCleanPath.size());
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
    auto row = int();
    if (auto *const existingDirInfo = findDirInfo(dirId, row)) {
        return &dirs.emplace_back(std::move(*existingDirInfo));
    } else {
        return &dirs.emplace_back(dirId);
    }
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
 * \brief Returns the full path for \a relativePath within the directory specified by \a dirId.
 * \remarks Returns an empty string if the directory doesn't exist.
 */
QString SyncthingConnection::fullPath(const QString &dirId, const QString &relativePath) const
{
    auto row = int();
    auto dirInfo = findDirInfo(dirId, row);
    return dirInfo ? (dirInfo->path % QChar('/') % relativePath) : QString();
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
            return dev.displayName();
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
 * \brief Returns whether the web-based GUI requires authentication.
 */
bool SyncthingConnection::isGuiRequiringAuth() const
{
    return !m_rawConfig.value(QLatin1String("gui")).toObject().value(QLatin1String("user")).toString().isEmpty();
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
    auto row = int();
    if (SyncthingDev *const existingDevInfo = findDevInfo(devId, row)) {
        return &devs.emplace_back(std::move(*existingDevInfo));
    } else {
        return &devs.emplace_back(devId);
    }
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
    requestErrors();
    requestVersion();
    for (const SyncthingDir &dir : m_dirs) {
        if (!m_requestCompletion || dir.paused) {
            continue;
        }
        for (const QString &devId : dir.deviceIds) {
            requestCompletion(devId, dir.id);
        }
    }

    // poll for events according to polling flags
    requestEvents();
    requestDiskEvents(m_diskEventLimit);
}

#ifndef QT_NO_SSL
/*!
 * \brief Locates and loads the (self-signed) certificate used by the Syncthing GUI.
 * \remarks
 *  - Ensures any previous certificates are cleared in any case.
 *  - Emits error() when an error occurs.
 *  - Loading the certificate is only possible if the connection object is configured
 *    to connect to the locally running Syncthing instance. Otherwise this method will
 *    only do the cleanup of previous certificates but not emit any errors.
 *  - This function uses m_certificatePath which is set by applySettings() if the user
 *    specified a certificate path manually. Otherwise the path is detected automatically
 *    and stored in m_dynamicallyDeterminedCertificatePath so the certificate path is
 *    known in handleSslErrors().
 * \returns Returns whether a certificate could be loaded.
 */
bool SyncthingConnection::loadSelfSignedCertificate(const QUrl &url)
{
    // ensure current exceptions for self-signed certificates are cleared
    m_expectedSslErrors.clear();

    // not required when not using secure connection
    const auto syncthingUrl = url.isEmpty() ? m_syncthingUrl : url;
    if (!syncthingUrl.scheme().endsWith(QChar('s'))) {
        if (m_loggingFlags && SyncthingConnectionLoggingFlags::CertLoading) {
            std::cerr << Phrases::Info << "Not loading self-signed certificate as URL scheme doesn't end with 's'." << Phrases::End;
        }
        return false;
    }

    // auto-determining the path is only possible if the Syncthing instance is running locally
    if (m_certificatePath.isEmpty() && !::Data::isLocal(syncthingUrl)) {
        if (m_loggingFlags && SyncthingConnectionLoggingFlags::CertLoading) {
            std::cerr << Phrases::Info << "Not auto-loading self-signed certificate as URL is not considered local." << Phrases::End;
        }
        return false;
    }

    // find cert
    const auto certPath = !m_certificatePath.isEmpty()
        ? m_certificatePath
        : (!m_configDir.isEmpty() ? (m_configDir + QStringLiteral("/https-cert.pem")) : SyncthingConfig::locateHttpsCertificate());
    if (certPath.isEmpty()) {
        if (m_loggingFlags && SyncthingConnectionLoggingFlags::CertLoading) {
            std::cerr << Phrases::Error << "Unable to locate self-signed certificate." << Phrases::End;
        }
        emit error(tr("Unable to locate certificate used by Syncthing."), SyncthingErrorCategory::OverallConnection, QNetworkReply::NoError);
        return false;
    }
    if (m_loggingFlags && SyncthingConnectionLoggingFlags::CertLoading) {
        const auto pathSpec = m_certificatePath.isEmpty() ? "auto-detected" : "manually specified";
        std::cerr << Phrases::Info << "Loading self-signed certificate from " << pathSpec << " path: " << certPath.toStdString() << Phrases::End;
    }
    // add exception
    const auto certs = QSslCertificate::fromPath(certPath);
    if (certs.isEmpty() || certs.at(0).isNull()) {
        if (m_loggingFlags && SyncthingConnectionLoggingFlags::CertLoading) {
            std::cerr << Phrases::Error << "Unable to load self-signed certificate." << Phrases::End;
        }
        emit error(tr("Unable to load certificate used by Syncthing."), SyncthingErrorCategory::OverallConnection, QNetworkReply::NoError);
        return false;
    }
    m_expectedSslErrors = SyncthingConnectionSettings::compileSslErrors(certs.at(0));
    // keep track of the dynamically determined certificate path for handleSslErrors()
    if (m_certificatePath.isEmpty()) {
        m_dynamicallyDeterminedCertificatePath = certPath;
        m_certificateLastModified = QFileInfo(certPath).lastModified();
    }
    return true;
}

/*!
 * \brief Clears the self-signed certificate that might be loaded via loadSelfSignedCertificate().
 * \remarks This function mainly exists to ease testing; one normally doesn't need to invoke it explicitly.
 */
void SyncthingConnection::clearSelfSignedCertificate()
{
    m_expectedSslErrors.clear();
    m_certificatePath.clear();
    m_dynamicallyDeterminedCertificatePath.clear();
    m_certificateLastModified = QDateTime();
}
#endif

/*!
 * \brief Applies the specified configuration.
 * \remarks
 * - The expected SSL errors are taken from the specified \a connectionSettings. If empty, this
 *   function attempts to load expected SSL errors automatically as needed/possible via
 *   loadSelfSignedCertificate(). It then writes back those SSL errors to \a connectionSettings.
 *   This way \a connectionSettings can act as a cache for SSL exceptions.
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
    if (localPath() != connectionSettings.localPath) {
        setLocalPath(connectionSettings.localPath);
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
#ifndef QT_NO_SSL
    m_certificatePath = connectionSettings.httpsCertPath;
    m_certificateLastModified = connectionSettings.httpCertLastModified;
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
#endif

    setTrafficPollInterval(connectionSettings.trafficPollInterval);
    setDevStatsPollInterval(connectionSettings.devStatsPollInterval);
    setErrorsPollInterval(connectionSettings.errorsPollInterval);
    setAutoReconnectInterval(connectionSettings.reconnectInterval);
    setRequestTimeout(connectionSettings.requestTimeout);
    setLongPollingTimeout(connectionSettings.longPollingTimeout);
    setDiskEventLimit(connectionSettings.diskEventLimit);
    setStatusComputionFlags(connectionSettings.statusComputionFlags);
    setPausingOnMeteredConnection(connectionSettings.pauseOnMeteredConnection);

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
 * 2. SyncthingStatus::RemoteNotInSync
 * 3. SyncthingStatus::Scanning
 * 4. SyncthingStatus::Paused
 * 5. SyncthingStatus::NoRemoteConnected
 * 6. SyncthingStatus::Idle
 *
 * \remarks
 * - The "out-of-sync" status is (currently) *not* handled by this function. One needs to query this via
 *   the SyncthingConnection::hasOutOfSyncDirs() function.
 * - Whether notifications are available is *not* handled by this function. One needs to query this via
 *   SyncthingConnection::hasUnreadNotifications().
 * \returns Returns whether the status has been changed; the the status remained the same as before false
 *          is returned. Returns always true if the connection is being destroyed.
 */
bool SyncthingConnection::setStatus(SyncthingStatus status)
{
    if (m_status == SyncthingStatus::BeingDestroyed) {
        return true;
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
        auto scanning = false, synchronizing = false, remoteSynchronizing = false, noRemoteConnected = true, devPaused = false;
        if (m_statusComputionFlags && (SyncthingStatusComputionFlags::Synchronizing | SyncthingStatusComputionFlags::Scanning)) {
            for (const SyncthingDir &dir : m_dirs) {
                switch (dir.status) {
                case SyncthingDirStatus::WaitingToSync:
                case SyncthingDirStatus::PreparingToSync:
                case SyncthingDirStatus::Synchronizing:
                    synchronizing = m_statusComputionFlags && SyncthingStatusComputionFlags::Synchronizing;
                    break;
                case SyncthingDirStatus::WaitingToScan:
                case SyncthingDirStatus::Scanning:
                    scanning = m_statusComputionFlags && SyncthingStatusComputionFlags::Scanning;
                    break;
                default:;
                }
                if (synchronizing) {
                    break; // skip remaining dirs, "synchronizing" overrides "scanning" anyway
                }
            }
        }

        // check whether at least one device is synchronizing
        // check whether at least one device is paused
        // check whether at least one devices is connected
        if (!synchronizing
            && (m_statusComputionFlags
                && (SyncthingStatusComputionFlags::RemoteSynchronizing | SyncthingStatusComputionFlags::NoRemoteConnected
                    | SyncthingStatusComputionFlags::DevicePaused))) {
            for (const SyncthingDev &dev : m_devs) {
                if (dev.status == SyncthingDevStatus::Synchronizing) {
                    remoteSynchronizing = true;
                    if (m_statusComputionFlags && SyncthingStatusComputionFlags::RemoteSynchronizing) {
                        break;
                    }
                }
                if (dev.isConnected()) {
                    noRemoteConnected = false;
                }
                if (dev.paused && dev.status != SyncthingDevStatus::ThisDevice) {
                    devPaused = true;
                }
            }
        }

        if (synchronizing) {
            status = SyncthingStatus::Synchronizing;
        } else if ((m_statusComputionFlags && SyncthingStatusComputionFlags::RemoteSynchronizing) && remoteSynchronizing) {
            status = SyncthingStatus::RemoteNotInSync;
        } else if (scanning) {
            status = SyncthingStatus::Scanning;
        } else if ((m_statusComputionFlags && SyncthingStatusComputionFlags::DevicePaused) && devPaused) {
            status = SyncthingStatus::Paused;
        } else if ((m_statusComputionFlags && SyncthingStatusComputionFlags::NoRemoteConnected) && noRemoteConnected) {
            status = SyncthingStatus::NoRemoteConnected;
        }
    }
    const auto hasStatusChanged = m_status != status || status == SyncthingStatus::Disconnected;
    if (hasStatusChanged) {
        // emit event if status changed always for disconnects so isConnecting() is re-evaluated
        emit statusChanged(m_status = status);
    }
    return hasStatusChanged;
}

/*!
 * \brief Internally called to emit a JSON parsing error.
 * \remarks Since in this case the reply has already been read, its response must be passed as extra argument.
 */
void SyncthingConnection::emitError(const QString &message, const QJsonParseError &jsonError, QNetworkReply *reply, const QByteArray &response)
{
    if (loggingFlags() && SyncthingConnectionLoggingFlags::ApiReplies) {
        std::cerr << Phrases::Error << "JSON parsing error: " << message.toLocal8Bit().data() << jsonError.errorString().toLocal8Bit().data()
                  << " (at offset " << jsonError.offset << ')' << Phrases::End;
    }
    emit error(message % jsonError.errorString() % QChar(' ') % QChar('(') % tr("at offset %1").arg(jsonError.offset) % QChar(')'),
        SyncthingErrorCategory::Parsing, QNetworkReply::NoError, reply->request(), response);
}

/*!
 * \brief Internally called to emit a network error (server replied error code or server could not be reached at all).
 */
void SyncthingConnection::emitError(const QString &message, SyncthingErrorCategory category, QNetworkReply *reply)
{
    if (loggingFlags() && SyncthingConnectionLoggingFlags::ApiReplies) {
        std::cerr << Phrases::Error << "Syncthing connection error: " << message.toLocal8Bit().data() << reply->errorString().toLocal8Bit().data()
                  << Phrases::End;
    }
    emit error(message + reply->errorString(), category, reply->error(), reply->request(), reply->bytesAvailable() ? reply->readAll() : QByteArray());

    // request errors immediately after a failed API request so errors like "Decoding posted config: folder has empty ID" show up immediately
    if (category == SyncthingErrorCategory::SpecificRequest && m_errorsPollTimer.isActive()) {
        requestErrors();
        m_errorsPollTimer.start(); // this stops and restarts the active timer to reset the remaining time
    }
}

/*!
 * \brief Internally called to emit a network error for a specific request (server replied error code or server could not be reached at all).
 * \remarks The \a message is supposed to already contain the error string of the reply.
 */
void SyncthingConnection::emitError(const QString &message, QNetworkReply *reply)
{
    if (loggingFlags() && SyncthingConnectionLoggingFlags::ApiReplies) {
        std::cerr << Phrases::Error << "Syncthing API error: " << message.toLocal8Bit().data() << reply->errorString().toLocal8Bit().data()
                  << Phrases::End;
    }
    emit error(message, SyncthingErrorCategory::SpecificRequest, reply->error(), reply->request(),
        reply->bytesAvailable() ? reply->readAll() : QByteArray());
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
            if (dev.status != SyncthingDevStatus::ThisDevice) {
                dev.status = SyncthingDevStatus::ThisDevice;
                dev.paused = false;
                emit devStatusChanged(dev, row);
            }
        } else if (dev.status == SyncthingDevStatus::ThisDevice) {
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
 * \brief Internally called to handle a fatal error when reading config (dirs/devs), status and events.
 */
void SyncthingConnection::handleFatalConnectionError()
{
    // start the timer before emitting the event so its active state can be observed in event handler
    if (m_autoReconnectTimer.interval() && !m_autoReconnectTimer.isActive()) {
        m_autoReconnectTimer.start();
    }
    setStatus(SyncthingStatus::Disconnected);
    abortAllRequests();
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
        // if reconnect-flag is set, instantly etstablish a new connection via reconnect()
        continueReconnecting();
    } else if (m_abortingToConnect) {
        // if connect-flag is set, instantly establish a new connection via connect()
        connect();
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
 * \brief Invalidates whether there are currently out-of-sync dirs and provokes subscribers to the
 *        hasOutOfSyncDirsChanged() event to re-evaluate it.
 */
void SyncthingConnection::invalidateHasOutOfSyncDirs()
{
    if (m_hasOutOfSyncDirs.has_value()) {
        m_hasOutOfSyncDirs.reset();
        emit hasOutOfSyncDirsChanged();
    }
}

/*!
 * \brief Returns syncthingUrl() with userName() and password().
 */
QUrl SyncthingConnection::makeUrlWithCredentials() const
{
    auto url = QUrl(m_syncthingUrl);
    url.setUserName(m_user);
    url.setPassword(m_password);
    return url;
}

/*!
 * \brief Computes the overall completion of all connected devices.
 */
SyncthingCompletion SyncthingConnection::computeOverallRemoteCompletion() const
{
    auto completion = SyncthingCompletion();
    for (const auto &dev : m_devs) {
        if (dev.isConnected()) {
            completion += dev.overallCompletion;
        }
    }
    completion.recomputePercentage();
    return completion;
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
 * \fn SyncthingConnection::allEventsProcessed()
 * \brief Indicates all new events have been processed.
 * \remarks
 * This event is emitted after newEvents(), dirStatusChanged(), devStatusChanged() and other specific events.
 * If you would go through the list of all directories on every dirStatusChanged() event then using allEventsProcessed()
 * instead might be a more efficient alternative. Just set a flag on dirStatusChanged() and go though the list of
 * directories only once on the allEventsProcessed() event when the flag has been set.
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

/*!
 * \fn SyncthingConnection::hasOutOfSyncDirsChanged()
 * \brief Indicates that hasOutOfSyncDirs() might has changed.
 * \remarks
 * - This signal is only emitted if hasOutOfSyncDirs() has been called at least once
 *   since the connection has been established.
 * - This signal is *not* emitted if the status has changed at the same time. Then only
 *   statusChanged() is emitted. This is so that UI components subscribed to the event
 *   are only updated once (as they will also be subscribed to statusChanged()).
 */

} // namespace Data
