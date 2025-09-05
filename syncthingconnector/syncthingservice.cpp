#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD

#include "./syncthingservice.h"
#include "./utils.h"

#include "loginmanagerinterface.h"
#include "managerinterface.h"
#include "propertiesinterface.h"
#include "serviceinterface.h"
#include "unitinterface.h"

#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusMetaType>
#include <QDBusObjectPath>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDBusServiceWatcher>

#ifdef SYNCTHINGCONNECTION_SUPPORT_METERED
#include <QNetworkInformation>
#endif

#include <functional>

using namespace std;
using namespace std::placeholders;
using namespace CppUtilities;

namespace Data {

/*!
 * \class SyncthingService
 * \brief The SyncthingService class controls and monitors a Syncthing as systemd user service.
 * \remarks Internally systemd's D-Bus interface is used. So the service "org.freedesktop.systemd1" must
 *          be running on the user-session D-Bus.
 *
 * This class is actually not Syncthing-specific. It could be used to control and monitor any systemd
 * user service.
 */

/// \cond

QDBusArgument &operator<<(QDBusArgument &argument, const ManagerDBusUnitFileChange &unitFileChange)
{
    argument.beginStructure();
    argument << unitFileChange.type << unitFileChange.path << unitFileChange.source;
    argument.endStructure();
    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, ManagerDBusUnitFileChange &unitFileChange)
{
    argument.beginStructure();
    argument >> unitFileChange.type >> unitFileChange.path >> unitFileChange.source;
    argument.endStructure();
    return argument;
}

constexpr DateTime dateTimeFromSystemdTimeStamp(qulonglong timeStamp)
{
    return DateTime(DateTime::unixEpochStart().totalTicks() + timeStamp * 10);
}

SyncthingService *SyncthingService::s_mainInstance = nullptr;
OrgFreedesktopSystemd1ManagerInterface *SyncthingService::s_systemdUserInterface = nullptr;
OrgFreedesktopSystemd1ManagerInterface *SyncthingService::s_systemdSystemInterface = nullptr;
OrgFreedesktopLogin1ManagerInterface *SyncthingService::s_loginManager = nullptr;
DateTime SyncthingService::s_lastWakeUp = DateTime();
bool SyncthingService::s_fallingAsleep = false;

/// \endcond

/*!
 * \brief Creates a new SyncthingService instance.
 */
SyncthingService::SyncthingService(SystemdScope scope, QObject *parent)
    : QObject(parent)
    , m_unit(nullptr)
    , m_service(nullptr)
    , m_properties(nullptr)
    , m_currentSystemdInterface(nullptr)
    , m_scope(scope)
    , m_manuallyStopped(false)
    , m_stoppedMetered(false)
    , m_unitAvailable(false)
    , m_stopOnMeteredConnection(false)
{
    setupFreedesktopLoginInterface();

#ifdef LIB_SYNCTHING_CONNECTOR_SERVICE_MOCKED
    // let the mocked service initially be stopped and simulate start after 5 seconds, then stop after 10 seconds and start after 15 seconds
    QTimer::singleShot(5000, this, [this] {
        m_activeSince = DateTime::gmtNow() - TimeSpan::fromMilliseconds(250);
        handlePropertiesChanged(QStringLiteral("syncthing.mocked-service"),
            QVariantMap{ { QStringLiteral("ActiveState"), QStringLiteral("active") }, { QStringLiteral("SubState"), QStringLiteral("running") },
                { QStringLiteral("Description"), QStringLiteral("This service is fake.") } },
            QStringList());
    });
    QTimer::singleShot(10000, this, [this] {
        m_activeSince = DateTime();
        handlePropertiesChanged(QStringLiteral("syncthing.mocked-service"),
            QVariantMap{ { QStringLiteral("ActiveState"), QStringLiteral("inactive") }, { QStringLiteral("SubState"), QStringLiteral("dead") },
                { QStringLiteral("Description"), QStringLiteral("This service is still fake.") } },
            QStringList());
    });
    QTimer::singleShot(15000, this, [this] {
        m_activeSince = DateTime::gmtNow();
        handlePropertiesChanged(QStringLiteral("syncthing.mocked-service"),
            QVariantMap{ { QStringLiteral("ActiveState"), QStringLiteral("active") }, { QStringLiteral("SubState"), QStringLiteral("running") },
                { QStringLiteral("Description"), QStringLiteral("This service is alive again!") } },
            QStringList());
    });
#endif

    // initialize handling of metered connections
#ifdef SYNCTHINGCONNECTION_SUPPORT_METERED
    if (const auto [networkInformation, isInitiallyMetered] = loadNetworkInformationBackendForMetered(true); networkInformation) {
        connect(networkInformation, &QNetworkInformation::isMeteredChanged, this, [this](bool isMetered) { setNetworkConnectionMetered(isMetered); });
        setNetworkConnectionMetered(isInitiallyMetered);
    }
#endif
}

/*!
 * \brief Initializes m_currentSystemdInterface and its connection and service watcher for the current m_scope.
 */
void SyncthingService::setupSystemdInterface()
{
#ifndef LIB_SYNCTHING_CONNECTOR_SERVICE_MOCKED
    clearSystemdInterface();

    // ensure the static systemd interface for the current scope is initialized
    const auto isUserScope = m_scope == SystemdScope::User;
    OrgFreedesktopSystemd1ManagerInterface *&staticSystemdInterface = isUserScope ? s_systemdUserInterface : s_systemdSystemInterface;
    if (!staticSystemdInterface) {
        // register custom data types
        qDBusRegisterMetaType<ManagerDBusUnitFileChange>();
        qDBusRegisterMetaType<ManagerDBusUnitFileChangeList>();

        staticSystemdInterface = new OrgFreedesktopSystemd1ManagerInterface(QStringLiteral("org.freedesktop.systemd1"),
            QStringLiteral("/org/freedesktop/systemd1"), isUserScope ? QDBusConnection::sessionBus() : QDBusConnection::systemBus());

        // enable systemd to emit signals
        staticSystemdInterface->Subscribe();
    }

    // use the static systemd interface for the current scope
    m_currentSystemdInterface = staticSystemdInterface;
    connect(m_currentSystemdInterface, &OrgFreedesktopSystemd1ManagerInterface::UnitNew, this, &SyncthingService::handleUnitAdded);
    connect(m_currentSystemdInterface, &OrgFreedesktopSystemd1ManagerInterface::UnitRemoved, this, &SyncthingService::handleUnitRemoved);
    connect(m_currentSystemdInterface, &OrgFreedesktopSystemd1ManagerInterface::Reloading, this, &SyncthingService::handleReloading);
    m_serviceWatcher = new QDBusServiceWatcher(m_currentSystemdInterface->service(), m_currentSystemdInterface->connection());
    connect(m_serviceWatcher, &QDBusServiceWatcher::serviceRegistered, this, &SyncthingService::handleServiceRegisteredChanged);
    connect(m_serviceWatcher, &QDBusServiceWatcher::serviceUnregistered, this, &SyncthingService::handleServiceRegisteredChanged);
#endif
}

/*!
 * \brief Initializes s_loginManager.
 */
void SyncthingService::setupFreedesktopLoginInterface()
{
#ifndef LIB_SYNCTHING_CONNECTOR_SERVICE_MOCKED
    // ensure the static login interface is initialized and use it
    if (s_loginManager) {
        return;
    }
    s_loginManager = new OrgFreedesktopLogin1ManagerInterface(
        QStringLiteral("org.freedesktop.login1"), QStringLiteral("/org/freedesktop/login1"), QDBusConnection::systemBus());
    connect(s_loginManager, &OrgFreedesktopLogin1ManagerInterface::PrepareForSleep, &SyncthingService::handlePrepareForSleep);
#endif
}

/*!
 * \brief Registers the specified D-Bus \a call to invoke \a handler when it has been concluded.
 */
template <typename HandlerType> void SyncthingService::makeAsyncCall(const QDBusPendingCall &call, HandlerType &&handler, bool removeHandler)
{
    if (m_currentSystemdInterface && removeHandler) {
        // disconnect from unit add/removed signals because these seem to be spammed when waiting for permissions
        disconnect(m_currentSystemdInterface, &OrgFreedesktopSystemd1ManagerInterface::UnitNew, this, &SyncthingService::handleUnitAdded);
        disconnect(m_currentSystemdInterface, &OrgFreedesktopSystemd1ManagerInterface::UnitRemoved, this, &SyncthingService::handleUnitRemoved);
    }
    auto *const watcher = new QDBusPendingCallWatcher(call, this);
    m_pendingCalls.emplace(watcher);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, handler);
}

