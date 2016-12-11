#include "./traywidget.h"
#include "./traymenu.h"
#include "./trayicon.h"
#include "./settingsdialog.h"
#include "./webviewdialog.h"
#include "./textviewdialog.h"

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
#include <QFontDatabase>

#include <functional>
#include <algorithm>

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
TrayWidget::TrayWidget(TrayMenu *parent) :
    QWidget(parent),
    m_menu(parent),
    m_ui(new Ui::TrayWidget),
#ifndef SYNCTHINGTRAY_NO_WEBVIEW
    m_webViewDlg(nullptr),
#endif
    m_dirModel(m_connection),
    m_devModel(m_connection),
    m_dlModel(m_connection),
    m_selectedConnection(nullptr)
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
    viewIdButton->setIcon(QIcon::fromTheme(QStringLiteral("view-barcode"), QIcon(QStringLiteral(":/icons/hicolor/scalable/actions/view-barcode.svg"))));
    viewIdButton->setFlat(true);
    cornerFrameLayout->addWidget(viewIdButton);
    auto *restartButton = new QPushButton(m_cornerFrame);
    restartButton->setToolTip(tr("Restart Syncthing"));
    restartButton->setIcon(QIcon::fromTheme(QStringLiteral("system-reboot"), QIcon(QStringLiteral(":/icons/hicolor/scalable/actions/view-refresh.svg"))));
    restartButton->setFlat(true);
    cornerFrameLayout->addWidget(restartButton);
    auto *showLogButton = new QPushButton(m_cornerFrame);
    showLogButton->setToolTip(tr("Show Syncthing log"));
    showLogButton->setIcon(QIcon::fromTheme(QStringLiteral("text-x-generic"), QIcon(QStringLiteral(":/icons/hicolor/scalable/mimetypes/text-x-generic.svg"))));
    showLogButton->setFlat(true);
    cornerFrameLayout->addWidget(showLogButton);
    auto *scanAllButton = new QPushButton(m_cornerFrame);
    scanAllButton->setToolTip(tr("Rescan all directories"));
    scanAllButton->setIcon(QIcon::fromTheme(QStringLiteral("folder-sync"), QIcon(QStringLiteral(":/icons/hicolor/scalable/actions/folder-sync.svg"))));
    scanAllButton->setFlat(true);
    cornerFrameLayout->addWidget(scanAllButton);
    m_ui->tabWidget->setCornerWidget(m_cornerFrame, Qt::BottomRightCorner);

    // setup connection menu
    m_connectionsActionGroup = new QActionGroup(m_connectionsMenu = new QMenu(tr("Connection"), this));
    m_connectionsMenu->setIcon(QIcon::fromTheme(QStringLiteral("network-connect"), QIcon(QStringLiteral(":/icons/hicolor/scalable/actions/network-connect.svg"))));
    m_ui->connectionsPushButton->setText(Settings::values().connection.primary.label);
    m_ui->connectionsPushButton->setMenu(m_connectionsMenu);

    // apply settings, this also establishes the connection to Syncthing (according to settings)
    applySettings();

    // setup other widgets
    m_ui->notificationsPushButton->setHidden(true);
    m_ui->trafficIconLabel->setPixmap(QIcon::fromTheme(QStringLiteral("network-card"), QIcon(QStringLiteral(":/icons/hicolor/scalable/devices/network-card.svg"))).pixmap(32));

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
    connect(m_ui->devsTreeView, &DevView::pauseResumeDev, this, &TrayWidget::pauseResumeDev);
    connect(m_ui->downloadsTreeView, &DownloadView::openDir, this, &TrayWidget::openDir);
    connect(m_ui->downloadsTreeView, &DownloadView::openItemDir, this, &TrayWidget::openItemDir);
    connect(scanAllButton, &QPushButton::clicked, &m_connection, &SyncthingConnection::rescanAllDirs);
    connect(viewIdButton, &QPushButton::clicked, this, &TrayWidget::showOwnDeviceId);
    connect(showLogButton, &QPushButton::clicked, this, &TrayWidget::showLog);
    connect(m_ui->notificationsPushButton, &QPushButton::clicked, this, &TrayWidget::showNotifications);
    connect(restartButton, &QPushButton::clicked, this, &TrayWidget::restartSyncthing);
    connect(m_connectionsActionGroup, &QActionGroup::triggered, this, &TrayWidget::handleConnectionSelected);
}

