#include "./settingsdialog.h"

#include "../application/settings.h"
#include "../data/syncthingconnection.h"
#include "../data/syncthingconfig.h"

#include "ui_connectionoptionpage.h"
#include "ui_notificationsoptionpage.h"
#include "ui_appearanceoptionpage.h"
#include "ui_autostartoptionpage.h"
#include "ui_webviewoptionpage.h"

#include "resources/config.h"

#include <tagparser/mediafileinfo.h>
#include <tagparser/backuphelper.h>

#include <qtutilities/settingsdialog/optioncategory.h>
#include <qtutilities/settingsdialog/optioncategorymodel.h>
#include <qtutilities/settingsdialog/qtsettings.h>

#include <QFileDialog>
#include <QMessageBox>
#include <QHostAddress>

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
    QObject::connect(ui()->insertFromConfigFilePushButton, &QPushButton::clicked, bind(&ConnectionOptionPage::insertFromConfigFile, this));
    return w;
}

void ConnectionOptionPage::insertFromConfigFile()
{
    if(hasBeenShown()) {
        QString configFile = SyncthingConfig::locateConfigFile();
        if(configFile.isEmpty()) {
            // allow user to select config file manually if it could not be located
            configFile = QFileDialog::getOpenFileName(widget(), QCoreApplication::translate("QtGui::ConnectionOptionPage", "Select Syncthing config file") + QStringLiteral(" - " APP_NAME));
        }
        if(configFile.isEmpty()) {
            return;
        }
        SyncthingConfig config;
        if(!config.restore(configFile)) {
            QMessageBox::critical(widget(), widget()->windowTitle() + QStringLiteral(" - " APP_NAME), QCoreApplication::translate("QtGui::ConnectionOptionPage", "Unable to parse the Syncthing config file."));
            return;
        }
        if(!config.guiAddress.isEmpty()) {
            ui()->urlLineEdit->selectAll();
            ui()->urlLineEdit->insert(((config.guiEnforcesSecureConnection || !QHostAddress(config.guiAddress.mid(0, config.guiAddress.indexOf(QChar(':')))).isLoopback()) ? QStringLiteral("https://") : QStringLiteral("http://")) + config.guiAddress);
        }
        if(!config.guiUser.isEmpty() || !config.guiPasswordHash.isEmpty()) {
            ui()->authCheckBox->setChecked(true);
            ui()->userNameLineEdit->selectAll();
            ui()->userNameLineEdit->insert(config.guiUser);
        } else {
            ui()->authCheckBox->setChecked(false);
        }
        if(!config.guiApiKey.isEmpty()) {
            ui()->apiKeyLineEdit->selectAll();
            ui()->apiKeyLineEdit->insert(config.guiApiKey);
        }
    }
}

void ConnectionOptionPage::updateConnectionStatus()
{
    if(hasBeenShown()) {
        ui()->statusLabel->setText(m_connection->statusText());
    }
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
        notifyOnInternalErrors() = ui()->notifyOnErrorsCheckBox->isChecked();
        notifyOnSyncComplete() = ui()->notifyOnSyncCompleteCheckBox->isChecked();
        showSyncthingNotifications() = ui()->showSyncthingNotificationsCheckBox->isChecked();
    }
    return true;
}

void NotificationsOptionPage::reset()
{
    if(hasBeenShown()) {
        ui()->notifyOnDisconnectCheckBox->setChecked(notifyOnDisconnect());
        ui()->notifyOnErrorsCheckBox->setChecked(notifyOnInternalErrors());
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
        trayMenuSize().setWidth(ui()->widthSpinBox->value());
        trayMenuSize().setHeight(ui()->heightSpinBox->value());
        showTraffic() = ui()->showTrafficCheckBox->isChecked();
    }
    return true;
}

void AppearanceOptionPage::reset()
{
    if(hasBeenShown()) {
        ui()->widthSpinBox->setValue(trayMenuSize().width());
        ui()->heightSpinBox->setValue(trayMenuSize().height());
        ui()->showTrafficCheckBox->setChecked(showTraffic());
    }
}

// LauncherOptionPage
AutostartOptionPage::AutostartOptionPage(QWidget *parentWidget) :
    AutostartOptionPageBase(parentWidget)
{}

AutostartOptionPage::~AutostartOptionPage()
{}

bool AutostartOptionPage::apply()
{
    if(hasBeenShown()) {
    }
    return true;
}

void AutostartOptionPage::reset()
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
                          << new AppearanceOptionPage << new AutostartOptionPage);
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
