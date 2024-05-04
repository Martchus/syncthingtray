#include "./syncthingfilemodel.h"
#include "./syncthingicons.h"

#include <syncthingconnector/syncthingconnection.h>
#include <syncthingconnector/utils.h>

#include <qtutilities/misc/desktoputils.h>

#include <c++utilities/conversion/stringconversion.h>

#include <QClipboard>
#include <QGuiApplication>
#include <QStringBuilder>

using namespace std;
using namespace CppUtilities;

namespace Data {

/// \cond
static void populatePath(const QString &root, std::vector<std::unique_ptr<SyncthingItem>> &items)
{
    if (root.isEmpty()) {
        for (auto &item : items) {
            populatePath(item->path = item->name, item->children);
        }
    } else {
        for (auto &item : items) {
            populatePath(item->path = root % QChar('/') % item->name, item->children);
        }
    }
}
/// \endcond

SyncthingFileModel::SyncthingFileModel(SyncthingConnection &connection, const SyncthingDir &dir, QObject *parent)
    : SyncthingModel(connection, parent)
    , m_connection(connection)
    , m_dirId(dir.id)
    , m_root(std::make_unique<SyncthingItem>())
{
    if (m_connection.isLocal()) {
        m_localPath = dir.pathWithoutTrailingSlash().toString();
    }
    m_root->name = dir.displayName();
    m_root->modificationTime = dir.lastFileTime;
    m_root->size = dir.globalStats.bytes;
    m_root->type = SyncthingItemType::Directory;
    m_fetchQueue.append(QString());
    m_connection.browse(m_dirId, QString(), 1, [this](std::vector<std::unique_ptr<SyncthingItem>> &&items, QString &&errorMessage) {
        Q_UNUSED(errorMessage)

        m_fetchQueue.removeAll(QString());
        if (items.empty()) {
            return;
        }
        const auto last = items.size() - 1;
        beginInsertRows(index(0, 0), 0, last < std::numeric_limits<int>::max() ? static_cast<int>(last) : std::numeric_limits<int>::max());
        populatePath(QString(), items);
        m_root->children = std::move(items);
        m_root->childrenPopulated = true;
        endInsertRows();
    });
}

SyncthingFileModel::~SyncthingFileModel()
{
    QObject::disconnect(m_pendingRequest);
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
    if (row < 0 || column < 0 || column > 2 || parent.column() > 0) {
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
    auto parts = path.split(QChar('/'), Qt::SkipEmptyParts);
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
    auto res = QString();
    if (!index.isValid()) {
        return res;
    }
    auto parts = QStringList();
    auto size = QString::size_type();
    parts.reserve(reinterpret_cast<SyncthingItem *>(index.internalPointer())->level + 1);
    for (auto i = index; i.isValid(); i = i.parent()) {
        const auto *const item = reinterpret_cast<SyncthingItem *>(i.internalPointer());
        if (item == m_root.get()) {
            break;
        }
        parts.append(reinterpret_cast<SyncthingItem *>(i.internalPointer())->name);
        size += parts.back().size();
    }
    res.reserve(size + parts.size());
    for (auto i = parts.rbegin(), end = parts.rend(); i != end; ++i) {
        res += *i;
        res += QChar('/');
    }
    return res;
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
            }
            break;
        default:;
        }
        break;
    default:;
    }
    return QVariant();
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
            default:
                return icons.cogs;
            }
        }
        break;
    }
    case Qt::ToolTipRole:
        switch (index.column()) {
        case 0:
            return item->path;
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
        return item->path;
    case Actions: {
        auto res = QStringList();
        res.reserve(3);
        if (item->type == SyncthingItemType::Directory) {
            res << QStringLiteral("refresh");
        }
        if (!m_localPath.isEmpty()) {
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
        if (!m_localPath.isEmpty()) {
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
        if (!m_localPath.isEmpty()) {
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
    Q_UNUSED(index)
    Q_UNUSED(value)
    Q_UNUSED(role)
    return false;
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
    return 3;
}

bool SyncthingFileModel::canFetchMore(const QModelIndex &parent) const
{
    if (!parent.isValid()) {
        return false;
    }
    auto *const parentItem = reinterpret_cast<SyncthingItem *>(parent.internalPointer());
    return !parentItem->childrenPopulated && parentItem->type == SyncthingItemType::Directory;
}

/// \cond
static void addLevel(std::vector<std::unique_ptr<SyncthingItem>> &items, int level)
{
    for (auto &item : items) {
        item->level += level;
        addLevel(item->children, level);
    }
}
/// \endcond

void SyncthingFileModel::fetchMore(const QModelIndex &parent)
{
    if (!parent.isValid()) {
        return;
    }
    m_fetchQueue.append(path(parent));
    if (m_fetchQueue.size() == 1) {
        processFetchQueue();
    }
}

void SyncthingFileModel::triggerAction(const QString &action, const QModelIndex &index)
{
    if (action == QLatin1String("refresh")) {
        fetchMore(index);
    }
    if (m_localPath.isEmpty()) {
        return;
    }
    const auto relPath = index.data(PathRole).toString();
    const auto path = relPath.isEmpty() ? m_localPath : QString(m_localPath % QChar('/') % relPath);
    if (action == QLatin1String("open")) {
        QtUtilities::openLocalFileOrDir(path);
    } else if (action == QLatin1String("copy-path")) {
        if (auto *const clipboard = QGuiApplication::clipboard()) {
            clipboard->setText(path);
        }
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

void SyncthingFileModel::processFetchQueue()
{
    if (m_fetchQueue.isEmpty()) {
        return;
    }
    const auto &path = m_fetchQueue.front();
    m_pendingRequest = m_connection.browse(
        m_dirId, path, 1, [this, p = path](std::vector<std::unique_ptr<SyncthingItem>> &&items, QString &&errorMessage) mutable {
            Q_UNUSED(errorMessage)

            m_fetchQueue.removeAll(p);
            const auto refreshedIndex = index(p);
            if (!refreshedIndex.isValid()) {
                processFetchQueue();
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
                addLevel(items, refreshedItem->level);
                for (auto &item : items) {
                    item->parent = refreshedItem;
                }
                populatePath(refreshedItem->path, items);
                beginInsertRows(refreshedIndex, 0, last < std::numeric_limits<int>::max() ? static_cast<int>(last) : std::numeric_limits<int>::max());
                refreshedItem->children = std::move(items);
                refreshedItem->childrenPopulated = true;
                endInsertRows();
            }
            if (refreshedItem->children.size() != previousChildCount) {
                const auto sizeIndex = refreshedIndex.siblingAtColumn(1);
                emit dataChanged(sizeIndex, sizeIndex, QList<int>{ Qt::DisplayRole });
            }
            processFetchQueue();
        });
}

} // namespace Data