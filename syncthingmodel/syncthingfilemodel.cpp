#include "./syncthingfilemodel.h"
#include "./colors.h"
#include "./syncthingicons.h"

#include <syncthingconnector/syncthingconnection.h>
#include <syncthingconnector/utils.h>

#include <qtutilities/misc/desktoputils.h>

#include <c++utilities/conversion/stringconversion.h>

#include <QAction>
#include <QClipboard>
#include <QGuiApplication>
#include <QNetworkReply>
#include <QStringBuilder>
#include <QtConcurrent>

#include <limits>

using namespace std;
using namespace CppUtilities;

namespace Data {

static constexpr auto beforeFirstLine = std::numeric_limits<std::size_t>::max();

/// \cond
static void populatePath(const QString &root, QChar pathSeparator, std::vector<std::unique_ptr<SyncthingItem>> &items)
{
    if (root.isEmpty()) {
        for (auto &item : items) {
            populatePath(item->path = item->name, pathSeparator, item->children);
        }
    } else {
        for (auto &item : items) {
            populatePath(item->path = root % pathSeparator % item->name, pathSeparator, item->children);
        }
    }
}

static void addErrorItem(std::vector<std::unique_ptr<SyncthingItem>> &items, QString &&errorMessage)
{
    if (errorMessage.isEmpty()) {
        return;
    }
    auto &errorItem = items.emplace_back(std::make_unique<SyncthingItem>());
    errorItem->name = std::move(errorMessage);
    errorItem->type = SyncthingItemType::Error;
    errorItem->childrenPopulated = true;
}

static void addLoadingItem(std::vector<std::unique_ptr<SyncthingItem>> &items)
{
    if (!items.empty()) {
        return;
    }
    auto &loadingItem = items.emplace_back(std::make_unique<SyncthingItem>());
    loadingItem->name = QStringLiteral("Loading…");
    loadingItem->type = SyncthingItemType::Loading;
    loadingItem->childrenPopulated = true;
}
/// \endcond

SyncthingFileModel::SyncthingFileModel(SyncthingConnection &connection, const SyncthingDir &dir, QObject *parent)
    : SyncthingModel(connection, parent)
    , m_connection(connection)
    , m_dirId(dir.id)
    , m_root(std::make_unique<SyncthingItem>())
    , m_selectionMode(false)
    , m_hasIgnorePatterns(false)
{
    if (m_connection.isLocal()) {
        m_localPath = dir.pathWithoutTrailingSlash().toString();
        connect(&m_localItemLookup, &QFutureWatcherBase::finished, this, &SyncthingFileModel::handleLocalLookupFinished);
    }
    m_pathSeparator = m_connection.pathSeparator().size() == 1 ? m_connection.pathSeparator().front() : QDir::separator();
    m_root->name = dir.displayName();
    m_root->modificationTime = dir.lastFileTime;
    m_root->size = dir.globalStats.bytes;
    m_root->type = SyncthingItemType::Directory;
    m_root->path = QStringLiteral(""); // assign an empty QString that is not null
    m_fetchQueue.append(m_root->path);
    queryIgnores();
    processFetchQueue();
}

SyncthingFileModel::~SyncthingFileModel()
{
    QObject::disconnect(m_pendingRequest.connection);
    QObject::disconnect(m_ignorePatternsRequest.connection);
    delete m_pendingRequest.reply;
    delete m_ignorePatternsRequest.reply;
}

QHash<int, QByteArray> SyncthingFileModel::roleNames() const
{
    const static auto roles = QHash<int, QByteArray>{
        { NameRole, "name" },
        { SizeRole, "size" },
        { ModificationTimeRole, "modificationTime" },
        { PathRole, "path" },
        { Actions, "actions" },
        { ActionNames, "actionNames" },
        { ActionIcons, "actionIcons" },
    };
    return roles;
}

QModelIndex SyncthingFileModel::index(int row, int column, const QModelIndex &parent) const
{
    if (row < 0 || column < 0 || column > 3 || parent.column() > 0) {
        return QModelIndex();
    }
    if (!parent.isValid()) {
        return static_cast<std::size_t>(row) ? QModelIndex() : createIndex(row, column, m_root.get());
    }
    auto *const parentItem = reinterpret_cast<SyncthingItem *>(parent.internalPointer());
    if (!parentItem) {
        return QModelIndex();
    }
    auto &items = parentItem->children;
    if (static_cast<std::size_t>(row) >= items.size()) {
        return QModelIndex();
    }
    auto &item = items[static_cast<std::size_t>(row)];
    item->parent = parentItem;
    return createIndex(row, column, item.get());
}

QModelIndex SyncthingFileModel::index(const QString &path) const
{
    auto parts = path.split(m_pathSeparator, Qt::SkipEmptyParts);
    auto *parent = m_root.get();
    auto res = createIndex(0, 0, parent);
    for (const auto &part : parts) {
        auto index = 0;
        for (const auto &child : parent->children) {
            if (child->name == part) {
                child->parent = parent;
                parent = child.get();
                res = createIndex(index, 0, parent);
                index = -1;
                break;
            }
            ++index;
        }
        if (index >= 0) {
            res = QModelIndex();
            return res;
        }
    }
    return res;
}

QString SyncthingFileModel::path(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return QString();
    }
    auto *item = reinterpret_cast<SyncthingItem *>(index.internalPointer());
    return item->isFilesystemItem() ? item->path : QString();
}

