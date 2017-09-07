#ifndef DATA_SYNCTHINGDOWNLOADMODEL_H
#define DATA_SYNCTHINGDOWNLOADMODEL_H

#include "./syncthingmodel.h"

#include <QFileIconProvider>
#include <QIcon>

#include <vector>

namespace Data {

struct SyncthingDir;
struct SyncthingItemDownloadProgress;

class LIB_SYNCTHING_MODEL_EXPORT SyncthingDownloadModel : public SyncthingModel {
    Q_OBJECT
    Q_PROPERTY(unsigned int pendingDownloads READ pendingDownloads NOTIFY pendingDownloadsChanged)
    Q_PROPERTY(bool singleColumnMode READ singleColumnMode WRITE setSingleColumnMode)
public:
    explicit SyncthingDownloadModel(SyncthingConnection &connection, QObject *parent = nullptr);

    enum SyncthingDownloadModelRole { ItemPercentage = Qt::UserRole + 1, ItemProgressLabel, ItemPath };

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
    const SyncthingItemDownloadProgress *progressInfo(const QModelIndex &index) const;
    unsigned int pendingDownloads() const;
    bool singleColumnMode() const;
    void setSingleColumnMode(bool singleColumnModeEnabled);

Q_SIGNALS:
    void pendingDownloadsChanged(unsigned int pendingDownloads);

private Q_SLOTS:
    void newConfig();
    void newDirs();
    void downloadProgressChanged();

private:
    static QHash<int, QByteArray> initRoleNames();

    const std::vector<SyncthingDir> &m_dirs;
    const QIcon m_unknownIcon;
    const QFileIconProvider m_fileIconProvider;
    std::vector<const SyncthingDir *> m_pendingDirs;
    unsigned int m_pendingDownloads;
    bool m_singleColumnMode;
};

inline unsigned int SyncthingDownloadModel::pendingDownloads() const
{
    return m_pendingDownloads;
}

inline bool SyncthingDownloadModel::singleColumnMode() const
{
    return m_singleColumnMode;
}

} // namespace Data

#endif // DATA_SYNCTHINGDOWNLOADMODEL_H
