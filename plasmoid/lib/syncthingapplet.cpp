#include "./syncthingapplet.h"

#include "../../widgets/misc/otherdialogs.h"
#include "../../widgets/misc/textviewdialog.h"
#include "../../widgets/settings/settings.h"
#include "../../widgets/settings/settingsdialog.h"
#include "../../widgets/webview/webviewdialog.h"

#include "../../model/syncthingicons.h"

#include "../../connector/utils.h"

#include "resources/config.h"

#include <qtutilities/misc/desktoputils.h>
#include <qtutilities/misc/dialogutils.h>
#include <qtutilities/resources/resources.h>
#include <qtutilities/settingsdialog/optioncategory.h>
#include <qtutilities/settingsdialog/settingsdialog.h>

#include <QDesktopServices>
#include <QQmlEngine>

#include <iostream>

using namespace std;
using namespace Data;
using namespace Plasma;
using namespace Dialogs;
using namespace QtGui;

SyncthingApplet::SyncthingApplet(QObject *parent, const QVariantList &data)
    : Applet(parent, data)
    , m_aboutDlg(nullptr)
    , m_connection()
    , m_dirModel(m_connection)
    , m_devModel(m_connection)
    , m_downloadModel(m_connection)
    , m_connectionSettingsDlg(nullptr)
#ifndef SYNCTHINGWIDGETS_NO_WEBVIEW
    , m_webViewDlg(nullptr)
#endif
{
    qRegisterMetaType<Data::SyncthingStatus>("Data::SyncthingStatus");
    qmlRegisterUncreatableMetaObject(Data::staticMetaObject, "martchus.syncthingplasmoid", 0, 6, "Data", QStringLiteral("only enums"));
}

SyncthingApplet::~SyncthingApplet()
{
    delete m_connectionSettingsDlg;
#ifndef SYNCTHINGWIDGETS_NO_WEBVIEW
    delete m_webViewDlg;
#endif
}

QIcon SyncthingApplet::statusIcon() const
{
    return m_statusInfo.statusIcon();
}

QString SyncthingApplet::incomingTraffic() const
{
    return trafficString(m_connection.totalIncomingTraffic(), m_connection.totalIncomingRate());
}

QString SyncthingApplet::outgoingTraffic() const
{
    return trafficString(m_connection.totalOutgoingTraffic(), m_connection.totalOutgoingRate());
}

QString SyncthingApplet::statusText() const
{
    return m_statusInfo.statusText();
}

QString SyncthingApplet::additionalStatusText() const
{
    return m_statusInfo.additionalStatusText();
}

void SyncthingApplet::init()
{
    LOAD_QT_TRANSLATIONS;

    Applet::init();

    connect(&m_connection, &SyncthingConnection::statusChanged, this, &SyncthingApplet::handleConnectionStatusChanged);
    connect(&m_connection, &SyncthingConnection::trafficChanged, this, &SyncthingApplet::trafficChanged);

    // restore settings
    Settings::restore();
    applyConnectionSettings();
}

void SyncthingApplet::showConnectionSettingsDlg()
{
    if (!m_connectionSettingsDlg) {
        m_connectionSettingsDlg = new Dialogs::SettingsDialog;
        m_connectionSettingsDlg->setTabBarAlwaysVisible(false);
        auto *const webViewPage = new QtGui::WebViewOptionPage;
        auto *const webViewWidget = webViewPage->widget();
        webViewWidget->setWindowTitle(tr("Web view"));
        webViewWidget->setWindowIcon(QIcon::fromTheme(QStringLiteral("internet-web-browser")));
        auto *const category = new Dialogs::OptionCategory(m_connectionSettingsDlg);
        category->assignPages({ new QtGui::ConnectionOptionPage(&m_connection), webViewPage });
        m_connectionSettingsDlg->setSingleCategory(category);
        m_connectionSettingsDlg->resize(860, 620);
        connect(m_connectionSettingsDlg, &Dialogs::SettingsDialog::applied, this, &SyncthingApplet::applyConnectionSettings);
        connect(m_connectionSettingsDlg, &Dialogs::SettingsDialog::applied, &Settings::save);
    }
    Dialogs::centerWidget(m_connectionSettingsDlg);
    m_connectionSettingsDlg->show();
    m_connectionSettingsDlg->activateWindow();
}

void SyncthingApplet::showWebUI()
{
#ifndef SYNCTHINGWIDGETS_NO_WEBVIEW
    if (Settings::values().webView.disabled) {
#endif
        QDesktopServices::openUrl(m_connection.syncthingUrl());
#ifndef SYNCTHINGWIDGETS_NO_WEBVIEW
    } else {
        if (!m_webViewDlg) {
            m_webViewDlg = new WebViewDialog;
            //if(m_selectedConnection) {
            m_webViewDlg->applySettings(Settings::values().connection.primary);
            //}
            connect(m_webViewDlg, &WebViewDialog::destroyed, this, &SyncthingApplet::handleWebViewDeleted);
        }
        m_webViewDlg->show();
        m_webViewDlg->activateWindow();
    }
#endif
}

void SyncthingApplet::showLog()
{
    auto *const dlg = TextViewDialog::forLogEntries(m_connection);
    dlg->setAttribute(Qt::WA_DeleteOnClose, true);
    centerWidget(dlg);
    dlg->show();
}

void SyncthingApplet::showOwnDeviceId()
{
    auto *const dlg = ownDeviceIdDialog(m_connection);
    dlg->setAttribute(Qt::WA_DeleteOnClose, true);
    centerWidget(dlg);
    dlg->show();
}

void SyncthingApplet::showAboutDialog()
{
    if (!m_aboutDlg) {
        m_aboutDlg = new AboutDialog(nullptr, QString(), QStringLiteral(APP_AUTHOR "\nSyncthing icons from Syncthing project"), QString(), QString(),
            QStringLiteral(APP_DESCRIPTION), QImage(QStringLiteral(":/icons/hicolor/scalable/app/syncthingtray.svg")));
        m_aboutDlg->setWindowTitle(tr("About") + QStringLiteral(" - " APP_NAME));
        m_aboutDlg->setWindowIcon(QIcon(QStringLiteral(":/icons/hicolor/scalable/app/syncthingtray.svg")));
        m_aboutDlg->setAttribute(Qt::WA_DeleteOnClose);
        connect(m_aboutDlg, &QObject::destroyed, this, &SyncthingApplet::handleAboutDialogDeleted);
    }
    centerWidget(m_aboutDlg);
    m_aboutDlg->show();
    m_aboutDlg->activateWindow();
}

void SyncthingApplet::applyConnectionSettings()
{
    m_connection.connect(Settings::values().connection.primary);
    if (m_webViewDlg) {
        m_webViewDlg->applySettings(Settings::values().connection.primary);
    }
}

void SyncthingApplet::handleConnectionStatusChanged()
{
    m_statusInfo.update(m_connection);
    emit connectionStatusChanged();
}

void SyncthingApplet::handleAboutDialogDeleted()
{
    m_aboutDlg = nullptr;
}

void SyncthingApplet::handleWebViewDeleted()
{
    m_webViewDlg = nullptr;
}

K_EXPORT_PLASMA_APPLET_WITH_JSON(syncthing, SyncthingApplet, "metadata.json")

#include "syncthingapplet.moc"
