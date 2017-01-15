#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD

#include "./syncthingservice.h"

#include "managerinterface.h"
#include "unitinterface.h"
#include "serviceinterface.h"
#include "propertiesinterface.h"
#include "loginmanagerinterface.h"

#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusServiceWatcher>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDBusObjectPath>
#include <QDBusMetaType>

#include <functional>

using namespace std;
using namespace std::placeholders;
using namespace ChronoUtilities;

namespace Data {

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
    return DateTime(621355968000000000 + timeStamp * 10);
}

OrgFreedesktopSystemd1ManagerInterface *SyncthingService::s_manager = nullptr;
OrgFreedesktopLogin1ManagerInterface *SyncthingService::s_loginManager = nullptr;
DateTime SyncthingService::s_lastWakeUp = DateTime();
bool SyncthingService::s_fallingAsleep = false;

SyncthingService::SyncthingService(QObject *parent) :
    QObject(parent),
    m_unit(nullptr),
    m_service(nullptr),
    m_properties(nullptr),
    m_manuallyStopped(false)
{
    if(!s_manager) {
        // register custom data types
        qDBusRegisterMetaType<ManagerDBusUnitFileChange>();
        qDBusRegisterMetaType<ManagerDBusUnitFileChangeList>();

        s_manager = new OrgFreedesktopSystemd1ManagerInterface(
                        QStringLiteral("org.freedesktop.systemd1"),
                        QStringLiteral("/org/freedesktop/systemd1"),
                        QDBusConnection::sessionBus()
                    );

        // enable systemd to emit signals
        s_manager->Subscribe();
    }
    if(!s_loginManager) {
        s_loginManager = new OrgFreedesktopLogin1ManagerInterface(
                        QStringLiteral("org.freedesktop.login1"),
                        QStringLiteral("/org/freedesktop/login1"),
                        QDBusConnection::systemBus()
                    );
        connect(s_loginManager, &OrgFreedesktopLogin1ManagerInterface::PrepareForSleep, &SyncthingService::handlePrepareForSleep);
    }
    connect(s_manager, &OrgFreedesktopSystemd1ManagerInterface::UnitNew, this, &SyncthingService::handleUnitAdded);
    connect(s_manager, &OrgFreedesktopSystemd1ManagerInterface::UnitRemoved, this, &SyncthingService::handleUnitRemoved);
    m_serviceWatcher = new QDBusServiceWatcher(s_manager->service(), s_manager->connection());
    connect(m_serviceWatcher, &QDBusServiceWatcher::serviceRegistered, this, &SyncthingService::handleServiceRegisteredChanged);
    connect(m_serviceWatcher, &QDBusServiceWatcher::serviceUnregistered, this, &SyncthingService::handleServiceRegisteredChanged);
}

void SyncthingService::setUnitName(const QString &unitName)
{
    if(m_unitName != unitName) {
        m_unitName = unitName;

        delete m_service, delete m_unit, delete m_properties;
        m_service = nullptr, m_unit = nullptr, m_properties = nullptr;
        setProperties(QString(), QString(), QString(), QString());

        if(s_manager->isValid()) {
            connect(new QDBusPendingCallWatcher(s_manager->GetUnit(m_unitName), this), &QDBusPendingCallWatcher::finished, this, &SyncthingService::handleUnitGet);
        }
    }
}

bool SyncthingService::isSystemdAvailable() const
{
    return s_manager && s_manager->isValid();
}

bool SyncthingService::isUnitAvailable() const
{
    return m_unit && m_unit->isValid();
}

bool SyncthingService::isActiveWithoutSleepFor(unsigned int atLeastSeconds) const
{
    if(!atLeastSeconds) {
        return true;
    }
    if(m_activeSince.isNull() || s_fallingAsleep) {
        return false;
    }

    const DateTime now(DateTime::gmtNow());
    return ((now - m_activeSince).totalSeconds() > atLeastSeconds)
            && (s_lastWakeUp.isNull() || ((now - s_lastWakeUp).totalSeconds() > atLeastSeconds));
}

void SyncthingService::setRunning(bool running)
{
    m_manuallyStopped = !running;
    if(running) {
        registerErrorHandler(s_manager->StartUnit(m_unitName, QStringLiteral("replace")), QT_TR_NOOP_UTF8("start unit"));
    } else {
        registerErrorHandler(s_manager->StopUnit(m_unitName, QStringLiteral("replace")), QT_TR_NOOP_UTF8("stop unit"));
    }
}

void SyncthingService::setEnabled(bool enabled)
{
    if(enabled) {
        registerErrorHandler(s_manager->EnableUnitFiles(QStringList(m_unitName), false, true), QT_TR_NOOP_UTF8("enable unit"));
    } else {
        registerErrorHandler(s_manager->DisableUnitFiles(QStringList(m_unitName), false), QT_TR_NOOP_UTF8("disable unit"));
    }
}

void SyncthingService::handleUnitAdded(const QString &unitName, const QDBusObjectPath &unitPath)
{
    if(unitName == m_unitName) {
        setUnit(unitPath);
    }
}

void SyncthingService::handleUnitRemoved(const QString &unitName, const QDBusObjectPath &unitPath)
{
    Q_UNUSED(unitPath)
    if(unitName == m_unitName) {
        setUnit(QDBusObjectPath());
    }
}

void SyncthingService::handleUnitGet(QDBusPendingCallWatcher *watcher)
{
    watcher->deleteLater();

    const QDBusPendingReply<QDBusObjectPath> unitReply = *watcher;
    if(unitReply.isError()) {
        return;
    }

    setUnit(unitReply.value());
}

