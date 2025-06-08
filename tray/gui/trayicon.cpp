#include "./trayicon.h"
#include "./traywidget.h"

#include <syncthingwidgets/misc/internalerrorsdialog.h>
#include <syncthingwidgets/misc/statusinfo.h>
#include <syncthingwidgets/misc/textviewdialog.h>
#include <syncthingwidgets/settings/settings.h>

#include <syncthingmodel/syncthingicons.h>

#include <syncthingconnector/syncthingconnection.h>
#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
#include <syncthingconnector/syncthingservice.h>
#endif
#include <syncthingconnector/utils.h>

#include <qtutilities/misc/dialogutils.h>

#include <QCoreApplication>
#include <QPainter>
#include <QPixmap>
#include <QStringBuilder>
#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
#include <QNetworkReply>
#endif

using namespace std;
using namespace QtUtilities;
using namespace Data;

namespace QtGui {

/*!
 * \brief Instantiates a new tray icon.
 */
TrayIcon::TrayIcon(const QString &connectionConfig, QObject *parent)
    : QSystemTrayIcon(parent)
    , m_trayMenu(new TrayMenu(this, &m_parentWidget))
#ifdef QT_UTILITIES_SUPPORT_DBUS_NOTIFICATIONS
    , m_dbusNotificationsEnabled(Settings::values().dbusNotifications)
#endif
    , m_notifyOnSyncthingErrors(Settings::values().notifyOn.syncthingErrors)
    , m_messageClickedAction(TrayIconMessageClickedAction::None)
{
    // get widget, connection and notifier
    const auto &widget(trayMenu().widget());
    const auto &connection(widget.connection());
    const auto &notifier(widget.notifier());

    // set context menu
#ifndef SYNCTHINGTRAY_UNIFY_TRAY_MENUS
    connect(m_contextMenu.addAction(
                QIcon::fromTheme(QStringLiteral("syncthing"), QIcon(QStringLiteral(":/icons/hicolor/scalable/status/syncthing-default.svg"))),
                tr("Open Syncthing")),
        &QAction::triggered, &widget, &TrayWidget::showWebUI);
    connect(m_contextMenu.addAction(
                QIcon::fromTheme(QStringLiteral("preferences-other"), QIcon(QStringLiteral(":/icons/hicolor/scalable/apps/preferences-other.svg"))),
                tr("Settings")),
        &QAction::triggered, &widget, &TrayWidget::showSettingsDialog);
    connect(m_contextMenu.addAction(
                QIcon::fromTheme(QStringLiteral("folder-sync"), QIcon(QStringLiteral(":/icons/hicolor/scalable/actions/folder-sync.svg"))),
                tr("Rescan all")),
        &QAction::triggered, &widget.connection(), &SyncthingConnection::rescanAllDirs);
    connect(m_contextMenu.addAction(
                QIcon::fromTheme(QStringLiteral("text-x-generic"), QIcon(QStringLiteral(":/icons/hicolor/scalable/mimetypes/text-x-generic.svg"))),
                tr("Log")),
        &QAction::triggered, &widget, &TrayWidget::showLog);
    m_errorsAction = m_contextMenu.addAction(
        QIcon::fromTheme(QStringLiteral("emblem-error"), QIcon(QStringLiteral(":/icons/hicolor/scalable/emblems/8/emblem-error.svg"))),
        tr("Show internal errors"));
    m_errorsAction->setVisible(false);
    connect(m_errorsAction, &QAction::triggered, this, &TrayIcon::showInternalErrorsDialog);
    auto *const notificationsAction = m_contextMenu.addAction(
        QIcon::fromTheme(QStringLiteral("emblem-warning"), QIcon(QStringLiteral(":/icons/hicolor/scalable/emblems/8/emblem-warning.svg"))),
        tr("Show notifications/errors"));
    notificationsAction->setVisible(widget.connection().hasErrors());
    connect(&widget.connection(), &SyncthingConnection::newErrors, notificationsAction,
        [notificationsAction](const std::vector<SyncthingError> &errors) { notificationsAction->setVisible(!errors.empty()); });
    connect(notificationsAction, &QAction::triggered, &widget, &TrayWidget::showNotifications);
    m_contextMenu.addMenu(trayMenu().widget().connectionsMenu());
    connect(m_contextMenu.addAction(
                QIcon::fromTheme(QStringLiteral("help-about"), QIcon(QStringLiteral(":/icons/hicolor/scalable/apps/help-about.svg"))), tr("About")),
        &QAction::triggered, &widget, &TrayWidget::showAboutDialog);
    m_contextMenu.addSeparator();
    connect(m_contextMenu.addAction(
                QIcon::fromTheme(QStringLiteral("window-close"), QIcon(QStringLiteral(":/icons/hicolor/scalable/actions/window-close.svg"))),
                tr("Close")),
        &QAction::triggered, this, &TrayIcon::deleteLater);
    setContextMenu(&m_contextMenu);
#endif

    // connect signals and slots
    connect(this, &TrayIcon::activated, this, &TrayIcon::handleActivated);
    connect(this, &TrayIcon::messageClicked, this, &TrayIcon::handleMessageClicked);
    connect(&connection, &SyncthingConnection::error, this, &TrayIcon::showInternalError);
    connect(&connection, &SyncthingConnection::newNotification, this, &TrayIcon::showSyncthingNotification);
    connect(&notifier, &SyncthingNotifier::syncthingProcessError, this, &TrayIcon::showLauncherError);
    connect(&notifier, &SyncthingNotifier::disconnected, this, &TrayIcon::showDisconnected);
    connect(&notifier, &SyncthingNotifier::syncComplete, this, &TrayIcon::showSyncComplete);
    connect(&notifier, &SyncthingNotifier::newDevice, this, &TrayIcon::showNewDev);
    connect(&notifier, &SyncthingNotifier::newDir, this, &TrayIcon::showNewDir);
    connect(&connection, &SyncthingConnection::statusChanged, this, &TrayIcon::updateStatusIconAndText);
    connect(&connection, &SyncthingConnection::autoReconnectIntervalChanged, this, &TrayIcon::updateStatusIconAndText);
    connect(&connection, &SyncthingConnection::hasOutOfSyncDirsChanged, this, &TrayIcon::updateStatusIconAndText);
    connect(&connection, &SyncthingConnection::newDevices, this, &TrayIcon::updateStatusIconAndText);
    connect(&connection, &SyncthingConnection::devStatusChanged, this, &TrayIcon::updateStatusIconAndText);
    connect(&IconManager::instance(), &IconManager::statusIconsChanged, this, &TrayIcon::updateStatusIconAndText);
#ifdef QT_UTILITIES_SUPPORT_DBUS_NOTIFICATIONS
    connect(&m_dbusNotifier, &DBusStatusNotifier::connectRequested, &connection,
        static_cast<void (SyncthingConnection::*)(void)>(&SyncthingConnection::connect));
    connect(&m_dbusNotifier, &DBusStatusNotifier::dismissNotificationsRequested, &connection, &SyncthingConnection::requestClearingErrors);
    connect(&m_dbusNotifier, &DBusStatusNotifier::showNotificationsRequested, &widget, &TrayWidget::showNotifications);
    connect(&m_dbusNotifier, &DBusStatusNotifier::errorDetailsRequested, this, &TrayIcon::showInternalErrorsDialog);
    connect(&m_dbusNotifier, &DBusStatusNotifier::webUiRequested, &widget, &TrayWidget::showWebUI);
    connect(&m_dbusNotifier, &DBusStatusNotifier::updateSettingsRequested, &widget, &TrayWidget::showUpdateSettings);
    connect(&notifier, &SyncthingNotifier::connected, &m_dbusNotifier, &DBusStatusNotifier::hideDisconnect);
#endif

    // apply settings, this also establishes the connection to Syncthing (according to settings)
    // note: It is important to apply settings only after all Signals & Slots have been connected (e.g. to handle SyncthingConnection::error()).
    // note: This weirdly calls updateStatusIconAndText(). So there is not need to call it again within this constructor.
    trayMenu().widget().applySettings(connectionConfig);
}

void TrayIcon::handleActivated(QSystemTrayIcon::ActivationReason reason)
{
    switch (reason) {
    case QSystemTrayIcon::Context:
        // can't catch that event on Plasma anyway
        break;
    case QSystemTrayIcon::MiddleClick:
        trayMenu().widget().showWebUI();
        break;
    case QSystemTrayIcon::Trigger:
        trayMenu().showUsingPositioningSettings();
        break;
    default:;
    }
}

void TrayIcon::handleMessageClicked()
{
    switch (m_messageClickedAction) {
    case TrayIconMessageClickedAction::None:
        return;
    case TrayIconMessageClickedAction::DismissNotification:
        trayMenu().widget().connection().requestClearingErrors();
        break;
    case TrayIconMessageClickedAction::ShowInternalErrors:
        showInternalErrorsDialog();
        break;
    case TrayIconMessageClickedAction::ShowWebUi:
        trayMenu().widget().showWebUI();
        break;
    case TrayIconMessageClickedAction::ShowUpdateSettings:
        trayMenu().widget().showUpdateSettings();
        break;
    }
}

void TrayIcon::showDisconnected()
{
#ifdef QT_UTILITIES_SUPPORT_DBUS_NOTIFICATIONS
    if (m_dbusNotificationsEnabled) {
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
    if (m_dbusNotificationsEnabled) {
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
#ifndef SYNCTHINGTRAY_UNIFY_TRAY_MENUS
    m_errorsAction->setVisible(false);
#endif
}

void TrayIcon::showInternalError(
    const QString &errorMessage, SyncthingErrorCategory category, int networkError, const QNetworkRequest &request, const QByteArray &response)
{
    if (!InternalError::isRelevant(trayMenu().widget().connection(), category, errorMessage, networkError)) {
        return;
    }
    auto error = InternalError(errorMessage, request.url(), response);
    if (Settings::values().notifyOn.internalErrors) {
#ifdef QT_UTILITIES_SUPPORT_DBUS_NOTIFICATIONS
        if (m_dbusNotificationsEnabled) {
            m_dbusNotifier.showInternalError(error);
        } else
#endif
        {
            m_messageClickedAction = TrayIconMessageClickedAction::ShowInternalErrors;
            showMessage(tr("Error"), errorMessage, QSystemTrayIcon::Critical);
        }
    }
    InternalErrorsDialog::addError(std::move(error));
#ifdef SYNCTHINGTRAY_UNIFY_TRAY_MENUS
    trayMenu().widget().showInternalErrorsButton();
#else
    m_errorsAction->setVisible(true);
#endif
}

void TrayIcon::showLauncherError(const QString &errorMessage, const QString &additionalInfo)
{
#ifdef QT_UTILITIES_SUPPORT_DBUS_NOTIFICATIONS
    if (m_dbusNotificationsEnabled) {
        m_dbusNotifier.showLauncherError(errorMessage, additionalInfo);
    } else
#endif
    {
        m_messageClickedAction = TrayIconMessageClickedAction::None;
        showMessage(tr("Launcher error"), QStringList({ errorMessage, additionalInfo }).join(QChar('\n')), QSystemTrayIcon::Critical);
    }
}

void TrayIcon::showSyncthingNotification(CppUtilities::DateTime when, const QString &message)
{
    if (m_notifyOnSyncthingErrors) {
#ifdef QT_UTILITIES_SUPPORT_DBUS_NOTIFICATIONS
        if (m_dbusNotificationsEnabled) {
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
    auto &trayWidget = trayMenu().widget();
    const auto statusInfo = StatusInfo(trayMenu().widget().connection(),
        TrayWidget::instances().size() > 1 && trayWidget.selectedConnection() ? trayWidget.selectedConnection()->label : QString());
    if (statusInfo.additionalStatusText().isEmpty()) {
        setToolTip(statusInfo.statusText());
    } else {
        setToolTip(statusInfo.statusText() % QChar('\n') % statusInfo.additionalStatusText());
    }
    setIcon(statusInfo.statusIcon());
}

void TrayIcon::showNewDev(const QString &devId, const QString &message)
{
#ifdef QT_UTILITIES_SUPPORT_DBUS_NOTIFICATIONS
    if (m_dbusNotificationsEnabled) {
        m_dbusNotifier.showNewDev(devId, message);
    } else
#else
    Q_UNUSED(devId)
#endif
    {
        m_messageClickedAction = TrayIconMessageClickedAction::ShowWebUi;
        showMessage(tr("Syncthing device wants to connect - click for web UI"), message, QSystemTrayIcon::Information);
    }
}

void TrayIcon::showNewDir(const QString &devId, const QString &dirId, const QString &dirLabel, const QString &message)
{
#ifdef QT_UTILITIES_SUPPORT_DBUS_NOTIFICATIONS
    if (m_dbusNotificationsEnabled) {
        m_dbusNotifier.showNewDir(devId, dirId, dirLabel, message);
    } else
#else
    Q_UNUSED(devId)
    Q_UNUSED(dirId)
    Q_UNUSED(dirLabel)
#endif
    {
        m_messageClickedAction = TrayIconMessageClickedAction::ShowWebUi;
        showMessage(tr("New Syncthing folder - click for web UI"), message, QSystemTrayIcon::Information);
    }
}

void TrayIcon::showNewVersionAvailable(const QString &version, const QString &additionalInfo)
{
#ifdef QT_UTILITIES_SUPPORT_DBUS_NOTIFICATIONS
    if (m_dbusNotificationsEnabled) {
        m_dbusNotifier.showNewVersionAvailable(version, additionalInfo);
    } else
#else
    Q_UNUSED(additionalInfo)
#endif
    {
        m_messageClickedAction = TrayIconMessageClickedAction::ShowUpdateSettings;
        showMessage(tr("New version - click to open updater"), tr("Version %1 is available").arg(version), QSystemTrayIcon::Information);
    }
}

void TrayIcon::showInternalErrorsDialog()
{
    if (!InternalErrorsDialog::hasInstance()) {
        connect(InternalErrorsDialog::instance(), &InternalErrorsDialog::errorsCleared, this, &TrayIcon::handleErrorsCleared);
    }
    trayMenu().widget().showInternalErrorsDialog();
}
} // namespace QtGui
