#if defined(LIB_SYNCTHING_CONNECTOR_SUPPORT_SERVICE) && !defined(DATA_SYNCTHINGSERVICE_H)
#define DATA_SYNCTHINGSERVICE_H

#include "./runtimecondition.h"
#include "./syncthingprocess.h"

#include <c++utilities/chrono/datetime.h>

#include <QObject>
#include <QVariantMap>

#include <optional>
#include <unordered_set>

#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
QT_FORWARD_DECLARE_CLASS(QDBusServiceWatcher)
QT_FORWARD_DECLARE_CLASS(QDBusArgument)
QT_FORWARD_DECLARE_CLASS(QDBusObjectPath)
QT_FORWARD_DECLARE_CLASS(QDBusPendingCall)
QT_FORWARD_DECLARE_CLASS(QDBusPendingCallWatcher)

class OrgFreedesktopSystemd1ManagerInterface;
class OrgFreedesktopSystemd1UnitInterface;
class OrgFreedesktopSystemd1ServiceInterface;
class OrgFreedesktopDBusPropertiesInterface;
class OrgFreedesktopLogin1ManagerInterface;
#elif defined(LIB_SYNCTHING_CONNECTOR_SUPPORT_WINDOWS_SERVICE)
#include <memory>
#include <mutex>
#endif

namespace Data {

#if defined(LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD)
struct ManagerDBusUnitFileChange {
    QString type;
    QString path;
    QString source;
};

QDBusArgument &operator<<(QDBusArgument &argument, const ManagerDBusUnitFileChange &unitFileChange);
const QDBusArgument &operator>>(const QDBusArgument &argument, ManagerDBusUnitFileChange &unitFileChange);

using ManagerDBusUnitFileChangeList = QList<ManagerDBusUnitFileChange>;
#elif defined(LIB_SYNCTHING_CONNECTOR_SUPPORT_WINDOWS_SERVICE)
LIB_SYNCTHING_CONNECTOR_EXPORT int runAsWindowsService(std::function<int(void)> &&serviceRoutine, std::function<void(void)> &&stopRoutine);
LIB_SYNCTHING_CONNECTOR_EXPORT int handleInstallService(
    const QString &serviceName, const QString &homeDir = QString(), const QString &logFile = QString());
LIB_SYNCTHING_CONNECTOR_EXPORT int handleUninstallService(const QString &serviceName);
LIB_SYNCTHING_CONNECTOR_EXPORT int handleEnableService(const QString &serviceName, bool enable);
LIB_SYNCTHING_CONNECTOR_EXPORT int handleStartService(const QString &serviceName);
LIB_SYNCTHING_CONNECTOR_EXPORT int handleStopService(const QString &serviceName);
#endif

enum class ServiceScope { System, User };

class LIB_SYNCTHING_CONNECTOR_EXPORT SyncthingService : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString unitName READ unitName WRITE setUnitName NOTIFY unitNameChanged)
    Q_PROPERTY(bool serviceManagerAvailable READ isServiceManagerAvailable NOTIFY serviceManagerAvailableChanged)
    Q_PROPERTY(bool unitAvailable READ isUnitAvailable NOTIFY unitAvailableChanged)
    Q_PROPERTY(QString activeState READ activeState NOTIFY activeStateChanged)
    Q_PROPERTY(QString subState READ subState NOTIFY subStateChanged)
    Q_PROPERTY(CppUtilities::DateTime activeSince READ activeSince NOTIFY activeStateChanged)
    Q_PROPERTY(QString unitFileState READ unitFileState NOTIFY unitFileStateChanged)
    Q_PROPERTY(QString description READ description NOTIFY descriptionChanged)
    Q_PROPERTY(bool running READ isRunning WRITE setRunning NOTIFY runningChanged)
    Q_PROPERTY(bool enable READ isEnabled WRITE setEnabled NOTIFY enabledChanged)
    Q_PROPERTY(bool manuallyStopped READ isManuallyStopped)
    Q_PROPERTY(ServiceScope scope READ scope WRITE setScope NOTIFY scopeChanged)
    Q_PROPERTY(bool userScope READ isUserScope NOTIFY scopeChanged)
    Q_PROPERTY(Data::RuntimeCondition *runtimeCondition READ runtimeCondition CONSTANT)
    Q_PROPERTY(QString serviceManagerName READ serviceManagerName CONSTANT)
    Q_PROPERTY(bool canInstallService READ canInstallService CONSTANT)
    Q_PROPERTY(bool canOpenServiceManager READ canOpenServiceManager CONSTANT)

