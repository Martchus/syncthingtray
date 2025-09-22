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

#ifdef Q_OS_ANDROID
#include <atomic>
#endif
#include <optional>

namespace QtGui {

enum class ServiceAction : int;

class SYNCTHINGWIDGETS_EXPORT AppService : public AppBase {
    Q_OBJECT
    Q_PROPERTY(Data::SyncthingLauncher *launcher READ launcher CONSTANT)
    Q_PROPERTY(bool syncthingRunning READ isSyncthingRunning)

public:
    explicit AppService(bool insecure, QObject *parent = nullptr);
    ~AppService();

    // properties
    const QString &status() override final;
    bool isSyncthingRunning() const final
    {
        return m_launcher.isRunning();
    }
    Data::SyncthingLauncher *launcher()
    {
        return &m_launcher;
    }

Q_SIGNALS:
#ifndef Q_OS_ANDROID
    void launcherStatusChanged(const QVariant &status);
    void logsAvailable(const QString &newLogMessages);
#endif

public:
    Q_INVOKABLE void broadcastLauncherStatus();
    Q_INVOKABLE bool applyLauncherSettings();
    Q_INVOKABLE bool reloadSettings();
    Q_INVOKABLE void terminateSyncthing();
    Q_INVOKABLE void stopLibSyncthing();
    Q_INVOKABLE void restartSyncthing();
    Q_INVOKABLE void shutdownSyncthing();
    Q_INVOKABLE void clearLog();
    Q_INVOKABLE void replayLog();
#ifdef Q_OS_ANDROID
    Q_INVOKABLE void showError(const QString &error);
    Q_INVOKABLE void clearInternalErrors();
    Q_INVOKABLE void handleMessageFromActivity(ServiceAction action, int arg1, int arg2, const QString &str);
#endif

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
#ifdef SYNCTHINGTRAY_SERVICE_WITH_ICON_RENDERING
    void invalidateAndroidIconCache();
    QJniObject &makeAndroidIcon(const QIcon &icon);
#endif
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
    QString m_log;
#ifdef Q_OS_ANDROID
#ifdef SYNCTHINGTRAY_SERVICE_WITH_ICON_RENDERING
    QHash<const QIcon *, QJniObject> m_androidIconCache;
#endif
    int m_androidNotificationId = 100000000;
    mutable std::optional<bool> m_storagePermissionGranted;
    mutable std::optional<bool> m_notificationPermissionGranted;
    std::atomic_bool m_clientsFollowingLog;
#endif
};

} // namespace QtGui

#endif // SYNCTHING_TRAY_APP_SERVICE_H
