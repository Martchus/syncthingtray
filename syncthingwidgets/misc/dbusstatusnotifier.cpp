#ifdef QT_UTILITIES_SUPPORT_DBUS_NOTIFICATIONS
#include "./dbusstatusnotifier.h"

#include <syncthingmodel/syncthingicons.h>

// use meta-data of syncthingtray application here
#include "resources/../../tray/resources/config.h"

#include <QCoreApplication>
#include <QPixmap>

using namespace Data;
using namespace QtUtilities;

namespace QtGui {

inline QImage makeImage(const QIcon &icon)
{
    return icon.pixmap(QSize(128, 128)).toImage();
}

DBusStatusNotifier::DBusStatusNotifier(QObject *parent)
    : QObject(parent)
    , m_disconnectedNotification(QStringLiteral(APP_NAME), QStringLiteral("network-disconnect"), 5000)
    , m_internalErrorNotification(QStringLiteral(APP_NAME) + tr(" - internal error"), NotificationIcon::Critical, 5000)
    , m_launcherErrorNotification(QStringLiteral(APP_NAME) + tr(" - launcher error"), NotificationIcon::Critical, 5000)
    , m_syncthingNotification(tr("Syncthing notification"), NotificationIcon::Warning, 10000)
    , m_syncCompleteNotification(QStringLiteral(APP_NAME), NotificationIcon::Information, 5000)
    , m_newDevNotification(QStringLiteral(APP_NAME) + tr(" - new device"), NotificationIcon::Information, 5000)
    , m_newDirNotification(QStringLiteral(APP_NAME) + tr(" - new folder"), NotificationIcon::Information, 5000)
    , m_newVersionNotification(QStringLiteral(APP_NAME) + tr(" - new version"), NotificationIcon::Information, 5000)
{
    m_disconnectedNotification.setApplicationName(QStringLiteral(APP_NAME));
    m_disconnectedNotification.setMessage(tr("Disconnected from Syncthing"));
    m_disconnectedNotification.setActions(QStringList({ QStringLiteral("reconnect"), tr("Try to reconnect") }));
    connect(&m_disconnectedNotification, &DBusNotification::actionInvoked, this, &DBusStatusNotifier::connectRequested);

    m_internalErrorNotification.setApplicationName(QStringLiteral(APP_NAME));
    m_internalErrorNotification.setActions(QStringList({ QStringLiteral("details"), tr("View details") }));
    connect(&m_internalErrorNotification, &DBusNotification::actionInvoked, this, &DBusStatusNotifier::errorDetailsRequested);

    m_internalErrorNotification.setApplicationName(QStringLiteral(APP_NAME));
    m_internalErrorNotification.setActions(QStringList({ QStringLiteral("details"), tr("View details") }));
    connect(&m_internalErrorNotification, &DBusNotification::actionInvoked, this, &DBusStatusNotifier::errorDetailsRequested);

    m_syncthingNotification.setApplicationName(QStringLiteral(APP_NAME));
    m_syncthingNotification.setActions(QStringList({ QStringLiteral("show"), tr("Show"), QStringLiteral("dismiss"), tr("Dismiss") }));
    connect(&m_syncthingNotification, &DBusNotification::actionInvoked, this, &DBusStatusNotifier::handleSyncthingNotificationAction);

    m_syncCompleteNotification.setApplicationName(QStringLiteral(APP_NAME));

    m_newDevNotification.setApplicationName(QStringLiteral(APP_NAME));
    m_newDevNotification.setActions(QStringList({ QStringLiteral("webui"), tr("Open web UI") }));
    connect(&m_newDevNotification, &DBusNotification::actionInvoked, this, &DBusStatusNotifier::webUiRequested);

    m_newDirNotification.setApplicationName(QStringLiteral(APP_NAME));
    m_newDirNotification.setActions(m_newDevNotification.actions());
    connect(&m_newDirNotification, &DBusNotification::actionInvoked, this, &DBusStatusNotifier::webUiRequested);

    m_newVersionNotification.setApplicationName(QStringLiteral(APP_NAME));
    m_newVersionNotification.setActions(QStringList({ QStringLiteral("update"), tr("Open updater") }));
    connect(&m_newVersionNotification, &DBusNotification::actionInvoked, this, &DBusStatusNotifier::updateSettingsRequested);

    const auto &iconManager = IconManager::instance();
    connect(&iconManager, &Data::IconManager::statusIconsChanged, this, &DBusStatusNotifier::setIcons);
    setIcons(iconManager.statusIcons(), iconManager.trayIcons());
}

void DBusStatusNotifier::setIcons(const StatusIcons &, const StatusIcons &icons)
{
    if (!icons.isValid) {
        return;
    }
    m_launcherErrorNotification.setImage(makeImage(icons.error));
    m_syncthingNotification.setImage(m_launcherErrorNotification.image());
    m_syncthingNotification.setImage(makeImage(icons.notify));
    m_syncCompleteNotification.setImage(makeImage(icons.syncComplete));
    m_newDevNotification.setImage(makeImage(icons.newItem));
    m_newDirNotification.setImage(m_newDevNotification.image());
    m_newVersionNotification.setImage(makeImage(icons.idling));
}

void DBusStatusNotifier::handleSyncthingNotificationAction(const QString &action)
{
    if (action == QLatin1String("dismiss")) {
        emit dismissNotificationsRequested();
    } else if (action == QLatin1String("show")) {
        emit showNotificationsRequested();
    }
}

} // namespace QtGui

#endif
