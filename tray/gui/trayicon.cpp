#include "./trayicon.h"
#include "./traywidget.h"

#include "../../widgets/misc/errorviewdialog.h"
#include "../../widgets/misc/statusinfo.h"
#include "../../widgets/misc/textviewdialog.h"
#include "../../widgets/settings/settings.h"

#include "../../model/syncthingicons.h"

#include "../../connector/syncthingconnection.h"
#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
#include "../../connector/syncthingservice.h"
#endif
#include "../../connector/utils.h"

#include <qtutilities/misc/dialogutils.h>

#include <QCoreApplication>
#include <QPainter>
#include <QPixmap>
#include <QStringBuilder>
#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
#include <QNetworkReply>
#endif

using namespace std;
using namespace Dialogs;
using namespace Data;
#ifdef QT_UTILITIES_SUPPORT_DBUS_NOTIFICATIONS
using namespace MiscUtils;
#endif

namespace QtGui {

/*!
 * \brief Instantiates a new tray icon.
 */
TrayIcon::TrayIcon(const QString &connectionConfig, QObject *parent)
    : QSystemTrayIcon(parent)
    , m_trayMenu(connectionConfig, this)
    , m_messageClickedAction(TrayIconMessageClickedAction::None)
{
    // set context menu
    connect(m_contextMenu.addAction(QIcon::fromTheme(QStringLiteral("internet-web-browser"),
                                        QIcon(QStringLiteral(":/icons/hicolor/scalable/apps/internet-web-browser.svg"))),
                tr("Web UI")),
        &QAction::triggered, &m_trayMenu.widget(), &TrayWidget::showWebUi);
    connect(m_contextMenu.addAction(
                QIcon::fromTheme(QStringLiteral("preferences-other"), QIcon(QStringLiteral(":/icons/hicolor/scalable/apps/preferences-other.svg"))),
                tr("Settings")),
        &QAction::triggered, &m_trayMenu.widget(), &TrayWidget::showSettingsDialog);
    connect(m_contextMenu.addAction(
                QIcon::fromTheme(QStringLiteral("folder-sync"), QIcon(QStringLiteral(":/icons/hicolor/scalable/actions/folder-sync.svg"))),
                tr("Rescan all")),
        &QAction::triggered, &m_trayMenu.widget().connection(), &SyncthingConnection::rescanAllDirs);
    connect(m_contextMenu.addAction(
                QIcon::fromTheme(QStringLiteral("text-x-generic"), QIcon(QStringLiteral(":/icons/hicolor/scalable/mimetypes/text-x-generic.svg"))),
                tr("Log")),
        &QAction::triggered, &m_trayMenu.widget(), &TrayWidget::showLog);
    m_errorsAction = m_contextMenu.addAction(
        QIcon::fromTheme(QStringLiteral("emblem-error"), QIcon(QStringLiteral(":/icons/hicolor/scalable/emblems/8/emblem-error.svg"))),
        tr("Show internal errors"));
    m_errorsAction->setVisible(false);
    connect(m_errorsAction, &QAction::triggered, this, &TrayIcon::showInternalErrorsDialog);
    m_contextMenu.addMenu(m_trayMenu.widget().connectionsMenu());
    connect(m_contextMenu.addAction(
                QIcon::fromTheme(QStringLiteral("help-about"), QIcon(QStringLiteral(":/icons/hicolor/scalable/apps/help-about.svg"))), tr("About")),
        &QAction::triggered, &m_trayMenu.widget(), &TrayWidget::showAboutDialog);
    m_contextMenu.addSeparator();
    connect(m_contextMenu.addAction(
                QIcon::fromTheme(QStringLiteral("window-close"), QIcon(QStringLiteral(":/icons/hicolor/scalable/actions/window-close.svg"))),
                tr("Close")),
        &QAction::triggered, this, &TrayIcon::deleteLater);
    setContextMenu(&m_contextMenu);

    // connect signals and slots
    const SyncthingConnection &connection = m_trayMenu.widget().connection();
    const SyncthingNotifier &notifier = m_trayMenu.widget().notifier();
    connect(this, &TrayIcon::activated, this, &TrayIcon::handleActivated);
    connect(this, &TrayIcon::messageClicked, this, &TrayIcon::handleMessageClicked);
    connect(&connection, &SyncthingConnection::error, this, &TrayIcon::showInternalError);
    connect(&connection, &SyncthingConnection::newNotification, this, &TrayIcon::showSyncthingNotification);
    connect(&notifier, &SyncthingNotifier::disconnected, this, &TrayIcon::showDisconnected);
    connect(&notifier, &SyncthingNotifier::syncComplete, this, &TrayIcon::showSyncComplete);
    connect(&connection, &SyncthingConnection::statusChanged, this, &TrayIcon::updateStatusIconAndText);
    connect(&connection, &SyncthingConnection::newDevices, this, &TrayIcon::updateStatusIconAndText);
    connect(&connection, &SyncthingConnection::devStatusChanged, this, &TrayIcon::updateStatusIconAndText);
#ifdef QT_UTILITIES_SUPPORT_DBUS_NOTIFICATIONS
    connect(&m_dbusNotifier, &DBusStatusNotifier::connectRequested, &connection,
        static_cast<void (SyncthingConnection::*)(void)>(&SyncthingConnection::connect));
    connect(&m_dbusNotifier, &DBusStatusNotifier::dismissNotificationsRequested, &m_trayMenu.widget(), &TrayWidget::dismissNotifications);
    connect(&m_dbusNotifier, &DBusStatusNotifier::showNotificationsRequested, &m_trayMenu.widget(), &TrayWidget::showNotifications);
    connect(&m_dbusNotifier, &DBusStatusNotifier::errorDetailsRequested, this, &TrayIcon::showInternalErrorsDialog);
    connect(&notifier, &SyncthingNotifier::connected, &m_dbusNotifier, &DBusStatusNotifier::hideDisconnect);
#endif
}

/*!
 * \brief Moves the specified \a point in the specified \a rect.
 */
void moveInside(QPoint &point, const QRect &rect)
{
    if (point.y() < rect.top()) {
        point.setY(rect.top());
    } else if (point.y() > rect.bottom()) {
        point.setY(rect.bottom());
    }
    if (point.x() < rect.left()) {
        point.setX(rect.left());
    } else if (point.x() > rect.right()) {
        point.setX(rect.right());
    }
}

void TrayIcon::handleActivated(QSystemTrayIcon::ActivationReason reason)
{
    switch (reason) {
    case QSystemTrayIcon::Context:
        // can't catch that event on Plasma 5 anyways
        break;
    case QSystemTrayIcon::MiddleClick:
        m_trayMenu.widget().showWebUi();
        break;
    case QSystemTrayIcon::Trigger: {
        m_trayMenu.showAtCursor();
        break;
    }
    default:;
    }
}

void TrayIcon::handleMessageClicked()
{
    switch (m_messageClickedAction) {
    case TrayIconMessageClickedAction::None:
        return;
    case TrayIconMessageClickedAction::DismissNotification:
        m_trayMenu.widget().dismissNotifications();
        break;
    case TrayIconMessageClickedAction::ShowInternalErrors:
        showInternalErrorsDialog();
        break;
    }
}

void TrayIcon::showDisconnected()
{
#ifdef QT_UTILITIES_SUPPORT_DBUS_NOTIFICATIONS
    if (Settings::values().dbusNotifications) {
        m_dbusNotifier.showDisconnect();
    } else
#endif
    {
        m_messageClickedAction = TrayIconMessageClickedAction::None;
        showMessage(QCoreApplication::applicationName(), tr("Disconnected from Syncthing"), QSystemTrayIcon::Warning);
    }
}

void TrayIcon::showSyncComplete(const QString &message)
{
#ifdef QT_UTILITIES_SUPPORT_DBUS_NOTIFICATIONS
    if (Settings::values().dbusNotifications) {
        m_dbusNotifier.showSyncComplete(message);
    } else
#endif
    {
        m_messageClickedAction = TrayIconMessageClickedAction::None;
        showMessage(QCoreApplication::applicationName(), message, QSystemTrayIcon::Information);
    }
}

void TrayIcon::handleErrorsCleared()
{
    m_errorsAction->setVisible(false);
}

void TrayIcon::showInternalError(
    const QString &errorMsg, SyncthingErrorCategory category, int networkError, const QNetworkRequest &request, const QByteArray &response)
{
    if (!InternalError::isRelevant(m_trayMenu.widget().connection(), category, networkError)) {
        return;
    }
    InternalError error(errorMsg, request.url(), response);
#ifdef QT_UTILITIES_SUPPORT_DBUS_NOTIFICATIONS
    if (Settings::values().dbusNotifications) {
        m_dbusNotifier.showInternalError(error);
    } else
#endif
    {
        m_messageClickedAction = TrayIconMessageClickedAction::ShowInternalErrors;
        showMessage(tr("Error"), errorMsg, QSystemTrayIcon::Critical);
    }
    ErrorViewDialog::addError(move(error));
    m_errorsAction->setVisible(true);
}

void TrayIcon::showSyncthingNotification(ChronoUtilities::DateTime when, const QString &message)
{
    const auto &settings(Settings::values());
    if (settings.notifyOn.syncthingErrors) {
#ifdef QT_UTILITIES_SUPPORT_DBUS_NOTIFICATIONS
        if (settings.dbusNotifications) {
            m_dbusNotifier.showSyncthingNotification(when, message);
        } else
#else
        Q_UNUSED(when)
#endif
        {
            m_messageClickedAction = TrayIconMessageClickedAction::DismissNotification;
            showMessage(tr("Syncthing notification - click to dismiss"), message, QSystemTrayIcon::Warning);
        }
    }
    updateStatusIconAndText();
}

void TrayIcon::updateStatusIconAndText()
{
    const StatusInfo statusInfo(trayMenu().widget().connection());
    if (statusInfo.additionalStatusText().isEmpty()) {
        setToolTip(statusInfo.statusText());
    } else {
        setToolTip(statusInfo.statusText() % QChar('\n') % statusInfo.additionalStatusText());
    }
    setIcon(statusInfo.statusIcon());
}

void TrayIcon::showInternalErrorsDialog()
{
    auto *const errorViewDlg = ErrorViewDialog::instance();
    connect(errorViewDlg, &ErrorViewDialog::errorsCleared, this, &TrayIcon::handleErrorsCleared);
    centerWidget(errorViewDlg);
    errorViewDlg->show();
}
} // namespace QtGui
