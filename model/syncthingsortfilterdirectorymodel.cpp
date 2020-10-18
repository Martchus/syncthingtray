#include "./syncthingsortfilterdirectorymodel.h"

#include <QSortFilterProxyModel>

namespace Data {

bool SyncthingSortFilterDirectoryModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    // show all nested structures
    if (sourceParent.isValid()) {
        return true;
    }
    // use default filtering for top-level
    return QSortFilterProxyModel::filterAcceptsRow(sourceRow, sourceParent);
}

bool SyncthingSortFilterDirectoryModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    // keep order within nested structures
    if (m_behavior == SyncthingDirectorySortBehavior::KeepRawOrder || left.parent().isValid() || right.parent().isValid()) {
        return left.row() < right.row();
    }
    // use the default sorting for the top-level
    return QSortFilterProxyModel::lessThan(left, right);
}

} // namespace Data