public:
    explicit SyncthingService(ServiceScope scope = ServiceScope::User, QObject *parent = nullptr);
    ~SyncthingService() override;

    const QString &unitName() const;
    bool isServiceManagerAvailable() const;
    bool isUnitAvailable() const;
    const QString &activeState() const;
    const QString &subState() const;
    inline CppUtilities::DateTime activeSince() const;
    inline bool isActiveFor(unsigned int atLeastSeconds) const;
    inline bool isActiveWithoutSleepFor(unsigned int atLeastSeconds) const;
    inline static bool isActiveWithoutSleepFor(CppUtilities::DateTime activeSince, unsigned int atLeastSeconds);
    inline static CppUtilities::DateTime lastWakeUp();
    const QString &unitFileState() const;
    const QString &description() const;
    bool isRunning() const;
    bool isEnabled() const;
    bool isRunningOrEnabled() const;
    bool isDisabled() const;
    bool canEnableOrStart() const;
    bool isManuallyStopped() const;
    ServiceScope scope() const;
    void setScope(ServiceScope scope);
    void setScopeAndUnitName(ServiceScope scope, const QString &unitName);
    inline bool isUserScope() const;
    QString displayName() const;
    inline QString serviceManagerName() const;
    inline bool canInstallService() const;
    inline bool canOpenServiceManager() const;
    inline RuntimeCondition *runtimeCondition();
    inline const RuntimeCondition *runtimeCondition() const;
    inline static SyncthingService *mainInstance();
    inline static void setMainInstance(SyncthingService *mainInstance);

public Q_SLOTS:
    void setUnitName(const QString &unitName);
    void setRunning(bool running);
    void start();
    void stop();
    void toggleRunning();
    void setEnabled(bool enable);
    inline void enable();
    inline void disable();
    void reloadAllUnitFiles();
    void openLogs() const;
    void openServiceManager() const;
    void installService(const QString &homeDir = QString());
    void uninstallService();

Q_SIGNALS:
    void unitNameChanged(const QString &unitName);
    void serviceManagerAvailableChanged(bool available);
    void unitAvailableChanged(bool available);
    void stateChanged(const QString &activeState, const QString &subState, CppUtilities::DateTime activeSince);
    void activeStateChanged(const QString &activeState);
    void subStateChanged(const QString &subState);
    void unitFileStateChanged(const QString &unitFileState);
    void descriptionChanged(const QString &description);
    void runningChanged(bool running);
    void enabledChanged(bool enable);
    void errorOccurred(const QString &context, const QString &name, const QString &message);
    void scopeChanged(Data::ServiceScope scope);
    void serviceInstalled();

#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
private Q_SLOTS:
    void handleUnitAdded(const QString &unitName, const QDBusObjectPath &unitPath);
    void handleUnitRemoved(const QString &unitName, const QDBusObjectPath &unitPath);
    void handleReloading(bool started);
    void handleUnitGet(QDBusPendingCallWatcher *watcher);
    void handleGetUnitFileState(QDBusPendingCallWatcher *watcher);
    void handlePropertiesChanged(const QString &interface, const QVariantMap &changedProperties, const QStringList &invalidatedProperties);
    void handleError(const char *error, QDBusPendingCallWatcher *watcher, bool reload = false);
    void handleServiceRegisteredChanged(const QString &service);
    static void handlePrepareForSleep(bool rightBefore);
    void setUnit(const QDBusObjectPath &objectPath);
    void setProperties(
        bool unitAvailable, const QString &activeState, const QString &subState, const QString &unitFileState, const QString &description);

