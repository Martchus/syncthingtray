#include "./syncthingstatusselectionmodel.h"

#include <syncthingconnector/syncthingconnection.h>

using namespace QtUtilities;

namespace Data {

inline static ChecklistItem itemFor(SyncthingStatus status)
{
    return ChecklistItem(static_cast<int>(status), QString(), Qt::Unchecked);
}

SyncthingStatusSelectionModel::SyncthingStatusSelectionModel(QObject *parent)
    : ChecklistModel(parent)
{
    setItems({
        itemFor(SyncthingStatus::Disconnected),
        itemFor(SyncthingStatus::Reconnecting),
        itemFor(SyncthingStatus::Idle),
        itemFor(SyncthingStatus::Scanning),
        itemFor(SyncthingStatus::Paused),
        itemFor(SyncthingStatus::Synchronizing),
        itemFor(SyncthingStatus::RemoteNotInSync),
    });
}

QString SyncthingStatusSelectionModel::labelForId(const QVariant &id) const
{
    return SyncthingConnection::statusText(static_cast<SyncthingStatus>(id.toInt()));
}

} // namespace Data
