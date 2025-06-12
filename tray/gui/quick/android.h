#ifndef SYNCTHING_TRAY_ANDROID_H
#define SYNCTHING_TRAY_ANDROID_H

#include <QtCore/private/qandroidextras_p.h>

namespace QtGui {

class AppService;
class App;

class SyncthingServiceBinder : public QAndroidBinder {
public:
    enum SyncthingServiceAction : int {
        ReloadSettings = 1,
        TerminateSyncthing,
        BroadcastLauncherStatus,
    };

    explicit SyncthingServiceBinder();

    bool onTransact(int code, const QAndroidParcel &data, const QAndroidParcel &reply, QAndroidBinder::CallType flags) override;
    void setService(AppService *service)
    {
        m_service = service;
    }

private:
    AppService *m_service;
};

class SyncthingServiceConnection : public QAndroidServiceConnection {
public:
    explicit SyncthingServiceConnection();

    bool connect();
    const QAndroidBinder &binder() const
    {
        return m_binder;
    }
    void onServiceConnected(const QString &name, const QAndroidBinder &serviceBinder) override;
    void onServiceDisconnected(const QString &name) override;

private:
    QAndroidBinder m_binder;
};

namespace JniFn {
void registerServiceJniMethods(AppService *appService);
void unregisterServiceJniMethods(AppService *appService);
void registerActivityJniMethods(App *app);
void unregisterActivityJniMethods(App *app);
} // namespace JniFn

} // namespace QtGui

#endif // SYNCTHING_TRAY_ANDROID_H