/*!
 * \brief Computes a diff between the present ignore patterns and staged changes.
 */
QString SyncthingFileModel::computeIgnorePatternDiff() const
{
    auto diff = QString();
    auto index = std::size_t();
    const auto appendNewLines = [&diff] (const auto &change) {
        for (const auto &line : change->newLines) {
            diff.append(QChar('+'));
            diff.append(line);
            diff.append(QChar('\n'));
        }
    };
    if (const auto change = m_stagedChanges.find(beforeFirstLine); change != m_stagedChanges.end()) {
        appendNewLines(change);
    }
    for (const auto &pattern : m_presentIgnorePatterns) {
        auto change = m_stagedChanges.find(index++);
        diff.append(change == m_stagedChanges.end() || change->append ? QChar(' ') : QChar('-'));
        diff.append(pattern.pattern);
        diff.append(QChar('\n'));
        if (change != m_stagedChanges.end()) {
            appendNewLines(change);
        }
    }
    return diff;
}

/*!
 * \brief Computes new ignore patterns based on the present ignore patterns and staged changes.
 */
SyncthingIgnores SyncthingFileModel::computeNewIgnorePatterns() const
{
    auto newIgnorePatterns = SyncthingIgnores();
    auto index = std::size_t();
    if (const auto change = m_stagedChanges.find(beforeFirstLine); change != m_stagedChanges.end()) {
        for (const auto &line : change->newLines) {
            newIgnorePatterns.ignore.append(line);
        }
    }
    for (const auto &pattern : m_presentIgnorePatterns) {
        auto change = m_stagedChanges.find(index++);
        if (change == m_stagedChanges.end() || change->append) {
            newIgnorePatterns.ignore.append(pattern.pattern);
        }
        if (change == m_stagedChanges.end()) {
            continue;
        }
        for (const auto &line : change->newLines) {
            newIgnorePatterns.ignore.append(line);
        }
    }
    return newIgnorePatterns;
}

QModelIndex SyncthingFileModel::parent(const QModelIndex &child) const
{
    if (!child.isValid()) {
        return QModelIndex();
    }
    auto *const childItem = reinterpret_cast<SyncthingItem *>(child.internalPointer());
    if (!childItem) {
        return QModelIndex();
    }
    return !childItem->parent ? QModelIndex() : createIndex(static_cast<int>(childItem->parent->index), 0, childItem->parent);
}

