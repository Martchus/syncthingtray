#include "./setupdetection.h"
#include "./settingsdialog.h"

// use meta-data of syncthingtray application here
#include "resources/../../tray/resources/config.h"

#include <qtutilities/misc/compat.h>

#if defined(LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD) && (defined(PLATFORM_UNIX) || defined(PLATFORM_MINGW) || defined(PLATFORM_CYGWIN))
#define PLATFORM_HAS_GETLOGIN
#include <unistd.h>
#endif

namespace QtGui {

SetupDetection::SetupDetection(QObject *parent)
    : QObject(parent)
{
    // assume default service names
    const auto defaultUserUnit = qEnvironmentVariable(PROJECT_VARNAME_UPPER "_SYSTEMD_USER_UNIT", QStringLiteral("syncthing.service"));
#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
    userService.setUnitName(defaultUserUnit);
    systemService.setUnitName(QStringLiteral("syncthing@") %
#ifdef PLATFORM_HAS_GETLOGIN
        QString::fromLocal8Bit(getlogin()) %
#endif
        QStringLiteral(".service"));
#endif

    // recognize env variable SYNCTHING_PATH like the testsuite does
    if (const auto syncthingPathFromEnv = qEnvironmentVariable("SYNCTHING_PATH"); !syncthingPathFromEnv.isEmpty()) {
        launcherSettings.syncthingPath = syncthingPathFromEnv;
    }

    // configure launcher to check version of Syncthing binary
    defaultSyncthingArgs = launcherSettings.syncthingArgs;
    launcher.setEmittingOutput(true);

    // configure timeout
    auto hasConfiguredTimeout = false;
    auto configuredTimeout = qEnvironmentVariableIntValue(PROJECT_VARNAME_UPPER "_WIZARD_SETUP_DETECTION_TIMEOUT", &hasConfiguredTimeout);
    timeout.setInterval(hasConfiguredTimeout ? configuredTimeout : 20000);
    timeout.setSingleShot(true);

    // connect signals & slots
    connect(&connection, &Data::SyncthingConnection::error, this, &SetupDetection::handleConnectionError);
    connect(&connection, &Data::SyncthingConnection::statusChanged, this, &SetupDetection::checkDone);
#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
    connect(&userService, &Data::SyncthingService::unitFileStateChanged, this, &SetupDetection::checkDone);
    connect(&systemService, &Data::SyncthingService::unitFileStateChanged, this, &SetupDetection::checkDone);
#endif
    connect(&launcher, &Data::SyncthingLauncher::outputAvailable, this, &SetupDetection::handleLauncherOutput);
    connect(&launcher, &Data::SyncthingLauncher::exited, this, &SetupDetection::handleLauncherExit);
    connect(&launcher, &Data::SyncthingLauncher::errorOccurred, this, &SetupDetection::handleLauncherError);
    connect(&timeout, &QTimer::timeout, this, &SetupDetection::handleTimeout);
}

void SetupDetection::determinePaths()
{
    configFilePath = Data::SyncthingConfig::locateConfigFile();
#ifndef QT_NO_SSL
    certPath = Data::SyncthingConfig::locateHttpsCertificate();
#endif
}

void SetupDetection::restoreConfig()
{
    configOk = config.restore(configFilePath);
}

void SetupDetection::initConnection()
{
    auto settings = Data::SyncthingConnectionSettings();
    settings.syncthingUrl = config.syncthingUrl();
    settings.apiKey = config.guiApiKey.toLocal8Bit();
#ifndef QT_NO_SSL
    settings.httpsCertPath = certPath;
    settings.loadHttpsCert();
#endif
    connection.applySettings(settings);
}

bool SetupDetection::hasConfig() const
{
    return configOk && !config.guiAddress.isEmpty() && !config.guiApiKey.isEmpty();
}

bool SetupDetection::isDone() const
{
    return timedOut
        || (!connection.isConnecting() && (launcherExitCode.has_value() || launcherError.has_value())
#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
            && !userService.unitFileState().isEmpty() && !systemService.unitFileState().isEmpty()
#endif
        );
}

void SetupDetection::reset()
{
    timeout.stop();
    timedOut = false;
    configOk = false;
    autostartEnabled = false;
    autostartConfiguredPath.reset();
    config.guiAddress.clear();
    config.guiApiKey.clear();
    connection.disconnect();
    launcher.terminate();
    connectionErrors.clear();
    launcherExitCode.reset();
    launcherExitStatus.reset();
    launcherError.reset();
    launcherOutput.clear();
    m_testStarted = false;
}

void SetupDetection::startTest()
{
    if (m_testStarted) {
        return;
    }
    m_testStarted = true;
    restoreConfig();
    initConnection();
    connection.reconnect();
    launcherSettings.syncthingArgs = QStringLiteral("version"); // test invocation of "syncthing version" and "syncthing --version"
    additionalArgsToProbe = QStringList({ QStringLiteral("--version") });
    launcher.launch(launcherSettings);
    autostartConfiguredPath = configuredAutostartPath();
    autostartEnabled = autostartConfiguredPath.has_value() ? !autostartConfiguredPath.value().isEmpty() : isAutostartEnabled();
    autostartSupposedPath = supposedAutostartPath();
    timeout.start();
}

void SetupDetection::handleConnectionError(const QString &error)
{
    connectionErrors << error;
}

void SetupDetection::handleLauncherExit(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (exitCode != 0 && !additionalArgsToProbe.isEmpty()) {
        launcherSettings.syncthingArgs = additionalArgsToProbe.takeLast();
        launcher.launch(launcherSettings);
        return;
    }
    launcherExitCode = exitCode;
    launcherExitStatus = exitStatus;
    checkDone();
}

void SetupDetection::handleLauncherError(QProcess::ProcessError error)
{
    launcherError = error;
    checkDone();
}

void SetupDetection::handleLauncherOutput(const QByteArray &output)
{
    launcherOutput.append(output);
}

void SetupDetection::handleTimeout()
{
    timedOut = true;
    checkDone();
}

void SetupDetection::checkDone()
{
    if (!m_testStarted) {
        return;
    }
    if (isDone()) {
        timeout.stop();
        m_testStarted = false;
        emit done();
    }
}

} // namespace QtGui
