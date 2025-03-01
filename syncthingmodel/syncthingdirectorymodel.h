#ifndef DATA_SYNCTHINGDIRECTORYMODEL_H
#define DATA_SYNCTHINGDIRECTORYMODEL_H

#include "./syncthingmodel.h"

#include <QIcon>

#include <vector>

namespace Data {

struct SyncthingDir;

class LIB_SYNCTHING_MODEL_EXPORT SyncthingDirectoryModel : public SyncthingModel {
    Q_OBJECT
public:
    enum SyncthingDirectoryModelRole {
        DirectoryStatus = SyncthingModelUserRole + 1,
        DirectoryPaused,
        DirectoryStatusString,
        DirectoryStatusColor,
        DirectoryId,
        DirectoryPath,
        DirectoryPullErrorCount,
        DirectoryDetail,
        DirectoryDetailIcon,
        DirectoryNeededItemsCount,
        DirectoryDetailTooltip,
        DirectoryOverrideRevertAction,
        DirectoryOverrideRevertActionLabel,
    };

    explicit SyncthingDirectoryModel(SyncthingConnection &connection, QObject *parent = nullptr);

public:
    QHash<int, QByteArray> roleNames() const override;
    const QVector<int> &colorRoles() const override;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &child) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    int rowCount(const QModelIndex &parent) const override;
    Q_INVOKABLE const SyncthingDir *dirInfo(const QModelIndex &index) const;
    Q_INVOKABLE const SyncthingDir *info(const QModelIndex &index) const;

private Q_SLOTS:
    void dirStatusChanged(const Data::SyncthingDir &dir, int index);
    void handleConfigInvalidated() override;
    void handleNewConfigAvailable() override;
    void handleStatusIconsChanged() override;
    void handleForkAwesomeIconsChanged() override;

private:
    QVariant dirStatusColor(const SyncthingDir &dir) const;
    void updateRowCount();

    const std::vector<SyncthingDir> &m_dirs;
    std::vector<int> m_rowCount;
};

inline const SyncthingDir *SyncthingDirectoryModel::info(const QModelIndex &index) const
{
    return dirInfo(index);
}

} // namespace Data

Q_DECLARE_METATYPE(QModelIndex)

#endif // DATA_SYNCTHINGDIRECTORYMODEL_H
