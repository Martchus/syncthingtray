#ifndef DATA_SYNCTHINGDEV_H
#define DATA_SYNCTHINGDEV_H

#include "./global.h"

#include <c++utilities/chrono/datetime.h>

#include <QString>
#include <QStringList>

namespace Data {

enum class SyncthingDevStatus
{
    Unknown,
    Disconnected,
    OwnDevice,
    Idle,
    Synchronizing,
    OutOfSync,
    Rejected
};

struct LIB_SYNCTHING_CONNECTOR_EXPORT SyncthingDev
{
    QString id;
    QString name;
    QStringList addresses;
    QString compression;
    QString certName;
    SyncthingDevStatus status;
    int progressPercentage = 0;
    int progressRate = 0;
    bool introducer = false;
    bool paused = false;
    int totalIncomingTraffic = 0;
    int totalOutgoingTraffic = 0;
    QString connectionAddress;
    QString connectionType;
    QString clientVersion;
    ChronoUtilities::DateTime lastSeen;
};

} // namespace Data

#endif // DATA_SYNCTHINGDEV_H