private:
    void setupSystemdInterface();
    void setupFreedesktopLoginInterface();
    template <typename HandlerType> void makeAsyncCall(const QDBusPendingCall &call, HandlerType &&handler, bool removeHandler = true);
    void registerErrorHandler(const QDBusPendingCall &call, const char *context, bool reload = false, bool removeHandler = true);
    bool concludeAsyncCall(QDBusPendingCallWatcher *watcher, bool reload = false);
    void clearSystemdInterface();
    void clearUnitData();
    void queryUnitFromSystemdInterface();
    bool handlePropertyChanged(QString &variable, void (SyncthingService::*signal)(const QString &), const QString &propertyName,
        const QVariantMap &changedProperties, const QStringList &invalidatedProperties);
    bool handlePropertyChanged(CppUtilities::DateTime &variable, const QString &propertyName, const QVariantMap &changedProperties,
        const QStringList &invalidatedProperties);

    static OrgFreedesktopSystemd1ManagerInterface *s_systemdUserInterface;
    static OrgFreedesktopSystemd1ManagerInterface *s_systemdSystemInterface;
    static OrgFreedesktopLogin1ManagerInterface *s_loginManager;
    QDBusServiceWatcher *m_serviceWatcher;
    OrgFreedesktopSystemd1UnitInterface *m_unit;
    OrgFreedesktopSystemd1ServiceInterface *m_service;
    OrgFreedesktopDBusPropertiesInterface *m_properties;
    OrgFreedesktopSystemd1ManagerInterface *m_currentSystemdInterface;
    std::unordered_set<QDBusPendingCallWatcher *> m_pendingCalls;

#elif defined(LIB_SYNCTHING_CONNECTOR_SUPPORT_WINDOWS_SERVICE)
public Q_SLOTS:
    void queryServiceStatus();
    void queryServiceConfig();

private:
    class ServiceWatcherThread;
    ServiceWatcherThread *m_watcherThread;
    void startServiceWatcher();
    void stopServiceWatcher();
#endif
    void stopDueToRuntimeCond();

    static bool s_fallingAsleep;
    static CppUtilities::DateTime s_lastWakeUp;
    static SyncthingService *s_mainInstance;
    QString m_unitName;
    QString m_description;
    QString m_activeState;
    QString m_subState;
    QString m_unitFileState;
    CppUtilities::DateTime m_activeSince;
    ServiceScope m_scope;
    bool m_manuallyStopped;
    bool m_stoppedDueToRuntimeCond;
    bool m_unitAvailable;
    RuntimeCondition m_runtimeCondition;
};

/*!
 * \brief Returns the name of the systemd user service unit to be controlled/monitored, e.g. "syncthing.service".
 */
inline const QString &SyncthingService::unitName() const
{
    return m_unitName;
}

/*!
 * \brief Returns the active state of the unit.
 * \remarks The unit must be available, \sa SyncthingService::isUnitAvailable().
 */
inline const QString &SyncthingService::activeState() const
{
    return m_activeState;
}

/*!
 * \brief Returns the sub state of the unit.
 * \remarks The unit must be available, \sa SyncthingService::isUnitAvailable().
 */
inline const QString &SyncthingService::subState() const
{
    return m_subState;
}

/*!
 * \brief Returns the unit file state.
 * \remarks The unit must be available, \sa SyncthingService::isUnitAvailable().
 */
inline const QString &SyncthingService::unitFileState() const
{
    return m_unitFileState;
}

/*!
 * \brief Returns the unit description.
 * \remarks The unit must be available, \sa SyncthingService::isUnitAvailable().
 */
inline const QString &SyncthingService::description() const
{
    return m_description;
}

/*!
 * \brief Returns whether the unit is running.
 * \remarks The unit must be available, \sa SyncthingService::isUnitAvailable().
 */
inline bool SyncthingService::isRunning() const
{
    return m_activeState == QLatin1String("active") && m_subState == QLatin1String("running");
}

/*!
 * \brief Starts the unit.
 */
inline void SyncthingService::start()
{
    setRunning(true);
}

/*!
 * \brief Stops the unit.
 */
inline void SyncthingService::stop()
{
    setRunning(false);
}

/*!
 * \brief Toggles the running state.
 */
inline void SyncthingService::toggleRunning()
{
    setRunning(!isRunning());
}

/*!
 * \brief Returns whether the unit is enabled.
 * \remarks The unit must be available, \sa SyncthingService::isUnitAvailable().
 */
inline bool SyncthingService::isEnabled() const
{
    return m_unitFileState == QLatin1String("enabled");
}

/*!
 * \brief Returns whether the unit is running or enabled.
 */
inline bool SyncthingService::isRunningOrEnabled() const
{
    return isRunning() || isEnabled();
}

/*!
 * \brief Returns whether the unit is disabled.
 * \remarks The unit must be available, \sa SyncthingService::isUnitAvailable().
 */
inline bool SyncthingService::isDisabled() const
{
    return m_unitFileState == QLatin1String("disabled");
}

