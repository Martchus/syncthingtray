#include "./trayicon.h"
#include "./traywidget.h"

#include "../application/settings.h"
#include "../data/syncthingconnection.h"

#include <qtutilities/misc/dialogutils.h>

#include <QApplication>
#include <QSvgRenderer>
#include <QPainter>
#include <QPixmap>

using namespace Dialogs;
using namespace Data;

namespace QtGui {

/*!
 * \brief Instantiates a new tray icon.
 */
TrayIcon::TrayIcon(QObject *parent) :
    QSystemTrayIcon(parent),
    m_size(QSize(128, 128)),
    m_statusIconDisconnected(QIcon(renderSvgImage(QStringLiteral(":/icons/hicolor/scalable/status/syncthing-disconnected.svg")))),
    m_statusIconIdling(QIcon(renderSvgImage(QStringLiteral(":/icons/hicolor/scalable/status/syncthing-ok.svg")))),
    m_statusIconScanning(QIcon(renderSvgImage(QStringLiteral(":/icons/hicolor/scalable/status/syncthing-default.svg")))),
    m_statusIconNotify(QIcon(renderSvgImage(QStringLiteral(":/icons/hicolor/scalable/status/syncthing-notify.svg")))),
    m_statusIconPause(QIcon(renderSvgImage(QStringLiteral(":/icons/hicolor/scalable/status/syncthing-pause.svg")))),
    m_statusIconSync(QIcon(renderSvgImage(QStringLiteral(":/icons/hicolor/scalable/status/syncthing-sync.svg")))),
    m_trayMenu(this),
    m_status(SyncthingStatus::Disconnected)
{
    // set context menu
    connect(m_contextMenu.addAction(QIcon::fromTheme(QStringLiteral("internet-web-browser"), QIcon(QStringLiteral(":/icons/hicolor/scalable/apps/internet-web-browser.svg"))), tr("Web UI")), &QAction::triggered, m_trayMenu.widget(), &TrayWidget::showWebUi);
    connect(m_contextMenu.addAction(QIcon::fromTheme(QStringLiteral("preferences-other"), QIcon(QStringLiteral(":/icons/hicolor/scalable/apps/preferences-other.svg"))), tr("Settings")), &QAction::triggered, m_trayMenu.widget(), &TrayWidget::showSettingsDialog);
    connect(m_contextMenu.addAction(QIcon::fromTheme(QStringLiteral("folder-sync"), QIcon(QStringLiteral(":/icons/hicolor/scalable/actions/folder-sync.svg"))), tr("Rescan all")), &QAction::triggered, &m_trayMenu.widget()->connection(), &SyncthingConnection::rescanAllDirs);
    m_contextMenu.addMenu(m_trayMenu.widget()->connectionsMenu());
    connect(m_contextMenu.addAction(QIcon::fromTheme(QStringLiteral("help-about"), QIcon(QStringLiteral(":/icons/hicolor/scalable/apps/help-about.svg"))), tr("About")), &QAction::triggered, m_trayMenu.widget(), &TrayWidget::showAboutDialog);
    m_contextMenu.addSeparator();
    connect(m_contextMenu.addAction(QIcon::fromTheme(QStringLiteral("window-close"), QIcon(QStringLiteral(":/icons/hicolor/scalable/actions/window-close.svg"))), tr("Close")), &QAction::triggered, this, &TrayIcon::deleteLater);
    setContextMenu(&m_contextMenu);

    // set initial status
    updateStatusIconAndText(SyncthingStatus::Disconnected);

    // connect signals and slots
    SyncthingConnection *connection = &(m_trayMenu.widget()->connection());
    connect(this, &TrayIcon::activated, this, &TrayIcon::handleActivated);
    connect(this, &TrayIcon::messageClicked, this, &TrayIcon::handleMessageClicked);
    connect(connection, &SyncthingConnection::error, this, &TrayIcon::showInternalError);
    connect(connection, &SyncthingConnection::newNotification, this, &TrayIcon::showSyncthingNotification);
    connect(connection, &SyncthingConnection::statusChanged, this, &TrayIcon::updateStatusIconAndText);
}

void TrayIcon::handleActivated(QSystemTrayIcon::ActivationReason reason)
{
    switch(reason) {
    case QSystemTrayIcon::Context:
        // can't catch that event on Plasma 5 anyways
        break;
    case QSystemTrayIcon::MiddleClick:
        m_trayMenu.widget()->showWebUi();
        break;
    case QSystemTrayIcon::Trigger:
        m_trayMenu.resize(m_trayMenu.sizeHint());
        // when showing the menu manually
        // move the menu to the closest of the currently available screen
        // this implies that the tray icon is located near the edge of the screen; otherwise this behavior makes no sense
        cornerWidget(&m_trayMenu);
        m_trayMenu.show();
        break;
    default:
        ;
    }
}

void TrayIcon::handleMessageClicked()
{
    m_trayMenu.widget()->connection().considerAllNotificationsRead();
}

void TrayIcon::showInternalError(const QString &errorMsg)
{
    if(Settings::notifyOnInternalErrors()) {
        showMessage(tr("Error"), errorMsg, QSystemTrayIcon::Critical);
    }
}

void TrayIcon::showSyncthingNotification(ChronoUtilities::DateTime when, const QString &message)
{
    if(Settings::showSyncthingNotifications()) {
        showMessage(tr("Syncthing notification - click to dismiss"), message, QSystemTrayIcon::Warning);
    }
}

void TrayIcon::updateStatusIconAndText(SyncthingStatus status)
{
    switch(status) {
    case SyncthingStatus::Disconnected:
        setIcon(m_statusIconDisconnected);
        setToolTip(tr("Not connected to Syncthing"));
        if(Settings::notifyOnDisconnect()) {
            showMessage(QCoreApplication::applicationName(), tr("Disconnected from Syncthing"), QSystemTrayIcon::Warning);
        }
        break;
    case SyncthingStatus::Reconnecting:
        setIcon(m_statusIconDisconnected);
        setToolTip(tr("Reconnecting ..."));
        break;
    case SyncthingStatus::Idle:
        setIcon(m_statusIconIdling);
        setToolTip(tr("Syncthing is idling"));
        break;
    case SyncthingStatus::Scanning:
        setIcon(m_statusIconScanning);
        setToolTip(tr("Syncthing is scanning"));
        break;
    case SyncthingStatus::NotificationsAvailable:
        setIcon(m_statusIconNotify);
        setToolTip(tr("Notifications available"));
        break;
    case SyncthingStatus::Paused:
        setIcon(m_statusIconPause);
        setToolTip(tr("At least one device is paused"));
        break;
    case SyncthingStatus::Synchronizing:
        setIcon(m_statusIconSync);
        setToolTip(tr("Synchronization is ongoing"));
        break;
    }
    switch(status) {
    case SyncthingStatus::Disconnected:
    case SyncthingStatus::Reconnecting:
    case SyncthingStatus::Synchronizing:
        break;
    default:
        if(m_status == SyncthingStatus::Synchronizing && Settings::notifyOnSyncComplete()) {
            showMessage(QCoreApplication::applicationName(), tr("Synchronization complete"), QSystemTrayIcon::Information);
        }
    }

    m_status = status;
}

/*!
 * \brief Renders an SVG image to a QPixmap.
 * \remarks If instantiating QIcon directly from SVG image the icon is not displayed under Plasma 5. It would work
 *          with Tint2, tough.
 */
QPixmap TrayIcon::renderSvgImage(const QString &path)
{
    QSvgRenderer renderer(path);
    QPixmap pm(m_size);
    pm.fill(QColor(Qt::transparent));
    QPainter painter(&pm);
    renderer.render(&painter, pm.rect());
    return pm;
}

}
