#include "./traywidget.h"
#include "./trayicon.h"
#include "./traymenu.h"

#include "../../widgets/misc/otherdialogs.h"
#include "../../widgets/misc/textviewdialog.h"
#include "../../widgets/settings/settingsdialog.h"
#include "../../widgets/webview/webviewdialog.h"

#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
#include "../../connector/syncthingservice.h"
#include "../../connector/utils.h"
#endif

// use meta-data of syncthingtray application here
#include "resources/../../tray/resources/config.h"

#include "ui_traywidget.h"

#include <qtutilities/aboutdialog/aboutdialog.h>
#include <qtutilities/misc/desktoputils.h>
#include <qtutilities/misc/dialogutils.h>
#include <qtutilities/resources/qtconfigarguments.h>
#include <qtutilities/resources/resources.h>
#include <qtutilities/settingsdialog/qtsettings.h>

#include <c++utilities/conversion/stringconversion.h>

#include <QClipboard>
#include <QCoreApplication>
#include <QCursor>
#include <QDesktopServices>
#include <QDir>
#include <QFontDatabase>
#include <QMessageBox>
#include <QStringBuilder>
#include <QTextBrowser>

#include <algorithm>
#include <functional>

using namespace ApplicationUtilities;
using namespace ConversionUtilities;
using namespace ChronoUtilities;
using namespace Dialogs;
using namespace Data;
using namespace std;

