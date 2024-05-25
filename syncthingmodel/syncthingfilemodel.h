#ifndef DATA_SYNCTHINGFILEMODEL_H
#define DATA_SYNCTHINGFILEMODEL_H

#include "./syncthingmodel.h"

#include <syncthingconnector/syncthingconnection.h>
#include <syncthingconnector/syncthingignorepattern.h>

#include <QFuture>
#include <QFutureWatcher>

#include <map>
#include <memory>
#include <vector>

QT_FORWARD_DECLARE_CLASS(QAction)

namespace Data {

class LIB_SYNCTHING_MODEL_EXPORT SyncthingFileModel : public SyncthingModel {
    Q_OBJECT
    Q_PROPERTY(bool selectionModeEnabled READ isSelectionModeEnabled WRITE setSelectionModeEnabled)

public:
    enum SyncthingFileModelRole {
        NameRole = SyncthingModelUserRole + 1,
        SizeRole,
        ModificationTimeRole,
        PathRole,
        Actions,
        ActionNames,
        ActionIcons
    };

    explicit SyncthingFileModel(SyncthingConnection &connection, const SyncthingDir &dir, QObject *parent = nullptr);
    ~SyncthingFileModel() override;

public Q_SLOTS:
    QHash<int, QByteArray> roleNames() const override;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex index(const QString &path) const;
    QModelIndex parent(const QModelIndex &child) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;
    bool canFetchMore(const QModelIndex &parent) const override;
    void fetchMore(const QModelIndex &parent) override;
    void triggerAction(const QString &action, const QModelIndex &index);
    QList<QAction *> selectionActions();
    bool isSelectionModeEnabled() const;
    void setSelectionModeEnabled(bool selectionModeEnabled);

public:
    QString path(const QModelIndex &path) const;

Q_SIGNALS:
    void fetchQueueEmpty();

private Q_SLOTS:
    void handleConfigInvalidated() override;
    void handleNewConfigAvailable() override;
    void handleForkAwesomeIconsChanged() override;
    void handleBrightColorsChanged() override;
    void handleLocalLookupFinished();

private:
    void setCheckState(const QModelIndex &index, Qt::CheckState checkState, bool skipChildren = false);
    void processFetchQueue(const QString &lastItemPath = QString());
    void queryIgnores();
    void matchItemAgainstIgnorePatterns(SyncthingItem &item) const;

private:
    using SyncthingItems = std::vector<std::unique_ptr<SyncthingItem>>;
    using LocalLookupRes = std::shared_ptr<std::map<QString, SyncthingItem>>;
    struct QueryResult : SyncthingConnection::QueryResult {
        QString forPath;
        QFuture<LocalLookupRes> localLookup;
        QPersistentModelIndex refreshedIndex;
        QueryResult &operator=(SyncthingConnection::QueryResult &&);
    };

    SyncthingConnection &m_connection;
    QString m_dirId;
    QString m_localPath;
    std::vector<SyncthingIgnorePattern> m_presentIgnorePatterns;
    QStringList m_fetchQueue;
    SyncthingConnection::QueryResult m_ignorePatternsRequest;
    QueryResult m_pendingRequest;
    QFutureWatcher<LocalLookupRes> m_localItemLookup;
    std::unique_ptr<SyncthingItem> m_root;
    QChar m_pathSeparator;
    bool m_selectionMode;
    bool m_hasIgnorePatterns;
};

inline bool SyncthingFileModel::isSelectionModeEnabled() const
{
    return m_selectionMode;
}

} // namespace Data

#endif // DATA_SYNCTHINGFILEMODEL_H
