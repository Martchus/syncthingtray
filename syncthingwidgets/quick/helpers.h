#ifndef SYNCTHINGWIDGETS_QUICK_HELPERS_H
#define SYNCTHINGWIDGETS_QUICK_HELPERS_H

#include <syncthingconnector/syncthingconnection.h>

#include <QJSEngine>
#include <QQmlEngine>

#ifdef Q_OS_WINDOWS
#define SYNCTHING_APP_STRING_CONVERSION(s) (s).toStdWString()
#define SYNCTHING_APP_PATH_CONVERSION(s) QString::fromStdWString((s).wstring())
#else
#define SYNCTHING_APP_STRING_CONVERSION(s) (s).toLocal8Bit().toStdString()
#define SYNCTHING_APP_PATH_CONVERSION(s) QString::fromLocal8Bit((s).string())
#endif

namespace QtGui {

template <typename SyncthingClass> void dataObjectToProperty(QJSEngine *engine, SyncthingClass *dataObject)
{
    engine->setProperty(SyncthingClass::staticMetaObject.className(), QVariant::fromValue(dataObject));
}

template <typename SyncthingClass> SyncthingClass *dataObjectFromProperty(QQmlEngine *qmlEngine, QJSEngine *engine)
{
    Q_UNUSED(qmlEngine)
    auto *const dataObject = engine->property(SyncthingClass::staticMetaObject.className()).template value<SyncthingClass *>();
    QJSEngine::setObjectOwnership(dataObject, QJSEngine::CppOwnership);
    return dataObject;
}

template <typename App, typename AppService> void connectAppAndService(App &quickApp, AppService &appService)
{
    QObject::connect(&quickApp, &App::syncthingTerminationRequested, &appService, &AppService::terminateSyncthing);
    QObject::connect(&quickApp, &App::syncthingRestartRequested, &appService, &AppService::restartSyncthing);
    QObject::connect(&quickApp, &App::syncthingShutdownRequested, &appService, &AppService::shutdownSyncthing);
    QObject::connect(&quickApp, &App::syncthingConnectRequested, appService.data()->connection(),
        static_cast<void (Data::SyncthingConnection::*)()>(&Data::SyncthingConnection::connect));
    QObject::connect(&quickApp, &App::syncthingReconnectRequested, appService.data()->connection(),
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
