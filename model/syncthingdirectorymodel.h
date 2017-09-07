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
        DirectoryStatus = Qt::UserRole + 1,
        DirectoryPaused,
        DirectoryStatusString,
        DirectoryStatusColor,
        DirectoryId,
        DirectoryPath,
        DirectoryDetail,
    };

    explicit SyncthingDirectoryModel(SyncthingConnection &connection, QObject *parent = nullptr);

public Q_SLOTS:
    QHash<int, QByteArray> roleNames() const;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex &child) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    QVariant data(const QModelIndex &index, int role) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role);
    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    const SyncthingDir *dirInfo(const QModelIndex &index) const;

private Q_SLOTS:
    void newConfig();
    void newDirs();
    void dirStatusChanged(const SyncthingDir &, int index);

private:
    static QHash<int, QByteArray> initRoleNames();
    static QString dirStatusString(const SyncthingDir &dir);
    QVariant dirStatusColor(const SyncthingDir &dir) const;

    const std::vector<SyncthingDir> &m_dirs;
};

} // namespace Data

Q_DECLARE_METATYPE(QModelIndex)

#endif // DATA_SYNCTHINGDIRECTORYMODEL_H
