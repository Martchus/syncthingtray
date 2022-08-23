#ifndef SETTINGS_SETUP_DETECTION_H
#define SETTINGS_SETUP_DETECTION_H

#include "./settings.h"

#include "../misc/syncthinglauncher.h"

#include <syncthingconnector/syncthingconfig.h>
#include <syncthingconnector/syncthingconnection.h>
#include <syncthingconnector/syncthingconnectionsettings.h>
#include <syncthingconnector/syncthingservice.h>

#include <QByteArray>
#include <QProcess>
#include <QStringBuilder>
#include <QTimer>

#include <memory>
#include <optional>

namespace QtGui {

class SetupDetection : public QObject {
    Q_OBJECT

public:
    explicit SetupDetection(QObject *parent = nullptr);
    bool hasConfig() const;
    bool isDone() const;

public Q_SLOTS:
    void determinePaths();
    void restoreConfig();
    void initConnection();
    void reset();
    void startTest();

Q_SIGNALS:
    void done();

private Q_SLOTS:
    void handleConnectionError(const QString &error);
    void handleLauncherExit(int exitCode, QProcess::ExitStatus exitStatus);
    void handleLauncherError(QProcess::ProcessError error);
    void handleLauncherOutput(const QByteArray &output);
    void handleTimeout();
    void checkDone();

public:
    QString configFilePath;
    QString certPath;
    QStringList connectionErrors;
    Data::SyncthingConfig config;
    Data::SyncthingConnection connection;
#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
    Data::SyncthingService userService = Data::SyncthingService(Data::SystemdScope::User);
    Data::SyncthingService systemService = Data::SyncthingService(Data::SystemdScope::System);
#endif
    Settings::Launcher launcherSettings;
    Data::SyncthingLauncher launcher;
    std::optional<int> launcherExitCode;
    std::optional<QProcess::ExitStatus> launcherExitStatus;
    std::optional<QProcess::ProcessError> launcherError;
    QByteArray launcherOutput;
    QTimer timeout;
    bool timedOut = false;
    bool configOk = false;
};

} // namespace QtGui

#endif // SETTINGS_SETUP_DETECTION_H
