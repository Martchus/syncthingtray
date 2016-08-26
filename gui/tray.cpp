#include "./tray.h"
#include "./settingsdialog.h"
#include "./webviewdialog.h"
#include "./dirbuttonsitemdelegate.h"

#include "../application/settings.h"

#include "resources/config.h"

#include "ui_traywidget.h"

#include <qtutilities/resources/qtconfigarguments.h>
#include <qtutilities/resources/resources.h>
#include <qtutilities/resources/importplugin.h>
#include <qtutilities/settingsdialog/qtsettings.h>
#include <qtutilities/aboutdialog/aboutdialog.h>
#include <qtutilities/misc/dialogutils.h>
#include <qtutilities/misc/desktoputils.h>

#include <QApplication>
#include <QSvgRenderer>
#include <QPainter>
#include <QPixmap>
#include <QMenu>
#include <QDesktopServices>
#include <QMainWindow>
#include <QMessageBox>
#include <QCursor>
#include <QDesktopWidget>
#include <QLabel>
#include <QClipboard>
#include <QUrl>
#include <QDir>
#include <QTextBrowser>
#include <QStringBuilder>

#include <functional>

using namespace ApplicationUtilities;
using namespace Dialogs;
using namespace Data;
using namespace std;

namespace QtGui {

/*!
 * \brief Instantiates a new tray widget.
 */
TrayWidget::TrayWidget(TrayMenu *parent) :
    QWidget(parent),
    m_menu(parent),
    m_ui(new Ui::TrayWidget),
    m_settingsDlg(nullptr),
    m_aboutDlg(nullptr),
#ifndef SYNCTHINGTRAY_NO_WEBVIEW
    m_webViewDlg(nullptr),
#endif
    m_dirModel(m_connection),
    m_devModel(m_connection)
{
    m_ui->setupUi(this);

    // setup model and view
    m_ui->dirsTreeView->setModel(&m_dirModel);
    m_ui->devsTreeView->setModel(&m_devModel);

    // apply settings, this also establishes the connection to Syncthing
    applySettings();

    // setup sync-all button
    auto *cornerFrame = new QFrame(this);
    cornerFrame->setFrameStyle(QFrame::StyledPanel), cornerFrame->setFrameShadow(QFrame::Sunken);
    auto *cornerFrameLayout = new QHBoxLayout(cornerFrame);
    cornerFrameLayout->setSpacing(0), cornerFrameLayout->setMargin(0);
    cornerFrameLayout->addStretch();
    cornerFrame->setLayout(cornerFrameLayout);
    auto *viewIdButton = new QPushButton(cornerFrame);
    viewIdButton->setToolTip(tr("View own device ID"));
    viewIdButton->setIcon(QIcon::fromTheme(QStringLiteral("view-barcode")));
    viewIdButton->setFlat(true);
    cornerFrameLayout->addWidget(viewIdButton);
    auto *showLogButton = new QPushButton(cornerFrame);
    showLogButton->setToolTip(tr("Show Syncthing log"));
    showLogButton->setIcon(QIcon::fromTheme(QStringLiteral("text-plain")));
    showLogButton->setFlat(true);
    connect(showLogButton, &QPushButton::clicked, this, &TrayWidget::showLog);
    cornerFrameLayout->addWidget(showLogButton);
    auto *scanAllButton = new QPushButton(cornerFrame);
    scanAllButton->setToolTip(tr("Rescan all directories"));
    scanAllButton->setIcon(QIcon::fromTheme(QStringLiteral("folder-sync")));
    scanAllButton->setFlat(true);

    cornerFrameLayout->addWidget(scanAllButton);
    m_ui->tabWidget->setCornerWidget(cornerFrame, Qt::BottomRightCorner);

    // connect signals and slots
    connect(m_ui->statusPushButton, &QPushButton::clicked, this, &TrayWidget::handleStatusButtonClicked);
    connect(m_ui->closePushButton, &QPushButton::clicked, &QApplication::quit);
    connect(m_ui->aboutPushButton, &QPushButton::clicked, this, &TrayWidget::showAboutDialog);
    connect(m_ui->webUiPushButton, &QPushButton::clicked, this, &TrayWidget::showWebUi);
    connect(m_ui->settingsPushButton, &QPushButton::clicked, this, &TrayWidget::showSettingsDialog);
    connect(&m_connection, &SyncthingConnection::statusChanged, this, &TrayWidget::updateStatusButton);
    connect(m_ui->dirsTreeView, &DirView::openDir, this, &TrayWidget::openDir);
    connect(m_ui->dirsTreeView, &DirView::scanDir, this, &TrayWidget::scanDir);
    connect(m_ui->devsTreeView, &DevView::pauseResumeDev, this, &TrayWidget::pauseResumeDev);
    connect(scanAllButton, &QPushButton::clicked, &m_connection, &SyncthingConnection::rescanAllDirs);
    connect(viewIdButton, &QPushButton::clicked, this, &TrayWidget::showOwnDeviceId);
}

TrayWidget::~TrayWidget()
{}

void TrayWidget::showSettingsDialog()
{
    if(!m_settingsDlg) {
        m_settingsDlg = new SettingsDialog(&m_connection, this);
        m_settingsDlg->setWindowTitle(tr("Settings - Syncthing tray"));
        connect(m_settingsDlg, &SettingsDialog::applied, this, &TrayWidget::applySettings);
#ifndef SYNCTHINGTRAY_NO_WEBVIEW
        if(m_webViewDlg) {
            connect(m_settingsDlg, &SettingsDialog::applied, m_webViewDlg, &WebViewDialog::applySettings);
        }
#endif
    }
    m_settingsDlg->show();
    centerWidget(m_settingsDlg);
    if(m_menu) {
        m_menu->close();
    }
    m_settingsDlg->activateWindow();
}

void TrayWidget::showAboutDialog()
{
    if(!m_aboutDlg) {
        m_aboutDlg = new AboutDialog(this, tr("Tray application for Syncthing"), QImage(QStringLiteral(":/icons/hicolor/scalable/app/syncthingtray.svg")));
        m_aboutDlg->setWindowTitle(tr("About - Syncthing Tray"));
        m_aboutDlg->setWindowIcon(QIcon(QStringLiteral(":/icons/hicolor/scalable/app/syncthingtray.svg")));
    }
    m_aboutDlg->show();
    centerWidget(m_aboutDlg);
    if(m_menu) {
        m_menu->close();
    }
    m_aboutDlg->activateWindow();
}

void TrayWidget::showWebUi()
{
#ifndef SYNCTHINGTRAY_NO_WEBVIEW
    if(Settings::webViewDisabled()) {
#endif
        QDesktopServices::openUrl(Settings::syncthingUrl());
#ifndef SYNCTHINGTRAY_NO_WEBVIEW
    } else {
        if(!m_webViewDlg) {
            m_webViewDlg = new WebViewDialog(this);
            connect(m_webViewDlg, &WebViewDialog::destroyed, this, &TrayWidget::handleWebViewDeleted);
            if(m_settingsDlg) {
                connect(m_settingsDlg, &SettingsDialog::applied, m_webViewDlg, &WebViewDialog::applySettings);
            }
        }
        m_webViewDlg->show();
        if(m_menu) {
            m_menu->close();
        }
        m_webViewDlg->activateWindow();
    }
#endif
}

void TrayWidget::showOwnDeviceId()
{
    auto *dlg = new QDialog(this);
    dlg->setWindowTitle(tr("Own device ID - Syncthing Tray"));
    dlg->setWindowIcon(QIcon(QStringLiteral(":/icons/hicolor/scalable/app/syncthingtray.svg")));
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->setBackgroundRole(QPalette::Background);
    auto *layout = new QVBoxLayout(dlg);
    layout->setAlignment(Qt::AlignCenter);
    auto *pixmapLabel = new QLabel(dlg);
    pixmapLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(pixmapLabel);
    auto *textLabel = new QLabel(dlg);
    textLabel->setText(m_connection.myId().isEmpty() ? tr("device ID is unknown") : m_connection.myId());
    QFont defaultFont = textLabel->font();
    defaultFont.setBold(true);
    defaultFont.setPointSize(defaultFont.pointSize() + 2);
    textLabel->setFont(defaultFont);
    textLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(textLabel);
    auto *copyPushButton = new QPushButton(dlg);
    copyPushButton->setText(tr("Copy to clipboard"));
    connect(copyPushButton, &QPushButton::clicked, bind(&QClipboard::setText, QGuiApplication::clipboard(), m_connection.myId(), QClipboard::Clipboard));
    layout->addWidget(copyPushButton);
    m_connection.requestQrCode(m_connection.myId(), bind(&QLabel::setPixmap, pixmapLabel, placeholders::_1));
    dlg->setLayout(layout);
    dlg->show();
    centerWidget(dlg);
    if(m_menu) {
        m_menu->close();
    }
    dlg->activateWindow();
}

void TrayWidget::showLog()
{
    auto *dlg = new QDialog(this);
    dlg->setWindowTitle(tr("Log - Syncthing"));
    dlg->setWindowIcon(QIcon(QStringLiteral(":/icons/hicolor/scalable/app/syncthingtray.svg")));
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    auto *layout = new QVBoxLayout(dlg);
    layout->setAlignment(Qt::AlignCenter);
    auto *browser = new QTextBrowser(dlg);
    m_connection.requestLog([browser] (const std::vector<SyncthingLogEntry> &entries) {
        for(const SyncthingLogEntry &entry : entries) {
            browser->append(entry.when % QChar(':') % QChar(' ') % QChar('\n') % entry.message % QChar('\n'));
        }
    });
    layout->addWidget(browser);
    dlg->setLayout(layout);
    dlg->show();
    dlg->resize(600, 500);
    centerWidget(dlg);
    if(m_menu) {
        m_menu->close();
    }
    dlg->activateWindow();
}

void TrayWidget::updateStatusButton(SyncthingStatus status)
{
    switch(status) {
    case SyncthingStatus::Disconnected:
        m_ui->statusPushButton->setText(tr("Connect"));
        m_ui->statusPushButton->setToolTip(tr("Not connected to Syncthing, click to connect"));
        m_ui->statusPushButton->setIcon(QIcon::fromTheme(QStringLiteral("view-refresh")));
        break;
    case SyncthingStatus::Default:
    case SyncthingStatus::NotificationsAvailable:
    case SyncthingStatus::Synchronizing:
        m_ui->statusPushButton->setText(tr("Pause"));
        m_ui->statusPushButton->setToolTip(tr("Syncthing is running, click to pause all devices"));
        m_ui->statusPushButton->setIcon(QIcon::fromTheme(QStringLiteral("media-playback-pause")));
        break;
    case SyncthingStatus::Paused:
        m_ui->statusPushButton->setText(tr("Continue"));
        m_ui->statusPushButton->setToolTip(tr("At least one device is paused, click to resume"));
        m_ui->statusPushButton->setIcon(QIcon::fromTheme(QStringLiteral("media-playback-start")));
        break;
    }
}

void TrayWidget::applySettings()
{
    m_connection.setSyncthingUrl(Settings::syncthingUrl());
    m_connection.setApiKey(Settings::apiKey());
    if(Settings::authEnabled()) {
        m_connection.setCredentials(Settings::userName(), Settings::password());
    } else {
        m_connection.setCredentials(QString(), QString());
    }
    m_connection.reconnect();
}

void TrayWidget::openDir(const QModelIndex &dirIndex)
{
    if(const SyncthingDir *dir = m_dirModel.dirInfo(dirIndex)) {
        if(QDir(dir->path).exists()) {
            DesktopUtils::openLocalFileOrDir(dir->path);
        } else {
            QMessageBox::warning(this, QCoreApplication::applicationName(), tr("The directly <i>%1</i> does not exist on the local machine.").arg(dir->path));
        }
    }
}

void TrayWidget::scanDir(const QModelIndex &dirIndex)
{
    if(const SyncthingDir *dir = m_dirModel.dirInfo(dirIndex)) {
        m_connection.rescan(dir->id);
    }
}

void TrayWidget::pauseResumeDev(const QModelIndex &devIndex)
{
    if(const SyncthingDev *dev = m_devModel.devInfo(devIndex)) {
        if(dev->paused) {
            m_connection.resume(dev->id);
        } else {
            m_connection.pause(dev->id);
        }
    }
}

void TrayWidget::handleStatusButtonClicked()
{
    switch(m_connection.status()) {
    case SyncthingStatus::Disconnected:
        m_connection.connect();
        break;
    case SyncthingStatus::Default:
    case SyncthingStatus::NotificationsAvailable:
    case SyncthingStatus::Synchronizing:
        m_connection.pauseAllDevs();
        break;
    case SyncthingStatus::Paused:
        m_connection.resumeAllDevs();
        break;
    }
}

void TrayWidget::handleWebViewDeleted()
{
    m_webViewDlg = nullptr;
}

TrayMenu::TrayMenu(QWidget *parent) :
    QMenu(parent)
{
    auto *menuLayout = new QHBoxLayout;
    menuLayout->setMargin(0), menuLayout->setSpacing(0);
    menuLayout->addWidget(m_trayWidget = new TrayWidget(this));
    setLayout(menuLayout);
    setPlatformMenu(nullptr);
}

QSize TrayMenu::sizeHint() const
{
    return QSize(350, 300);
}

/*!
 * \brief Instantiates a new tray icon.
 */
TrayIcon::TrayIcon(QObject *parent) :
    QSystemTrayIcon(parent),
    m_size(QSize(128, 128)),
    m_statusIconDisconnected(QIcon(renderSvgImage(QStringLiteral(":/icons/hicolor/scalable/status/syncthing-disconnected.svg")))),
    m_statusIconDefault(QIcon(renderSvgImage(QStringLiteral(":/icons/hicolor/scalable/status/syncthing-default.svg")))),
    m_statusIconNotify(QIcon(renderSvgImage(QStringLiteral(":/icons/hicolor/scalable/status/syncthing-notify.svg")))),
    m_statusIconPause(QIcon(renderSvgImage(QStringLiteral(":/icons/hicolor/scalable/status/syncthing-pause.svg")))),
    m_statusIconSync(QIcon(renderSvgImage(QStringLiteral(":/icons/hicolor/scalable/status/syncthing-sync.svg")))),
    m_status(SyncthingStatus::Disconnected)
{
    // set context menu
    connect(m_contextMenu.addAction(QIcon::fromTheme(QStringLiteral("internet-web-browser")), tr("Web UI")), &QAction::triggered, m_trayMenu.widget(), &TrayWidget::showWebUi);
    connect(m_contextMenu.addAction(QIcon::fromTheme(QStringLiteral("preferences-other")), tr("Settings")), &QAction::triggered, m_trayMenu.widget(), &TrayWidget::showSettingsDialog);
    connect(m_contextMenu.addAction(QIcon::fromTheme(QStringLiteral("help-about")), tr("About")), &QAction::triggered, m_trayMenu.widget(), &TrayWidget::showAboutDialog);
    m_contextMenu.addSeparator();
    connect(m_contextMenu.addAction(QIcon::fromTheme(QStringLiteral("window-close")), tr("Close")), &QAction::triggered, &QCoreApplication::quit);
    setContextMenu(&m_contextMenu);

    // set initial status
    updateStatusIconAndText(SyncthingStatus::Disconnected);

    // connect signals and slots
    SyncthingConnection *connection = &(m_trayMenu.widget()->connection());
    connect(this, &TrayIcon::activated, this, &TrayIcon::handleActivated);
    connect(connection, &SyncthingConnection::error, this, &TrayIcon::showSyncthingError);
    connect(connection, &SyncthingConnection::newNotification, this, &TrayIcon::showSyncthingNotification);
    connect(connection, &SyncthingConnection::statusChanged, this, &TrayIcon::updateStatusIconAndText);
}

void TrayIcon::handleActivated(QSystemTrayIcon::ActivationReason reason)
{
    switch(reason) {
    case QSystemTrayIcon::Context:
        // can't catch that event on Plasma 5 anyways
        break;
    case QSystemTrayIcon::Trigger:
        // either show web UI or context menu
        if(false) {
            m_trayMenu.widget()->showWebUi();
        } else {
            // when showing the menu manually
            // move the menu to the closest of the currently available screen
            // this implies that the tray icon is located near the edge of the screen; otherwise this behavior makes no sense
            const QPoint cursorPos(QCursor::pos());
            const QRect availableGeometry(QApplication::desktop()->availableGeometry(cursorPos));
            Qt::Alignment alignment = 0;
            alignment |= (cursorPos.x() - availableGeometry.left() < availableGeometry.right() - cursorPos.x() ? Qt::AlignLeft : Qt::AlignRight);
            alignment |= (cursorPos.y() - availableGeometry.top() < availableGeometry.bottom() - cursorPos.y() ? Qt::AlignTop : Qt::AlignBottom);
            m_trayMenu.setGeometry(
                QStyle::alignedRect(
                    Qt::LeftToRight,
                    alignment,
                    m_trayMenu.sizeHint(),
                    availableGeometry
                )
            );
            m_trayMenu.show();
        }
        break;
    default:
        ;
    }
}

void TrayIcon::showSyncthingError(const QString &errorMsg)
{
    if(Settings::notifyOnErrors()) {
        showMessage(tr("Syncthing error"), errorMsg, QSystemTrayIcon::Critical);
    }
}

void TrayIcon::showSyncthingNotification(const QString &message)
{
    if(Settings::showSyncthingNotifications()) {
        showMessage(tr("Syncthing notification"), message, QSystemTrayIcon::Information);
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
    case SyncthingStatus::Default:
        setIcon(m_statusIconDefault);
        setToolTip(tr("Syncthing is running"));
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