namespace QtGui {

SettingsDialog *TrayWidget::m_settingsDlg = nullptr;
Dialogs::AboutDialog *TrayWidget::m_aboutDlg = nullptr;
vector<TrayWidget *> TrayWidget::m_instances;

/*!
 * \brief Instantiates a new tray widget.
 */
TrayWidget::TrayWidget(const QString &connectionConfig, TrayMenu *parent)
    : QWidget(parent)
    , m_menu(parent)
    , m_ui(new Ui::TrayWidget)
    ,
#ifndef SYNCTHINGWIDGETS_NO_WEBVIEW
    m_webViewDlg(nullptr)
    ,
#endif
    m_dirModel(m_connection)
    , m_devModel(m_connection)
    , m_dlModel(m_connection)
    , m_selectedConnection(nullptr)
{
    m_instances.push_back(this);

    m_ui->setupUi(this);

    // setup model and view
    m_ui->dirsTreeView->setModel(&m_dirModel);
    m_ui->devsTreeView->setModel(&m_devModel);
    m_ui->downloadsTreeView->setModel(&m_dlModel);

    // setup sync-all button
    m_cornerFrame = new QFrame(this);
    auto *cornerFrameLayout = new QHBoxLayout(m_cornerFrame);
    cornerFrameLayout->setSpacing(0), cornerFrameLayout->setMargin(0);
    //cornerFrameLayout->addStretch();
    m_cornerFrame->setLayout(cornerFrameLayout);
    auto *viewIdButton = new QPushButton(m_cornerFrame);
    viewIdButton->setToolTip(tr("View own device ID"));
    viewIdButton->setIcon(
        QIcon::fromTheme(QStringLiteral("view-barcode"), QIcon(QStringLiteral(":/icons/hicolor/scalable/actions/view-barcode.svg"))));
    viewIdButton->setFlat(true);
    cornerFrameLayout->addWidget(viewIdButton);
    auto *restartButton = new QPushButton(m_cornerFrame);
    restartButton->setToolTip(tr("Restart Syncthing"));
    restartButton->setIcon(
        QIcon::fromTheme(QStringLiteral("system-reboot"), QIcon(QStringLiteral(":/icons/hicolor/scalable/actions/view-refresh.svg"))));
    restartButton->setFlat(true);
    cornerFrameLayout->addWidget(restartButton);
    auto *showLogButton = new QPushButton(m_cornerFrame);
    showLogButton->setToolTip(tr("Show Syncthing log"));
    showLogButton->setIcon(
        QIcon::fromTheme(QStringLiteral("text-x-generic"), QIcon(QStringLiteral(":/icons/hicolor/scalable/mimetypes/text-x-generic.svg"))));
    showLogButton->setFlat(true);
    cornerFrameLayout->addWidget(showLogButton);
    auto *scanAllButton = new QPushButton(m_cornerFrame);
    scanAllButton->setToolTip(tr("Rescan all directories"));
    scanAllButton->setIcon(
        QIcon::fromTheme(QStringLiteral("folder-sync"), QIcon(QStringLiteral(":/icons/hicolor/scalable/actions/folder-sync.svg"))));
    scanAllButton->setFlat(true);
    cornerFrameLayout->addWidget(scanAllButton);
    m_ui->tabWidget->setCornerWidget(m_cornerFrame, Qt::BottomRightCorner);

    // setup connection menu
    m_connectionsActionGroup = new QActionGroup(m_connectionsMenu = new QMenu(tr("Connection"), this));
    m_connectionsMenu->setIcon(
        QIcon::fromTheme(QStringLiteral("network-connect"), QIcon(QStringLiteral(":/icons/hicolor/scalable/actions/network-connect.svg"))));
    m_ui->connectionsPushButton->setMenu(m_connectionsMenu);

    // setup notifications menu
    m_notificationsMenu = new QMenu(tr("New notifications"), this);
    m_notificationsMenu->addAction(m_ui->actionShowNotifications);
    m_notificationsMenu->addAction(m_ui->actionDismissNotifications);
    m_ui->notificationsPushButton->setMenu(m_notificationsMenu);

    // apply settings, this also establishes the connection to Syncthing (according to settings)
    applySettings(connectionConfig);

    // setup other widgets
    m_ui->notificationsPushButton->setHidden(true);
    m_ui->trafficIconLabel->setPixmap(
        QIcon::fromTheme(QStringLiteral("network-card"), QIcon(QStringLiteral(":/icons/hicolor/scalable/devices/network-card.svg"))).pixmap(32));
#ifndef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
    delete m_ui->startStopPushButton;
#endif

    // connect signals and slots
    connect(m_ui->statusPushButton, &QPushButton::clicked, this, &TrayWidget::changeStatus);
    connect(m_ui->closePushButton, &QPushButton::clicked, this, &TrayWidget::quitTray);
    connect(m_ui->aboutPushButton, &QPushButton::clicked, this, &TrayWidget::showAboutDialog);
    connect(m_ui->webUiPushButton, &QPushButton::clicked, this, &TrayWidget::showWebUi);
    connect(m_ui->settingsPushButton, &QPushButton::clicked, this, &TrayWidget::showSettingsDialog);
    connect(&m_connection, &SyncthingConnection::statusChanged, this, &TrayWidget::handleStatusChanged);
    connect(&m_connection, &SyncthingConnection::trafficChanged, this, &TrayWidget::updateTraffic);
    connect(&m_connection, &SyncthingConnection::newNotification, this, &TrayWidget::handleNewNotification);
    connect(m_ui->dirsTreeView, &DirView::openDir, this, &TrayWidget::openDir);
    connect(m_ui->dirsTreeView, &DirView::scanDir, this, &TrayWidget::scanDir);
    connect(m_ui->dirsTreeView, &DirView::pauseResumeDir, this, &TrayWidget::pauseResumeDir);
    connect(m_ui->devsTreeView, &DevView::pauseResumeDev, this, &TrayWidget::pauseResumeDev);
    connect(m_ui->downloadsTreeView, &DownloadView::openDir, this, &TrayWidget::openDir);
    connect(m_ui->downloadsTreeView, &DownloadView::openItemDir, this, &TrayWidget::openItemDir);
    connect(scanAllButton, &QPushButton::clicked, &m_connection, &SyncthingConnection::rescanAllDirs);
    connect(viewIdButton, &QPushButton::clicked, this, &TrayWidget::showOwnDeviceId);
    connect(showLogButton, &QPushButton::clicked, this, &TrayWidget::showLog);
    connect(m_ui->notificationsPushButton, &QPushButton::clicked, this, &TrayWidget::showNotifications);
    connect(restartButton, &QPushButton::clicked, this, &TrayWidget::restartSyncthing);
    connect(m_connectionsActionGroup, &QActionGroup::triggered, this, &TrayWidget::handleConnectionSelected);
    connect(m_ui->actionShowNotifications, &QAction::triggered, this, &TrayWidget::showNotifications);
    connect(m_ui->actionDismissNotifications, &QAction::triggered, this, &TrayWidget::dismissNotifications);
#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
    const SyncthingService &service = syncthingService();
    connect(m_ui->startStopPushButton, &QPushButton::clicked, &service, &SyncthingService::toggleRunning);
    connect(&service, &SyncthingService::systemdAvailableChanged, this, &TrayWidget::handleSystemdStatusChanged);
    connect(&service, &SyncthingService::stateChanged, this, &TrayWidget::handleSystemdStatusChanged);
#endif
}

TrayWidget::~TrayWidget()
{
    auto i = std::find(m_instances.begin(), m_instances.end(), this);
    if (i != m_instances.end()) {
        m_instances.erase(i);
    }
    if (m_instances.empty()) {
        QCoreApplication::quit();
    }
}

void TrayWidget::showSettingsDialog()
{
    if (!m_settingsDlg) {
        m_settingsDlg = new SettingsDialog(&m_connection, this);
        connect(m_settingsDlg, &SettingsDialog::applied, &TrayWidget::applySettingsOnAllInstances);
    }
    centerWidget(m_settingsDlg);
    showDialog(m_settingsDlg);
}

void TrayWidget::showAboutDialog()
{
    if (!m_aboutDlg) {
        m_aboutDlg = new AboutDialog(this, QString(),
            QStringLiteral(APP_AUTHOR "\nfallback icons from KDE/Breeze project\nSyncthing icons from Syncthing project"), QString(), QString(),
            QStringLiteral(APP_DESCRIPTION), QImage(QStringLiteral(":/icons/hicolor/scalable/app/syncthingtray.svg")));
        m_aboutDlg->setWindowTitle(tr("About") + QStringLiteral(" - " APP_NAME));
        m_aboutDlg->setWindowIcon(QIcon(QStringLiteral(":/icons/hicolor/scalable/app/syncthingtray.svg")));
    }
    centerWidget(m_aboutDlg);
    showDialog(m_aboutDlg);
}

void TrayWidget::showWebUi()
{
#ifndef SYNCTHINGWIDGETS_NO_WEBVIEW
    if (Settings::values().webView.disabled) {
#endif
        QDesktopServices::openUrl(m_connection.syncthingUrl());
#ifndef SYNCTHINGWIDGETS_NO_WEBVIEW
    } else {
        if (!m_webViewDlg) {
            m_webViewDlg = new WebViewDialog(this);
            if (m_selectedConnection) {
                m_webViewDlg->applySettings(*m_selectedConnection);
            }
            connect(m_webViewDlg, &WebViewDialog::destroyed, this, &TrayWidget::handleWebViewDeleted);
        }
        showDialog(m_webViewDlg);
    }
#endif
}

void TrayWidget::showOwnDeviceId()
{
    auto *const dlg = ownDeviceIdDialog(m_connection);
    dlg->setAttribute(Qt::WA_DeleteOnClose, true);
    centerWidget(dlg);
    showDialog(dlg);
}

void TrayWidget::showLog()
{
    auto *const dlg = TextViewDialog::forLogEntries(m_connection);
    dlg->setAttribute(Qt::WA_DeleteOnClose, true);
    centerWidget(dlg);
    showDialog(dlg);
}

void TrayWidget::showNotifications()
{
    auto *dlg = new TextViewDialog(tr("New notifications"), this);
    for (const SyncthingLogEntry &entry : m_notifications) {
        dlg->browser()->append(entry.when % QChar(':') % QChar(' ') % QChar('\n') % entry.message % QChar('\n'));
    }
    m_notifications.clear();
    showDialog(dlg);
    dismissNotifications();
}

void TrayWidget::showAtCursor()
{
    if (m_menu) {
        m_menu->showAtCursor();
    } else {
        move(QCursor::pos());
        show();
    }
}

void TrayWidget::dismissNotifications()
{
    m_connection.considerAllNotificationsRead();
    m_ui->notificationsPushButton->setHidden(true);
    if (m_menu && m_menu->icon()) {
        m_menu->icon()->updateStatusIconAndText();
    }
}

void TrayWidget::restartSyncthing()
{
    if (QMessageBox::warning(
            this, QCoreApplication::applicationName(), tr("Do you really want to restart Syncthing?"), QMessageBox::Yes, QMessageBox::No)
        == QMessageBox::Yes) {
        m_connection.restart();
    }
}

void TrayWidget::quitTray()
{
    QObject *parent;
    if (m_menu) {
        if (m_menu->icon()) {
            parent = m_menu->icon();
        } else {
            parent = m_menu;
        }
    } else {
        parent = this;
    }
    parent->deleteLater();
}

void TrayWidget::handleStatusChanged(SyncthingStatus status)
{
    switch (status) {
    case SyncthingStatus::Disconnected:
        m_ui->statusPushButton->setText(tr("Connect"));
        m_ui->statusPushButton->setToolTip(tr("Not connected to Syncthing, click to connect"));
        m_ui->statusPushButton->setIcon(
            QIcon::fromTheme(QStringLiteral("view-refresh"), QIcon(QStringLiteral(":/icons/hicolor/scalable/actions/view-refresh.svg"))));
        m_ui->statusPushButton->setHidden(false);
        updateTraffic(); // ensure previous traffic statistics are no longer shown
        break;
    case SyncthingStatus::Reconnecting:
        m_ui->statusPushButton->setHidden(true);
        break;
    case SyncthingStatus::Idle:
    case SyncthingStatus::Scanning:
    case SyncthingStatus::Synchronizing:
        m_ui->statusPushButton->setText(tr("Pause"));
        m_ui->statusPushButton->setToolTip(tr("Syncthing is running, click to pause all devices"));
        m_ui->statusPushButton->setIcon(QIcon::fromTheme(
            QStringLiteral("media-playback-pause"), QIcon(QStringLiteral(":/icons/hicolor/scalable/actions/media-playback-pause.svg"))));
        m_ui->statusPushButton->setHidden(false);
        break;
    case SyncthingStatus::Paused:
        m_ui->statusPushButton->setText(tr("Continue"));
        m_ui->statusPushButton->setToolTip(tr("At least one device is paused, click to resume"));
        m_ui->statusPushButton->setIcon(QIcon::fromTheme(
            QStringLiteral("media-playback-start"), QIcon(QStringLiteral(":/icons/hicolor/scalable/actions/media-playback-start.svg"))));
        m_ui->statusPushButton->setHidden(false);
        break;
    default:;
    }
}

void TrayWidget::applySettings(const QString &connectionConfig)
{
    // update connections menu
    int connectionIndex = 0;
    auto &settings = Settings::values();
    auto &primaryConnectionSettings = settings.connection.primary;
    auto &secondaryConnectionSettings = settings.connection.secondary;
    const int connectionCount = static_cast<int>(1 + secondaryConnectionSettings.size());
    const QList<QAction *> connectionActions = m_connectionsActionGroup->actions();
    m_selectedConnection = nullptr;
    bool specifiedConnectionConfigFound = false;
    for (; connectionIndex < connectionCount; ++connectionIndex) {
        SyncthingConnectionSettings &connectionSettings
            = (connectionIndex == 0 ? primaryConnectionSettings : secondaryConnectionSettings[static_cast<size_t>(connectionIndex - 1)]);
        QAction *action;
        if (connectionIndex < connectionActions.size()) {
            action = connectionActions.at(connectionIndex);
            action->setText(connectionSettings.label);
            if (action->isChecked() && !m_selectedConnection) {
                m_selectedConnection = &connectionSettings;
            }
        } else {
            action = m_connectionsMenu->addAction(connectionSettings.label);
            action->setCheckable(true);
            m_connectionsActionGroup->addAction(action);
        }
        if (!connectionConfig.isEmpty() && !connectionSettings.label.compare(connectionConfig, Qt::CaseInsensitive)) {
            m_selectedConnection = &connectionSettings;
            specifiedConnectionConfigFound = true;
            action->setChecked(true);
        }
    }
    for (; connectionIndex < connectionActions.size(); ++connectionIndex) {
        delete connectionActions.at(connectionIndex);
    }
    if (!m_selectedConnection) {
        m_selectedConnection = &primaryConnectionSettings;
        m_connectionsMenu->actions().at(0)->setChecked(true);
    }
    m_ui->connectionsPushButton->setText(m_selectedConnection->label);
    const bool reconnectRequired = m_connection.applySettings(*m_selectedConnection);

#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
    // reconnect to apply settings considering systemd
    const bool couldReconnect = handleSystemdStatusChanged();
    if (reconnectRequired && couldReconnect) {
        m_connection.reconnect();
    }
#else
    m_connection.reconnect();
#endif

#ifndef SYNCTHINGWIDGETS_NO_WEBVIEW
    // web view
    if (m_webViewDlg) {
        m_webViewDlg->applySettings(*m_selectedConnection);
    }
#endif

    // update visual appearance
    m_ui->trafficFormWidget->setVisible(settings.appearance.showTraffic);
    m_ui->trafficIconLabel->setVisible(settings.appearance.showTraffic);
    m_ui->trafficHorizontalSpacer->changeSize(
        0, 20, settings.appearance.showTraffic ? QSizePolicy::Expanding : QSizePolicy::Ignored, QSizePolicy::Minimum);
    if (settings.appearance.showTraffic) {
        updateTraffic();
    }
    m_ui->infoFrame->setFrameStyle(settings.appearance.frameStyle);
    m_ui->buttonsFrame->setFrameStyle(settings.appearance.frameStyle);
    if (QApplication::style() && !QApplication::style()->objectName().compare(QLatin1String("adwaita"), Qt::CaseInsensitive)) {
        m_cornerFrame->setFrameStyle(QFrame::NoFrame);
    } else {
        m_cornerFrame->setFrameStyle(settings.appearance.frameStyle);
    }
    if (settings.appearance.tabPosition >= QTabWidget::North && settings.appearance.tabPosition <= QTabWidget::East) {
        m_ui->tabWidget->setTabPosition(static_cast<QTabWidget::TabPosition>(settings.appearance.tabPosition));
    }
    m_dirModel.setBrightColors(settings.appearance.brightTextColors);
    m_devModel.setBrightColors(settings.appearance.brightTextColors);
    m_dlModel.setBrightColors(settings.appearance.brightTextColors);

    // show warning when explicitely specified connection configuration was not found
    if (!specifiedConnectionConfigFound && !connectionConfig.isEmpty()) {
        auto *const msgBox = new QMessageBox(QMessageBox::Warning, QCoreApplication::applicationName(),
            tr("The specified connection configuration <em>%1</em> is not defined and hence ignored.").arg(connectionConfig));
        msgBox->setAttribute(Qt::WA_DeleteOnClose);
        msgBox->show();
    }
}

void TrayWidget::applySettingsOnAllInstances()
{
    for (TrayWidget *instance : m_instances) {
        instance->applySettings();
    }
}

void TrayWidget::openDir(const SyncthingDir &dir)
{
    if (QDir(dir.path).exists()) {
        DesktopUtils::openLocalFileOrDir(dir.path);
    } else {
        QMessageBox::warning(
            this, QCoreApplication::applicationName(), tr("The directory <i>%1</i> does not exist on the local machine.").arg(dir.path));
    }
}

void TrayWidget::openItemDir(const SyncthingItemDownloadProgress &item)
{
    const QDir containingDir(item.fileInfo.absoluteDir());
    if (containingDir.exists()) {
        DesktopUtils::openLocalFileOrDir(containingDir.path());
    } else {
        QMessageBox::warning(this, QCoreApplication::applicationName(),
            tr("The containing directory <i>%1</i> does not exist on the local machine.").arg(item.fileInfo.filePath()));
    }
}

void TrayWidget::scanDir(const SyncthingDir &dir)
{
    m_connection.rescan(dir.id);
}

void TrayWidget::pauseResumeDev(const SyncthingDev &dev)
{
    if (dev.paused) {
        m_connection.resumeDevice(QStringList(dev.id));
    } else {
        m_connection.pauseDevice(QStringList(dev.id));
    }
}

void TrayWidget::pauseResumeDir(const SyncthingDir &dir)
{
    if (dir.paused) {
        m_connection.resumeDirectories(QStringList(dir.id));
    } else {
        m_connection.pauseDirectories(QStringList(dir.id));
    }
}

void TrayWidget::changeStatus()
{
    switch (m_connection.status()) {
    case SyncthingStatus::Disconnected:
        m_connection.connect();
        break;
    case SyncthingStatus::Reconnecting:
        break;
    case SyncthingStatus::Idle:
    case SyncthingStatus::Scanning:
    case SyncthingStatus::Synchronizing:
        m_connection.pauseAllDevs();
        break;
    case SyncthingStatus::Paused:
        m_connection.resumeAllDevs();
        break;
    default:;
    }
}

void TrayWidget::updateTraffic()
{
    if (m_ui->trafficFormWidget->isHidden()) {
        return;
    }
    m_ui->inTrafficLabel->setText(trafficString(m_connection.totalIncomingTraffic(), m_connection.totalIncomingRate()));
    m_ui->outTrafficLabel->setText(trafficString(m_connection.totalOutgoingTraffic(), m_connection.totalOutgoingRate()));
}

#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
bool TrayWidget::handleSystemdStatusChanged()
{
    const SyncthingService &service = syncthingService();
    const Settings::Systemd &settings = Settings::values().systemd;
    const bool serviceRelevant = service.isSystemdAvailable() && isLocal(QUrl(m_connection.syncthingUrl()));
    bool couldConnectNow = true;

    if (serviceRelevant) {
        const bool isRunning = service.isRunning();
        if (settings.showButton) {
            m_ui->startStopPushButton->setVisible(true);
            if (isRunning) {
                m_ui->startStopPushButton->setText(tr("Stop"));
                m_ui->startStopPushButton->setToolTip(QStringLiteral("systemctl --user stop ") + service.unitName());
                m_ui->startStopPushButton->setIcon(
                    QIcon::fromTheme(QStringLiteral("process-stop"), QIcon(QStringLiteral(":/icons/hicolor/scalable/actions/process-stop.svg"))));
            } else {
                m_ui->startStopPushButton->setText(tr("Start"));
                m_ui->startStopPushButton->setToolTip(QStringLiteral("systemctl --user start ") + service.unitName());
                m_ui->startStopPushButton->setIcon(
                    QIcon::fromTheme(QStringLiteral("system-run"), QIcon(QStringLiteral(":/icons/hicolor/scalable/apps/system-run.svg"))));
            }
        }
        if (settings.considerForReconnect) {
            if (isRunning && m_selectedConnection) {
                // auto-reconnect might have been disabled when unit was inactive before, so re-enable it according current connection settings
                m_connection.setAutoReconnectInterval(m_selectedConnection->reconnectInterval);
                if (!m_connection.isConnected()) {
                    // FIXME: This will fail if Syncthing has just been started and isn't ready yet
                    m_connection.connect();
                }
            } else {
                // disable auto-reconnect if unit isn't running
                m_connection.setAutoReconnectInterval(0);
                couldConnectNow = false;
            }
        }
    }

    if (!settings.showButton || !serviceRelevant) {
        m_ui->startStopPushButton->setVisible(false);
    }
    if ((!settings.considerForReconnect || !serviceRelevant) && m_selectedConnection) {
        m_connection.setAutoReconnectInterval(m_selectedConnection->reconnectInterval);
    }

    return couldConnectNow;
}

void TrayWidget::connectIfServiceRunning()
{
    if (Settings::values().systemd.considerForReconnect && isLocal(QUrl(m_connection.syncthingUrl())) && syncthingService().isRunning()) {
        m_connection.connect();
    }
}
#endif

#ifndef SYNCTHINGWIDGETS_NO_WEBVIEW
void TrayWidget::handleWebViewDeleted()
{
    m_webViewDlg = nullptr;
}
#endif

void TrayWidget::handleNewNotification(DateTime when, const QString &msg)
{
    m_notifications.emplace_back(QString::fromLocal8Bit(when.toString(DateTimeOutputFormat::DateAndTime, true).data()), msg);
    m_ui->notificationsPushButton->setHidden(false);
}

void TrayWidget::handleConnectionSelected(QAction *connectionAction)
{
    int index = m_connectionsMenu->actions().indexOf(connectionAction);
    if (index >= 0) {
        m_selectedConnection
            = (index == 0) ? &Settings::values().connection.primary : &Settings::values().connection.secondary[static_cast<size_t>(index - 1)];
        m_ui->connectionsPushButton->setText(m_selectedConnection->label);
        m_connection.reconnect(*m_selectedConnection);
#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
        handleSystemdStatusChanged();
#endif
#ifndef SYNCTHINGWIDGETS_NO_WEBVIEW
        if (m_webViewDlg) {
            m_webViewDlg->applySettings(*m_selectedConnection);
        }
#endif
    }
}

void TrayWidget::showDialog(QWidget *dlg)
{
    if (m_menu) {
        m_menu->close();
    }
    dlg->show();
    dlg->activateWindow();
}
}