void SyncthingService::handlePropertiesChanged(const QString &interface, const QVariantMap &changedProperties, const QStringList &invalidatedProperties)
{
    if(interface == m_unit->interface()) {
        handlePropertyChanged(m_activeSince, QStringLiteral("ActiveEnterTimestamp"), changedProperties, invalidatedProperties);

        const bool wasRunningBefore = isRunning();
        if(handlePropertyChanged(m_activeState, &SyncthingService::activeStateChanged, QStringLiteral("ActiveState"), changedProperties, invalidatedProperties)
                | handlePropertyChanged(m_subState, &SyncthingService::subStateChanged, QStringLiteral("SubState"), changedProperties, invalidatedProperties)) {
            emit stateChanged(m_activeState, m_subState, m_activeSince);
        }
        const bool currentlyRunning = isRunning();
        if(currentlyRunning) {
            m_manuallyStopped = false;
        }
        if(wasRunningBefore != currentlyRunning) {
            emit runningChanged(currentlyRunning);
        }

        const bool wasEnabledBefore = isEnabled();
        handlePropertyChanged(m_unitFileState, &SyncthingService::unitFileStateChanged, QStringLiteral("UnitFileState"), changedProperties, invalidatedProperties);
        if(wasEnabledBefore != isEnabled()) {
            emit enabledChanged(isEnabled());
        }

        handlePropertyChanged(m_description, &SyncthingService::descriptionChanged, QStringLiteral("Description"), changedProperties, invalidatedProperties);
    }
}

void SyncthingService::handleError(const char *context, QDBusPendingCallWatcher *watcher)
{
    watcher->deleteLater();
    const QDBusError error = watcher->error();
    if(error.isValid()) {
        emit errorOccurred(tr(context), error.name(), error.message());
    }
}

void SyncthingService::handleServiceRegisteredChanged(const QString &service)
{
    if(service == s_manager->service()) {
        emit systemdAvailableChanged(s_manager->isValid());
    }
}

void SyncthingService::handlePrepareForSleep(bool rightBefore)
{
    if(!(s_fallingAsleep = rightBefore)) {
        s_lastWakeUp = DateTime::gmtNow();
    }
}

bool SyncthingService::handlePropertyChanged(QString &variable, void (SyncthingService::*signal)(const QString &), const QString &propertyName, const QVariantMap &changedProperties, const QStringList &invalidatedProperties)
{
    const QVariant valueVariant(changedProperties[propertyName]);
    if(valueVariant.isValid()) {
        const QString valueString(valueVariant.toString());
        if(valueString != variable) {
            emit (this->*signal)(variable = valueString);
            return true;
        }
    } else if(invalidatedProperties.contains(propertyName) && !variable.isEmpty()) {
        variable.clear();
        emit (this->*signal)(variable);
        return true;
    }
    return false;
}

bool SyncthingService::handlePropertyChanged(DateTime &variable, const QString &propertyName, const QVariantMap &changedProperties, const QStringList &invalidatedProperties)
{
    const QVariant valueVariant(changedProperties[propertyName]);
    if(valueVariant.isValid()) {
        bool ok;
        const qulonglong valueInt = valueVariant.toULongLong(&ok);
        if(ok) {
            variable = dateTimeFromSystemdTimeStamp(valueInt);
            return true;
        }
    } else if(invalidatedProperties.contains(propertyName) && !variable.isNull()) {
        variable = DateTime();
        return true;
    }
    return false;
}

void SyncthingService::registerErrorHandler(const QDBusPendingCall &call, const char *context)
{
    connect(new QDBusPendingCallWatcher(call, this), &QDBusPendingCallWatcher::finished, bind(&SyncthingService::handleError, this, context, _1));
}

void SyncthingService::setUnit(const QDBusObjectPath &objectPath)
{
    // cleanup
    delete m_service, delete m_unit, delete m_properties;
    m_service = nullptr, m_unit = nullptr, m_properties = nullptr;

    const QString path = objectPath.path();
    if(path.isEmpty()) {
        setProperties(QString(), QString(), QString(), QString());
        return;
    }

    // init unit
    m_unit = new OrgFreedesktopSystemd1UnitInterface(s_manager->service(), path, s_manager->connection());
    m_activeSince = dateTimeFromSystemdTimeStamp(m_unit->activeEnterTimestamp());
    setProperties(m_unit->activeState(), m_unit->subState(), m_unit->unitFileState(), m_unit->description());

    // init properties
    m_properties = new OrgFreedesktopDBusPropertiesInterface(s_manager->service(), path, s_manager->connection());
    connect(m_properties, &OrgFreedesktopDBusPropertiesInterface::PropertiesChanged, this, &SyncthingService::handlePropertiesChanged);
}

void SyncthingService::setProperties(const QString &activeState, const QString &subState, const QString &unitFileState, const QString &description)
{
    const bool running = isRunning();
    bool anyStateChanged = false;
    if(m_activeState != activeState) {
        emit activeStateChanged(m_activeState = activeState);
        anyStateChanged = true;
    }
    if(m_subState != subState) {
        emit subStateChanged(m_subState = subState);
        anyStateChanged = true;
    }
    if(anyStateChanged) {
        emit stateChanged(m_activeState, m_subState, m_activeSince);
    }
    if(running != isRunning()) {
        emit runningChanged(isRunning());
    }

    const bool enabled = isEnabled();
    if(m_unitFileState != unitFileState) {
        emit unitFileStateChanged(m_unitFileState = unitFileState);
    }
    if(enabled != isEnabled()) {
        emit enabledChanged(isEnabled());
    }

    if(m_description != description) {
        emit descriptionChanged(m_description = description);
    }
}

SyncthingService &syncthingService()
{
    static SyncthingService service;
    return service;
}

} // namespace Data

#endif