QVariant SyncthingFileModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    switch (orientation) {
    case Qt::Horizontal:
        switch (role) {
        case Qt::DisplayRole:
            switch (section) {
            case 0:
                return tr("Name");
            case 1:
                return tr("Size");
            case 2:
                return tr("Last modified");
            case 3:
                return tr("Ignore pattern");
            }
            break;
        default:;
        }
        break;
    default:;
    }
    return QVariant();
}

Qt::ItemFlags SyncthingFileModel::flags(const QModelIndex &index) const
{
    auto f = QAbstractItemModel::flags(index);
    if (index.isValid()) {
        const auto *const item = reinterpret_cast<SyncthingItem *>(index.internalPointer());
        switch (item->type) {
        case SyncthingItemType::File:
        case SyncthingItemType::Symlink:
        case SyncthingItemType::Error:
        case SyncthingItemType::Loading:
            f |= Qt::ItemNeverHasChildren;
            break;
        default:;
        }
    }
    if (m_selectionMode) {
        f |= Qt::ItemIsUserCheckable;
    }
    return f;
}

QVariant SyncthingFileModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }
    auto *const item = reinterpret_cast<SyncthingItem *>(index.internalPointer());
    switch (role) {
    case Qt::DisplayRole:
        switch (index.column()) {
        case 0:
            return item->name;
        case 1:
            switch (item->type) {
            case SyncthingItemType::File:
                return QString::fromStdString(CppUtilities::dataSizeToString(item->size));
            case SyncthingItemType::Directory:
                return item->childrenPopulated ? tr("%1 elements").arg(item->children.size()) : QString();
            default:
                return QString();
            }
        case 2:
            switch (item->type) {
            case SyncthingItemType::File:
            case SyncthingItemType::Directory:
                return QString::fromStdString(item->modificationTime.toString());
            default:
                return QString();
            }
        case 3:
            if (item->ignorePattern == SyncthingItem::ignorePatternNotInitialized) {
                matchItemAgainstIgnorePatterns(*item);
            }
            if (item->ignorePattern < m_presentIgnorePatterns.size()) {
                return m_presentIgnorePatterns[item->ignorePattern].pattern;
            }
        }
        break;
    case Qt::CheckStateRole:
        if (!m_selectionMode) {
            return QVariant();
        }
        switch (index.column()) {
        case 0:
            return QVariant(item->checked);
        }
        break;
    case Qt::DecorationRole: {
        const auto &icons = commonForkAwesomeIcons();
        switch (index.column()) {
        case 0:
            switch (item->type) {
            case SyncthingItemType::File:
                return icons.file;
            case SyncthingItemType::Directory:
                return icons.folder;
            case SyncthingItemType::Symlink:
                return icons.link;
            case SyncthingItemType::Error:
                return icons.exclamationTriangle;
            default:
                return icons.cogs;
            }
        }
        break;
    }
    case Qt::ForegroundRole:
        if (!item->existsInDb) {
            return Colors::gray(m_brightColors);
        }
        break;
    case Qt::ToolTipRole:
        switch (index.column()) {
        case 0:
            return item->isFilesystemItem() ? item->path : item->name;
        case 2:
            return agoString(item->modificationTime);
        }
        break;
    case NameRole:
        return item->name;
    case SizeRole:
        return static_cast<qsizetype>(item->size);
    case ModificationTimeRole:
        return QString::fromStdString(item->modificationTime.toString());
    case PathRole:
        return item->isFilesystemItem() ? item->path : QString();
    case Actions: {
        auto res = QStringList();
        res.reserve(3);
        if (item->type == SyncthingItemType::Directory) {
            res << QStringLiteral("refresh");
        }
        res << QStringLiteral("toggle-selection-recursively") << QStringLiteral("toggle-selection-single");
        if (!m_localPath.isEmpty() && item->isFilesystemItem()) {
            res << QStringLiteral("open") << QStringLiteral("copy-path");
        }
        return res;
    }
    case ActionNames: {
        auto res = QStringList();
        res.reserve(3);
        if (item->type == SyncthingItemType::Directory) {
            res << tr("Refresh");
        }
        res << (item->checked == Qt::Checked ? tr("Deselect recursively") : tr("Select recursively"));
        res << (item->checked == Qt::Checked ? tr("Deselect single item") : tr("Select single item"));
        if (!m_localPath.isEmpty() && item->isFilesystemItem()) {
            res << (item->type == SyncthingItemType::Directory ? tr("Browse locally") : tr("Open local version")) << tr("Copy local path");
        }
        return res;
    }
    case ActionIcons: {
        auto res = QVariantList();
        res.reserve(3);
        if (item->type == SyncthingItemType::Directory) {
            res << QIcon::fromTheme(QStringLiteral("view-refresh"), QIcon(QStringLiteral(":/icons/hicolor/scalable/actions/view-refresh.svg")));
        }
        res << QIcon::fromTheme(QStringLiteral("edit-select"));
        res << res.back();
        if (!m_localPath.isEmpty() && item->isFilesystemItem()) {
            res << QIcon::fromTheme(QStringLiteral("folder"), QIcon(QStringLiteral(":/icons/hicolor/scalable/places/folder-open.svg")));
            res << QIcon::fromTheme(QStringLiteral("edit-copy"), QIcon(QStringLiteral(":/icons/hicolor/scalable/places/edit-copy.svg")));
        }
        return res;
    }
    }
    return QVariant();
}

