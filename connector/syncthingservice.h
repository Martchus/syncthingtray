#if defined(LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD) && !defined(DATA_SYNCTHINGSERVICE_H)
#define DATA_SYNCTHINGSERVICE_H

#include <c++utilities/chrono/datetime.h>

#include <QObject>
#include <QVariantMap>

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

namespace Data {

struct ManagerDBusUnitFileChange {
    QString type;
    QString path;
    QString source;
};

QDBusArgument &operator<<(QDBusArgument &argument, const ManagerDBusUnitFileChange &unitFileChange);
const QDBusArgument &operator>>(const QDBusArgument &argument, ManagerDBusUnitFileChange &unitFileChange);

typedef QList<ManagerDBusUnitFileChange> ManagerDBusUnitFileChangeList;

class SyncthingService : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString unitName READ unitName WRITE setUnitName)
    Q_PROPERTY(bool systemdAvailable READ isSystemdAvailable NOTIFY systemdAvailableChanged)
    Q_PROPERTY(bool unitAvailable READ isUnitAvailable)
    Q_PROPERTY(QString activeState READ activeState NOTIFY activeStateChanged)
    Q_PROPERTY(QString subState READ subState NOTIFY subStateChanged)
    Q_PROPERTY(ChronoUtilities::DateTime activeSince READ activeSince NOTIFY activeStateChanged)
    Q_PROPERTY(QString unitFileState READ unitFileState NOTIFY unitFileStateChanged)
    Q_PROPERTY(QString description READ description NOTIFY descriptionChanged)
    Q_PROPERTY(bool running READ isRunning WRITE setRunning NOTIFY runningChanged)
    Q_PROPERTY(bool enable READ isEnabled WRITE setEnabled NOTIFY enabledChanged)
    Q_PROPERTY(bool manuallyStopped READ isManuallyStopped)

public:
    explicit SyncthingService(QObject *parent = nullptr);

    const QString &unitName() const;
    bool isSystemdAvailable() const;
    bool isUnitAvailable() const;
    const QString &activeState() const;
    const QString &subState() const;
    ChronoUtilities::DateTime activeSince() const;
    bool isActiveFor(unsigned int atLeastSeconds) const;
    bool isActiveWithoutSleepFor(unsigned int atLeastSeconds) const;
    static ChronoUtilities::DateTime lastWakeUp();
    const QString &unitFileState() const;
    const QString &description() const;
    bool isRunning() const;
    bool isEnabled() const;
    bool isManuallyStopped() const;

public Q_SLOTS:
    void setUnitName(const QString &unitName);
    void setRunning(bool running);
    void start();
    void stop();
    void toggleRunning();
    void setEnabled(bool enable);
    void enable();
    void disable();

Q_SIGNALS:
    void systemdAvailableChanged(bool available);
    void stateChanged(const QString &activeState, const QString &subState, ChronoUtilities::DateTime activeSince);
    void activeStateChanged(const QString &activeState);
    void subStateChanged(const QString &subState);
    void unitFileStateChanged(const QString &unitFileState);
    void descriptionChanged(const QString &description);
    void runningChanged(bool running);
    void enabledChanged(bool enable);
    void errorOccurred(const QString &context, const QString &name, const QString &message);

private Q_SLOTS:
    void handleUnitAdded(const QString &unitName, const QDBusObjectPath &unitPath);
    void handleUnitRemoved(const QString &unitName, const QDBusObjectPath &unitPath);
    void handleUnitGet(QDBusPendingCallWatcher *watcher);
    void handlePropertiesChanged(const QString &interface, const QVariantMap &changedProperties, const QStringList &invalidatedProperties);
    void handleError(const char *error, QDBusPendingCallWatcher *watcher);
    void handleServiceRegisteredChanged(const QString &service);
    static void handlePrepareForSleep(bool rightBefore);
    void setUnit(const QDBusObjectPath &objectPath);
    void setProperties(const QString &activeState, const QString &subState, const QString &unitFileState, const QString &description);

private:
    bool handlePropertyChanged(QString &variable, void(SyncthingService::*signal)(const QString &), const QString &propertyName, const QVariantMap &changedProperties, const QStringList &invalidatedProperties);
    bool handlePropertyChanged(ChronoUtilities::DateTime &variable, const QString &propertyName, const QVariantMap &changedProperties, const QStringList &invalidatedProperties);
    void registerErrorHandler(const QDBusPendingCall &call, const char *context);

    static OrgFreedesktopSystemd1ManagerInterface *s_manager;
    static OrgFreedesktopLogin1ManagerInterface *s_loginManager;
    static bool s_fallingAsleep;
    static ChronoUtilities::DateTime s_lastWakeUp;
    QString m_unitName;
    QDBusServiceWatcher *m_serviceWatcher;
    OrgFreedesktopSystemd1UnitInterface *m_unit;
    OrgFreedesktopSystemd1ServiceInterface *m_service;
    OrgFreedesktopDBusPropertiesInterface *m_properties;
    QString m_description;
    QString m_activeState;
    QString m_subState;
    QString m_unitFileState;
    bool m_manuallyStopped;
    ChronoUtilities::DateTime m_activeSince;
};

inline const QString &SyncthingService::unitName() const
{
    return m_unitName;
}

inline const QString &SyncthingService::activeState() const
{
    return m_activeState;
}

inline const QString &SyncthingService::subState() const
{
    return m_subState;
}

inline const QString &SyncthingService::unitFileState() const
{
    return m_unitFileState;
}

inline const QString &SyncthingService::description() const
{
    return m_description;
}

inline bool SyncthingService::isRunning() const
{
    return m_activeState == QLatin1String("active") && m_subState == QLatin1String("running");
}

inline void SyncthingService::start()
{
    setRunning(true);
}

inline void SyncthingService::stop()
{
    setRunning(false);
}

inline void SyncthingService::toggleRunning()
{
    setRunning(!isRunning());
}

inline bool SyncthingService::isEnabled() const
{
    return m_unitFileState == QLatin1String("enabled");
}

inline bool SyncthingService::isManuallyStopped() const
{
    return m_manuallyStopped;
}

inline ChronoUtilities::DateTime SyncthingService::activeSince() const
{
    return m_activeSince;
}

inline bool SyncthingService::isActiveFor(unsigned int atLeastSeconds) const
{
    return !m_activeSince.isNull() && (ChronoUtilities::DateTime::gmtNow() - m_activeSince).totalSeconds() > atLeastSeconds;
}

inline ChronoUtilities::DateTime SyncthingService::lastWakeUp()
{
    return s_lastWakeUp;
}

inline void SyncthingService::enable()
{
    setEnabled(true);
}

inline void SyncthingService::disable()
{
    setEnabled(false);
}

SyncthingService &syncthingService();

} // namespace Data

Q_DECLARE_METATYPE(Data::ManagerDBusUnitFileChange)
Q_DECLARE_METATYPE(Data::ManagerDBusUnitFileChangeList)

#endif // defined(LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD) && !defined(DATA_SYNCTHINGSERVICE_H)