TrayWidget::~TrayWidget()
{
    auto i = std::find(m_instances.begin(), m_instances.end(), this);
    if(i != m_instances.end()) {
        m_instances.erase(i);
    }
    if(m_instances.empty()) {
        QCoreApplication::quit();
    }
}

void TrayWidget::showSettingsDialog()
{
    if(!m_settingsDlg) {
        m_settingsDlg = new SettingsDialog(&m_connection, this);
        connect(m_settingsDlg, &SettingsDialog::applied, &TrayWidget::applySettings);
    }
    centerWidget(m_settingsDlg);
    showDialog(m_settingsDlg);
}

void TrayWidget::showAboutDialog()
{
    if(!m_aboutDlg) {
        m_aboutDlg = new AboutDialog(this, QString(), QStringLiteral(APP_AUTHOR "\nfallback icons from KDE/Breeze project\nSyncthing icons from Syncthing project"), QString(), QString(), QStringLiteral(APP_DESCRIPTION), QImage(QStringLiteral(":/icons/hicolor/scalable/app/syncthingtray.svg")));
        m_aboutDlg->setWindowTitle(tr("About") + QStringLiteral(" - " APP_NAME));
        m_aboutDlg->setWindowIcon(QIcon(QStringLiteral(":/icons/hicolor/scalable/app/syncthingtray.svg")));
    }
    centerWidget(m_aboutDlg);
    showDialog(m_aboutDlg);
}

