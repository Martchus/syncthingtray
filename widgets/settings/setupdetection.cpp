#include "./setupdetection.h"
#include "./settingsdialog.h"

// use meta-data of syncthingtray application here
#include "resources/../../tray/resources/config.h"

#if defined(LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD) && (defined(PLATFORM_UNIX) || defined(PLATFORM_MINGW) || defined(PLATFORM_CYGWIN))
#define PLATFORM_HAS_GETLOGIN
#include <unistd.h>
#endif

namespace QtGui {

SetupDetection::SetupDetection(QObject *parent)
    : QObject(parent)
{
    // assume default service names
    const auto defaultUserUnit =
#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
        qEnvironmentVariable(PROJECT_VARNAME_UPPER "_SYSTEMD_USER_UNIT",
#endif
            QStringLiteral("syncthing.service")
#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
        )
#endif
        ;
#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
    userService.setUnitName(defaultUserUnit);
    systemService.setUnitName(QStringLiteral("syncthing@") %
#ifdef PLATFORM_HAS_GETLOGIN
        QString::fromLocal8Bit(getlogin()) %
#endif
        QStringLiteral(".service"));
#endif

    // configure launcher to test invocation of "syncthing --version" capturing output
    defaultSyncthingArgs = launcherSettings.syncthingArgs;
    launcherSettings.syncthingArgs = QStringLiteral("--version");
    launcher.setEmittingOutput(true);

    // configure timeout
    timeout.setInterval(2500);
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
    certPath = Data::SyncthingConfig::locateHttpsCertificate();
}

void SetupDetection::restoreConfig()
{
    configOk = config.restore(configFilePath);
}

void SetupDetection::initConnection()
{
    connection.setSyncthingUrl(config.syncthingUrl());
    connection.setApiKey(config.guiApiKey.toLocal8Bit());
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
    config.guiAddress.clear();
    config.guiApiKey.clear();
    connection.disconnect();
    launcher.terminate();
    connectionErrors.clear();
    launcherExitCode.reset();
    launcherExitStatus.reset();
    launcherError.reset();
    launcherOutput.clear();
}

void SetupDetection::startTest()
{
    restoreConfig();
    initConnection();
    connection.reconnect();
    launcher.launch(launcherSettings);
    autostartEnabled = isAutostartEnabled();
    timeout.start();
}

void SetupDetection::handleConnectionError(const QString &error)
{
    connectionErrors << error;
}

void SetupDetection::handleLauncherExit(int exitCode, QProcess::ExitStatus exitStatus)
{
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
    if (isDone()) {
        timeout.stop();
        emit done();
    }
}

} // namespace QtGui