/*!
 * \brief Registers a generic error handler for the specifeid D-Bus \a call.
 */
void SyncthingService::registerErrorHandler(const QDBusPendingCall &call, const char *context, bool reload, bool removeHandler)
{
    makeAsyncCall(call, bind(&SyncthingService::handleError, this, context, _1, reload), removeHandler);
}

/*!
 * \brief Determines whether the specified \a watcher is still relevant and ensures it is being deleted later.
 */
bool SyncthingService::concludeAsyncCall(QDBusPendingCallWatcher *watcher, bool reload)
{
    watcher->deleteLater();

    const auto i = m_pendingCalls.find(watcher);
    const auto resultStillRelevant = i != m_pendingCalls.cend();
    if (resultStillRelevant) {
        m_pendingCalls.erase(i);
    }
    if (m_currentSystemdInterface) {
        if (m_pendingCalls.empty()) {
            // ensure we listen to unit add/removed signals again if there are no pending calls anymore
            connect(m_currentSystemdInterface, &OrgFreedesktopSystemd1ManagerInterface::UnitNew, this, &SyncthingService::handleUnitAdded);
            connect(m_currentSystemdInterface, &OrgFreedesktopSystemd1ManagerInterface::UnitRemoved, this, &SyncthingService::handleUnitRemoved);
        }
        if (reload && !watcher->isError()) {
            if (m_scope != SystemdScope::System) {
                // reload unit files if reload flag was set (for enable/disable to make the change immediately apparent)
                reloadAllUnitFiles();
            } else {
                // we don't have the permission; at least try to refresh the unit again
                queryUnitFromSystemdInterface();
            }
        }
    }
    return resultStillRelevant;
}

