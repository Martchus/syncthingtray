#include "./traywidget.h"
#include "./traymenu.h"
#include "./settingsdialog.h"
#include "./webviewdialog.h"

#include "../application/settings.h"

#include "resources/config.h"
#include "ui_traywidget.h"

#include <qtutilities/resources/qtconfigarguments.h>
#include <qtutilities/resources/resources.h>
#include <qtutilities/settingsdialog/qtsettings.h>
#include <qtutilities/aboutdialog/aboutdialog.h>
#include <qtutilities/misc/dialogutils.h>
#include <qtutilities/misc/desktoputils.h>

#include <c++utilities/conversion/stringconversion.h>

#include <QCoreApplication>
#include <QDesktopServices>
#include <QMessageBox>
#include <QClipboard>
#include <QDir>
#include <QTextBrowser>
#include <QStringBuilder>

#include <functional>

using namespace ApplicationUtilities;
using namespace ConversionUtilities;
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
    viewIdButton->setIcon(QIcon::fromTheme(QStringLiteral("view-barcode"), QIcon(QStringLiteral(":/icons/hicolor/scalable/actions/view-barcode.svg"))));
    viewIdButton->setFlat(true);
    cornerFrameLayout->addWidget(viewIdButton);
    auto *showLogButton = new QPushButton(cornerFrame);
    showLogButton->setToolTip(tr("Show Syncthing log"));
    showLogButton->setIcon(QIcon::fromTheme(QStringLiteral("text-x-generic"), QIcon(QStringLiteral(":/icons/hicolor/scalable/actions/text-x-generic.svg"))));
    showLogButton->setFlat(true);
    connect(showLogButton, &QPushButton::clicked, this, &TrayWidget::showLog);
    cornerFrameLayout->addWidget(showLogButton);
    auto *scanAllButton = new QPushButton(cornerFrame);
    scanAllButton->setToolTip(tr("Rescan all directories"));
    scanAllButton->setIcon(QIcon::fromTheme(QStringLiteral("folder-sync"), QIcon(QStringLiteral(":/icons/hicolor/scalable/actions/folder-sync.svg"))));
    scanAllButton->setFlat(true);

    cornerFrameLayout->addWidget(scanAllButton);
    m_ui->tabWidget->setCornerWidget(cornerFrame, Qt::BottomRightCorner);

    m_ui->trafficIconLabel->setPixmap(QIcon::fromTheme(QStringLiteral("network-card"), QIcon(QStringLiteral(":/icons/hicolor/scalable/devices/network-card.svg"))).pixmap(32));

    // connect signals and slots
    connect(m_ui->statusPushButton, &QPushButton::clicked, this, &TrayWidget::changeStatus);
    connect(m_ui->closePushButton, &QPushButton::clicked, &QCoreApplication::quit);
    connect(m_ui->aboutPushButton, &QPushButton::clicked, this, &TrayWidget::showAboutDialog);
    connect(m_ui->webUiPushButton, &QPushButton::clicked, this, &TrayWidget::showWebUi);
    connect(m_ui->settingsPushButton, &QPushButton::clicked, this, &TrayWidget::showSettingsDialog);
    connect(&m_connection, &SyncthingConnection::statusChanged, this, &TrayWidget::updateStatusButton);
    connect(&m_connection, &SyncthingConnection::trafficChanged, this, &TrayWidget::updateTraffic);
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
        m_aboutDlg = new AboutDialog(this, QString(), QStringLiteral(APP_AUTHOR "\nfallback icons from KDE/Breeze project\nSyncthing icons from Syncthing project"), QString(), QString(), tr("Tray application for Syncthing"), QImage(QStringLiteral(":/icons/hicolor/scalable/app/syncthingtray.svg")));
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
        m_ui->statusPushButton->setIcon(QIcon::fromTheme(QStringLiteral("view-refresh"), QIcon(QStringLiteral(":/icons/hicolor/scalable/actions/view-refresh.svg"))));
        break;
    case SyncthingStatus::Default:
    case SyncthingStatus::NotificationsAvailable:
    case SyncthingStatus::Synchronizing:
        m_ui->statusPushButton->setText(tr("Pause"));
        m_ui->statusPushButton->setToolTip(tr("Syncthing is running, click to pause all devices"));
        m_ui->statusPushButton->setIcon(QIcon::fromTheme(QStringLiteral("media-playback-pause"), QIcon(QStringLiteral(":/icons/hicolor/scalable/actions/media-playback-pause.svg"))));
        break;
    case SyncthingStatus::Paused:
        m_ui->statusPushButton->setText(tr("Continue"));
        m_ui->statusPushButton->setToolTip(tr("At least one device is paused, click to resume"));
        m_ui->statusPushButton->setIcon(QIcon::fromTheme(QStringLiteral("media-playback-start"), QIcon(QStringLiteral(":/icons/hicolor/scalable/actions/media-playback-resume.svg"))));
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
    m_ui->trafficFrame->setVisible(Settings::showTraffic());
    if(Settings::showTraffic()) {
        updateTraffic();
    }
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

void TrayWidget::changeStatus()
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

void TrayWidget::updateTraffic()
{
    if(m_ui->trafficFrame->isHidden()) {
        return;
    }
    if(m_connection.totalIncomingRate() != 0.0) {
        m_ui->inTrafficLabel->setText(m_connection.totalIncomingTraffic() >= 0
                                      ? QStringLiteral("%1 (%2)").arg(QString::fromUtf8(bitrateToString(m_connection.totalIncomingRate(), true).data()), QString::fromUtf8(dataSizeToString(m_connection.totalIncomingTraffic()).data()))
                                      : QString::fromUtf8(bitrateToString(m_connection.totalIncomingRate(), true).data()));
    } else {
        m_ui->inTrafficLabel->setText(m_connection.totalIncomingTraffic() >= 0 ? QString::fromUtf8(dataSizeToString(m_connection.totalIncomingTraffic()).data()) : tr("unknown"));
    }
    if(m_connection.totalOutgoingRate() != 0.0) {
        m_ui->outTrafficLabel->setText(m_connection.totalIncomingTraffic() >= 0
                                      ? QStringLiteral("%1 (%2)").arg(QString::fromUtf8(bitrateToString(m_connection.totalOutgoingRate(), true).data()), QString::fromUtf8(dataSizeToString(m_connection.totalOutgoingTraffic()).data()))
                                      : QString::fromUtf8(bitrateToString(m_connection.totalOutgoingRate(), true).data()));
    } else {
        m_ui->outTrafficLabel->setText(m_connection.totalOutgoingTraffic() >= 0 ? QString::fromUtf8(dataSizeToString(m_connection.totalOutgoingTraffic()).data()) : tr("unknown"));
    }

}

void TrayWidget::handleWebViewDeleted()
{
    m_webViewDlg = nullptr;
}

}
