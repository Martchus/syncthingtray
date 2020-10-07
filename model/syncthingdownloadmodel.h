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
    QHash<int, QByteArray> roleNames() const override;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &child) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;
    const SyncthingDir *dirInfo(const QModelIndex &index) const;
    const SyncthingItemDownloadProgress *progressInfo(const QModelIndex &index) const;
    QPair<const SyncthingDir *, const SyncthingItemDownloadProgress *> info(const QModelIndex &index) const;
    unsigned int pendingDownloads() const;
    bool singleColumnMode() const;
    void setSingleColumnMode(bool singleColumnModeEnabled);

Q_SIGNALS:
    void pendingDownloadsChanged(unsigned int pendingDownloads);

private Q_SLOTS:
    void handleConfigInvalidated() override;
    void handleNewConfigAvailable() override;
    void downloadProgressChanged();

private:
    struct PendingDir {
        const SyncthingDir *syncthingDir;
        std::size_t pendingItems;

        PendingDir(const SyncthingDir *syncthingDir, unsigned int pendingItems);
        bool operator==(const SyncthingDir *dir) const;
    };

    const std::vector<SyncthingDir> &m_dirs;
    const QIcon m_unknownIcon;
    const QFileIconProvider m_fileIconProvider;
    std::vector<PendingDir> m_pendingDirs;
    unsigned int m_pendingDownloads;
    bool m_singleColumnMode;
};

inline QPair<const SyncthingDir *, const SyncthingItemDownloadProgress *> SyncthingDownloadModel::info(const QModelIndex &index) const
{
    return qMakePair(dirInfo(index), progressInfo(index));
}

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
