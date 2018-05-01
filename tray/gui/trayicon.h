#ifndef TRAY_ICON_H
#define TRAY_ICON_H

#include "./traymenu.h"

#include "../../widgets/misc/dbusstatusnotifier.h"

#include <c++utilities/chrono/datetime.h>

#include <QIcon>
#include <QSystemTrayIcon>

QT_FORWARD_DECLARE_CLASS(QPixmap)
QT_FORWARD_DECLARE_CLASS(QNetworkRequest)

namespace Data {
enum class SyncthingStatus;
enum class SyncthingErrorCategory;
struct SyncthingDir;
struct SyncthingDev;
} // namespace Data

namespace QtGui {

enum class TrayIconMessageClickedAction { None, DismissNotification, ShowInternalErrors, ShowWebUi };

class TrayIcon : public QSystemTrayIcon {
    Q_OBJECT

public:
    TrayIcon(const QString &connectionConfig = QString(), QObject *parent = nullptr);
    TrayMenu &trayMenu();

public slots:
    void showInternalError(
        const QString &errorMsg, Data::SyncthingErrorCategory category, int networkError, const QNetworkRequest &request, const QByteArray &response);
    void showSyncthingNotification(ChronoUtilities::DateTime when, const QString &message);
    void showInternalErrorsDialog();
    void updateStatusIconAndText();
    void showNewDev(const QString &devId, const QString &message);
    void showNewDir(const QString &devId, const QString &dirId, const QString &message);

private slots:
    void handleActivated(QSystemTrayIcon::ActivationReason reason);
    void handleMessageClicked();
    void showDisconnected();
    void showSyncComplete(const QString &message);
    void handleErrorsCleared();

private:
    TrayMenu m_trayMenu;
    QMenu m_contextMenu;
    QAction *m_errorsAction;
#ifdef QT_UTILITIES_SUPPORT_DBUS_NOTIFICATIONS
    DBusStatusNotifier m_dbusNotifier;
    bool &m_dbusNotificationsEnabled;
    bool &m_notifyOnSyncthingErrors;
#endif
    TrayIconMessageClickedAction m_messageClickedAction;
};

inline TrayMenu &TrayIcon::trayMenu()
{
    return m_trayMenu;
}

} // namespace QtGui

#endif // TRAY_ICON_H
