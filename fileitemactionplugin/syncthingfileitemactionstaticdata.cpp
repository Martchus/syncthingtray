#include "./syncthingfileitemactionstaticdata.h"

#include "../model/syncthingicons.h"

#include "../connector/syncthingconfig.h"
#include "../connector/syncthingconnection.h"
#include "../connector/syncthingconnectionsettings.h"

#include <c++utilities/application/argumentparser.h>

#include <qtutilities/aboutdialog/aboutdialog.h>
#include <qtutilities/resources/resources.h>

#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>

#include <iostream>

#include "resources/config.h"

using namespace std;
using namespace Dialogs;
using namespace Data;

SyncthingFileItemActionStaticData::SyncthingFileItemActionStaticData()
    : m_initialized(false)
{
}

void SyncthingFileItemActionStaticData::initialize()
{
    if (m_initialized) {
        return;
    }

    LOAD_QT_TRANSLATIONS;

    // load settings
    const QSettings settingsFile(QSettings::IniFormat, QSettings::UserScope, QStringLiteral(PROJECT_NAME));

    // determine path of Syncthing config file
    m_configFilePath = [&] {
        const QByteArray configPathFromEnv(qgetenv("KIO_SYNCTHING_CONFIG_PATH"));
        if (!configPathFromEnv.isEmpty()) {
            return QString::fromLocal8Bit(configPathFromEnv);
        }
        const QString configPathFromSettings = settingsFile.value(QStringLiteral("syncthingConfigPath")).toString();
        if (!configPathFromSettings.isEmpty()) {
            return configPathFromSettings;
        }
        return SyncthingConfig::locateConfigFile();
    }();
    applySyncthingConfiguration(m_configFilePath);

    // prevent unnecessary API calls (for the purpose of the context menu)
    m_connection.disablePolling();

    // connect Signals & Slots for logging
    connect(&m_connection, &SyncthingConnection::error, this, &SyncthingFileItemActionStaticData::logConnectionError);
    if (qEnvironmentVariableIsSet("KIO_SYNCTHING_LOG_STATUS")) {
        connect(&m_connection, &SyncthingConnection::statusChanged, this, &SyncthingFileItemActionStaticData::logConnectionStatus);
    }

    m_initialized = true;
}

void SyncthingFileItemActionStaticData::logConnectionStatus()
{
    cerr << "Syncthing connection status changed to: " << m_connection.statusText().toLocal8Bit().data() << endl;
}

void SyncthingFileItemActionStaticData::logConnectionError(const QString &errorMessage, SyncthingErrorCategory errorCategory)
{
    switch (errorCategory) {
    case SyncthingErrorCategory::Parsing:
    case SyncthingErrorCategory::SpecificRequest:
        QMessageBox::critical(nullptr, tr("Syncthing connection error"), errorMessage);
        break;
    default:
        cerr << "Syncthing connection error: " << errorMessage.toLocal8Bit().data() << endl;
    }
}

void SyncthingFileItemActionStaticData::rescanDir(const QString &dirId, const QString &relpath)
{
    m_connection.rescan(dirId, relpath);
}

void SyncthingFileItemActionStaticData::showAboutDialog()
{
    auto *aboutDialog = new AboutDialog(nullptr, QStringLiteral(APP_NAME), QStringLiteral(APP_AUTHOR "\nSyncthing icons from Syncthing project"),
        QStringLiteral(APP_VERSION), ApplicationUtilities::dependencyVersions2, QStringLiteral(APP_URL), QStringLiteral(APP_DESCRIPTION),
        QImage(statusIcons().scanninig.pixmap(128).toImage()));
    aboutDialog->setWindowTitle(tr("About") + QStringLiteral(" - " APP_NAME));
    aboutDialog->setWindowIcon(QIcon::fromTheme(QStringLiteral("syncthingtray")));
    aboutDialog->setAttribute(Qt::WA_DeleteOnClose);
    aboutDialog->show();
}

void SyncthingFileItemActionStaticData::selectSyncthingConfig()
{
    const auto configFilePath = QFileDialog::getOpenFileName(nullptr, tr("Select Syncthing config file") + QStringLiteral(" - " APP_NAME));
    if (!configFilePath.isEmpty() && applySyncthingConfiguration(configFilePath)) {
        QSettings(QSettings::IniFormat, QSettings::UserScope, QStringLiteral(PROJECT_NAME))
            .setValue(QStringLiteral("syncthingConfigPath"), m_configFilePath = configFilePath);
    }
}

bool SyncthingFileItemActionStaticData::applySyncthingConfiguration(const QString &syncthingConfigFilePath)
{
    clearCurrentError();

    // check for empty path
    if (syncthingConfigFilePath.isEmpty()) {
        setCurrentError(tr("Syncthing config file can not be automatically located"));
        return false;
    }

    // load Syncthing config
    SyncthingConfig config;
    if (!config.restore(syncthingConfigFilePath)) {
        auto errorMessage = tr("Unable to load Syncthing config from \"%1\"").arg(syncthingConfigFilePath);
        if (!m_configFilePath.isEmpty() && m_configFilePath != syncthingConfigFilePath) {
            errorMessage += QChar('\n');
            errorMessage += tr("(still using config from \"%1\")").arg(m_configFilePath);
        }
        setCurrentError(errorMessage);
        return false;
    }
    cerr << "Syncthing config loaded from \"" << syncthingConfigFilePath.toLocal8Bit().data() << "\"" << endl;

    // make connection settings
    SyncthingConnectionSettings settings;
    settings.syncthingUrl = config.syncthingUrl();
    settings.apiKey.append(config.guiApiKey);

    // establish connection
    bool ok;
    int reconnectInterval = qEnvironmentVariableIntValue("KIO_SYNCTHING_RECONNECT_INTERVAL", &ok);
    if (!ok || reconnectInterval < 0) {
        reconnectInterval = 10000;
    }
    m_connection.setAutoReconnectInterval(reconnectInterval);
    m_connection.reconnect(settings);
    return true;
}

void SyncthingFileItemActionStaticData::setCurrentError(const QString &currentError)
{
    if (m_currentError == currentError) {
        return;
    }
    const bool hadError = hasError();
    m_currentError = currentError;
    if (hadError != hasError()) {
        emit hasErrorChanged(hasError());
    }
    emit currentErrorChanged(m_currentError);
}
