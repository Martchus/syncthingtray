#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD

#include "./syncthingservice.h"

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
OrgFreedesktopSystemd1ManagerInterface *SyncthingService::s_manager = nullptr;
OrgFreedesktopLogin1ManagerInterface *SyncthingService::s_loginManager = nullptr;
DateTime SyncthingService::s_lastWakeUp = DateTime();
bool SyncthingService::s_fallingAsleep = false;

/// \endcond

/*!
 * \brief Creates a new SyncthingService instance.
 */
SyncthingService::SyncthingService(QObject *parent)
    : QObject(parent)
    , m_unit(nullptr)
    , m_service(nullptr)
    , m_properties(nullptr)
    , m_manuallyStopped(false)
    , m_unitAvailable(false)
{
#ifndef LIB_SYNCTHING_CONNECTOR_SERVICE_MOCKED
    if (!s_manager) {
        // register custom data types
        qDBusRegisterMetaType<ManagerDBusUnitFileChange>();
        qDBusRegisterMetaType<ManagerDBusUnitFileChangeList>();

        s_manager = new OrgFreedesktopSystemd1ManagerInterface(
            QStringLiteral("org.freedesktop.systemd1"), QStringLiteral("/org/freedesktop/systemd1"), QDBusConnection::sessionBus());

        // enable systemd to emit signals
        s_manager->Subscribe();
    }
    if (!s_loginManager) {
        s_loginManager = new OrgFreedesktopLogin1ManagerInterface(
            QStringLiteral("org.freedesktop.login1"), QStringLiteral("/org/freedesktop/login1"), QDBusConnection::systemBus());
        connect(s_loginManager, &OrgFreedesktopLogin1ManagerInterface::PrepareForSleep, &SyncthingService::handlePrepareForSleep);
    }
    connect(s_manager, &OrgFreedesktopSystemd1ManagerInterface::UnitNew, this, &SyncthingService::handleUnitAdded);
    connect(s_manager, &OrgFreedesktopSystemd1ManagerInterface::UnitRemoved, this, &SyncthingService::handleUnitRemoved);
    m_serviceWatcher = new QDBusServiceWatcher(s_manager->service(), s_manager->connection());
    connect(m_serviceWatcher, &QDBusServiceWatcher::serviceRegistered, this, &SyncthingService::handleServiceRegisteredChanged);
    connect(m_serviceWatcher, &QDBusServiceWatcher::serviceUnregistered, this, &SyncthingService::handleServiceRegisteredChanged);
#else
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

    delete m_service, delete m_unit, delete m_properties;
    m_service = nullptr, m_unit = nullptr, m_properties = nullptr;
    setProperties(false, QString(), QString(), QString(), QString());

#ifndef LIB_SYNCTHING_CONNECTOR_SERVICE_MOCKED
    if (s_manager->isValid()) {
        connect(new QDBusPendingCallWatcher(s_manager->GetUnit(m_unitName), this), &QDBusPendingCallWatcher::finished, this,
            &SyncthingService::handleUnitGet);
    }
#endif

    emit unitNameChanged(unitName);
}

/*!
 * \brief Returns whether systemd (and specificly its D-Bus interface for user services) is available.
 * \remarks The availability might not be instantly detected and may change at any time. Use the systemdAvailableChanged()
 *          to react to availability changes.
 */