/*!
 * \brief Returns whether the unit can be enabled or started.
 */
inline bool Data::SyncthingService::canEnableOrStart() const
{
    return (isUnitAvailable() && !isRunning()) || isDisabled();
}

/*!
 * \brief Returns whether the unit has been manually stopped via stop(), toggleRunning() or setRunning().
 */
inline bool SyncthingService::isManuallyStopped() const
{
    return m_manuallyStopped;
}

/*!
 * \brief Returns the scope the current instance is tuned to.
 */
inline ServiceScope SyncthingService::scope() const
{
    return m_scope;
}

/*!
 * \brief Returns whether the scope is ServiceScope::User.
 */
inline bool SyncthingService::isUserScope() const
{
    return m_scope == ServiceScope::User;
}

/*!
 * \brief Returns since when the unit is active.
 */
inline CppUtilities::DateTime SyncthingService::activeSince() const
{
    return m_activeSince;
}

/*!
 * \brief Returns whether the unit has been active for at least \a atLeastSeconds.
 * \remarks An interruption due to standby is *not* taken into account.
 */
inline bool SyncthingService::isActiveFor(unsigned int atLeastSeconds) const
{
    return SyncthingProcess::isActiveFor(m_activeSince, atLeastSeconds);
}

/*!
 * \brief Returns whether the unit has been active for at least \a atLeastSeconds.
 * \remarks An interruption due to standby is taken into account. So if the last standby-wakeup was less than \a atLeastSeconds
 *          ago this function returns false regardless of the unit's activeness.
 */
inline bool SyncthingService::isActiveWithoutSleepFor(unsigned int atLeastSeconds) const
{
    return isActiveWithoutSleepFor(m_activeSince, atLeastSeconds);
}

/*!
 * \brief Returns whether \a activeSince or the last standby-wake-up is longer ago than \a atLeastSeconds.
 */
inline bool SyncthingService::isActiveWithoutSleepFor(CppUtilities::DateTime activeSince, unsigned int atLeastSeconds)
{
    return SyncthingProcess::isActiveWithoutSleepFor(s_lastWakeUp, s_fallingAsleep, activeSince, atLeastSeconds);
}

/*!
 * \brief Returns when the last standby-wakeup happened.
 */
inline CppUtilities::DateTime SyncthingService::lastWakeUp()
{
    return s_lastWakeUp;
}

/*!
 * \brief Enables the unit.
 */
inline void SyncthingService::enable()
{
    setEnabled(true);
}

/*!
 * \brief Disables the unit.
 */
inline void SyncthingService::disable()
{
    setEnabled(false);
}

inline RuntimeCondition *SyncthingService::runtimeCondition()
{
    return &m_runtimeCondition;
}

inline const RuntimeCondition *SyncthingService::runtimeCondition() const
{
    return &m_runtimeCondition;
}

/*!
 * \brief Returns the name of the service manager ("systemd" or "Windows").
 */
inline QString SyncthingService::serviceManagerName() const
{
#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_WINDOWS_SERVICE
    return QStringLiteral("Windows");
#else
    return QStringLiteral("systemd");
#endif
}

/*!
 * \brief Returns whether the service can be installed by this class.
 */
inline bool SyncthingService::canInstallService() const
{
#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_WINDOWS_SERVICE
    return true;
#else
    return false;
#endif
}

/*!
 * \brief Returns whether the service management console can be opened by this class.
 */
inline bool SyncthingService::canOpenServiceManager() const
{
#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_WINDOWS_SERVICE
    return true;
#else
    return false;
#endif
}

/*!
 * \brief Returns the SyncthingService instance which has previously been assigned via SyncthingService::setMainInstance().
 */
inline SyncthingService *SyncthingService::mainInstance()
{
    return s_mainInstance;
}

/*!
 * \brief Assigns the "main" SyncthingService instance.
 */
inline void SyncthingService::setMainInstance(SyncthingService *mainInstance)
{
    s_mainInstance = mainInstance;
}

} // namespace Data

#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
Q_DECLARE_METATYPE(Data::ManagerDBusUnitFileChange)
Q_DECLARE_METATYPE(Data::ManagerDBusUnitFileChangeList)
#endif

#endif // defined(LIB_SYNCTHING_CONNECTOR_SUPPORT_SERVICE) && !defined(DATA_SYNCTHINGSERVICE_H)
