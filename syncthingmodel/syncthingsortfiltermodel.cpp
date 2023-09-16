#include "./syncthingsortfiltermodel.h"
#include "./syncthingmodel.h"

#include <QSortFilterProxyModel>

namespace Data {

bool SyncthingSortFilterModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    // show all nested structures
    if (sourceParent.isValid()) {
        return true;
    }
    // use default filtering for top-level
    return QSortFilterProxyModel::filterAcceptsRow(sourceRow, sourceParent);
}

bool SyncthingSortFilterModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    // keep order within nested structures
    if (m_behavior == SyncthingSortBehavior::KeepRawOrder || left.parent().isValid() || right.parent().isValid()) {
        return left.row() < right.row();
    }
    // show pinned items before all other items
    const auto leftPinned = left.data(SyncthingModel::IsPinned).toBool();
    const auto rightPinned = right.data(SyncthingModel::IsPinned).toBool();
    if (leftPinned != rightPinned) {
        return leftPinned;
    }
    // use the default sorting for the top-level
    return QSortFilterProxyModel::lessThan(left, right);
}

} // namespace Data