/*!
 * \brief Unties the current instance from its current systemd interface.
 */
void Data::SyncthingService::clearSystemdInterface()
{
    m_pendingCalls.clear();
    if (m_currentSystemdInterface) {
        disconnect(m_currentSystemdInterface, &OrgFreedesktopSystemd1ManagerInterface::UnitNew, this, &SyncthingService::handleUnitAdded);
        disconnect(m_currentSystemdInterface, &OrgFreedesktopSystemd1ManagerInterface::UnitRemoved, this, &SyncthingService::handleUnitRemoved);
        disconnect(m_currentSystemdInterface, &OrgFreedesktopSystemd1ManagerInterface::Reloading, this, &SyncthingService::handleReloading);
        delete m_serviceWatcher;
        m_currentSystemdInterface = nullptr;
    }
}

/*!
 * \brief Clears everything we know about the systemd unit.
 */
void SyncthingService::clearUnitData()
{
    // clean up data from previous unit
    delete m_service;
    m_service = nullptr;
    delete m_unit;
    m_unit = nullptr;
    delete m_properties;
    m_properties = nullptr;
}

/*!
 * \brief Queries m_unit from m_currentSystemdInterface.
 */
void Data::SyncthingService::queryUnitFromSystemdInterface()
{
    clearUnitData();
    setProperties(false, QString(), QString(), QString(), QString());

#ifndef LIB_SYNCTHING_CONNECTOR_SERVICE_MOCKED
    if (!m_currentSystemdInterface) {
        setupSystemdInterface();
    }
    if (!m_currentSystemdInterface->isValid()) {
        return;
    }
    makeAsyncCall(m_currentSystemdInterface->GetUnit(m_unitName), &SyncthingService::handleUnitGet);
#endif
}

