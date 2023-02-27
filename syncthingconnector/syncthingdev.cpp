#include "./syncthingdev.h"

#include <QCoreApplication>

namespace Data {

QString statusString(SyncthingDevStatus status)
{
    switch (status) {
    case SyncthingDevStatus::Unknown:
        return QCoreApplication::translate("SyncthingDevStatus", "unknown");
    case SyncthingDevStatus::Disconnected:
        return QCoreApplication::translate("SyncthingDevStatus", "disconnected");
    case SyncthingDevStatus::OwnDevice:
        return QCoreApplication::translate("SyncthingDevStatus", "own device");
    case SyncthingDevStatus::Idle:
        return QCoreApplication::translate("SyncthingDevStatus", "idle");
    case SyncthingDevStatus::Synchronizing:
        return QCoreApplication::translate("SyncthingDevStatus", "synchronizing");
    case SyncthingDevStatus::OutOfSync:
        return QCoreApplication::translate("SyncthingDevStatus", "out of sync");
    case SyncthingDevStatus::Rejected:
        return QCoreApplication::translate("SyncthingDevStatus", "rejected");
    default:
        return QString();
    }
}

QString SyncthingDev::statusString() const
{
    if (paused) {
        return QCoreApplication::translate("SyncthingDev", "paused");
    } else {
        return ::Data::statusString(status);
    }
}

} // namespace Data
