#ifndef SYNCTHINGWIDGETS_QUICK_HELPERS_H
#define SYNCTHINGWIDGETS_QUICK_HELPERS_H

#include <syncthingconnector/syncthingconnection.h>

namespace QtGui {

template <typename App, typename AppService> void connectAppAndService(App &quickApp, AppService &appService)
{
    QObject::connect(&quickApp, &App::syncthingTerminationRequested, &appService, &AppService::terminateSyncthing);
    QObject::connect(&quickApp, &App::syncthingRestartRequested, &appService, &AppService::restartSyncthing);
    QObject::connect(&quickApp, &App::syncthingShutdownRequested, &appService, &AppService::shutdownSyncthing);
    QObject::connect(&quickApp, &App::syncthingConnectRequested, appService.connection(),
        static_cast<void (Data::SyncthingConnection::*)()>(&Data::SyncthingConnection::connect));
    QObject::connect(&quickApp, &App::syncthingReconnectRequested, appService.connection(),
        static_cast<void (Data::SyncthingConnection::*)()>(&Data::SyncthingConnection::reconnect));
    QObject::connect(&quickApp, &App::settingsReloadRequested, &appService, &AppService::reloadSettings);
    QObject::connect(&quickApp, &App::launcherStatusRequested, &appService, &AppService::broadcastLauncherStatus);
    QObject::connect(&quickApp, &App::clearLogRequested, &appService, &AppService::clearLog);
    QObject::connect(&quickApp, &App::replayLogRequested, &appService, &AppService::replayLog);
    QObject::connect(&appService, &AppService::launcherStatusChanged, &quickApp, &App::handleLauncherStatusBroadcast);
    QObject::connect(&appService, &AppService::logsAvailable, &quickApp, &App::logsAvailable);
    QObject::connect(&appService, &AppService::error, &quickApp, &App::error);
}

} // namespace QtGui

#endif // SYNCTHINGWIDGETS_QUICK_HELPERS_H