void TrayWidget::showWebUi()
{
#ifndef SYNCTHINGTRAY_NO_WEBVIEW
    if(Settings::values().webView.disabled) {
#endif
        QDesktopServices::openUrl(m_connection.syncthingUrl());
#ifndef SYNCTHINGTRAY_NO_WEBVIEW
    } else {
        if(!m_webViewDlg) {
            m_webViewDlg = new WebViewDialog(this);
            if(m_selectedConnection) {
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
    auto *dlg = new QWidget(this, Qt::Window);
    dlg->setWindowTitle(tr("Own device ID") + QStringLiteral(" - " APP_NAME));
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
    connect(dlg, &QWidget::destroyed,
            bind(static_cast<bool(*)(const QMetaObject::Connection &)>(&QObject::disconnect),
                 m_connection.requestQrCode(m_connection.myId(), [pixmapLabel](const QByteArray &data) {
        QPixmap pixmap;
        pixmap.loadFromData(data);
        pixmapLabel->setPixmap(pixmap);
    })));
    dlg->setLayout(layout);
    centerWidget(dlg);
    showDialog(dlg);
}

void TrayWidget::showLog()
{
    auto *dlg = new TextViewDialog(tr("Log"), this);
    auto loadLog = [dlg, this] {
        connect(dlg, &QWidget::destroyed,
                bind(static_cast<bool(*)(const QMetaObject::Connection &)>(&QObject::disconnect),
                     m_connection.requestLog([dlg, this] (const std::vector<SyncthingLogEntry> &entries) {
                        dlg->browser()->clear();
                        for(const SyncthingLogEntry &entry : entries) {
                            dlg->browser()->append(entry.when % QChar(':') % QChar(' ') % QChar('\n') % entry.message % QChar('\n'));
                        }
            })
        ));
    };
    connect(dlg, &TextViewDialog::reload, loadLog);
    loadLog();
    showDialog(dlg);
}

void TrayWidget::showNotifications()
{
    auto *dlg = new TextViewDialog(tr("New notifications"), this);
    for(const SyncthingLogEntry &entry : m_notifications) {
        dlg->browser()->append(entry.when % QChar(':') % QChar(' ') % QChar('\n') % entry.message % QChar('\n'));
    }
    m_notifications.clear();
    showDialog(dlg);
    dismissNotifications();
}

void TrayWidget::dismissNotifications()
{
    m_connection.considerAllNotificationsRead();
    m_ui->notificationsPushButton->setHidden(true);
    if(m_menu && m_menu->icon()) {
        m_menu->icon()->updateStatusIconAndText(m_connection.status());
    }
}

void TrayWidget::restartSyncthing()
{
    if(QMessageBox::warning(this, QCoreApplication::applicationName(), tr("Do you really want to restart Syncthing?"), QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes) {
        m_connection.restart();
    }
}

void TrayWidget::quitTray()
{
    QObject *parent;
    if(m_menu) {
        if(m_menu->icon()) {
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
    switch(status) {
    case SyncthingStatus::Disconnected:
        m_ui->statusPushButton->setText(tr("Connect"));
        m_ui->statusPushButton->setToolTip(tr("Not connected to Syncthing, click to connect"));
        m_ui->statusPushButton->setIcon(QIcon::fromTheme(QStringLiteral("view-refresh"), QIcon(QStringLiteral(":/icons/hicolor/scalable/actions/view-refresh.svg"))));
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
        m_ui->statusPushButton->setIcon(QIcon::fromTheme(QStringLiteral("media-playback-pause"), QIcon(QStringLiteral(":/icons/hicolor/scalable/actions/media-playback-pause.svg"))));
        m_ui->statusPushButton->setHidden(false);
        break;
    case SyncthingStatus::Paused:
        m_ui->statusPushButton->setText(tr("Continue"));
        m_ui->statusPushButton->setToolTip(tr("At least one device is paused, click to resume"));
        m_ui->statusPushButton->setIcon(QIcon::fromTheme(QStringLiteral("media-playback-start"), QIcon(QStringLiteral(":/icons/hicolor/scalable/actions/media-playback-resume.svg"))));
        m_ui->statusPushButton->setHidden(false);
        break;
    default:
        ;
    }
}

void TrayWidget::applySettings()
{
    for(TrayWidget *instance : m_instances) {
        // update connections menu
        int connectionIndex = 0;
        auto &settings = Settings::values();
        auto &primaryConnectionSettings = settings.connection.primary;
        auto &secondaryConnectionSettings = settings.connection.secondary;
        const int connectionCount = static_cast<int>(1 + secondaryConnectionSettings.size());
        const QList<QAction *> connectionActions = instance->m_connectionsActionGroup->actions();
        instance->m_selectedConnection = nullptr;
        for(; connectionIndex < connectionCount; ++connectionIndex) {
            SyncthingConnectionSettings &connectionSettings = (connectionIndex == 0 ? primaryConnectionSettings : secondaryConnectionSettings[static_cast<size_t>(connectionIndex - 1)]);
            if(connectionIndex < connectionActions.size()) {
                QAction *action = connectionActions.at(connectionIndex);
                action->setText(connectionSettings.label);
                if(action->isChecked()) {
                    instance->m_selectedConnection = &connectionSettings;
                }
            } else {
                QAction *action = instance->m_connectionsMenu->addAction(connectionSettings.label);
                action->setCheckable(true);
                instance->m_connectionsActionGroup->addAction(action);
            }
        }
        for(; connectionIndex < connectionActions.size(); ++connectionIndex) {
            delete connectionActions.at(connectionIndex);
        }
        if(!instance->m_selectedConnection) {
            instance->m_selectedConnection = &primaryConnectionSettings;
            instance->m_connectionsMenu->actions().at(0)->setChecked(true);
        }
        instance->m_ui->connectionsPushButton->setText(instance->m_selectedConnection->label);
        instance->m_connection.reconnect(*instance->m_selectedConnection);

        // web view
#ifndef SYNCTHINGTRAY_NO_WEBVIEW
        if(instance->m_webViewDlg) {
            instance->m_webViewDlg->applySettings(*instance->m_selectedConnection);
        }
#endif

        // update visual appearance
        instance->m_ui->trafficFormWidget->setVisible(settings.appearance.showTraffic);
        instance->m_ui->trafficIconLabel->setVisible(settings.appearance.showTraffic);
        instance->m_ui->trafficHorizontalSpacer->changeSize(0, 20, settings.appearance.showTraffic ? QSizePolicy::Expanding : QSizePolicy::Ignored, QSizePolicy::Minimum);
        if(settings.appearance.showTraffic) {
            instance->updateTraffic();
        }
        instance->m_ui->infoFrame->setFrameStyle(settings.appearance.frameStyle);
        instance->m_ui->buttonsFrame->setFrameStyle(settings.appearance.frameStyle);
        if(QApplication::style() && !QApplication::style()->objectName().compare(QLatin1String("adwaita"), Qt::CaseInsensitive)) {
            instance->m_cornerFrame->setFrameStyle(QFrame::NoFrame);
        } else {
            instance->m_cornerFrame->setFrameStyle(settings.appearance.frameStyle);
        }
        if(settings.appearance.tabPosition >= QTabWidget::North && settings.appearance.tabPosition <= QTabWidget::East) {
            instance->m_ui->tabWidget->setTabPosition(static_cast<QTabWidget::TabPosition>(settings.appearance.tabPosition));
        }
        instance->m_dirModel.setBrightColors(settings.appearance.brightTextColors);
        instance->m_devModel.setBrightColors(settings.appearance.brightTextColors);
        instance->m_dlModel.setBrightColors(settings.appearance.brightTextColors);
    }
}

void TrayWidget::openDir(const SyncthingDir &dir)
{
    if(QDir(dir.path).exists()) {
        DesktopUtils::openLocalFileOrDir(dir.path);
    } else {
        QMessageBox::warning(this, QCoreApplication::applicationName(), tr("The directory <i>%1</i> does not exist on the local machine.").arg(dir.path));
    }
}

void TrayWidget::openItemDir(const SyncthingItemDownloadProgress &item)
{
    if(item.fileInfo.exists()) {
        DesktopUtils::openLocalFileOrDir(item.fileInfo.path());
    } else {
        QMessageBox::warning(this, QCoreApplication::applicationName(), tr("The file <i>%1</i> does not exist on the local machine.").arg(item.fileInfo.filePath()));
    }
}

void TrayWidget::scanDir(const SyncthingDir &dir)
{
    m_connection.rescan(dir.id);
}

void TrayWidget::pauseResumeDev(const SyncthingDev &dev)
{
    if(dev.paused) {
        m_connection.resume(dev.id);
    } else {
        m_connection.pause(dev.id);
    }
}

void TrayWidget::changeStatus()
{
    switch(m_connection.status()) {
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
    default:
        ;
    }
}

void TrayWidget::updateTraffic()
{
    if(m_ui->trafficFormWidget->isHidden()) {
        return;
    }
    static const QString unknownStr(tr("unknown"));
    if(m_connection.isConnected()) {
        if(m_connection.totalIncomingRate() != 0.0) {
            m_ui->inTrafficLabel->setText(m_connection.totalIncomingTraffic() != 0
                                          ? QStringLiteral("%1 (%2)").arg(QString::fromUtf8(bitrateToString(m_connection.totalIncomingRate(), true).data()), QString::fromUtf8(dataSizeToString(m_connection.totalIncomingTraffic()).data()))
                                          : QString::fromUtf8(bitrateToString(m_connection.totalIncomingRate(), true).data()));
        } else {
            m_ui->inTrafficLabel->setText(m_connection.totalIncomingTraffic() != 0 ? QString::fromUtf8(dataSizeToString(m_connection.totalIncomingTraffic()).data()) : unknownStr);
        }
        if(m_connection.totalOutgoingRate() != 0.0) {
            m_ui->outTrafficLabel->setText(m_connection.totalIncomingTraffic() != 0
                                          ? QStringLiteral("%1 (%2)").arg(QString::fromUtf8(bitrateToString(m_connection.totalOutgoingRate(), true).data()), QString::fromUtf8(dataSizeToString(m_connection.totalOutgoingTraffic()).data()))
                                          : QString::fromUtf8(bitrateToString(m_connection.totalOutgoingRate(), true).data()));
        } else {
            m_ui->outTrafficLabel->setText(m_connection.totalOutgoingTraffic() != 0 ? QString::fromUtf8(dataSizeToString(m_connection.totalOutgoingTraffic()).data()) : unknownStr);
        }
    } else {
        m_ui->inTrafficLabel->setText(unknownStr);
        m_ui->outTrafficLabel->setText(unknownStr);
    }

}

#ifndef SYNCTHINGTRAY_NO_WEBVIEW
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
    if(index >= 0) {
        m_selectedConnection = (index == 0)
                ? &Settings::values().connection.primary
                : &Settings::values().connection.secondary[static_cast<size_t>(index - 1)];
        m_ui->connectionsPushButton->setText(m_selectedConnection->label);
        m_connection.reconnect(*m_selectedConnection);
#ifndef SYNCTHINGTRAY_NO_WEBVIEW
        if(m_webViewDlg) {
            m_webViewDlg->applySettings(*m_selectedConnection);
        }
#endif
    }
}

void TrayWidget::showDialog(QWidget *dlg)
{
    if(m_menu) {
        m_menu->close();
    }
    dlg->show();
    dlg->activateWindow();
}

}
