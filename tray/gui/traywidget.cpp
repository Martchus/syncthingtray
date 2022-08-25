#include "./traywidget.h"
#include "./helper.h"
#include "./trayicon.h"
#include "./traymenu.h"

#include <syncthingwidgets/misc/internalerrorsdialog.h>
#include <syncthingwidgets/misc/otherdialogs.h>
#include <syncthingwidgets/misc/syncthinglauncher.h>
#include <syncthingwidgets/misc/textviewdialog.h>
#include <syncthingwidgets/settings/settingsdialog.h>
#include <syncthingwidgets/webview/webviewdialog.h>

#include <syncthingmodel/syncthingicons.h>

#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
#include <syncthingconnector/syncthingservice.h>
#endif
#include <syncthingconnector/utils.h>

// use meta-data of syncthingtray application here
#include "resources/../../tray/resources/config.h"

#include "ui_traywidget.h"

#include <qtforkawesome/icon.h>

#include <qtutilities/aboutdialog/aboutdialog.h>
#include <qtutilities/misc/desktoputils.h>
#include <qtutilities/misc/dialogutils.h>
#include <qtutilities/resources/qtconfigarguments.h>
#include <qtutilities/resources/resources.h>
#include <qtutilities/settingsdialog/qtsettings.h>

#include <c++utilities/application/argumentparser.h>
#include <c++utilities/conversion/stringconversion.h>

#include <QActionGroup>
#include <QClipboard>
#include <QCoreApplication>
#include <QDesktopServices>
#include <QDir>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QMessageBox>
#include <QPalette>
#include <QStringBuilder>
#include <QTextBrowser>

#include <algorithm>
#include <functional>

using namespace CppUtilities;
using namespace QtUtilities;
using namespace Data;
using namespace std;