/*!
 * \brief Sets the \a unitName of the systemd user service to be controlled/monitored, e.g. "syncthing.service".
 */
void SyncthingService::setUnitName(const QString &unitName)
{
    if (m_unitName == unitName) {
        return;
    }
    m_unitName = unitName;

    if (!m_unitName.isEmpty()) {
        queryUnitFromSystemdInterface();
    }
    emit unitNameChanged(unitName);
}

/*!
 * \brief Sets the \a scope and \a unitName (see scope() and unitName()).
 */
void SyncthingService::setScopeAndUnitName(SystemdScope scope, const QString &unitName)
{
    const auto scopeChanged = m_scope != scope;
    const auto unitNameChanged = m_unitName != unitName;
    if (!scopeChanged && !unitNameChanged) {
        return;
    }
    if (scopeChanged) {
        m_scope = scope;
        clearSystemdInterface();
    }
    if (unitNameChanged) {
        m_unitName = unitName;
    }
    if (!unitName.isEmpty()) {
        queryUnitFromSystemdInterface();
    }
    if (scopeChanged) {
        emit this->scopeChanged(scope);
    }
    if (unitNameChanged) {
        emit this->unitNameChanged(unitName);
    }
}

/*!
 * \brief Returns a display name for this service.
 * \remarks
 * So far this just returns "user/system … unit …" but if this class was extended to support different types of
 * services it might return different names.
 */
QString SyncthingService::displayName() const
{
    if (isUserScope()) {
        return tr("user unit \"%1\"").arg(unitName());
    } else {
        return tr("system unit \"%1\"").arg(unitName());
    }
}

/*!
 * \brief Returns whether systemd (and specifically its D-Bus interface for user services) is available.
 * \remarks The availability might not be instantly detected and may change at any time. Use the systemdAvailableChanged()
 *          to react to availability changes.
 */
bool SyncthingService::isSystemdAvailable() const
{
#ifndef LIB_SYNCTHING_CONNECTOR_SERVICE_MOCKED
    return m_currentSystemdInterface && m_currentSystemdInterface->isValid();
#else
    return true;
#endif
}

/*!
 * \brief Returns whether the unit specified with \a unitName is available.
 * \remarks
 * - The availability might not be instantly detected and may change at any time. Use the unitAvailableChanged()
 *   to react to availability changes.
 * - Unless this function returns true the other unit-related functions will return false/empty values.
 */
bool SyncthingService::isUnitAvailable() const
{
#ifndef LIB_SYNCTHING_CONNECTOR_SERVICE_MOCKED
    return m_unit && m_unit->isValid();
#else
    return true;
#endif
}

/*!
 * \brief Sets the scope the current instance is tuned to.
 */
void SyncthingService::setScope(SystemdScope scope)
{
    if (m_scope == scope) {
        return;
    }
    m_scope = scope;
    clearSystemdInterface();
    queryUnitFromSystemdInterface();
    emit scopeChanged(scope);
}

/*!
 * \brief Starts the unit if \a running is true and stops the unit if \a running is false.
 */
void SyncthingService::setRunning(bool running)
{
#ifndef LIB_SYNCTHING_CONNECTOR_SERVICE_MOCKED
    m_manuallyStopped = !running;
    m_stoppedMetered = false;
    if (!m_currentSystemdInterface) {
        setupSystemdInterface();
    }
    if (running) {
        registerErrorHandler(
            m_currentSystemdInterface->StartUnit(m_unitName, QStringLiteral("replace")), QT_TR_NOOP_UTF8("start unit"), m_activeState.isEmpty());
    } else {
        registerErrorHandler(m_currentSystemdInterface->StopUnit(m_unitName, QStringLiteral("replace")), QT_TR_NOOP_UTF8("stop unit"));
    }
#endif
}

