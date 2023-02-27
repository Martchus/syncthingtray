#ifndef DATA_SYNCTHINGSORTFILTERDIRECTORYMODEL_H
#define DATA_SYNCTHINGSORTFILTERDIRECTORYMODEL_H

#include "./global.h"

#include <QSortFilterProxyModel>

namespace Data {

enum class SyncthingSortBehavior {
    KeepRawOrder,
    Alphabetically,
};

class LIB_SYNCTHING_MODEL_EXPORT SyncthingSortFilterModel : public QSortFilterProxyModel {
    Q_OBJECT
public:
    explicit SyncthingSortFilterModel(QAbstractItemModel *sourceModel = nullptr, QObject *parent = nullptr);

    SyncthingSortBehavior behavior() const;
    void setBehavior(SyncthingSortBehavior behavior);

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;

private:
    SyncthingSortBehavior m_behavior;
};

inline SyncthingSortFilterModel::SyncthingSortFilterModel(QAbstractItemModel *sourceModel, QObject *parent)
    : QSortFilterProxyModel(parent)
    , m_behavior(SyncthingSortBehavior::Alphabetically)
{
    setSortCaseSensitivity(Qt::CaseInsensitive);
    setFilterCaseSensitivity(Qt::CaseInsensitive);
    setSourceModel(sourceModel);
}

inline SyncthingSortBehavior SyncthingSortFilterModel::behavior() const
{
    return m_behavior;
}

inline void SyncthingSortFilterModel::setBehavior(SyncthingSortBehavior behavior)
{
    if (behavior != m_behavior) {
        m_behavior = behavior;
        invalidate();
    }
}

} // namespace Data

#endif // DATA_SYNCTHINGSORTFILTERDIRECTORYMODEL_H