namespace QtGui {

QWidget *TrayWidget::s_dialogParent = nullptr;
SettingsDialog *TrayWidget::s_settingsDlg = nullptr;
QtUtilities::AboutDialog *TrayWidget::s_aboutDlg = nullptr;
vector<TrayWidget *> TrayWidget::s_instances;

/*!
 * \brief Instantiates a new tray widget.
 * \remarks Doesn't apply the settings (and won't connect according to settings). This must be done manually by calling
 *          TrayWidget::applySettings(). This allows postponing connecting until all signals of the TrayWidget::connection()
 *          are connected as required.
 */
TrayWidget::TrayWidget(TrayMenu *parent)
    : QWidget(parent)
    , m_menu(parent)
    , m_ui(new Ui::TrayWidget)
#ifndef SYNCTHINGWIDGETS_NO_WEBVIEW
    , m_webViewDlg(nullptr)
#endif
    , m_internalErrorsButton(nullptr)
    , m_notifier(m_connection)
    , m_dirModel(m_connection)
    , m_sortFilterDirModel(&m_dirModel)
    , m_devModel(m_connection)
    , m_sortFilterDevModel(&m_devModel)
    , m_dlModel(m_connection)
    , m_recentChangesModel(m_connection)
    , m_selectedConnection(nullptr)
    , m_startStopButtonTarget(StartStopButtonTarget::None)
{
    // don't show connection status within connection settings if there are multiple tray widgets/icons (would be ambiguous)
    if (!s_instances.empty() && s_settingsDlg) {
        s_settingsDlg->hideConnectionStatus();
    }

    // take record of the newly created instance
    s_instances.push_back(this);

    // show the connection configuration in the previous icon's tooltip as soon as there's a 2nd icon
    if (s_instances.size() == 2) {
        s_instances.front()->updateIconAndTooltip();
    }

    m_ui->setupUi(this);

    // setup models and views
    m_ui->dirsTreeView->header()->setSortIndicator(0, Qt::AscendingOrder);
    m_ui->dirsTreeView->setModel(&m_sortFilterDirModel);
    m_ui->devsTreeView->header()->setSortIndicator(0, Qt::AscendingOrder);
    m_ui->devsTreeView->setModel(&m_sortFilterDevModel);
    m_ui->downloadsTreeView->setModel(&m_dlModel);
    m_ui->recentChangesTreeView->setModel(&m_recentChangesModel);
    m_ui->recentChangesTreeView->setContextMenuPolicy(Qt::CustomContextMenu);
    setBrightColorsOfModelsAccordingToPalette();

    // setup sync-all button
    m_cornerFrame = new QFrame(this);
    auto *cornerFrameLayout = new QHBoxLayout(m_cornerFrame);
    cornerFrameLayout->setSpacing(0);
    cornerFrameLayout->setContentsMargins(0, 0, 0, 0);
    m_cornerFrame->setLayout(cornerFrameLayout);
    auto *viewIdButton = new QPushButton(m_cornerFrame);
    viewIdButton->setToolTip(tr("View own device ID"));
    viewIdButton->setIcon(QIcon(QStringLiteral("qrcode.fa")));
    viewIdButton->setFlat(true);
    cornerFrameLayout->addWidget(viewIdButton);
    auto *restartButton = new QPushButton(m_cornerFrame);
    restartButton->setToolTip(tr("Restart Syncthing"));
    restartButton->setIcon(QIcon(QStringLiteral("power-off.fa")));
    restartButton->setFlat(true);
    cornerFrameLayout->addWidget(restartButton);
    auto *showLogButton = new QPushButton(m_cornerFrame);
    showLogButton->setToolTip(tr("Show Syncthing log"));
    showLogButton->setIcon(QIcon(QStringLiteral("file-text.fa")));
    showLogButton->setFlat(true);
    cornerFrameLayout->addWidget(showLogButton);
    auto *scanAllButton = new QPushButton(m_cornerFrame);
    scanAllButton->setToolTip(tr("Rescan all directories"));
    scanAllButton->setIcon(QIcon(QStringLiteral("refresh.fa")));
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

    // setup other widgets
    m_ui->notificationsPushButton->setHidden(true);
    m_ui->globalTextLabel->setPixmap(QIcon(QStringLiteral("globe.fa")).pixmap(16));
    m_ui->localTextLabel->setPixmap(QIcon(QStringLiteral("home.fa")).pixmap(16));
    updateTraffic();

    // add actions from right-click menu if it is not available
#ifndef SYNCTHINGTRAY_UNIFY_TRAY_MENUS
    if (!m_menu) {
#endif
        m_internalErrorsButton = new QPushButton(m_cornerFrame);
        m_internalErrorsButton->setToolTip(tr("Show internal errors"));
        m_internalErrorsButton->setIcon(
            QIcon::fromTheme(QStringLiteral("emblem-error"), QIcon(QStringLiteral(":/icons/hicolor/scalable/emblems/8/emblem-error.svg"))));
        m_internalErrorsButton->setFlat(true);
        m_internalErrorsButton->setVisible(false);
        connect(m_internalErrorsButton, &QPushButton::clicked, this, &TrayWidget::showInternalErrorsDialog);
        cornerFrameLayout->addWidget(m_internalErrorsButton);
        if (!m_menu) {
            connect(&m_connection, &SyncthingConnection::error, this, &TrayWidget::showInternalError);
        }
#ifndef SYNCTHINGTRAY_UNIFY_TRAY_MENUS
    }
#endif
#ifdef SYNCTHINGTRAY_UNIFY_TRAY_MENUS
    auto *quitButton = new QPushButton(m_cornerFrame);
    quitButton->setToolTip(tr("Quit Syncthing Tray"));
    quitButton->setIcon(QIcon::fromTheme(QStringLiteral("window-close"), QIcon(QStringLiteral(":/icons/hicolor/scalable/actions/window-close.svg"))));
    quitButton->setFlat(true);
    connect(quitButton, &QPushButton::clicked, this, &TrayWidget::quitTray);
    cornerFrameLayout->addWidget(quitButton);
#endif

    // connect signals and slots
    connect(m_ui->statusPushButton, &QPushButton::clicked, this, &TrayWidget::changeStatus);
    connect(m_ui->aboutPushButton, &QPushButton::clicked, this, &TrayWidget::showAboutDialog);
    connect(m_ui->webUiPushButton, &QPushButton::clicked, this, &TrayWidget::showWebUi);
    connect(m_ui->settingsPushButton, &QPushButton::clicked, this, &TrayWidget::showSettingsDialog);
    connect(&m_connection, &SyncthingConnection::statusChanged, this, &TrayWidget::handleStatusChanged);
    connect(&m_connection, &SyncthingConnection::trafficChanged, this, &TrayWidget::updateTraffic);
    connect(&m_connection, &SyncthingConnection::dirStatisticsChanged, this, &TrayWidget::updateOverallStatistics);
    connect(&m_connection, &SyncthingConnection::newNotification, this, &TrayWidget::handleNewNotification);
    connect(m_ui->dirsTreeView, &DirView::openDir, this, &TrayWidget::openDir);
    connect(m_ui->dirsTreeView, &DirView::scanDir, this, &TrayWidget::scanDir);
    connect(m_ui->dirsTreeView, &DirView::pauseResumeDir, this, &TrayWidget::pauseResumeDir);
    connect(m_ui->devsTreeView, &DevView::pauseResumeDev, this, &TrayWidget::pauseResumeDev);
    connect(m_ui->downloadsTreeView, &DownloadView::openDir, this, &TrayWidget::openDir);
    connect(m_ui->downloadsTreeView, &DownloadView::openItemDir, this, &TrayWidget::openItemDir);
    connect(m_ui->recentChangesTreeView, &QTreeView::customContextMenuRequested, this, &TrayWidget::showRecentChangesContextMenu);
    connect(scanAllButton, &QPushButton::clicked, &m_connection, &SyncthingConnection::rescanAllDirs);
    connect(viewIdButton, &QPushButton::clicked, this, &TrayWidget::showOwnDeviceId);
    connect(showLogButton, &QPushButton::clicked, this, &TrayWidget::showLog);
    connect(m_ui->notificationsPushButton, &QPushButton::clicked, this, &TrayWidget::showNotifications);
    connect(restartButton, &QPushButton::clicked, this, &TrayWidget::restartSyncthing);
    connect(m_connectionsActionGroup, &QActionGroup::triggered, this, &TrayWidget::handleConnectionSelected);
    connect(m_ui->actionShowNotifications, &QAction::triggered, this, &TrayWidget::showNotifications);
    connect(m_ui->actionDismissNotifications, &QAction::triggered, this, &TrayWidget::dismissNotifications);
    connect(m_ui->startStopPushButton, &QPushButton::clicked, this, &TrayWidget::toggleRunning);
    if (const auto *const launcher = SyncthingLauncher::mainInstance()) {
        connect(launcher, &SyncthingLauncher::runningChanged, this, &TrayWidget::handleLauncherStatusChanged);
    }
#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
    if (const auto *const service = SyncthingService::mainInstance()) {
        connect(service, &SyncthingService::systemdAvailableChanged, this, &TrayWidget::handleSystemdStatusChanged);
        connect(service, &SyncthingService::stateChanged, this, &TrayWidget::handleSystemdStatusChanged);
    }
#endif
}

TrayWidget::~TrayWidget()
{
    auto i = std::find(s_instances.begin(), s_instances.end(), this);
    if (i != s_instances.end()) {
        s_instances.erase(i);
    }
    if (s_instances.empty()) {
        delete s_dialogParent;
        QCoreApplication::quit();
    } else if (s_instances.size() == 1) {
        s_instances.front()->updateIconAndTooltip();
    }
}

SettingsDialog *TrayWidget::settingsDialog()
{
    if (!s_dialogParent) {
        s_dialogParent = new QWidget();
    }
    if (!s_settingsDlg) {
        s_settingsDlg = new SettingsDialog(s_instances.size() < 2 ? &m_connection : nullptr, s_dialogParent);
        connect(s_settingsDlg, &SettingsDialog::applied, &TrayWidget::applySettingsOnAllInstances);

        // save settings to disk when applied
        // note: QCoreApplication::aboutToQuit() does not work reliably but terminating only at the
        //       end of the session is a common use-case for the tray application. So workaround this
        //       by simply saving the settings immediately.
        connect(s_settingsDlg, &SettingsDialog::applied, &Settings::save);
    }
    return s_settingsDlg;
}

void TrayWidget::showSettingsDialog()
{
    auto *const dlg = settingsDialog();
    // show settings dialog centered or maximized if the relatively big windows would overflow
    showDialog(dlg, centerWidgetAvoidingOverflow(dlg));
}

void TrayWidget::showAboutDialog()
{
    if (!s_dialogParent) {
        s_dialogParent = new QWidget();
    }
    if (!s_aboutDlg) {
        s_aboutDlg = new AboutDialog(
            s_dialogParent, QString(), aboutDialogAttribution(), QString(), {}, QStringLiteral(APP_URL), QString(), aboutDialogImage());
        s_aboutDlg->setWindowTitle(tr("About") + QStringLiteral(" - " APP_NAME));
        s_aboutDlg->setWindowIcon(QIcon(QStringLiteral(":/icons/hicolor/scalable/app/syncthingtray.svg")));
    }
    centerWidget(s_aboutDlg);
    showDialog(s_aboutDlg);
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
                m_webViewDlg->applySettings(*m_selectedConnection, true);
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
    showDialog(dlg, centerWidgetAvoidingOverflow(dlg));
}

void TrayWidget::showLog()
{
    auto *const dlg = TextViewDialog::forLogEntries(m_connection);
    dlg->setAttribute(Qt::WA_DeleteOnClose, true);
    showDialog(dlg, centerWidgetAvoidingOverflow(dlg));
}

void TrayWidget::showNotifications()
{
    auto *const dlg = TextViewDialog::forLogEntries(m_notifications, tr("New notifications"));
    dlg->setAttribute(Qt::WA_DeleteOnClose, true);
    showDialog(dlg, centerWidgetAvoidingOverflow(dlg));
    m_notifications.clear();
    dismissNotifications();
}

void TrayWidget::showUsingPositioningSettings()
{
    if (m_menu) {
        m_menu->showUsingPositioningSettings();
    } else {
        move(Settings::values().appearance.positioning.positionToUse());
        show();
    }
}

void TrayWidget::showInternalError(
    const QString &errorMessage, SyncthingErrorCategory category, int networkError, const QNetworkRequest &request, const QByteArray &response)
{
    if (!InternalError::isRelevant(connection(), category, networkError)) {
        return;
    }
    InternalErrorsDialog::addError(errorMessage, request.url(), response);
    showInternalErrorsButton();
}

void TrayWidget::showInternalErrorsButton()
{
    if (m_internalErrorsButton) {
        m_internalErrorsButton->setVisible(true);
    }
}

void TrayWidget::showInternalErrorsDialog()
{
    auto *const errorViewDlg = InternalErrorsDialog::instance();
    if (m_internalErrorsButton) {
        connect(errorViewDlg, &InternalErrorsDialog::errorsCleared, m_internalErrorsButton, &QWidget::hide);
    }
    showDialog(errorViewDlg, centerWidgetAvoidingOverflow(errorViewDlg));
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
        m_ui->statusPushButton->setIcon(QIcon(QStringLiteral("refresh.fa")));
        m_ui->statusPushButton->setHidden(false);
        updateTraffic(); // ensure previous traffic statistics are no longer shown
        break;
    case SyncthingStatus::Reconnecting:
        m_ui->statusPushButton->setHidden(true);
        break;
    case SyncthingStatus::Idle:
    case SyncthingStatus::Scanning:
    case SyncthingStatus::Synchronizing:
    case SyncthingStatus::RemoteNotInSync:
        m_ui->statusPushButton->setText(tr("Pause"));
        m_ui->statusPushButton->setToolTip(tr("Syncthing is running, click to pause all devices"));
        m_ui->statusPushButton->setIcon(QIcon(QStringLiteral("pause.fa")));
        m_ui->statusPushButton->setHidden(false);
        break;
    case SyncthingStatus::Paused:
        m_ui->statusPushButton->setText(tr("Continue"));
        m_ui->statusPushButton->setToolTip(tr("At least one device is paused, click to resume"));
        m_ui->statusPushButton->setIcon(QIcon(QStringLiteral("play.fa")));
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
    m_ui->connectionsPushButton->setHidden(secondaryConnectionSettings.empty());
    const bool reconnectRequired = m_connection.applySettings(*m_selectedConnection);

    // apply notification settings
    settings.apply(m_notifier);

    // apply systemd and launcher settings enforcing a reconnect if required and possible
#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
    const auto systemdStatus = applySystemdSettings(reconnectRequired);
    const auto launcherStatus = applyLauncherSettings(reconnectRequired, systemdStatus.consideredForReconnect, systemdStatus.showStartStopButton);
    const auto showStartStopButton = systemdStatus.showStartStopButton || launcherStatus.showStartStopButton;
    const auto systemdOrLauncherRelevantForReconnect = systemdStatus.relevant || launcherStatus.relevant;
#else
    const auto launcherStatus = applyLauncherSettings(reconnectRequired);
    const auto showStartStopButton = launcherStatus.showStartStopButton;
    const auto systemdOrLauncherRelevantForReconnect = launcherStatus.relevant;
#endif
    m_ui->startStopPushButton->setVisible(showStartStopButton);
    if (reconnectRequired && !systemdOrLauncherRelevantForReconnect) {
        // simply enforce the reconnect for this connection if the systemd or launcher status are relevant for it
        m_connection.reconnect();
    }

#ifndef SYNCTHINGWIDGETS_NO_WEBVIEW
    // web view
    if (m_webViewDlg) {
        m_webViewDlg->applySettings(*m_selectedConnection, false);
    }
#endif

    // update visual appearance
    m_ui->trafficFormWidget->setVisible(settings.appearance.showTraffic);
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
    IconManager::instance().applySettings(&settings.icons.status, settings.icons.distinguishTrayIcons ? &settings.icons.tray : nullptr);

    // update status icon and text of tray icon because reconnect interval might have changed
    if (m_menu && m_menu->icon()) {
        m_menu->icon()->updateStatusIconAndText();
    }

    // show warning when explicitly specified connection configuration was not found
    if (!specifiedConnectionConfigFound && !connectionConfig.isEmpty()) {
        auto *const msgBox = new QMessageBox(QMessageBox::Warning, QCoreApplication::applicationName(),
            tr("The specified connection configuration <em>%1</em> is not defined and hence ignored.").arg(connectionConfig));
        msgBox->setAttribute(Qt::WA_DeleteOnClose);
        msgBox->show();
    }
}

bool TrayWidget::event(QEvent *event)
{
    switch (event->type()) {
    case QEvent::PaletteChange:
        setBrightColorsOfModelsAccordingToPalette();
        break;
    default:;
    }
    return QWidget::event(event);
}

void TrayWidget::applySettingsOnAllInstances()
{
    for (TrayWidget *instance : s_instances) {
        instance->applySettings();
    }
}

void TrayWidget::openDir(const SyncthingDir &dir)
{
    const auto path = substituteTilde(dir.path, m_connection.tilde(), m_connection.pathSeparator());
    if (QDir(path).exists()) {
        openLocalFileOrDir(path);
    } else {
        QMessageBox::warning(this, QCoreApplication::applicationName(), tr("The directory <i>%1</i> does not exist on the local machine.").arg(path));
    }
}

void TrayWidget::openItemDir(const SyncthingItemDownloadProgress &item)
{
    const QDir containingDir(item.fileInfo.absoluteDir());
    if (containingDir.exists()) {
        openLocalFileOrDir(containingDir.path());
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

void TrayWidget::showRecentChangesContextMenu(const QPoint &position)
{
    const auto *const selectionModel = m_ui->recentChangesTreeView->selectionModel();
    if (!selectionModel || selectionModel->selectedRows().size() != 1) {
        return;
    }
    const auto copyRole = [this](SyncthingRecentChangesModel::SyncthingRecentChangesModelRole role) {
        return [this, role] {
            const auto *const selectionModelToCopy = m_ui->recentChangesTreeView->selectionModel();
            if (selectionModelToCopy && selectionModelToCopy->selectedRows().size() == 1) {
                QGuiApplication::clipboard()->setText(m_recentChangesModel.data(selectionModelToCopy->selectedRows().at(0), role).toString());
            }
        };
    };
    QMenu menu(this);
    connect(menu.addAction(QIcon::fromTheme(QStringLiteral("edit-copy"), QIcon(QStringLiteral(":/icons/hicolor/scalable/actions/edit-copy.svg"))),
                tr("Copy path")),
        &QAction::triggered, this, copyRole(SyncthingRecentChangesModel::Path));
    connect(menu.addAction(QIcon::fromTheme(QStringLiteral("network-server-symbolic"),
                               QIcon(QStringLiteral(":/icons/hicolor/scalable/places/network-workgroup.svg"))),
                tr("Copy device ID")),
        &QAction::triggered, this, copyRole(SyncthingRecentChangesModel::ModifiedBy));
    connect(menu.addAction(QIcon::fromTheme(QStringLiteral("folder"), QIcon(QStringLiteral(":/icons/hicolor/scalable/places/folder.svg"))),
                tr("Copy directory ID")),
        &QAction::triggered, this, copyRole(SyncthingRecentChangesModel::DirectoryId));
    showViewMenu(position, *m_ui->recentChangesTreeView, menu);
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
    case SyncthingStatus::RemoteNotInSync:
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

    // render traffic icons the first time the function is called
    static const auto trafficIcons = [this]() {
        const auto size = QSize(16, 13);
        const auto &palette = m_ui->trafficFormWidget->palette(); // FIXME: make this aware of palette changes
        const auto colorBackground = palette.color(QPalette::Window);
        const auto colorActive = palette.color(QPalette::WindowText);
        const auto colorInactive = QColor((colorActive.red() + colorBackground.red()) / 2, (colorActive.green() + colorBackground.green()) / 2,
            (colorActive.blue() + colorBackground.blue()) / 2);
        const auto renderIcon = [&size](QtForkAwesome::Icon icon, const QColor &color) {
            return IconManager::instance().forkAwesomeRenderer().pixmap(icon, size, color);
        };
        struct {
            QPixmap uploadIconActive;
            QPixmap uploadIconInactive;
            QPixmap downloadIconActive;
            QPixmap downloadIconInactive;
        } icons;
        icons.uploadIconActive = renderIcon(QtForkAwesome::Icon::CloudUpload, colorActive);
        icons.uploadIconInactive = renderIcon(QtForkAwesome::Icon::CloudUpload, colorInactive);
        icons.downloadIconActive = renderIcon(QtForkAwesome::Icon::CloudDownload, colorActive);
        icons.downloadIconInactive = renderIcon(QtForkAwesome::Icon::CloudDownload, colorInactive);
        return icons;
    }();

    // update text and whether to use active/inactive icons
    m_ui->inTrafficLabel->setText(trafficString(m_connection.totalIncomingTraffic(), m_connection.totalIncomingRate()));
    m_ui->outTrafficLabel->setText(trafficString(m_connection.totalOutgoingTraffic(), m_connection.totalOutgoingRate()));
    m_ui->trafficInTextLabel->setPixmap(m_connection.totalIncomingRate() > 0.0 ? trafficIcons.downloadIconActive : trafficIcons.downloadIconInactive);
    m_ui->trafficOutTextLabel->setPixmap(m_connection.totalOutgoingRate() > 0.0 ? trafficIcons.uploadIconActive : trafficIcons.uploadIconInactive);
}

void TrayWidget::updateOverallStatistics()
{
    const auto overallStats = m_connection.computeOverallDirStatistics();
    m_ui->globalStatisticsLabel->setText(directoryStatusString(overallStats.global));
    m_ui->localStatisticsLabel->setText(directoryStatusString(overallStats.local));
}

void TrayWidget::updateIconAndTooltip()
{
    if (!m_menu) {
        return;
    }
    if (auto *const trayIcon = m_menu->icon()) {
        trayIcon->updateStatusIconAndText();
    }
}

void TrayWidget::toggleRunning()
{
    switch (m_startStopButtonTarget) {
#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
    case StartStopButtonTarget::Service:
        if (auto *const service = SyncthingService::mainInstance()) {
            service->toggleRunning();
        }
        break;
#endif
    case StartStopButtonTarget::Launcher:
        if (auto *const launcher = SyncthingLauncher::mainInstance()) {
            if (launcher->isRunning()) {
                launcher->terminate(&m_connection);
            } else {
                launcher->launch(Settings::values().launcher);
            }
        }
        break;
    default:;
    }
}

Settings::Launcher::LauncherStatus TrayWidget::handleLauncherStatusChanged()
{
#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
    const auto systemdStatus = Settings::values().systemd.status(m_connection);
    const auto launcherStatus = applyLauncherSettings(false, systemdStatus.consideredForReconnect, systemdStatus.showStartStopButton);
    const auto showStartStopButton = systemdStatus.showStartStopButton || launcherStatus.showStartStopButton;
#else
    const auto launcherStatus = applyLauncherSettings(false);
    const auto showStartStopButton = launcherStatus.showStartStopButton;
#endif
    m_ui->startStopPushButton->setVisible(showStartStopButton);
    return launcherStatus;
}

Settings::Launcher::LauncherStatus TrayWidget::applyLauncherSettings(bool reconnectRequired, bool skipApplyingToConnection, bool skipStartStopButton)
{
    // update connection
    const auto &launcherSettings = Settings::values().launcher;
    const auto launcherStatus = skipApplyingToConnection ? launcherSettings.status(m_connection)
                                                         : launcherSettings.apply(m_connection, m_selectedConnection, reconnectRequired);

    if (skipStartStopButton || !launcherStatus.showStartStopButton) {
        return launcherStatus;
    }

    // update start/stop button
    m_startStopButtonTarget = StartStopButtonTarget::Launcher;
    if (launcherStatus.running) {
        m_ui->startStopPushButton->setText(tr("Stop"));
        m_ui->startStopPushButton->setToolTip(tr("Stop Syncthing instance launched via tray icon"));
        m_ui->startStopPushButton->setIcon(QIcon(QStringLiteral("stop.fa")));
    } else {
        m_ui->startStopPushButton->setText(tr("Start"));
        m_ui->startStopPushButton->setToolTip(tr("Start Syncthing with the built-in launcher configured in the settings"));
        m_ui->startStopPushButton->setIcon(QIcon(QStringLiteral("play.fa")));
    }
    return launcherStatus;
}

#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
Settings::Systemd::ServiceStatus TrayWidget::handleSystemdStatusChanged()
{
    const auto systemdStatus = applySystemdSettings();
    if (systemdStatus.showStartStopButton) {
        m_ui->startStopPushButton->setVisible(true);
        return systemdStatus;
    }

    // update the start/stop button which might now control the internal launcher
    const auto launcherStatus = applyLauncherSettings(false, true, false);
    m_ui->startStopPushButton->setVisible(launcherStatus.showStartStopButton);

    return systemdStatus;
}

Settings::Systemd::ServiceStatus TrayWidget::applySystemdSettings(bool reconnectRequired)
{
    // update connection
    const auto &systemdSettings = Settings::values().systemd;
    const auto serviceStatus = systemdSettings.apply(m_connection, m_selectedConnection, reconnectRequired);

    if (!serviceStatus.showStartStopButton) {
        return serviceStatus;
    }

    // update start/stop button
    m_startStopButtonTarget = StartStopButtonTarget::Service;
    if (serviceStatus.running) {
        m_ui->startStopPushButton->setText(tr("Stop"));
        m_ui->startStopPushButton->setToolTip(
            (serviceStatus.userService ? QStringLiteral("systemctl --user stop ") : QStringLiteral("systemctl stop "))
            + systemdSettings.syncthingUnit);
        m_ui->startStopPushButton->setIcon(QIcon(QStringLiteral("stop.fa")));
    } else {
        m_ui->startStopPushButton->setText(tr("Start"));
        m_ui->startStopPushButton->setToolTip(
            (serviceStatus.userService ? QStringLiteral("systemctl --user start ") : QStringLiteral("systemctl start "))
            + systemdSettings.syncthingUnit);
        m_ui->startStopPushButton->setIcon(QIcon(QStringLiteral("play.fa")));
    }
    return serviceStatus;
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
    auto index = m_connectionsMenu->actions().indexOf(connectionAction);
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
            m_webViewDlg->applySettings(*m_selectedConnection, false);
        }
#endif
    }
}

void TrayWidget::showDialog(QWidget *dlg, bool maximized)
{
    if (m_menu) {
        m_menu->close();
    }
    if (maximized) {
        dlg->showMaximized();
    } else {
        dlg->show();
    }
    dlg->activateWindow();
}

void TrayWidget::setBrightColorsOfModelsAccordingToPalette()
{
    const auto brightColors = isPaletteDark(palette());
    m_dirModel.setBrightColors(brightColors);
    m_devModel.setBrightColors(brightColors);
    m_dlModel.setBrightColors(brightColors);
    m_recentChangesModel.setBrightColors(brightColors);
}

} // namespace QtGui