bool SyncthingService::isSystemdAvailable() const
{
#ifndef LIB_SYNCTHING_CONNECTOR_SERVICE_MOCKED
    return s_manager && s_manager->isValid();
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
 * \brief Returns whether \a activeSince or the last standby-wake-up is longer ago than \a atLeastSeconds.
 */
bool SyncthingService::isActiveWithoutSleepFor(DateTime activeSince, unsigned int atLeastSeconds)
{
    if (!atLeastSeconds) {
        return true;
    }
    if (activeSince.isNull() || s_fallingAsleep) {
        return false;
    }

    const DateTime now(DateTime::gmtNow());
    return ((now - activeSince).totalSeconds() > atLeastSeconds) && (s_lastWakeUp.isNull() || ((now - s_lastWakeUp).totalSeconds() > atLeastSeconds));
}

/*!
 * \brief Starts the unit if \a running is true and stops the unit if \a running is false.
 */
void SyncthingService::setRunning(bool running)
{
#ifndef LIB_SYNCTHING_CONNECTOR_SERVICE_MOCKED
    m_manuallyStopped = !running;
    if (running) {
        registerErrorHandler(s_manager->StartUnit(m_unitName, QStringLiteral("replace")), QT_TR_NOOP_UTF8("start unit"));
    } else {
        registerErrorHandler(s_manager->StopUnit(m_unitName, QStringLiteral("replace")), QT_TR_NOOP_UTF8("stop unit"));
    }
#endif
}

/*!
 * \brief Enables the unit if \a enabled is true and disables the unit if \a enabled is false.
 */
void SyncthingService::setEnabled(bool enabled)
{
#ifndef LIB_SYNCTHING_CONNECTOR_SERVICE_MOCKED
    if (enabled) {
        registerErrorHandler(s_manager->EnableUnitFiles(QStringList(m_unitName), false, true), QT_TR_NOOP_UTF8("enable unit"));
    } else {
        registerErrorHandler(s_manager->DisableUnitFiles(QStringList(m_unitName), false), QT_TR_NOOP_UTF8("disable unit"));
    }
#endif
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
 * \brief Consumes the results of the s_manager->GetUnit() call (in setUnitName()).
 */
void SyncthingService::handleUnitGet(QDBusPendingCallWatcher *watcher)
{
    watcher->deleteLater();

    const QDBusPendingReply<QDBusObjectPath> unitReply = *watcher;
    if (unitReply.isError()) {
        return;
    }

    setUnit(unitReply.value());
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

    const bool wasRunningBefore = isRunning();
    if (handlePropertyChanged(
            m_activeState, &SyncthingService::activeStateChanged, QStringLiteral("ActiveState"), changedProperties, invalidatedProperties)
        | handlePropertyChanged(
            m_subState, &SyncthingService::subStateChanged, QStringLiteral("SubState"), changedProperties, invalidatedProperties)) {
        emit stateChanged(m_activeState, m_subState, m_activeSince);
    }
    const bool currentlyRunning = isRunning();
    if (currentlyRunning) {
        m_manuallyStopped = false;
    }
    if (wasRunningBefore != currentlyRunning) {
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
void SyncthingService::handleError(const char *context, QDBusPendingCallWatcher *watcher)
{
    watcher->deleteLater();
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
    if (service == s_manager->service()) {
        emit systemdAvailableChanged(s_manager->isValid());
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
    const QVariant valueVariant(changedProperties[propertyName]);
    if (valueVariant.isValid()) {
        const QString valueString(valueVariant.toString());
        if (valueString != variable) {
            emit(this->*signal)(variable = valueString);
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
    const QVariant valueVariant(changedProperties[propertyName]);
    if (valueVariant.isValid()) {
        bool ok;
        const qulonglong valueInt = valueVariant.toULongLong(&ok);
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
 * \brief Registers error handler for D-Bus errors.
 */
void SyncthingService::registerErrorHandler(const QDBusPendingCall &call, const char *context)
{
    connect(new QDBusPendingCallWatcher(call, this), &QDBusPendingCallWatcher::finished, bind(&SyncthingService::handleError, this, context, _1));
}

/*!
 * \brief Sets the current unit data.
 */
void SyncthingService::setUnit(const QDBusObjectPath &objectPath)
{
    // cleanup
    delete m_service, delete m_unit, delete m_properties;
    m_service = nullptr, m_unit = nullptr, m_properties = nullptr;

    const QString path = objectPath.path();
    if (path.isEmpty()) {
        setProperties(false, QString(), QString(), QString(), QString());
        return;
    }

    // init unit
    m_unit = new OrgFreedesktopSystemd1UnitInterface(s_manager->service(), path, s_manager->connection());
    m_activeSince = dateTimeFromSystemdTimeStamp(m_unit->activeEnterTimestamp());
    setProperties(m_unit->isValid(), m_unit->activeState(), m_unit->subState(), m_unit->unitFileState(), m_unit->description());

    // init properties
    m_properties = new OrgFreedesktopDBusPropertiesInterface(s_manager->service(), path, s_manager->connection());
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

} // namespace Data

#endif
