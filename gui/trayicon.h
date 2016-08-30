#ifndef TRAY_ICON_H
#define TRAY_ICON_H

#include "./traymenu.h"

#include <c++utilities/application/global.h>

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

private slots:
    void handleActivated(QSystemTrayIcon::ActivationReason reason);
    void showSyncthingError(const QString &errorMsg);
    void showSyncthingNotification(const QString &message);
    void updateStatusIconAndText(Data::SyncthingStatus status);

private:
    QPixmap renderSvgImage(const QString &path);

    const QSize m_size;
    const QIcon m_statusIconDisconnected;
    const QIcon m_statusIconDefault;
    const QIcon m_statusIconNotify;
    const QIcon m_statusIconPause;
    const QIcon m_statusIconSync;
    TrayMenu m_trayMenu;
    QMenu m_contextMenu;
    Data::SyncthingStatus m_status;
};

}

#endif // TRAY_ICON_H
