#ifndef SYNCTHING_TRAY_ANDROID_H
#define SYNCTHING_TRAY_ANDROID_H

#include "../global.h"

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
    FlagManualStop,
};

namespace JniFn {
SYNCTHINGWIDGETS_EXPORT void registerServiceJniMethods(AppService *appService);
SYNCTHINGWIDGETS_EXPORT void unregisterServiceJniMethods(AppService *appService);
SYNCTHINGWIDGETS_EXPORT void registerActivityJniMethods(App *app);
SYNCTHINGWIDGETS_EXPORT void unregisterActivityJniMethods(App *app);
} // namespace JniFn

} // namespace QtGui

#endif // SYNCTHING_TRAY_ANDROID_H
