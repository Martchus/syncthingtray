#ifndef TRAY_ICON_H
#define TRAY_ICON_H

#include "./traymenu.h"

#include <syncthingwidgets/misc/dbusstatusnotifier.h>

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

enum class TrayIconMessageClickedAction { None, DismissNotification, ShowInternalErrors, ShowWebUi, ShowUpdateSettings };

class TrayIcon : public QSystemTrayIcon {
    Q_OBJECT

public:
    explicit TrayIcon(const QString &connectionConfig = QString(), QObject *parent = nullptr);
    TrayMenu &trayMenu();

public Q_SLOTS:
    void showInternalError(const QString &errorMessage, Data::SyncthingErrorCategory category, int networkError, const QNetworkRequest &request,
        const QByteArray &response);
    void showLauncherError(const QString &errorMessage, const QString &additionalInfo);
    void showSyncthingNotification(CppUtilities::DateTime when, const QString &message);
    void showInternalErrorsDialog();
    void updateStatusIconAndText();
    void showNewDev(const QString &devId, const QString &message);
    void showNewDir(const QString &devId, const QString &dirId, const QString &dirLabel, const QString &message);
    void showNewVersionAvailable(const QString &version, const QString &additionalInfo);

private Q_SLOTS:
    void handleActivated(QSystemTrayIcon::ActivationReason reason);
    void handleMessageClicked();
    void showDisconnected();
    void showSyncComplete(const QString &message);
    void handleErrorsCleared();

private:
    QWidget m_parentWidget;
    TrayMenu *m_trayMenu;
#ifndef SYNCTHINGTRAY_UNIFY_TRAY_MENUS
    QMenu m_contextMenu;
    QAction *m_errorsAction;
#endif
#ifdef QT_UTILITIES_SUPPORT_DBUS_NOTIFICATIONS
    DBusStatusNotifier m_dbusNotifier;
    bool &m_dbusNotificationsEnabled;
#endif
    bool &m_notifyOnSyncthingErrors;
    TrayIconMessageClickedAction m_messageClickedAction;
};

inline TrayMenu &TrayIcon::trayMenu()
{
    return *m_trayMenu;
}

} // namespace QtGui

#endif // TRAY_ICON_H
