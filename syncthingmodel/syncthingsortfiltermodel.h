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

    Q_INVOKABLE QString filterRegularExpressionPattern() const
    {
        return filterRegularExpression().pattern();
    }
    Q_INVOKABLE void setFilterRegularExpressionPattern(const QString &pattern)
    {
        setFilterRegularExpression(pattern);
    }

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;

private:
    SyncthingSortBehavior m_behavior;
};

inline SyncthingSortFilterModel::SyncthingSortFilterModel(QAbstractItemModel *sourceModel, QObject *parent)
    : QSortFilterProxyModel(parent)
    , m_behavior(SyncthingSortBehavior::Alphabetically)
    , m_sectionRole(sectionRole)
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

class LIB_SYNCTHING_MODEL_EXPORT SyncthingSectionModel : public SyncthingSortFilterModel {
    Q_OBJECT
public:
    explicit SyncthingSectionModel(QAbstractItemModel *sourceModel = nullptr, int sectionRole = -1, QObject *parent = nullptr);

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex mapToSource(const QModelIndex &proxyIndex) const override;
    QModelIndex mapFromSource(const QModelIndex &sourceIndex) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &proxyIndex, int role) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

private:
    int m_sectionRole;
    struct Section {
        QVariant value;
        int firstRow = 0;
        int lastRow = 0;
    };
    std::vector<Section> m_sections;
};

inline SyncthingSectionModel::SyncthingSectionModel(QAbstractItemModel *sourceModel, int sectionRole, QObject *parent)
    : SyncthingSortFilterModel(sourceModel, parent)
    , m_sectionRole(sectionRole)
{

}

} // namespace Data

#endif // DATA_SYNCTHINGSORTFILTERDIRECTORYMODEL_H
