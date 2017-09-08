#include "./syncthingapplet.h"

#include "../../connector/syncthingservice.h"
#include "../../connector/utils.h"

#include "../../widgets/misc/errorviewdialog.h"
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
#include <QNetworkReply>
#include <QQmlEngine>
#include <QStringBuilder>

#include <iostream>

using namespace std;
using namespace Data;
using namespace Plasma;
using namespace Dialogs;
using namespace QtGui;
using namespace ChronoUtilities;

SyncthingApplet::SyncthingApplet(QObject *parent, const QVariantList &data)
    : Applet(parent, data)
    , m_aboutDlg(nullptr)
    , m_connection()
    , m_dirModel(m_connection)
    , m_devModel(m_connection)
    , m_downloadModel(m_connection)
    , m_settingsDlg(nullptr)
#ifndef SYNCTHINGWIDGETS_NO_WEBVIEW
    , m_webViewDlg(nullptr)
#endif
    , m_currentConnectionConfig(-1)
{
    qmlRegisterUncreatableMetaObject(Data::staticMetaObject, "martchus.syncthingplasmoid", 0, 6, "Data", QStringLiteral("only enums"));
}

SyncthingApplet::~SyncthingApplet()
{
    delete m_settingsDlg;
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

QStringList SyncthingApplet::connectionConfigNames() const
{
    const auto &settings = Settings::values().connection;
    QStringList names;
    names.reserve(static_cast<int>(settings.secondary.size() + 1));
    names << settings.primary.label;
    for (const auto &setting : settings.secondary) {
        names << setting.label;
    }
    return names;
}

QString SyncthingApplet::currentConnectionConfigName() const
{
    const auto &settings = Settings::values().connection;
    if (m_currentConnectionConfig == 0) {
        return settings.primary.label;
    } else if (m_currentConnectionConfig > 0 && static_cast<unsigned>(m_currentConnectionConfig) <= settings.secondary.size()) {
        return settings.secondary[static_cast<unsigned>(m_currentConnectionConfig) - 1].label;
    }
    return QString();
}

inline void SyncthingApplet::setCurrentConnectionConfigIndex(int index)
{
    auto &settings = Settings::values().connection;
    if (index != m_currentConnectionConfig && index >= 0 && static_cast<unsigned>(index) <= settings.secondary.size()) {
        auto &selectedConfig = index == 0 ? settings.primary : settings.secondary[static_cast<unsigned>(index) - 1];
        m_connection.connect(selectedConfig);
#ifndef SYNCTHINGWIDGETS_NO_WEBVIEW
        if (m_webViewDlg) {
            m_webViewDlg->applySettings(selectedConfig);
        }
#endif
        emit currentConnectionConfigIndexChanged(m_currentConnectionConfig = index);
        emit localChanged();
    }
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

    // connect signals and slots
    connect(&m_connection, &SyncthingConnection::statusChanged, this, &SyncthingApplet::handleConnectionStatusChanged);
    connect(&m_connection, &SyncthingConnection::error, this, &SyncthingApplet::handleInternalError);
    connect(&m_connection, &SyncthingConnection::trafficChanged, this, &SyncthingApplet::trafficChanged);
    connect(&m_connection, &SyncthingConnection::newNotification, this, &SyncthingApplet::handleNewNotification);
    connect(&m_dbusNotifier, &DBusStatusNotifier::connectRequested, &m_connection,
        static_cast<void (SyncthingConnection::*)(void)>(&SyncthingConnection::connect));
    connect(&m_dbusNotifier, &DBusStatusNotifier::dismissNotificationsRequested, this, &SyncthingApplet::dismissNotifications);
    connect(&m_dbusNotifier, &DBusStatusNotifier::showNotificationsRequested, this, &SyncthingApplet::showNotificationsDialog);
    connect(&m_dbusNotifier, &DBusStatusNotifier::errorDetailsRequested, this, &SyncthingApplet::showInternalErrorsDialog);

    // restore settings
    Settings::restore();
    applyConnectionSettings();

    // load primary connection config
    setCurrentConnectionConfigIndex(0);

// initialize systemd service support
#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
    SyncthingService &service = syncthingService();
    service.setUnitName(Settings::values().systemd.syncthingUnit);
    connect(&service, &SyncthingService::errorOccurred, this, &SyncthingApplet::handleSystemdServiceError);
#endif
}

void SyncthingApplet::showSettingsDlg()
{
    if (!m_settingsDlg) {
        m_settingsDlg = new Dialogs::SettingsDialog;
        m_settingsDlg->setTabBarAlwaysVisible(false);
        auto *const webViewPage = new QtGui::WebViewOptionPage;
        auto *const webViewWidget = webViewPage->widget();
        webViewWidget->setWindowTitle(tr("Web view"));
        webViewWidget->setWindowIcon(QIcon::fromTheme(QStringLiteral("internet-web-browser")));
        auto *const category = new Dialogs::OptionCategory(m_settingsDlg);
        category->assignPages({ new QtGui::ConnectionOptionPage(&m_connection), new QtGui::NotificationsOptionPage(true),
#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
            new QtGui::SystemdOptionPage,
#endif
            webViewPage });
        m_settingsDlg->setSingleCategory(category);
        m_settingsDlg->resize(860, 620);
        connect(m_settingsDlg, &Dialogs::SettingsDialog::applied, this, &SyncthingApplet::applyConnectionSettings);
        connect(m_settingsDlg, &Dialogs::SettingsDialog::applied, &Settings::save);
    }
    Dialogs::centerWidget(m_settingsDlg);
    m_settingsDlg->show();
    m_settingsDlg->activateWindow();
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
        m_aboutDlg = new AboutDialog(nullptr, QStringLiteral(APP_NAME), QStringLiteral(APP_AUTHOR "\nSyncthing icons from Syncthing project"),
            QStringLiteral(APP_VERSION), QStringLiteral(APP_URL), QStringLiteral(APP_DESCRIPTION),
            QImage(statusIcons().scanninig.pixmap(128).toImage()));
        m_aboutDlg->setWindowTitle(tr("About") + QStringLiteral(" - " APP_NAME));
        m_aboutDlg->setWindowIcon(QIcon::fromTheme(QStringLiteral("syncthingtray")));
        m_aboutDlg->setAttribute(Qt::WA_DeleteOnClose);
        connect(m_aboutDlg, &QObject::destroyed, this, &SyncthingApplet::handleAboutDialogDeleted);
    }
    centerWidget(m_aboutDlg);
    m_aboutDlg->show();
    m_aboutDlg->activateWindow();
}