bool SyncthingFileModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid()) {
        return false;
    }
    switch (role) {
    case Qt::CheckStateRole:
        setCheckState(index, static_cast<Qt::CheckState>(value.toInt()));
        return true;
    }
    return false;
}

/// \brief Sets the whether the children of the specified \a item are checked.
static void setChildrenChecked(SyncthingItem *item, Qt::CheckState checkState)
{
    for (auto &childItem : item->children) {
        setChildrenChecked(childItem.get(), childItem->checked = checkState);
    }
}

/// \brief Sets the check state of the specified \a index updating child and parent indexes accordingly.
void SyncthingFileModel::setCheckState(const QModelIndex &index, Qt::CheckState checkState, bool recursively)
{
    static const auto roles = QVector<int>{ Qt::CheckStateRole };
    auto *const item = reinterpret_cast<SyncthingItem *>(index.internalPointer());
    auto affectedParentIndex = index;
    item->checked = checkState;

    if (recursively) {
        // set the checked state of child items as well
        if (checkState != Qt::PartiallyChecked) {
            setChildrenChecked(item, checkState);
        }

        // update the checked state of parent items accordingly
        for (auto *parentItem = item->parent; parentItem; parentItem = parentItem->parent) {
            auto hasUncheckedSiblings = false;
            auto hasCheckedSiblings = false;
            for (auto &siblingItem : parentItem->children) {
                switch (siblingItem->checked) {
                case Qt::Unchecked:
                    hasUncheckedSiblings = true;
                    break;
                case Qt::PartiallyChecked:
                    hasUncheckedSiblings = hasCheckedSiblings = true;
                    break;
                case Qt::Checked:
                    hasCheckedSiblings = true;
                }
                if (hasUncheckedSiblings && hasCheckedSiblings) {
                    break;
                }
            }
            auto parentChecked
                = hasUncheckedSiblings && hasCheckedSiblings ? Qt::PartiallyChecked : (hasUncheckedSiblings ? Qt::Unchecked : Qt::Checked);
            if (parentItem->checked == parentChecked) {
                break;
            }
            parentItem->checked = parentChecked;
            affectedParentIndex = createIndex(static_cast<int>(parentItem->index), 0, parentItem);
        }
    }

    // emit dataChanged() events
    if (m_selectionMode) {
        emit dataChanged(affectedParentIndex, index, roles);
        invalidateAllIndicies(roles, affectedParentIndex);
    }
}

int SyncthingFileModel::rowCount(const QModelIndex &parent) const
{
    auto res = std::size_t();
    if (!parent.isValid()) {
        res = 1;
    } else {
        auto *const parentItem = reinterpret_cast<SyncthingItem *>(parent.internalPointer());
        res = parentItem->childrenPopulated || parentItem->type != SyncthingItemType::Directory ? parentItem->children.size() : 1;
    }
    return res < std::numeric_limits<int>::max() ? static_cast<int>(res) : std::numeric_limits<int>::max();
}

int SyncthingFileModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return 4;
}

bool SyncthingFileModel::canFetchMore(const QModelIndex &parent) const
{
    if (!parent.isValid()) {
        return false;
    }
    auto *const parentItem = reinterpret_cast<SyncthingItem *>(parent.internalPointer());
    return !parentItem->childrenPopulated && parentItem->type == SyncthingItemType::Directory;
}

void SyncthingFileModel::fetchMore(const QModelIndex &parent)
{
    const auto parentPath = path(parent);
    if (parentPath.isNull() || m_fetchQueue.contains(parentPath)) {
        return;
    }
    m_fetchQueue.append(parentPath);
    if (m_fetchQueue.size() == 1) {
        processFetchQueue();
    }
}

void SyncthingFileModel::triggerAction(const QString &action, const QModelIndex &index)
{
    if (action == QLatin1String("refresh")) {
        fetchMore(index);
        return;
    } else if (action.startsWith(QLatin1String("toggle-selection-"))) {
        auto *const item = static_cast<SyncthingItem *>(index.internalPointer());
        setSelectionModeEnabled(true);
        setCheckState(index, item->checked != Qt::Checked ? Qt::Checked : Qt::Unchecked, action == QLatin1String("toggle-selection-recursively"));
    }
    if (m_localPath.isEmpty()) {
        return;
    }
    const auto relPath = index.data(PathRole).toString();
    const auto path = relPath.isEmpty() ? m_localPath : QString(m_localPath % m_pathSeparator % relPath);
    if (action == QLatin1String("open")) {
        QtUtilities::openLocalFileOrDir(path);
    } else if (action == QLatin1String("copy-path")) {
        if (auto *const clipboard = QGuiApplication::clipboard()) {
            clipboard->setText(path);
        }
    }
}

/// \brief Invokes \a callback for every filesystem item as of and including \a root.
/// \remarks Traverses children if \a callback returns true.
template <typename Callback> static void forEachItem(SyncthingItem *root, Callback &&callback)
{
    if (!root->isFilesystemItem() || !callback(root) || !root->childrenPopulated) {
        return;
    }
    for (auto &child : root->children) {
        forEachItem(child.get(), std::forward<Callback>(callback));
    }
}