/*!
 * \brief Enables the unit if \a enabled is true and disables the unit if \a enabled is false.
 */
void SyncthingService::setEnabled(bool enabled)
{
#ifndef LIB_SYNCTHING_CONNECTOR_SERVICE_MOCKED
    if (!m_currentSystemdInterface) {
        setupSystemdInterface();
    }
    if (enabled) {
        registerErrorHandler(m_currentSystemdInterface->EnableUnitFiles(QStringList(m_unitName), false, true), QT_TR_NOOP_UTF8("enable unit"), true);
    } else {
        registerErrorHandler(m_currentSystemdInterface->DisableUnitFiles(QStringList(m_unitName), false), QT_TR_NOOP_UTF8("disable unit"), true);
    }
#endif
}

/*!
 * \brief Reload all unit files.
 */
void SyncthingService::reloadAllUnitFiles()
{
    registerErrorHandler(m_currentSystemdInterface->Reload(), QT_TR_NOOP_UTF8("reload all unit files"), false, false);
}

/*!
 * \brief Handles when a new unit is added to react if it matches the name of the unit we're monitoring.
 */
void SyncthingService::handleUnitAdded(const QString &unitName, const QDBusObjectPath &unitPath)
{
    if (unitName == m_unitName) {
        setUnit(unitPath);
    }
}

/*!
 * \brief Handles when a unit is removed to react if it matches the name of the unit we're monitoring.
 */
void SyncthingService::handleUnitRemoved(const QString &unitName, const QDBusObjectPath &unitPath)
{
    Q_UNUSED(unitPath)
    if (unitName == m_unitName) {
        setUnit(QDBusObjectPath());
    }
}

/*!
 * \brief Handles when unit files have been reloaded.
 */
void SyncthingService::handleReloading(bool started)
{
    if (!started) {
        queryUnitFromSystemdInterface();
    }
}

/*!
 * \brief Consumes the results of the s_manager->GetUnit() call (in setUnitName()).
 */
void SyncthingService::handleUnitGet(QDBusPendingCallWatcher *watcher)
{
    if (!concludeAsyncCall(watcher)) {
        return;
    }
    setUnit(QDBusPendingReply<QDBusObjectPath>(*watcher).value());
}

/*!
 * \brief Consumes the results of the s_manager->GetUnitFileState() call (in setUnitName()).
 */
void SyncthingService::handleGetUnitFileState(QDBusPendingCallWatcher *watcher)
{
    if (!concludeAsyncCall(watcher)) {
        return;
    }
    auto fileState = QString();
    if (!watcher->isError()) {
        if (const auto &args = watcher->reply().arguments(); !args.empty()) {
            fileState = args.at(0).toString();
        }
    }
    clearUnitData();
    setProperties(!fileState.isEmpty(), QString(), QString(), fileState, QString());
}

/*!
 * \brief Handles when properties of the monitored unit change.
 */
void SyncthingService::handlePropertiesChanged(
    const QString &interface, const QVariantMap &changedProperties, const QStringList &invalidatedProperties)
{
#ifndef LIB_SYNCTHING_CONNECTOR_SERVICE_MOCKED
    if (interface != m_unit->interface()) {
        return;
    }
#else
    if (interface != QStringLiteral("syncthing.mocked-service")) {
        return;
    }
#endif
    handlePropertyChanged(m_activeSince, QStringLiteral("ActiveEnterTimestamp"), changedProperties, invalidatedProperties);

    const auto wasRunningBefore = isRunning();
    const auto activeStateChanged = handlePropertyChanged(
        m_activeState, &SyncthingService::activeStateChanged, QStringLiteral("ActiveState"), changedProperties, invalidatedProperties);
    const auto subStateChanged
        = handlePropertyChanged(m_subState, &SyncthingService::subStateChanged, QStringLiteral("SubState"), changedProperties, invalidatedProperties);
    if (activeStateChanged || subStateChanged) {
        emit stateChanged(m_activeState, m_subState, m_activeSince);
    }
    const bool currentlyRunning = isRunning();
    if (wasRunningBefore != currentlyRunning) {
        if (currentlyRunning) {
            m_manuallyStopped = false;
            m_stoppedMetered = false;
        }
        emit runningChanged(currentlyRunning);
    }

    const bool wasEnabledBefore = isEnabled();
    handlePropertyChanged(
        m_unitFileState, &SyncthingService::unitFileStateChanged, QStringLiteral("UnitFileState"), changedProperties, invalidatedProperties);
    if (wasEnabledBefore != isEnabled()) {
        emit enabledChanged(isEnabled());
    }

    handlePropertyChanged(
        m_description, &SyncthingService::descriptionChanged, QStringLiteral("Description"), changedProperties, invalidatedProperties);
}

