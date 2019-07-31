#if !defined(SYNCTHINGWIDGETS_DBUSSTATUSNOTIFIER_H) && defined(QT_UTILITIES_SUPPORT_DBUS_NOTIFICATIONS)
#define SYNCTHINGWIDGETS_DBUSSTATUSNOTIFIER_H

#include "./internalerror.h"

#include <qtutilities/misc/dbusnotification.h>

#include <c++utilities/chrono/datetime.h>

#include <QObject>
#include <QStringList>

namespace Data {
enum class SyncthingErrorCategory;
struct StatusIcons;
} // namespace Data

namespace QtGui {

class SYNCTHINGWIDGETS_EXPORT DBusStatusNotifier : public QObject {
    Q_OBJECT

public:
    explicit DBusStatusNotifier(QObject *parent = nullptr);

public Q_SLOTS:
    void showDisconnect();
    void hideDisconnect();
    void showInternalError(const InternalError &error);
    void showLauncherError(const QString &errorMessage, const QString &additionalInfo);
    void showSyncthingNotification(CppUtilities::DateTime when, const QString &message);
    void showSyncComplete(const QString &message);
    void showNewDev(const QString &devId, const QString &message);
    void showNewDir(const QString &devId, const QString &dirId, const QString &message);
    void setIcons(const Data::StatusIcons &statusIcons, const Data::StatusIcons &icons);

Q_SIGNALS:
    void connectRequested();
    void dismissNotificationsRequested();
    void showNotificationsRequested();
    void errorDetailsRequested();
    void webUiRequested();

private Q_SLOTS:
    void handleSyncthingNotificationAction(const QString &action);

private:
    QtUtilities::DBusNotification m_disconnectedNotification;
    QtUtilities::DBusNotification m_internalErrorNotification;
    QtUtilities::DBusNotification m_launcherErrorNotification;
    QtUtilities::DBusNotification m_syncthingNotification;
    QtUtilities::DBusNotification m_syncCompleteNotification;
    QtUtilities::DBusNotification m_newDevNotification;
    QtUtilities::DBusNotification m_newDirNotification;
};

inline void DBusStatusNotifier::showDisconnect()
{
    m_disconnectedNotification.show();
}

inline void DBusStatusNotifier::hideDisconnect()
{
    m_disconnectedNotification.hide();
}

inline void DBusStatusNotifier::showInternalError(const InternalError &error)
{
    m_internalErrorNotification.update(error.message);
}

inline void QtGui::DBusStatusNotifier::showLauncherError(const QString &errorMessage, const QString &additionalInfo)
{
    m_launcherErrorNotification.update(QStringList({ errorMessage, additionalInfo }).join(QStringLiteral("\n    ")));
}

inline void DBusStatusNotifier::showSyncthingNotification(CppUtilities::DateTime when, const QString &message)
{
    Q_UNUSED(when)
    m_syncthingNotification.update(message);
}

inline void DBusStatusNotifier::showSyncComplete(const QString &message)
{
    m_syncCompleteNotification.update(message);
}

inline void DBusStatusNotifier::showNewDev(const QString &devId, const QString &message)
{
    Q_UNUSED(devId)
    m_newDevNotification.update(message);
}

inline void DBusStatusNotifier::showNewDir(const QString &devId, const QString &dirId, const QString &message)
{
    Q_UNUSED(devId)
    Q_UNUSED(dirId)
    m_newDirNotification.update(message);
}

} // namespace QtGui

#endif // !defined(SYNCTHINGWIDGETS_DBUSSTATUSNOTIFIER_H) && defined(QT_UTILITIES_SUPPORT_DBUS_NOTIFICATIONS)