QList<QAction *> SyncthingFileModel::selectionActions()
{
    auto res = QList<QAction *>();
    if (!m_selectionMode) {
        return res;
    }
    auto *const discardAction = new QAction(tr("Discard selection and staged changes"), this);
    discardAction->setIcon(QIcon::fromTheme(QStringLiteral("edit-undo")));
    connect(discardAction, &QAction::triggered, this, [this] {
        if (const auto rootIndex = index(0, 0); rootIndex.isValid()) {
            setCheckState(index(0, 0), Qt::Unchecked);
        }
        setSelectionModeEnabled(false);
        m_stagedChanges.clear();
        m_stagedLocalFileDeletions.clear();
    });
    res << discardAction;

    auto *const removeIgnorePatternsAction = new QAction(tr("Remove related ignore patterns"), this);
    removeIgnorePatternsAction->setIcon(QIcon::fromTheme(QStringLiteral("edit-delete")));
    connect(removeIgnorePatternsAction, &QAction::triggered, this, [this]() {
        forEachItem(m_root.get(), [this](SyncthingItem *item) {
            if (item->checked != Qt::Checked) {
                return true;
            }
            if (item->ignorePattern == SyncthingItem::ignorePatternNotInitialized) {
                matchItemAgainstIgnorePatterns(*item);
            }
            if (item->ignorePattern != SyncthingItem::ignorePatternNoMatch) {
                m_stagedChanges[item->ignorePattern];
            }
            return true;
        });
    });
    res << removeIgnorePatternsAction;

    if (!m_stagedChanges.isEmpty() || !m_stagedLocalFileDeletions.isEmpty()) {
        auto *const applyStagedChangesAction = new QAction(tr("Review and apply staged changes"), this);
        applyStagedChangesAction->setIcon(QIcon::fromTheme(QStringLiteral("dialog-ok-apply")));
        connect(applyStagedChangesAction, &QAction::triggered, this, [this, action = applyStagedChangesAction, askedConfirmation = false]() mutable {
            // allow user to review changes before applying them
            if (!askedConfirmation) {
                askedConfirmation = true;
                emit actionNeedsConfirmation(action, tr("Do you want to apply the folliwng changes?"), computeIgnorePatternDiff());
                return;
            }

            // abort if there's a pending API request for ignore patterns
            if (m_ignorePatternsRequest.reply) {
                emit notification(
                    QStringLiteral("error"), tr("Cannot apply ignore patterns while a previous request for ignore patterns is still pending."));
                return;
            }

            // post new ignore patterns via Syncthing API
            m_ignorePatternsRequest = m_connection.setIgnores(m_dirId, computeNewIgnorePatterns(), [this](const QString &error) {
                m_ignorePatternsRequest.reply = nullptr;
                if (!error.isEmpty()) {
                    emit notification(QStringLiteral("error"), tr("Unable to change ignore patterns:\n%1").arg(error));
                    return;
                }

                // reset state and query ignore patterns again on success so matching ignore patterns are updated
                m_stagedChanges.clear();
                m_stagedLocalFileDeletions.clear();
                m_hasIgnorePatterns = false;
                forEachItem(m_root.get(), [](SyncthingItem *item) {
                    item->ignorePattern = SyncthingItem::ignorePatternNotInitialized;
                    return true;
                });
                queryIgnores();

                emit notification(QStringLiteral("info"), tr("Ignore patterns have been changed."));
            });
        });
        res << applyStagedChangesAction;
    }

    return res;
}

void SyncthingFileModel::setSelectionModeEnabled(bool selectionModeEnabled)
{
    if (m_selectionMode != selectionModeEnabled) {
        m_selectionMode = selectionModeEnabled;
        invalidateAllIndicies(QVector<int>{ Qt::CheckStateRole });
    }
}

void SyncthingFileModel::handleConfigInvalidated()
{
}

void SyncthingFileModel::handleNewConfigAvailable()
{
}

void SyncthingFileModel::handleForkAwesomeIconsChanged()
{
    invalidateAllIndicies(QVector<int>({ Qt::DecorationRole }));
}

void SyncthingFileModel::handleBrightColorsChanged()
{
    invalidateAllIndicies(QVector<int>({ Qt::ForegroundRole }));
}

