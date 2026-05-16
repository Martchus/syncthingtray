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
    // sort by group
    const auto leftGroup = left.data(SyncthingModel::Group).toString();
    const auto rightGroup = right.data(SyncthingModel::Group).toString();
    if (leftGroup.isEmpty() != rightGroup.isEmpty()) {
        return leftGroup.isEmpty();
    }
    if (!leftGroup.isEmpty() && !rightGroup.isEmpty()) {
        if (const auto cmp = QString::compare(leftGroup, rightGroup, Qt::CaseInsensitive)) {
            return cmp < 0;
        }
    }
    // show pinned items before other items within group
    const auto leftPinned = left.data(SyncthingModel::IsPinned).toBool();
    const auto rightPinned = right.data(SyncthingModel::IsPinned).toBool();
    if (leftPinned != rightPinned) {
        return leftPinned;
    }
    // use the default sorting for the top-level
    return QSortFilterProxyModel::lessThan(left, right);
}

QModelIndex Data::SyncthingSectionModel::index(int row, int column, const QModelIndex &parent) const
{

}

QModelIndex Data::SyncthingSectionModel::mapToSource(const QModelIndex &proxyIndex) const
{

}

QModelIndex Data::SyncthingSectionModel::mapFromSource(const QModelIndex &sourceIndex) const
{

}

int Data::SyncthingSectionModel::rowCount(const QModelIndex &parent) const
{

}

int Data::SyncthingSectionModel::columnCount(const QModelIndex &parent) const
{

}

QVariant Data::SyncthingSectionModel::data(const QModelIndex &proxyIndex, int role) const
{

}

bool Data::SyncthingSectionModel::setData(const QModelIndex &index, const QVariant &value, int role)
{

}

Qt::ItemFlags Data::SyncthingSectionModel::flags(const QModelIndex &index) const
{

}

} // namespace Data
