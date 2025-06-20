#ifndef SYNCTHINGCONNECTIONSETTINGS_H
#define SYNCTHINGCONNECTIONSETTINGS_H

#include "./global.h"

#include <c++utilities/misc/flagenumclass.h>

#include <QByteArray>
#include <QDateTime>
#include <QList>
#include <QSslError>
#include <QString>

QT_FORWARD_DECLARE_CLASS(QJsonObject)
QT_FORWARD_DECLARE_CLASS(QSslCertificate)

namespace Data {

/*!
 * \brief The SyncthingStatusComputionFlags enum specifies what information is considered to compute the overall state.
 * \remarks The enum is supposed to be used as flag-enum.
 */
enum class SyncthingStatusComputionFlags : quint64 {
    None = 0, /**< no further information is considered leaving SyncthingStatus::Disconnected, SyncthingStatus::Reconnecting,
                   SyncthingStatus::BeingDestroyed and SyncthingStatus::Idle the only possible states */
    Scanning = (1 << 0), /**< the status SyncthingStatus::Scanning might be set (in addition) */
    Synchronizing = (1 << 1), /**< the status SyncthingStatus::Synchronizing might be set (in addition) */
    RemoteSynchronizing = (1 << 2), /**< the status SyncthingStatus::RemoteNotInSync might be set (in addition) */
    DevicePaused = (1 << 3), /**< the status SyncthingStatus::Paused might be set if there's at least one paused device (in addition) */
    OutOfSync = (1
        << 4), /**< the return value of SyncthingConnection::hasOutOfSyncDirs() is considered by further displaying-related computations such as StatusInfo::updateConnectionStatus() */
    UnreadNotifications = (1
        << 5), /**< the return value of SyncthingConnection::hasUnreadNotifications() is considered by further displaying-related computations such as StatusInfo::updateConnectionStatus() */
    NoRemoteConnected = (1 << 6), /**< the status SyncthingStatus::NoRemoteConnected might be set (in addition) */
    Default = SyncthingStatusComputionFlags::Scanning | SyncthingStatusComputionFlags::Synchronizing | SyncthingStatusComputionFlags::DevicePaused
        | SyncthingStatusComputionFlags::OutOfSync | SyncthingStatusComputionFlags::UnreadNotifications
        | SyncthingStatusComputionFlags::NoRemoteConnected,
    /**< the default flags used all over the place */
};

struct LIB_SYNCTHING_CONNECTOR_EXPORT SyncthingConnectionSettings {
    QString label;
    QString syncthingUrl;
    QString localPath;
    bool authEnabled = false;
    QString userName;
    QString password;
    QByteArray apiKey;
    int trafficPollInterval = defaultTrafficPollInterval;
    int devStatsPollInterval = defaultDevStatusPollInterval;
    int errorsPollInterval = defaultErrorsPollInterval;
    int reconnectInterval = defaultReconnectInterval;
    int requestTimeout = defaultRequestTimeout;
    int longPollingTimeout = defaultLongPollingTimeout;
    int diskEventLimit = defaultDiskEventLimit;
#ifndef QT_NO_SSL
    QString httpsCertPath;
    QDateTime httpCertLastModified;
    QList<QSslError> expectedSslErrors;
#endif
    SyncthingStatusComputionFlags statusComputionFlags = SyncthingStatusComputionFlags::Default;
    bool autoConnect = false;
    bool pauseOnMeteredConnection = false;
#ifndef QT_NO_SSL
    static QList<QSslError> compileSslErrors(const QSslCertificate &trustedCert);
    bool loadHttpsCert();
#endif
    void storeToJson(QJsonObject &object);
    bool loadFromJson(const QJsonObject &object);

    static constexpr int defaultTrafficPollInterval = 5000;
    static constexpr int defaultDevStatusPollInterval = 60000;
    static constexpr int defaultErrorsPollInterval = 30000;
    static constexpr int defaultReconnectInterval = 30000;
    static constexpr int defaultRequestTimeout = 0;
    static constexpr int defaultLongPollingTimeout = 0;
    static constexpr int defaultDiskEventLimit = 200;
};
} // namespace Data

CPP_UTILITIES_MARK_FLAG_ENUM_CLASS(Data, Data::SyncthingStatusComputionFlags)

#endif // SYNCTHINGCONNECTIONSETTINGS_H
