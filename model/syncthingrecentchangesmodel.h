#ifndef DATA_SYNCTHINGRECENTCHANGESMODEL_H
#define DATA_SYNCTHINGRECENTCHANGESMODEL_H

#include "./syncthingmodel.h"

#include <syncthingconnector/syncthingconnectionstatus.h>
#include <syncthingconnector/syncthingdir.h>

#include <deque>

namespace Data {

struct LIB_SYNCTHING_MODEL_EXPORT SyncthingRecentChange {
    QString directoryId;
    QString directoryName;
    QString deviceId;
    QString deviceName;
    SyncthingFileChange fileChange;
};

class LIB_SYNCTHING_MODEL_EXPORT SyncthingRecentChangesModel : public SyncthingModel {
    Q_OBJECT
    Q_PROPERTY(int maxRows READ maxRows WRITE setMaxRows)
public:
    enum SyncthingRecentChangesModelRole {
        Action = Qt::UserRole + 1,
        ActionIcon,
        ModifiedBy,
        DirectoryId,
        DirectoryName,
        Path,
        EventTime,
        ExtendedAction,
        ItemType,
    };
    explicit SyncthingRecentChangesModel(SyncthingConnection &connection, int maxRows = 200, QObject *parent = nullptr);

public Q_SLOTS:
    QHash<int, QByteArray> roleNames() const override;
    const QVector<int> &colorRoles() const override;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &child) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;
    int maxRows() const;
    void setMaxRows(int maxRows);

private Q_SLOTS:
    void fileChanged(const SyncthingDir &dir, int index, const SyncthingFileChange &change);
    void handleConfigInvalidated() override;
    void handleNewConfigAvailable() override;
    void handleStatusChanged(SyncthingStatus status);
    void handleForkAwesomeIconsChanged() override;

private:
    void ensureWithinLimit();

    std::deque<SyncthingRecentChange> m_changes;
    int m_maxRows;
};

inline int SyncthingRecentChangesModel::maxRows() const
{
    return m_maxRows;
}

} // namespace Data

Q_DECLARE_METATYPE(Data::SyncthingRecentChange)

#endif // DATA_SYNCTHINGRECENTCHANGESMODEL_H