void SyncthingApplet::showNotificationsDialog()
{
    auto *const dlg = TextViewDialog::forLogEntries(m_notifications, tr("New notifications"));
    dlg->setAttribute(Qt::WA_DeleteOnClose, true);
    centerWidget(dlg);
    dlg->show();
    m_notifications.clear();
    dismissNotifications();
}

void SyncthingApplet::dismissNotifications()
{
    m_connection.considerAllNotificationsRead();
}

void SyncthingApplet::showInternalErrorsDialog()
{
    auto *const errorViewDlg = ErrorViewDialog::instance();
    connect(errorViewDlg, &ErrorViewDialog::errorsCleared, this, &SyncthingApplet::handleErrorsCleared);
    centerWidget(errorViewDlg);
    errorViewDlg->show();
}

void SyncthingApplet::showDirectoryErrors(unsigned int directoryIndex) const
{
    const auto &dirs = m_connection.dirInfo();
    if (directoryIndex < dirs.size()) {
        auto *const dlg = TextViewDialog::forDirectoryErrors(dirs[directoryIndex]);
        dlg->setAttribute(Qt::WA_DeleteOnClose, true);
        centerWidget(dlg);
        dlg->show();
    }
}

void SyncthingApplet::applyConnectionSettings()
{
    const int currentConfig = m_currentConnectionConfig;
    m_currentConnectionConfig = -1; // force update
    setCurrentConnectionConfigIndex(currentConfig);
    emit connectionConfigNamesChanged();
}

void SyncthingApplet::handleConnectionStatusChanged()
{
    m_statusInfo.update(m_connection);
    emit connectionStatusChanged();
}

void SyncthingApplet::handleInternalError(
    const QString &errorMsg, SyncthingErrorCategory category, int networkError, const QNetworkRequest &request, const QByteArray &response)
{
    if (InternalError::isRelevant(m_connection, category, networkError)) {
        InternalError error(errorMsg, request.url(), response);
        m_dbusNotifier.showInternalError(error);
        ErrorViewDialog::addError(move(error));
    }
}

void SyncthingApplet::handleErrorsCleared()
{
}

void SyncthingApplet::handleAboutDialogDeleted()
{
    m_aboutDlg = nullptr;
}

void SyncthingApplet::handleWebViewDeleted()
{
    m_webViewDlg = nullptr;
}

void SyncthingApplet::handleNewNotification(DateTime when, const QString &msg)
{
    m_notifications.emplace_back(QString::fromLocal8Bit(when.toString(DateTimeOutputFormat::DateAndTime, true).data()), msg);
    if (Settings::values().notifyOn.syncthingErrors) {
        m_dbusNotifier.showSyncthingNotification(when, msg);
    }
}

void SyncthingApplet::handleSystemdServiceError(const QString &context, const QString &name, const QString &message)
{
    handleInternalError(tr("D-Bus error - unable to ") % context % QChar('\n') % name % QChar(':') % message, SyncthingErrorCategory::SpecificRequest,
        QNetworkReply::NoError, QNetworkRequest(), QByteArray());
}

K_EXPORT_PLASMA_APPLET_WITH_JSON(syncthing, SyncthingApplet, "metadata.json")

#include "syncthingapplet.moc"
