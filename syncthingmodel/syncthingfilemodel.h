#ifndef DATA_SYNCTHINGFILEMODEL_H
#define DATA_SYNCTHINGFILEMODEL_H

#include "./syncthingmodel.h"

#include <syncthingconnector/syncthingconnection.h>
#include <syncthingconnector/syncthingignorepattern.h>

#include <QAction>
#include <QFuture>
#include <QFutureWatcher>
#include <QHash>
#include <QPixmap>
#include <QSet>

#include <map>
#include <memory>
#include <optional>
#include <vector>

class ModelTests;

namespace Data {

class LIB_SYNCTHING_MODEL_EXPORT RejectableAction : public QAction {
    Q_OBJECT
    Q_PROPERTY(bool needsConfirmation MEMBER needsConfirmation)

public:
    explicit RejectableAction(const QString &text, QObject *parent = nullptr)
        : QAction(text, parent)
    {
    }

    Q_INVOKABLE void dismiss()
    {
        needsConfirmation = true;
    }
    bool needsConfirmation = true;
};

class LIB_SYNCTHING_MODEL_EXPORT SyncthingFileModel : public SyncthingModel {
    Q_OBJECT
    Q_PROPERTY(bool hasIgnorePatterns READ hasIgnorePatterns)
    Q_PROPERTY(bool selectionModeEnabled READ isSelectionModeEnabled WRITE setSelectionModeEnabled NOTIFY selectionModeEnabledChanged)
    Q_PROPERTY(bool recursiveSelectionEnabled READ isRecursiveSelectionEnabled WRITE setRecursiveSelectionEnabled)
    Q_PROPERTY(QList<QAction *> selectionActions READ selectionActions NOTIFY selectionActionsChanged)
    Q_PROPERTY(bool hasStagedChanges READ hasStagedChanges NOTIFY hasStagedChangesChanged)

public:
    friend class ::ModelTests;

    enum SyncthingFileModelRole {
        NameRole = SyncthingModelUserRole + 1,
        SizeRole,
        ModificationTimeRole,
        PathRole,
        Actions,
        ActionNames,
        ActionIcons,
        DetailsRole,
        CheckableRole,
    };

    explicit SyncthingFileModel(SyncthingConnection &connection, const SyncthingDir &dir, QObject *parent = nullptr);
    ~SyncthingFileModel() override;

public:
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
    Q_INVOKABLE void triggerAction(const QString &action, const QModelIndex &index);
    Q_INVOKABLE QList<QAction *> selectionActions();
    bool isSelectionModeEnabled() const;
    void setSelectionModeEnabled(bool selectionModeEnabled);
    Q_INVOKABLE QString path(const QModelIndex &path) const;
    bool hasIgnorePatterns() const;
    bool hasStagedChanges() const;
    const std::vector<SyncthingIgnorePattern> &presentIgnorePatterns() const;
    SyncthingIgnores computeNewIgnorePatterns() const;
    Q_INVOKABLE void editIgnorePatternsManually(const QString &ignorePatterns);
    Q_INVOKABLE void editLocalDeletions(const QSet<QString> &localDeletions);
    Q_INVOKABLE void editLocalDeletionsFromVariantList(const QVariantList &localDeletions);
    bool isRecursiveSelectionEnabled() const;
    void setRecursiveSelectionEnabled(bool recursiveSelectionEnabled);

Q_SIGNALS:
    void fetchQueueEmpty();
    void notification(const QString &type, const QString &message, const QString &details = QString());
    void actionNeedsConfirmation(
        QAction *action, const QString &message, const QString &diff = QString(), const QSet<QString> &localDeletions = QSet<QString>());
    void selectionModeEnabledChanged(bool selectionModeEnabled);
    void selectionActionsChanged();
    void hasStagedChangesChanged(bool hasStagedChanged);

private Q_SLOTS:
    void handleConfigInvalidated() override;
    void handleNewConfigAvailable() override;
    void handleForkAwesomeIconsChanged() override;
    void handleBrightColorsChanged() override;
    void handleLocalLookupFinished();

private:
    void setCheckState(const QModelIndex &index, Qt::CheckState checkState, bool recursively = false);
    void processFetchQueue(const QString &lastItemPath = QString());
    void queryIgnores();
    void resetMatchingIgnorePatterns();
    void matchItemAgainstIgnorePatterns(SyncthingItem &item) const;
    void ignoreSelectedItems(bool ignore = true, bool deleteLocally = false);
    QString computeIgnorePatternDiff();
    QString availabilityNote(const SyncthingItem *item) const;

private:
    using SyncthingItems = std::vector<std::unique_ptr<SyncthingItem>>;
    struct LocalItem;
    using LocalItemMap = std::map<QString, LocalItem>;
    struct LocalItem : public SyncthingItem {
        LocalItemMap localChildren;
        bool localChildrenPopulated = false;
    };
    using LocalLookupRes = std::shared_ptr<LocalItemMap>;
    static void lookupDirLocally(const QDir &dir, SyncthingFileModel::LocalItemMap &items, int depth = 1);
    static void markItemsFromDatabaseAsLocallyExisting(
        std::vector<std::unique_ptr<SyncthingItem>> &items, SyncthingFileModel::LocalItemMap &localItems);
    void insertLocalItems(const QModelIndex &refreshedIndex, SyncthingFileModel::LocalItemMap &localItems);
    struct QueryResult : SyncthingConnection::QueryResult {
        QString forPath;
        QFuture<LocalLookupRes> localLookup;
        QPersistentModelIndex refreshedIndex;
        QueryResult &operator=(SyncthingConnection::QueryResult &&);
    };
    struct Change {
        QStringList prepend;
        QStringList append;
        bool replace = false;
    };

    SyncthingConnection &m_connection;
    QString m_dirId;
    QString m_localPath;
    std::vector<SyncthingIgnorePattern> m_presentIgnorePatterns;
    QHash<std::size_t, Change> m_stagedChanges;
    QSet<QString> m_stagedLocalFileDeletions;
    QStringList m_fetchQueue;
    SyncthingConnection::QueryResult m_ignorePatternsRequest;
    QueryResult m_pendingRequest;
    QFutureWatcher<LocalLookupRes> m_localItemLookup;
    std::unique_ptr<SyncthingItem> m_root;
    QString m_manuallyEditedIgnorePatterns;
    std::optional<QSet<QString>> m_manuallyEditedLocalDeletions;
    QString m_ignoreAllByDefaultPattern;
    QChar m_pathSeparator;
    mutable QPixmap m_statusIcons[4];
    int m_columns;
    bool m_selectionMode;
    bool m_hasIgnorePatterns;
    bool m_isIgnoringAllByDefault;
    bool m_recursiveSelectionEnabled;
};

inline bool SyncthingFileModel::isSelectionModeEnabled() const
{
    return m_selectionMode;
}

inline bool SyncthingFileModel::hasIgnorePatterns() const
{
    return m_hasIgnorePatterns;
}

inline bool SyncthingFileModel::hasStagedChanges() const
{
    return !m_stagedChanges.isEmpty();
}

inline const std::vector<SyncthingIgnorePattern> &SyncthingFileModel::presentIgnorePatterns() const
{
    return m_presentIgnorePatterns;
}

inline bool SyncthingFileModel::isRecursiveSelectionEnabled() const
{
    return m_recursiveSelectionEnabled;
}

inline void SyncthingFileModel::setRecursiveSelectionEnabled(bool recursiveSelectionEnabled)
{
    m_recursiveSelectionEnabled = recursiveSelectionEnabled;
}

} // namespace Data

#endif // DATA_SYNCTHINGFILEMODEL_H
