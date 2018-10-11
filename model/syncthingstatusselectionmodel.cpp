#include "./syncthingstatusselectionmodel.h"

#include "../connector/syncthingconnection.h"

using namespace Models;

namespace Data {

inline ChecklistItem itemFor(SyncthingStatus status)
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
        itemFor(SyncthingStatus::OutOfSync),
    });
}

QString SyncthingStatusSelectionModel::labelForId(const QVariant &id) const
{
    return SyncthingConnection::statusText(static_cast<SyncthingStatus>(id.toInt()));
}

} // namespace Data
