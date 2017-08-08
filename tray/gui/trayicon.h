#ifndef TRAY_ICON_H
#define TRAY_ICON_H

#include "./traymenu.h"

#include "../../widgets/misc/dbusstatusnotifier.h"

#include <c++utilities/chrono/datetime.h>

#include <QIcon>
#include <QSystemTrayIcon>

QT_FORWARD_DECLARE_CLASS(QPixmap)

namespace Data {
enum class SyncthingStatus;
enum class SyncthingErrorCategory;
}

namespace QtGui {

class TrayIcon : public QSystemTrayIcon {
    Q_OBJECT

public:
    TrayIcon(const QString &connectionConfig = QString(), QObject *parent = nullptr);
    TrayMenu &trayMenu();

public slots:
    void showInternalError(const QString &errorMsg, Data::SyncthingErrorCategory category, int networkError);
    void showSyncthingNotification(ChronoUtilities::DateTime when, const QString &message);
    void showStatusNotification(Data::SyncthingStatus status);
    void updateStatusIconAndText();

private slots:
    void handleActivated(QSystemTrayIcon::ActivationReason reason);
    void handleConnectionStatusChanged(Data::SyncthingStatus status);

private:
    bool m_initialized;
    TrayMenu m_trayMenu;
    QMenu m_contextMenu;
    Data::SyncthingStatus m_status;
#ifdef QT_UTILITIES_SUPPORT_DBUS_NOTIFICATIONS
    DBusStatusNotifier m_dbusNotifier;
#endif
};

inline TrayMenu &TrayIcon::trayMenu()
{
    return m_trayMenu;
}
}

#endif // TRAY_ICON_H
