#include "./syncthingdev.h"

#include <c++utilities/conversion/stringconversion.h>

#include <QCoreApplication>

using namespace CppUtilities;

namespace Data {

QString statusString(SyncthingDevStatus status)
{
    switch (status) {
    case SyncthingDevStatus::Unknown:
        return QCoreApplication::translate("SyncthingDevStatus", "Unknown");
    case SyncthingDevStatus::Disconnected:
        return QCoreApplication::translate("SyncthingDevStatus", "Disconnected");
    case SyncthingDevStatus::ThisDevice:
        return QCoreApplication::translate("SyncthingDevStatus", "This Device");
    case SyncthingDevStatus::Idle:
        return QCoreApplication::translate("SyncthingDevStatus", "Idle");
    case SyncthingDevStatus::Synchronizing:
        return QCoreApplication::translate("SyncthingDevStatus", "Syncing");
    case SyncthingDevStatus::OutOfSync:
        return QCoreApplication::translate("SyncthingDevStatus", "Out of Sync");
    case SyncthingDevStatus::Rejected:
        return QCoreApplication::translate("SyncthingDevStatus", "Rejected");
    default:
        return QString();
    }
}

QString SyncthingDev::statusString() const
{
    if (paused) {
        return QCoreApplication::translate("SyncthingDev", "Paused");
    }
    if (status == SyncthingDevStatus::Synchronizing && overallCompletion.needed.bytes) {
        return QCoreApplication::translate("SyncthingDev", "Syncing (%1 %, %2)")
            .arg(static_cast<int>(overallCompletion.percentage))
            .arg(QString::fromStdString(dataSizeToString(overallCompletion.needed.bytes)));
    }
    return Data::statusString(status);
}

} // namespace Data
