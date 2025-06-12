#ifndef SYNCTHING_TRAY_APP_SERVICE_H
#define SYNCTHING_TRAY_APP_SERVICE_H

#include "./appbase.h"

#include <syncthingwidgets/misc/internalerror.h>
#include <syncthingwidgets/misc/syncthinglauncher.h>

#include <QJsonObject>
#include <QUrl>
#include <QtVersion>

#ifdef Q_OS_ANDROID
#include <QHash>
#include <QJniObject>
#endif

#include <optional>

namespace QtGui {

class AppService : public AppBase {
    Q_OBJECT
    Q_PROPERTY(Data::SyncthingLauncher *launcher READ launcher CONSTANT)

public:
    explicit AppService(bool insecure, QObject *parent = nullptr);
    ~AppService();

    // properties
    const QString &status() override;
    Data::SyncthingLauncher *launcher()
    {
        return &m_launcher;
    }

Q_SIGNALS:
#ifndef Q_OS_ANDROID
    void launcherStatusChanged(const QVariant &status);
#endif

public Q_SLOTS:
    Q_INVOKABLE void broadcastLauncherStatus();
    Q_INVOKABLE bool applyLauncherSettings();
    Q_INVOKABLE bool applySettings();
    Q_INVOKABLE bool reloadSettings();
    Q_INVOKABLE void terminateSyncthing();

private Q_SLOTS:
    void handleConnectionError(const QString &errorMessage, Data::SyncthingErrorCategory category, int networkError, const QNetworkRequest &request,
        const QByteArray &response);
    void invalidateStatus() override;
    void gatherLogs(const QByteArray &newOutput);
    void handleRunningChanged(bool isRunning);
    void handleChangedDevices();
    void handleNewErrors(const std::vector<Data::SyncthingError> &errors);
    void handleConnectionStatusChanged(Data::SyncthingStatus newStatus);
#ifdef Q_OS_ANDROID
    void stopLibSyncthing();
    QJniObject &makeAndroidIcon(const QIcon &icon);
    void invalidateAndroidIconCache();
    void updateAndroidNotification();
    void updateExtraAndroidNotification(
        const QJniObject &title, const QJniObject &text, const QJniObject &subText, const QJniObject &page, const QJniObject &icon, int id = 0);
    void clearAndroidExtraNotifications(int firstId, int lastId = -1);
    void updateSyncthingErrorsNotification(const std::vector<Data::SyncthingError> &newErrors);
    void clearSyncthingErrorsNotification();
    void showInternalError(const InternalError &error);
    void showNewDevice(const QString &devId, const QString &message);
    void showNewDir(const QString &devId, const QString &dirId, const QString &dirLabel, const QString &message);
#endif

private:
    Data::SyncthingLauncher m_launcher;
#ifdef Q_OS_ANDROID
    QHash<const QIcon *, QJniObject> m_androidIconCache;
    int m_androidNotificationId = 100000000;
    mutable std::optional<bool> m_storagePermissionGranted;
    mutable std::optional<bool> m_notificationPermissionGranted;
#endif
};

} // namespace QtGui

#endif // SYNCTHING_TRAY_APP_SERVICE_H
