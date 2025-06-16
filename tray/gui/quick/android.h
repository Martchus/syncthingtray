#ifndef SYNCTHING_TRAY_ANDROID_H
#define SYNCTHING_TRAY_ANDROID_H

#include <QtCore/private/qandroidextras_p.h>

namespace QtGui {

class AppService;
class App;

enum class ServiceAction : int {
    ReloadSettings = 100,
    TerminateSyncthing,
    RestartSyncthing,
    ShutdownSyncthing,
    ConnectToSyncthing,
    BroadcastLauncherStatus,
    Reconnect,
    ClearInternalErrorNotifications,
    ClearLog,
    FollowLog,
    CloseLog,
};

enum class ActivityAction : int {
    ShowError = 100,
    AppendLog,
};

namespace JniFn {
void registerServiceJniMethods(AppService *appService);
void unregisterServiceJniMethods(AppService *appService);
void registerActivityJniMethods(App *app);
void unregisterActivityJniMethods(App *app);
} // namespace JniFn

} // namespace QtGui

#endif // SYNCTHING_TRAY_ANDROID_H