void SyncthingFileModel::processFetchQueue(const QString &lastItemPath)
{
    if (!lastItemPath.isNull()) {
        m_fetchQueue.removeAll(lastItemPath);
    }
    if (m_fetchQueue.isEmpty()) {
        emit fetchQueueEmpty();
        return;
    }
    const auto &path = m_fetchQueue.front();
    const auto rootIndex = index(path);
    if (!rootIndex.isValid()) {
        processFetchQueue(path);
        return;
    }

    // add loading item if there are items yet at all
    auto *rootItem = reinterpret_cast<SyncthingItem *>(rootIndex.internalPointer());
    if (rootItem->children.empty()) {
        beginInsertRows(rootIndex, 0, 0);
        addLoadingItem(rootItem->children);
        endInsertRows();
    }

    // query directory entries from Syncthing database
    if (rootItem->existsInDb) {
        m_pendingRequest
            = m_connection.browse(m_dirId, path, 1, [this](std::vector<std::unique_ptr<SyncthingItem>> &&items, QString &&errorMessage) mutable {
                  m_pendingRequest.reply = nullptr;
                  addErrorItem(items, std::move(errorMessage));

                  const auto refreshedIndex = index(m_pendingRequest.forPath);
                  if (!refreshedIndex.isValid()) {
                      processFetchQueue(m_pendingRequest.forPath);
                      return;
                  }
                  auto *const refreshedItem = reinterpret_cast<SyncthingItem *>(refreshedIndex.internalPointer());
                  const auto previousChildCount = refreshedItem->children.size();
                  if (previousChildCount) {
                      beginRemoveRows(refreshedIndex, 0, static_cast<int>(refreshedItem->children.size() - 1));
                      refreshedItem->children.clear();
                      endRemoveRows();
                  }
                  if (!items.empty()) {
                      const auto last = items.size() - 1;
                      for (auto &item : items) {
                          item->parent = refreshedItem;
                      }
                      populatePath(refreshedItem->path, m_pathSeparator, items);
                      beginInsertRows(
                          refreshedIndex, 0, last < std::numeric_limits<int>::max() ? static_cast<int>(last) : std::numeric_limits<int>::max());
                      refreshedItem->children = std::move(items);
                      refreshedItem->childrenPopulated = true;
                      switch (refreshedItem->checked) {
                      case Qt::Checked:
                          setChildrenChecked(refreshedItem, Qt::Checked);
                          break;
                      case Qt::PartiallyChecked:
                          setCheckState(refreshedIndex, Qt::Unchecked, false);
                          break;
                      default:;
                      }
                      endInsertRows();
                  }
                  if (refreshedItem->children.size() != previousChildCount) {
                      const auto sizeIndex = refreshedIndex.siblingAtColumn(1);
                      emit dataChanged(sizeIndex, sizeIndex, QVector<int>{ Qt::DisplayRole });
                  }
                  if (!m_pendingRequest.localLookup.isCanceled()) {
                      m_pendingRequest.refreshedIndex = refreshedIndex;
                      m_localItemLookup.setFuture(m_pendingRequest.localLookup);
                  } else {
                      processFetchQueue(m_pendingRequest.forPath);
                  }
              });
    } else {
        m_pendingRequest = SyncthingConnection::QueryResult();
    }
    m_pendingRequest.forPath = path;

    // lookup the directory entries locally to also show ignored files
    if (m_localPath.isEmpty()) {
        return;
    }
    m_pendingRequest.localLookup = QtConcurrent::run([dir = QDir(m_localPath % m_pathSeparator % path)] {
        auto items = std::make_shared<std::map<QString, SyncthingItem>>();
        auto entries = dir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot);
        for (const auto &entry : entries) {
            const auto entryName = entry.fileName();
            auto &item = (*items)[entryName];
            item.name = entryName;
            item.existsInDb = false;
            item.existsLocally = true;
            item.size = static_cast<std::size_t>(entry.size());
            item.modificationTime = DateTime::unixEpochStart() + TimeSpan(TimeSpan::ticksPerMillisecond * entry.lastModified().toMSecsSinceEpoch());
            if (entry.isSymbolicLink()) {
                item.type = SyncthingItemType::Symlink;
            } else if (entry.isDir()) {
                item.type = SyncthingItemType::Directory;
            } else {
                item.type = SyncthingItemType::File;
            }
        }
        return items;
    });
    if (!rootItem->existsInDb) {
        m_pendingRequest.refreshedIndex = rootIndex;
        m_localItemLookup.setFuture(m_pendingRequest.localLookup);
    }
}

void SyncthingFileModel::queryIgnores()
{
    if (m_ignorePatternsRequest.reply) {
        emit notification(QStringLiteral("error"), tr("Cannot query ignore patterns while a previous request for ignore patterns is still pending."));
        return;
    }
    m_ignorePatternsRequest = m_connection.ignores(m_dirId, [this](SyncthingIgnores &&ignores, QString &&errorMessage) {
        m_ignorePatternsRequest.reply = nullptr;
        m_hasIgnorePatterns = errorMessage.isEmpty();
        m_presentIgnorePatterns.clear();
        if (!m_hasIgnorePatterns) {
            return;
        }
        m_presentIgnorePatterns.reserve(static_cast<std::size_t>(ignores.ignore.size()));
        for (auto &ignorePattern : ignores.ignore) {
            m_presentIgnorePatterns.emplace_back(std::move(ignorePattern));
        }
        invalidateAllIndicies(QVector<int>{ Qt::DisplayRole }, 3, QModelIndex());
    });
}

