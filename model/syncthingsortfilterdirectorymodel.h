#ifndef DATA_SYNCTHINGSORTFILTERDIRECTORYMODEL_H
#define DATA_SYNCTHINGSORTFILTERDIRECTORYMODEL_H

#include "./global.h"

#include <QSortFilterProxyModel>

namespace Data {

enum class SyncthingDirectorySortBehavior {
    KeepRawOrder,
    Alphabetically,
};

class LIB_SYNCTHING_MODEL_EXPORT SyncthingSortFilterDirectoryModel : public QSortFilterProxyModel {
    Q_OBJECT
public:
    explicit SyncthingSortFilterDirectoryModel(QAbstractItemModel *sourceModel = nullptr, QObject *parent = nullptr);

    SyncthingDirectorySortBehavior behavior() const;
    void setBehavior(SyncthingDirectorySortBehavior behavior);

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;

private:
    SyncthingDirectorySortBehavior m_behavior;
};

inline SyncthingSortFilterDirectoryModel::SyncthingSortFilterDirectoryModel(QAbstractItemModel *sourceModel, QObject *parent)
    : QSortFilterProxyModel(parent)
    , m_behavior(SyncthingDirectorySortBehavior::Alphabetically)
{
    setSortCaseSensitivity(Qt::CaseInsensitive);
    setFilterCaseSensitivity(Qt::CaseInsensitive);
    setSourceModel(sourceModel);
}

inline SyncthingDirectorySortBehavior SyncthingSortFilterDirectoryModel::behavior() const
{
    return m_behavior;
}

inline void SyncthingSortFilterDirectoryModel::setBehavior(SyncthingDirectorySortBehavior behavior)
{
    if (behavior != m_behavior) {
        m_behavior = behavior;
        invalidate();
    }
}

} // namespace Data

#endif // DATA_SYNCTHINGSORTFILTERDIRECTORYMODEL_H