/*!
 * \brief Handles D-Bus errors.
 */
void SyncthingService::handleError(const char *context, QDBusPendingCallWatcher *watcher, bool reload)
{
    if (!concludeAsyncCall(watcher, reload)) {
        return;
    }
    const QDBusError error = watcher->error();
    if (error.isValid()) {
        emit errorOccurred(tr(context), error.name(), error.message());
    }
}

/*!
 * \brief Handles when the service availability changes.
 */
void SyncthingService::handleServiceRegisteredChanged(const QString &service)
{
    if (m_currentSystemdInterface && service == m_currentSystemdInterface->service()) {
        emit systemdAvailableChanged(m_currentSystemdInterface->isValid());
    }
}

/*!
 * \brief Logs the moment before when standby is enabled and the time of the last standby-wakeup.
 */
void SyncthingService::handlePrepareForSleep(bool rightBefore)
{
    if (!(s_fallingAsleep = rightBefore)) {
        s_lastWakeUp = DateTime::gmtNow();
    }
}

/*!
 * \brief Internal helper to handle property changes for QString-properties.
 */
bool SyncthingService::handlePropertyChanged(QString &variable, void (SyncthingService::*signal)(const QString &), const QString &propertyName,
    const QVariantMap &changedProperties, const QStringList &invalidatedProperties)
{
    const auto valueVariant = changedProperties.find(propertyName);
    if (valueVariant != changedProperties.end()) {
        auto valueString = valueVariant->toString();
        if (valueString != variable) {
            emit(this->*signal)(variable = std::move(valueString));
            return true;
        }
    } else if (invalidatedProperties.contains(propertyName) && !variable.isEmpty()) {
        variable.clear();
        emit(this->*signal)(variable);
        return true;
    }
    return false;
}

/*!
 * \brief Internal helper to handle property changes for DateTime-properties.
 */
bool SyncthingService::handlePropertyChanged(
    DateTime &variable, const QString &propertyName, const QVariantMap &changedProperties, const QStringList &invalidatedProperties)
{
    const auto valueVariant = changedProperties.find(propertyName);
    if (valueVariant != changedProperties.end()) {
        bool ok;
        const qulonglong valueInt = valueVariant->toULongLong(&ok);
        if (ok) {
            variable = dateTimeFromSystemdTimeStamp(valueInt);
            return true;
        }
    } else if (invalidatedProperties.contains(propertyName) && !variable.isNull()) {
        variable = DateTime();
        return true;
    }
    return false;
}

/*!
 * \brief Internal helper to stop the service when the network connection becomes metered.
 */
void SyncthingService::stopDueToMeteredConnection()
{
    if (isRunning()) {
        setRunning(false);
    }
    m_stoppedMetered = true;
}

/*!
 * \brief Sets the current unit data.
 */
