#ifndef DATA_SYNCTHINGDEV_H
#define DATA_SYNCTHINGDEV_H

#include "./qstringhash.h"
#include "./syncthingcompletion.h"

#include <c++utilities/chrono/datetime.h>

#include <QMetaType>
#include <QString>
#include <QStringList>

#include <cstdlib>
#include <unordered_map>
#include <vector>

namespace Data {

/// \brief The SyncthingDevStatus enum represents a Syncthing device status.
/// \remarks The device status is not directly provided by Syncthing and instead deduced by this library from
///          other information and events.
enum class SyncthingDevStatus {
    Unknown, /**< device status is unknown */
    Disconnected, /**< device is disconnected */
    OwnDevice, /**< device is the own device; the own device will always have this status assigned */
    Idle, /**< device is connected and all shared directories are up-to-date on its end */
    Synchronizing, /**< device is connected but not all shared directories are up-to-date on its end */
    OutOfSync, /**< device is connected but not all shared directories are up-to-date on its end due to an error (never set so far; seems not possible to determine) */
    Rejected, /**< device is rejected */
};

QString statusString(SyncthingDevStatus status);

struct LIB_SYNCTHING_CONNECTOR_EXPORT SyncthingDev {
    explicit SyncthingDev(const QString &id = QString(), const QString &name = QString());
    QString statusString() const;
    bool isConnected() const;
    void setConnectedStateAccordingToCompletion();
    const QString displayName() const;

    QString id;
    QString name;
    QStringList addresses;
    QString compression;
    QString certName;
    SyncthingDevStatus status = SyncthingDevStatus::Unknown;
    std::uint64_t totalIncomingTraffic = 0;
    std::uint64_t totalOutgoingTraffic = 0;
    QString connectionAddress;
    QString connectionType;
    QString clientVersion;
    CppUtilities::DateTime lastSeen;
    std::unordered_map<QString, SyncthingCompletion> completionByDir;
    SyncthingCompletion overallCompletion;
    bool introducer = false;
    bool paused = false;
};

inline SyncthingDev::SyncthingDev(const QString &id, const QString &name)
    : id(id)
    , name(name)
{
}

inline bool SyncthingDev::isConnected() const
{
    switch (status) {
    case SyncthingDevStatus::Unknown:
    case SyncthingDevStatus::Disconnected:
    case SyncthingDevStatus::OwnDevice:
    case SyncthingDevStatus::Rejected:
        return false;
    default:
        return true;
    }
}

inline void SyncthingDev::setConnectedStateAccordingToCompletion()
{
    status = overallCompletion.needed.isNull() ? SyncthingDevStatus::Idle : SyncthingDevStatus::Synchronizing;
}

inline const QString SyncthingDev::displayName() const
{
    return name.isEmpty() ? id : name;
}

} // namespace Data

Q_DECLARE_METATYPE(Data::SyncthingDev)

#endif // DATA_SYNCTHINGDEV_H
