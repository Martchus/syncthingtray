#ifndef SYNCTHING_TRAY_ANDROID_H
#define SYNCTHING_TRAY_ANDROID_H

namespace QtGui {

class AppService;
class App;

enum class ServiceAction : int {
    ReloadSettings = 100,
    TerminateSyncthing,
    RestartSyncthing,
    ShutdownSyncthing,
    ConnectToSyncthing,
    BroadcastLauncherStatus, // keep in-line with MSG_SERVICE_ACTION_BROADCAST_LAUNCHER_STATUS
    Reconnect,
    ClearInternalErrorNotifications,
    ClearLog,
    FollowLog,
    CloseLog,
    RequestErrors,
};

enum class ActivityAction : int {
    ShowError = 100,
    AppendLog,
    UpdateLauncherStatus,
};

namespace JniFn {
void registerServiceJniMethods(AppService *appService);
void unregisterServiceJniMethods(AppService *appService);
void registerActivityJniMethods(App *app);
void unregisterActivityJniMethods(App *app);
} // namespace JniFn

} // namespace QtGui

#endif // SYNCTHING_TRAY_ANDROID_H
