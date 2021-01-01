#include "./syncthingstatuscomputionmodel.h"

#include "../connector/syncthingconnectionsettings.h"

#include <type_traits>

using namespace QtUtilities;

namespace Data {

using FlagType = SyncthingStatusComputionFlags;
using UnderlyingFlagType = std::underlying_type_t<FlagType>;

inline static ChecklistItem itemFor(SyncthingStatusComputionFlags oneFlag)
{
    return ChecklistItem(
        static_cast<UnderlyingFlagType>(oneFlag), QString(), SyncthingStatusComputionFlags::Default & oneFlag ? Qt::Checked : Qt::Unchecked);
}

SyncthingStatusComputionModel::SyncthingStatusComputionModel(QObject *parent)
    : ChecklistModel(parent)
{
    setItems({
        itemFor(SyncthingStatusComputionFlags::Scanning),
        itemFor(SyncthingStatusComputionFlags::Synchronizing),
        itemFor(SyncthingStatusComputionFlags::RemoteSynchronizing),
        itemFor(SyncthingStatusComputionFlags::DevicePaused),
    });
}

QString SyncthingStatusComputionModel::labelForId(const QVariant &id) const
{
    switch (static_cast<SyncthingStatusComputionFlags>(id.toInt())) {
    case SyncthingStatusComputionFlags::Scanning:
        return tr("Local dir is scanning");
    case SyncthingStatusComputionFlags::Synchronizing:
        return tr("Local dir is synchronizing");
    case SyncthingStatusComputionFlags::RemoteSynchronizing:
        return tr("Remote dir has outstanding progress");
    case SyncthingStatusComputionFlags::DevicePaused:
        return tr("A device is paused");
    default:
        return id.toString();
    }
}

SyncthingStatusComputionFlags SyncthingStatusComputionModel::statusComputionFlags() const
{
    auto flags = SyncthingStatusComputionFlags::None;
    for (auto row = 0, rows = rowCount(); row != rows; ++row) {
        const auto i = index(row);
        if (i.data(Qt::CheckStateRole).toInt() == Qt::Checked) {
            flags += static_cast<SyncthingStatusComputionFlags>(i.data(idRole()).value<UnderlyingFlagType>());
        }
    }
    return flags;
}

void SyncthingStatusComputionModel::setStatusComputionFlags(SyncthingStatusComputionFlags flags)
{
    for (auto row = 0, rows = rowCount(); row != rows; ++row) {
        const auto i = index(row);
        setData(i, flags & static_cast<FlagType>(i.data(idRole()).value<UnderlyingFlagType>()) ? Qt::Checked : Qt::Unchecked, Qt::CheckStateRole);
    }
}

} // namespace Data
