#include "./settingsdialog.h"

#include "../application/settings.h"
#include "../data/syncthingconnection.h"

#include "ui_connectionoptionpage.h"
#include "ui_notificationsoptionpage.h"
#include "ui_appearanceoptionpage.h"
#include "ui_launcheroptionpage.h"
#include "ui_webviewoptionpage.h"

#include <tagparser/mediafileinfo.h>
#include <tagparser/backuphelper.h>

#include <qtutilities/settingsdialog/optioncategory.h>
#include <qtutilities/settingsdialog/optioncategorymodel.h>
#include <qtutilities/settingsdialog/qtsettings.h>

#include <functional>

using namespace std;
using namespace Settings;
using namespace Dialogs;
using namespace Data;

namespace QtGui {

// ConnectionOptionPage
ConnectionOptionPage::ConnectionOptionPage(Data::SyncthingConnection *connection, QWidget *parentWidget) :
    ConnectionOptionPageBase(parentWidget),
    m_connection(connection)
{}

ConnectionOptionPage::~ConnectionOptionPage()
{}

QWidget *ConnectionOptionPage::setupWidget()
{
    auto *w = ConnectionOptionPageBase::setupWidget();
    updateConnectionStatus();
    QObject::connect(m_connection, &SyncthingConnection::statusChanged, bind(&ConnectionOptionPage::updateConnectionStatus, this));
    QObject::connect(ui()->connectPushButton, &QPushButton::clicked, bind(&ConnectionOptionPage::applyAndReconnect, this));
    return w;
}

void ConnectionOptionPage::updateConnectionStatus()
{
    ui()->statusLabel->setText(m_connection->statusText());
}

bool ConnectionOptionPage::apply()
{
    if(hasBeenShown()) {
        syncthingUrl() = ui()->urlLineEdit->text();
        authEnabled() = ui()->authCheckBox->isChecked();
        userName() = ui()->userNameLineEdit->text();
        password() = ui()->passwordLineEdit->text();
        apiKey() = ui()->apiKeyLineEdit->text().toUtf8();

    }
    return true;
}

void ConnectionOptionPage::reset()
{
    if(hasBeenShown()) {
        ui()->urlLineEdit->setText(syncthingUrl());
        ui()->authCheckBox->setChecked(authEnabled());
        ui()->userNameLineEdit->setText(userName());
        ui()->passwordLineEdit->setText(password());
        ui()->apiKeyLineEdit->setText(apiKey());
    }
}

void ConnectionOptionPage::applyAndReconnect()
{
    apply();
    m_connection->setSyncthingUrl(Settings::syncthingUrl());
    m_connection->setApiKey(Settings::apiKey());
    if(Settings::authEnabled()) {
        m_connection->setCredentials(Settings::userName(), Settings::password());
    } else {
        m_connection->setCredentials(QString(), QString());
    }
    m_connection->reconnect();
}

// NotificationsOptionPage
NotificationsOptionPage::NotificationsOptionPage(QWidget *parentWidget) :
    NotificationsOptionPageBase(parentWidget)
{}

NotificationsOptionPage::~NotificationsOptionPage()
{}

bool NotificationsOptionPage::apply()
{
    if(hasBeenShown()) {
        notifyOnDisconnect() = ui()->notifyOnDisconnectCheckBox->isChecked();
        notifyOnErrors() = ui()->notifyOnErrorsCheckBox->isChecked();
        notifyOnSyncComplete() = ui()->notifyOnSyncCompleteCheckBox->isChecked();
        showSyncthingNotifications() = ui()->showSyncthingNotificationsCheckBox->isChecked();
    }
    return true;
}

void NotificationsOptionPage::reset()
{
    if(hasBeenShown()) {
        ui()->notifyOnDisconnectCheckBox->setChecked(notifyOnDisconnect());
        ui()->notifyOnErrorsCheckBox->setChecked(notifyOnErrors());
        ui()->notifyOnSyncCompleteCheckBox->setChecked(notifyOnSyncComplete());
        ui()->showSyncthingNotificationsCheckBox->setChecked(showSyncthingNotifications());
    }
}

// AppearanceOptionPage
AppearanceOptionPage::AppearanceOptionPage(QWidget *parentWidget) :
    AppearanceOptionPageBase(parentWidget)
{}

AppearanceOptionPage::~AppearanceOptionPage()
{}

bool AppearanceOptionPage::apply()
{
    if(hasBeenShown()) {
        showTraffic() = ui()->showTrafficCheckBox->isChecked();
    }
    return true;
}

void AppearanceOptionPage::reset()
{
    if(hasBeenShown()) {
        ui()->showTrafficCheckBox->setChecked(showTraffic());
    }
}

// LauncherOptionPage
LauncherOptionPage::LauncherOptionPage(QWidget *parentWidget) :
    LauncherOptionPageBase(parentWidget)
{}

LauncherOptionPage::~LauncherOptionPage()
{}

bool LauncherOptionPage::apply()
{
    if(hasBeenShown()) {
    }
    return true;
}

void LauncherOptionPage::reset()
{
    if(hasBeenShown()) {
    }
}

// WebViewOptionPage
WebViewOptionPage::WebViewOptionPage(QWidget *parentWidget) :
    WebViewOptionPageBase(parentWidget)
{}

WebViewOptionPage::~WebViewOptionPage()
{}

#if !defined(SYNCTHINGTRAY_USE_WEBENGINE) && !defined(SYNCTHINGTRAY_USE_WEBKIT)
QWidget *WebViewOptionPage::setupWidget()
{
    auto *label = new QLabel;
    label->setWindowTitle(QCoreApplication::translate("QtGui::WebViewOptionPage", "General"));
    label->setAlignment(Qt::AlignCenter);
    label->setText(QCoreApplication::translate("QtGui::WebViewOptionPage", "Syncthing Tray has not been built with vieb view support utilizing either Qt WebKit or Qt WebEngine.\nThe Web UI will be opened in the default web browser instead."));
    return label;
}
#endif

bool WebViewOptionPage::apply()
{
#if defined(SYNCTHINGTRAY_USE_WEBENGINE) || defined(SYNCTHINGTRAY_USE_WEBKIT)
    if(hasBeenShown()) {
        webViewDisabled() = ui()->disableCheckBox->isChecked();
        webViewZoomFactor() = ui()->zoomDoubleSpinBox->value();
        webViewKeepRunning() = ui()->keepRunningCheckBox->isChecked();
    }
#endif
    return true;
}

void WebViewOptionPage::reset()
{
#if defined(SYNCTHINGTRAY_USE_WEBENGINE) || defined(SYNCTHINGTRAY_USE_WEBKIT)
    if(hasBeenShown()) {
        ui()->disableCheckBox->setChecked(webViewDisabled());
        ui()->zoomDoubleSpinBox->setValue(webViewZoomFactor());
        ui()->keepRunningCheckBox->setChecked(webViewKeepRunning());
    }
#endif
}

SettingsDialog::SettingsDialog(Data::SyncthingConnection *connection, QWidget *parent) :
    Dialogs::SettingsDialog(parent)
{
    // setup categories
    QList<Dialogs::OptionCategory *> categories;
    Dialogs::OptionCategory *category;

    category = new OptionCategory(this);
    category->setDisplayName(tr("Tray"));
    category->assignPages(QList<Dialogs::OptionPage *>()
                          << new ConnectionOptionPage(connection) << new NotificationsOptionPage
                          << new AppearanceOptionPage << new LauncherOptionPage);
    category->setIcon(QIcon(QStringLiteral(":/icons/hicolor/scalable/app/syncthingtray.svg")));
    categories << category;

    category = new OptionCategory(this);
    category->setDisplayName(tr("Web view"));
    category->assignPages(QList<Dialogs::OptionPage *>() << new WebViewOptionPage);
    category->setIcon(QIcon::fromTheme(QStringLiteral("internet-web-browser"), QIcon(QStringLiteral(":/icons/hicolor/scalable/apps/internet-web-browser.svg"))));
    categories << category;

    categories << Settings::qtSettings().category();

    categoryModel()->setCategories(categories);

    setMinimumSize(800, 450);
    setWindowIcon(QIcon::fromTheme(QStringLiteral("preferences-other"), QIcon(QStringLiteral(":/icons/hicolor/scalable/apps/preferences-other.svg"))));

    // some settings could be applied without restarting the application, good idea?
    //connect(this, &Dialogs::SettingsDialog::applied, bind(&Dialogs::QtSettings::apply, &Settings::qtSettings()));
}

SettingsDialog::~SettingsDialog()
{}

}