void SyncthingFileModel::matchItemAgainstIgnorePatterns(SyncthingItem &item) const
{
    if (!m_hasIgnorePatterns) {
        item.ignorePattern = SyncthingItem::ignorePatternNotInitialized;
        return;
    }
    item.ignorePattern = SyncthingItem::ignorePatternNoMatch;
    if (!item.isFilesystemItem()) {
        return;
    }
    auto index = std::size_t();
    for (const auto &ignorePattern : m_presentIgnorePatterns) {
        if (ignorePattern.matches(item.path, m_pathSeparator)) {
            item.ignorePattern = index;
            break;
        } else {
            ++index;
        }
    }
}

void SyncthingFileModel::handleLocalLookupFinished()
{
    // get refreshed index/item
    const auto &refreshedIndex = m_pendingRequest.refreshedIndex;
    if (!refreshedIndex.isValid()) {
        processFetchQueue(m_pendingRequest.forPath);
        return;
    }
    auto *const refreshedItem = reinterpret_cast<SyncthingItem *>(refreshedIndex.internalPointer());
    auto &items = refreshedItem->children;
    const auto previousChildCount = items.size();
    refreshedItem->childrenPopulated = true;

    // clear loading item
    if (!refreshedItem->existsInDb && !items.empty()) {
        const auto last = items.size() - 1;
        beginRemoveRows(refreshedIndex, 0, last < std::numeric_limits<int>::max() ? static_cast<int>(last) : std::numeric_limits<int>::max());
        items.clear();
        endRemoveRows();
    }

    // get result from local lookup
    auto res = m_pendingRequest.localLookup.result();
    if (!res || res->empty()) {
        processFetchQueue(m_pendingRequest.forPath);
        return;
    }

    // mark items from the database query as locally existing if they do; mark items from local lookup as existing in the db if they do
    auto &localItems = *res;
    for (auto &child : items) {
        auto localItemIter = localItems.find(child->name);
        if (localItemIter == localItems.end()) {
            continue;
        }
        child->existsLocally = true;
        localItemIter->second.existsInDb = true;
    }

    // insert items from local lookup that are not already present via the database query (probably ignored files)
    for (auto &[localItemName, localItem] : localItems) {
        if (localItem.existsInDb) {
            continue;
        }
        const auto last = items.size();
        const auto index = last < std::numeric_limits<int>::max() ? static_cast<int>(last) : std::numeric_limits<int>::max();
        beginInsertRows(refreshedIndex, index, index);
        auto &item = items.emplace_back(std::make_unique<SyncthingItem>(std::move(localItem)));
        item->parent = refreshedItem;
        item->index = last;
        switch (refreshedItem->checked) {
        case Qt::Checked:
            setChildrenChecked(item.get(), item->checked = Qt::Checked);
            break;
        case Qt::PartiallyChecked:
            setCheckState(refreshedIndex, Qt::Unchecked, false);
            break;
        default:;
        }
        populatePath(item->path = refreshedItem->path % m_pathSeparator % item->name, m_pathSeparator, item->children);
        endInsertRows();
    }
    if (refreshedItem->children.size() != previousChildCount) {
        const auto sizeIndex = refreshedIndex.sibling(refreshedIndex.row(), 1);
        emit dataChanged(sizeIndex, sizeIndex, QVector<int>{ Qt::DisplayRole });
    }

    processFetchQueue(m_pendingRequest.forPath);
}

SyncthingFileModel::QueryResult &SyncthingFileModel::QueryResult::operator=(SyncthingConnection::QueryResult &&other)
{
    reply = other.reply;
    connection = std::move(other.connection);
    localLookup = QFuture<LocalLookupRes>();
    refreshedIndex = QModelIndex();
    return *this;
}

} // namespace Data
