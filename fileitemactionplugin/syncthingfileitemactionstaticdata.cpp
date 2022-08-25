#include "./syncthingfileitemactionstaticdata.h"

#include <syncthingmodel/syncthingicons.h>

#include <syncthingconnector/syncthingconfig.h>
#include <syncthingconnector/syncthingconnection.h>
#include <syncthingconnector/syncthingconnectionsettings.h>

#include <c++utilities/application/argumentparser.h>
#include <c++utilities/io/ansiescapecodes.h>

#include <qtutilities/aboutdialog/aboutdialog.h>
#include <qtutilities/misc/desktoputils.h>
#include <qtutilities/resources/resources.h>

#include <QAction>
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QSettings>

#include <iostream>

#include "resources/config.h"
#include "resources/qtconfig.h"

using namespace std;
using namespace CppUtilities::EscapeCodes;
using namespace QtUtilities;
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
    applySyncthingConfiguration(m_configFilePath, settingsFile.value(QStringLiteral("syncthingApiKey")).toString(), true);

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
    int row;
    const auto *const dirInfo = m_connection.findDirInfo(dirId, row);
    if (dirInfo && !dirInfo->paused) {
        m_connection.rescan(dirId, relpath);
    }
}

void SyncthingFileItemActionStaticData::showAboutDialog()
{
    auto *const aboutDialog = new AboutDialog(nullptr, QStringLiteral(APP_NAME), aboutDialogAttribution(), QStringLiteral(APP_VERSION),
        CppUtilities::applicationInfo.dependencyVersions, QStringLiteral(APP_URL), QStringLiteral(APP_DESCRIPTION), aboutDialogImage());
    aboutDialog->setWindowTitle(tr("About") + QStringLiteral(" - " APP_NAME));
    aboutDialog->setWindowIcon(QIcon::fromTheme(QStringLiteral("syncthingtray")));
    aboutDialog->setAttribute(Qt::WA_DeleteOnClose);
    aboutDialog->show();
}

void SyncthingFileItemActionStaticData::selectSyncthingConfig()
{
    const auto configFilePath = QFileDialog::getOpenFileName(nullptr, tr("Select Syncthing config file") + QStringLiteral(" - " APP_NAME));
    if (!configFilePath.isEmpty()) {
        applySyncthingConfiguration(configFilePath, QString(), false);
    }
}

void SyncthingFileItemActionStaticData::handlePaletteChanged(const QPalette &palette)
{
    applyBrightCustomColorsSetting(isPaletteDark(palette));
}

void SyncthingFileItemActionStaticData::appendNoteToError(QString &errorMessage, const QString &newSyncthingConfigFilePath) const
{
    if (!m_configFilePath.isEmpty() && m_configFilePath != newSyncthingConfigFilePath) {
        errorMessage += QChar('\n');
        errorMessage += tr("(still using config from \"%1\")").arg(m_configFilePath);
    }
}

bool SyncthingFileItemActionStaticData::applySyncthingConfiguration(
    const QString &syncthingConfigFilePath, const QString &syncthingApiKey, bool skipSavingConfig)
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
        appendNoteToError(errorMessage, syncthingConfigFilePath);
        setCurrentError(errorMessage);
        return false;
    }
    cerr << Phrases::Info << "Syncthing config loaded from \"" << syncthingConfigFilePath.toLocal8Bit().data() << "\"" << Phrases::End;

    // check whether the URL is present
    if (config.guiAddress.isEmpty()) {
        auto errorMessage = tr("Syncthing config from \"%1\" does not contain GUI address.").arg(syncthingConfigFilePath);
        appendNoteToError(errorMessage, syncthingConfigFilePath);
        setCurrentError(errorMessage);
        return false;
    }

    // check whether the API key is present
    if (config.guiApiKey.isEmpty()) {
        config.guiApiKey = syncthingApiKey;
    }
    if (config.guiApiKey.isEmpty()) {
        config.guiApiKey = QInputDialog::getText(
            nullptr, tr("Enter API key"), tr("The selected config file does not contain an API key. Please enter the API key manually:"));
        if (config.guiApiKey.isEmpty()) {
            auto errorMessage = tr("No API key supplied for \"%1\".").arg(config.guiAddress);
            appendNoteToError(errorMessage, syncthingConfigFilePath);
            setCurrentError(errorMessage);
            return false;
        }
    }

    // make connection settings
    SyncthingConnectionSettings connectionSettings;
    connectionSettings.syncthingUrl = config.syncthingUrl();
    connectionSettings.apiKey.append(config.guiApiKey.toUtf8());

    // establish connection
    bool ok;
    int reconnectInterval = qEnvironmentVariableIntValue("KIO_SYNCTHING_RECONNECT_INTERVAL", &ok);
    if (!ok || reconnectInterval < 0) {
        reconnectInterval = 10000;
    }
    m_connection.setAutoReconnectInterval(reconnectInterval);
    m_connection.reconnect(connectionSettings);

    // save new config persistently
    if (!skipSavingConfig) {
        QSettings settings(QSettings::IniFormat, QSettings::UserScope, QStringLiteral(PROJECT_NAME));
        settings.setValue(QStringLiteral("syncthingConfigPath"), m_configFilePath = syncthingConfigFilePath);
        settings.setValue(QStringLiteral("syncthingApiKey"), config.guiApiKey);
    }

    return true;
}

void SyncthingFileItemActionStaticData::applyBrightCustomColorsSetting(bool useBrightCustomColors)
{
    if (useBrightCustomColors) {
        static const auto settings = StatusIconSettings(StatusIconSettings::DarkTheme());
        IconManager::instance().applySettings(&settings);
    } else {
        static const auto settings = StatusIconSettings(StatusIconSettings::BrightTheme());
        IconManager::instance().applySettings(&settings);
    }
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
