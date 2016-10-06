#ifndef TRAY_ICON_H
#define TRAY_ICON_H

#include "./traymenu.h"

#include <c++utilities/chrono/datetime.h>

#include <QSystemTrayIcon>
#include <QIcon>

QT_FORWARD_DECLARE_CLASS(QPixmap)

namespace Data {
enum class SyncthingStatus;
}

namespace QtGui {

class TrayIcon : public QSystemTrayIcon
{
    Q_OBJECT

public:
    TrayIcon(QObject *parent = nullptr);
    TrayMenu &trayMenu();

private slots:
    void handleActivated(QSystemTrayIcon::ActivationReason reason);
    void handleMessageClicked();
    void showInternalError(const QString &errorMsg);
    void showSyncthingNotification(ChronoUtilities::DateTime when, const QString &message);
    void updateStatusIconAndText(Data::SyncthingStatus status);

private:
    QPixmap renderSvgImage(const QString &path);

    const QSize m_size;
    const QIcon m_statusIconDisconnected;
    const QIcon m_statusIconIdling;
    const QIcon m_statusIconScanning;
    const QIcon m_statusIconNotify;
    const QIcon m_statusIconPause;
    const QIcon m_statusIconSync;
    const QIcon m_statusIconError;
    const QIcon m_statusIconErrorSync;
    TrayMenu m_trayMenu;
    QMenu m_contextMenu;
    Data::SyncthingStatus m_status;
};

inline TrayMenu &TrayIcon::trayMenu()
{
    return m_trayMenu;
}

}

#endif // TRAY_ICON_H