void SyncthingService::setUnit(const QDBusObjectPath &objectPath)
{
    clearUnitData();

    if (!m_currentSystemdInterface) {
        setProperties(false, QString(), QString(), QString(), QString());
        return;
    }

    const auto path = objectPath.path();
    if (path.isEmpty()) {
        // fallback to querying unit file state
        makeAsyncCall(m_currentSystemdInterface->GetUnitFileState(m_unitName), &SyncthingService::handleGetUnitFileState);
        return;
    }

    // init unit
    m_unit = new OrgFreedesktopSystemd1UnitInterface(m_currentSystemdInterface->service(), path, m_currentSystemdInterface->connection());
    m_unit->setTimeout(2000);
    if (m_unit->isValid()) {
        m_activeSince = dateTimeFromSystemdTimeStamp(m_unit->activeEnterTimestamp());
        setProperties(true, m_unit->activeState(), m_unit->subState(), m_unit->unitFileState(), m_unit->description());
        // handle metered network connection: if the connection is metered and we care about it, then …
        if (isStoppingOnMeteredConnection() && isNetworkConnectionMetered().value_or(false)) {
            if (isRunning()) {
                // stop an already running service immediately
                stopDueToMeteredConnection();
            } else {
                // consider an already stopped service as stopped due to a metered connection; so we will start it as soon as the connection
                // is no longer metered
                m_stoppedMetered = true;
            }
        }
    } else {
        // fallback to querying unit file state
        makeAsyncCall(m_currentSystemdInterface->GetUnitFileState(m_unitName), &SyncthingService::handleGetUnitFileState);
    }

    // init properties
    m_properties = new OrgFreedesktopDBusPropertiesInterface(m_currentSystemdInterface->service(), path, m_currentSystemdInterface->connection());
    connect(m_properties, &OrgFreedesktopDBusPropertiesInterface::PropertiesChanged, this, &SyncthingService::handlePropertiesChanged);
}

/*!
 * \brief Updates the properties for the current unit.
 */
void SyncthingService::setProperties(
    bool unitAvailable, const QString &activeState, const QString &subState, const QString &unitFileState, const QString &description)
{
    if (m_unitAvailable != unitAvailable) {
        emit unitAvailableChanged(m_unitAvailable = unitAvailable);
    }

    const bool running = isRunning();
    bool anyStateChanged = false;
    if (m_activeState != activeState) {
        emit activeStateChanged(m_activeState = activeState);
        anyStateChanged = true;
    }
    if (m_subState != subState) {
        emit subStateChanged(m_subState = subState);
        anyStateChanged = true;
    }
    if (anyStateChanged) {
        emit stateChanged(m_activeState, m_subState, m_activeSince);
    }
    if (running != isRunning()) {
        emit runningChanged(isRunning());
    }

    const bool enabled = isEnabled();
    if (m_unitFileState != unitFileState) {
        emit unitFileStateChanged(m_unitFileState = unitFileState);
    }
    if (enabled != isEnabled()) {
        emit enabledChanged(isEnabled());
    }
    if (m_description != description) {
        emit descriptionChanged(m_description = description);
    }
}

/*!
 * \brief Sets whether the current network connection is metered and stops/starts Syncthing accordingly as needed.
 * \remarks
 * - This is detected and monitored automatically. A manually set value will be overridden again on the next change.
 * - One may set this manually for testing purposes or in case the automatic detection is not supported (then
 *   isNetworkConnectionMetered() returns a std::optional<bool> without value).
 */
void SyncthingService::setNetworkConnectionMetered(std::optional<bool> metered)
{
    if (metered != m_metered) {
        m_metered = metered;
        if (m_stopOnMeteredConnection) {
            if (metered.value_or(false)) {
                stopDueToMeteredConnection();
            } else if (!metered.value_or(true) && m_stoppedMetered) {
                start();
            }
        }
        emit networkConnectionMeteredChanged(metered);
    }
}

/*!
 * \brief Sets whether Syncthing should automatically be stopped as long as the network connection is metered.
 */
void SyncthingService::setStoppingOnMeteredConnection(bool stopOnMeteredConnection)
{
    if ((stopOnMeteredConnection != m_stopOnMeteredConnection) && (m_stopOnMeteredConnection = stopOnMeteredConnection) && m_metered) {
        stopDueToMeteredConnection();
    }
}

} // namespace Data

#endif
