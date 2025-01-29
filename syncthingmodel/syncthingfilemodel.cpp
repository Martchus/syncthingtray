#include "./syncthingfilemodel.h"
#include "./colors.h"
#include "./syncthingicons.h"

#include <syncthingconnector/syncthingconnection.h>
#include <syncthingconnector/utils.h>

#include <qtforkawesome/icon.h>

#include <qtutilities/misc/desktoputils.h>

#include <c++utilities/conversion/stringconversion.h>

#include <QClipboard>
#include <QFile>
#include <QGuiApplication>
#include <QNetworkReply>
#include <QPainter>
#include <QStringBuilder>
#include <QtConcurrent>

#include <cassert>
#include <limits>
#include <utility>

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

static void setLoadingItem(SyncthingItem *loadingItem)
{
    loadingItem->name = QStringLiteral("Loading…");
    loadingItem->type = SyncthingItemType::Loading;
    loadingItem->childrenPopulated = true;
}

static void addLoadingItem(std::vector<std::unique_ptr<SyncthingItem>> &items)
{
    if (items.empty()) {
        setLoadingItem(items.emplace_back(std::make_unique<SyncthingItem>()).get());
    }
}
/// \endcond

SyncthingFileModel::SyncthingFileModel(SyncthingConnection &connection, const SyncthingDir &dir, QObject *parent)
    : SyncthingModel(connection, parent)
    , m_connection(connection)
    , m_dirId(dir.id)
    , m_root(std::make_unique<SyncthingItem>())
    , m_columns(4)
    , m_selectionMode(false)
    , m_hasIgnorePatterns(false)
    , m_isIgnoringAllByDefault(false)
    , m_recursiveSelectionEnabled(false)
{
    connect(this, &SyncthingFileModel::selectionModeEnabledChanged, this, &SyncthingFileModel::selectionActionsChanged);
    connect(this, &SyncthingFileModel::hasStagedChangesChanged, this, &SyncthingFileModel::selectionActionsChanged);
    if (m_connection.isLocal()) {
        m_root->existsLocally = true;
        m_localPath = Data::substituteTilde(dir.pathWithoutTrailingSlash().toString(), m_connection.tilde(), m_connection.pathSeparator());
        m_columns += 1;
        connect(&m_localItemLookup, &QFutureWatcherBase::finished, this, &SyncthingFileModel::handleLocalLookupFinished);
    }
    m_pathSeparator = m_connection.pathSeparator().size() == 1 ? m_connection.pathSeparator().front() : QDir::separator();
    m_ignoreAllByDefaultPattern = m_pathSeparator + QStringLiteral("**");
    m_root->name = dir.displayName();
    m_root->existsInDb = true;
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
        { Qt::DisplayRole, "textData" },
        { Qt::DecorationRole, "decorationData" },
        { Qt::ToolTipRole, "toolTipData" },
        { Qt::CheckStateRole, "checkStateData" },
        { DetailsRole, "details" },
        { CheckableRole, "checkable" },
    };
    return roles;
}

QModelIndex SyncthingFileModel::index(int row, int column, const QModelIndex &parent) const
{
    if (row < 0 || column < 0 || column >= m_columns || parent.column() > 0) {
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
    for (const auto &part : std::as_const(parts)) {
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
QString SyncthingFileModel::computeIgnorePatternDiff()
{
    auto diff = QString();
    auto index = std::size_t();
    const auto appendNewLines = [&diff](const auto &lines) {
        for (const auto &line : lines) {
            diff.append(QChar('+'));
            diff.append(line);
            diff.append(QChar('\n'));
        }
    };
    if (const auto change = m_stagedChanges.find(beforeFirstLine); change != m_stagedChanges.end()) {
        appendNewLines(change->prepend);
        appendNewLines(change->append);
    }
    for (const auto &pattern : m_presentIgnorePatterns) {
        auto change = m_stagedChanges.find(index++);
        if (change != m_stagedChanges.end()) {
            if (change->replace && !change->prepend.isEmpty() && change->prepend.back() == pattern.pattern) {
                change->prepend.removeLast();
                change->replace = false;
            }
            appendNewLines(change->prepend);
        }
        diff.append(change == m_stagedChanges.end() || !change->replace ? QChar(' ') : QChar('-'));
        diff.append(pattern.pattern);
        diff.append(QChar('\n'));
        if (change != m_stagedChanges.end()) {
            appendNewLines(change->append);
        }
    }
    return diff;
}

QString SyncthingFileModel::availabilityNote(const SyncthingItem *item) const
{
    if (item->existsInDb.value_or(false) && item->existsLocally.value_or(false)) {
        return tr("Exists locally and globally");
    } else if (item->existsInDb.value_or(false) && item->existsLocally.has_value()) {
        return tr("Exists only globally");
    } else if (item->existsLocally.value_or(false) && item->existsInDb.has_value()) {
        return tr("Exists only locally");
    } else if (item->existsInDb.value_or(false)) {
        return tr("Exists globally and perhaps locally");
    } else if (item->existsLocally.value_or(false)) {
        return tr("Exists locally and perhaps globally");
    } else {
        return tr("Does not exist");
    }
}

/*!
 * \brief Computes new ignore patterns based on the present ignore patterns and staged changes.
 */
SyncthingIgnores SyncthingFileModel::computeNewIgnorePatterns() const
{
    auto newIgnorePatterns = SyncthingIgnores();
    if (!m_manuallyEditedIgnorePatterns.isEmpty()) {
        newIgnorePatterns.ignore = m_manuallyEditedIgnorePatterns.split(QChar('\n'));
        return newIgnorePatterns;
    }
    auto index = std::size_t();
    if (const auto change = m_stagedChanges.find(beforeFirstLine); change != m_stagedChanges.end()) {
        newIgnorePatterns.ignore.append(change->prepend);
        newIgnorePatterns.ignore.append(change->append);
    }
    for (const auto &pattern : m_presentIgnorePatterns) {
        auto change = m_stagedChanges.find(index++);
        if (change != m_stagedChanges.end()) {
            newIgnorePatterns.ignore.append(change->prepend);
        }
        if (change == m_stagedChanges.end() || !change->replace) {
            newIgnorePatterns.ignore.append(pattern.pattern);
        }
        if (change != m_stagedChanges.end()) {
            newIgnorePatterns.ignore.append(change->append);
        }
    }
    return newIgnorePatterns;
}

void SyncthingFileModel::editIgnorePatternsManually(const QString &ignorePatterns)
{
    m_manuallyEditedIgnorePatterns = ignorePatterns;
}

void SyncthingFileModel::editLocalDeletions(const QSet<QString> &localDeletions)
{
    m_manuallyEditedLocalDeletions = localDeletions;
}

void SyncthingFileModel::editLocalDeletionsFromVariantList(const QVariantList &localDeletions)
{
    m_manuallyEditedLocalDeletions.emplace();
    m_manuallyEditedLocalDeletions->reserve(localDeletions.size());
    for (const auto &path : localDeletions) {
        m_manuallyEditedLocalDeletions->insert(path.toString());
    }
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
            break;
        case 4: {
            auto &icon = m_statusIcons[(item->existsInDb.value_or(false) ? 0x01 : 0x00) | (item->existsLocally.value_or(false) ? 0x02 : 0x00)];
            if (icon.isNull()) {
                static constexpr auto size = 16;
                icon = QPixmap(size * 2, size);
                icon.fill(QColor(Qt::transparent));
                auto &manager = IconManager::instance();
                auto painter = QPainter(&icon);
                auto left = 0;
                if (item->existsInDb.value_or(false)) {
                    manager.renderForkAwesomeIcon(QtForkAwesome::Icon::Globe, &painter, QRect(left, 0, size, size));
                }
                left += size;
                if (item->existsLocally.value_or(false)) {
                    manager.renderForkAwesomeIcon(QtForkAwesome::Icon::Home, &painter, QRect(left, 0, size, size));
                }
            }
            return icon;
        } break;
        }
        break;
    }
    case Qt::ForegroundRole:
        if (!item->existsInDb.value_or(false)) {
            return Colors::gray(m_brightColors);
        }
        break;
    case Qt::ToolTipRole:
        switch (index.column()) {
        case 0:
            return item->isFilesystemItem() ? item->path : item->name;
        case 2:
            return agoString(item->modificationTime);
        case 4:
            return availabilityNote(item);
        }
        break;
    case Qt::SizeHintRole:
        switch (index.column()) {
        case 4:
            return QSize(32, 16);
        }
        break;
    case NameRole:
        return item->name;
    case SizeRole:
        return static_cast<qsizetype>(item->size);
    case ModificationTimeRole:
        return item->modificationTime.isNull() ? QString() : QString::fromStdString(item->modificationTime.toString());
    case PathRole:
        return item->isFilesystemItem() ? item->path : QString();
    case Actions: {
        auto res = QStringList();
        res.reserve(3);
        if (item->type == SyncthingItemType::Directory) {
            res << QStringLiteral("refresh");
        }
        if (m_recursiveSelectionEnabled) {
            res << QStringLiteral("toggle-selection-recursively");
        }
        res << QStringLiteral("toggle-selection-single");
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
        if (m_recursiveSelectionEnabled) {
            res << (item->checked == Qt::Checked ? tr("Deselect recursively") : tr("Select recursively"));
        }
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
        if (m_recursiveSelectionEnabled) {
            res << res.back();
        }
        if (!m_localPath.isEmpty() && item->isFilesystemItem()) {
            res << QIcon::fromTheme(QStringLiteral("folder"), QIcon(QStringLiteral(":/icons/hicolor/scalable/places/folder-open.svg")));
            res << QIcon::fromTheme(QStringLiteral("edit-copy"), QIcon(QStringLiteral(":/icons/hicolor/scalable/places/edit-copy.svg")));
        }
        return res;
    }
    case DetailsRole: {
        switch (item->type) {
        case SyncthingItemType::File:
        case SyncthingItemType::Directory:
        case SyncthingItemType::Symlink:
            break;
        default:
            return QString();
        }
        auto res = QString(availabilityNote(item)
            % (item->modificationTime.isNull() ? QString()
                                               : QStringLiteral("\nLast modified on: ") % QString::fromStdString(item->modificationTime.toString())));
        if (item->ignorePattern == SyncthingItem::ignorePatternNotInitialized) {
            matchItemAgainstIgnorePatterns(*item);
        }
        if (item->ignorePattern < m_presentIgnorePatterns.size()) {
            res += QStringLiteral("\nMatches: ") + m_presentIgnorePatterns[item->ignorePattern].pattern;
        }
        return res;
    }
    case CheckableRole: {
        switch (item->type) {
        case SyncthingItemType::File:
        case SyncthingItemType::Directory:
        case SyncthingItemType::Symlink:
            return true;
        default:
            return false;
        }
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
        setCheckState(index, static_cast<Qt::CheckState>(value.toInt()), m_recursiveSelectionEnabled);
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
    static const auto roles = QVector<int>{ Qt::CheckStateRole, ActionNames };
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
    if (!parent.isValid()) {
        return 1;
    }
    auto *const parentItem = reinterpret_cast<SyncthingItem *>(parent.internalPointer());
    if (!parentItem->childrenPopulated && parentItem->type == SyncthingItemType::Directory && parentItem->children.empty()) {
        auto &dummyItem = parentItem->children.emplace_back(std::make_unique<SyncthingItem>());
        dummyItem->name = QStringLiteral("Item not loaded yet.");
        dummyItem->type = SyncthingItemType::Unknown;
        dummyItem->childrenPopulated = true;
    }
    const auto res = parentItem->children.size();
    return res < std::numeric_limits<int>::max() ? static_cast<int>(res) : std::numeric_limits<int>::max();
}

int SyncthingFileModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return m_columns;
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
    auto *const parentItem = reinterpret_cast<SyncthingItem *>(parent.internalPointer());
    if (parentItem->type != SyncthingItemType::Directory) {
        return;
    }
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

void SyncthingFileModel::ignoreSelectedItems(bool ignore, bool deleteLocally)
{
    forEachItem(m_root.get(), [this, ignore, deleteLocally](const SyncthingItem *item) {
        if (item->checked != Qt::Checked || !item->isFilesystemItem()) {
            return true;
        }
        // remove the pattern and the reverse if those already exists
        const auto path = m_pathSeparator + item->path;
        const auto reversePattern = SyncthingIgnorePattern::forPath(path, !ignore);
        const auto wantedPattern = SyncthingIgnorePattern::forPath(path, ignore);
        auto line = std::size_t();
        for (auto &pattern : m_presentIgnorePatterns) {
            if (pattern.pattern == reversePattern || pattern.pattern == wantedPattern) {
                m_stagedChanges[line].replace = true;
            }
            ++line;
        }
        for (auto &change : m_stagedChanges) {
            change.prepend.removeAll(reversePattern);
            change.prepend.removeAll(wantedPattern);
            change.append.removeAll(reversePattern);
            change.append.removeAll(wantedPattern);
        }

        // add line to explicitly ignore/include the item
        static constexpr auto insertPattern = [](auto &list, auto &pattern, auto &relatedPath) {
            auto i = list.begin();
            for (; i != list.end(); ++i) {
                auto existingPattern = QStringView(*i);
                if (existingPattern.startsWith(QChar('!'))) {
                    existingPattern = existingPattern.mid(1);
                }
                if (relatedPath.startsWith(existingPattern) || relatedPath < existingPattern || existingPattern.contains(QChar('*'))) {
                    list.insert(i, pattern);
                    return;
                }
            }
            list.append(pattern);
        };
        line = std::size_t();
        for (auto &pattern : m_presentIgnorePatterns) {
            // reinstate a previously removed pattern
            if (pattern.pattern == wantedPattern) {
                if (auto change = m_stagedChanges.find(line); change != m_stagedChanges.end() && change->replace) {
                    change->replace = false;
                    break;
                }
            }
            // append pattern but keep alphabetical order and don't insert pattern after another one that would match
            if (pattern.glob > path || pattern.matches(item->path, m_pathSeparator)) {
                insertPattern(m_stagedChanges[line].prepend, wantedPattern, path);
                break;
            }
            ++line;
        }
        if (line == m_presentIgnorePatterns.size()) {
            insertPattern(m_stagedChanges[m_presentIgnorePatterns.size() - 1].append, wantedPattern, path);
        }

        // stage deletion of local file
        if (deleteLocally) {
            m_stagedLocalFileDeletions.insert(item->path);
        } else {
            m_stagedLocalFileDeletions.remove(item->path);
        }

        // prepend the new pattern making sure it is effective and not shadowed by an existing pattern
        return false; // no need to add ignore patterns for children as they are applied recursively anyway
    });
    emit hasStagedChangesChanged(hasStagedChanges());
}

QList<QAction *> SyncthingFileModel::selectionActions()
{
    auto res = QList<QAction *>();
    res.reserve(9);
    if (!m_selectionMode) {
        auto *const startSelectionAction = new QAction(tr("Select items to sync/ignore"), this);
        startSelectionAction->setIcon(QIcon::fromTheme(QStringLiteral("edit-select")));
        connect(startSelectionAction, &QAction::triggered, this, [this] { setSelectionModeEnabled(true); });
        res << startSelectionAction;

        if (!m_stagedChanges.isEmpty() || !m_stagedLocalFileDeletions.isEmpty()) {
            auto *const discardAction = new QAction(tr("Discard staged changes"), this);
            discardAction->setIcon(QIcon::fromTheme(QStringLiteral("edit-undo")));
            connect(discardAction, &QAction::triggered, this, [this] {
                m_stagedChanges.clear();
                m_stagedLocalFileDeletions.clear();
                emit hasStagedChangesChanged(hasStagedChanges());
            });
            res << discardAction;
        }
    } else {
        auto *const discardAction = new QAction(tr("Uncheck all and discard staged changes"), this);
        discardAction->setIcon(QIcon::fromTheme(QStringLiteral("edit-undo")));
        connect(discardAction, &QAction::triggered, this, [this] {
            if (const auto rootIndex = index(0, 0); rootIndex.isValid()) {
                setCheckState(index(0, 0), Qt::Unchecked, true);
            }
            setSelectionModeEnabled(false);
            m_stagedChanges.clear();
            m_stagedLocalFileDeletions.clear();
            emit hasStagedChangesChanged(hasStagedChanges());
        });
        res << discardAction;

        auto *const ignoreSelectedAction = new QAction(tr("Ignore checked items (and their children)"), this);
        ignoreSelectedAction->setIcon(QIcon::fromTheme(QStringLiteral("list-remove")));
        connect(ignoreSelectedAction, &QAction::triggered, this, [this]() { ignoreSelectedItems(); });
        res << ignoreSelectedAction;

        if (!m_localPath.isEmpty()) {
            auto *const ignoreAndDeleteSelectedAction = new QAction(tr("Ignore and locally delete checked items (and their children)"), this);
            ignoreAndDeleteSelectedAction->setIcon(QIcon::fromTheme(QStringLiteral("list-remove")));
            connect(ignoreAndDeleteSelectedAction, &QAction::triggered, this, [this]() { ignoreSelectedItems(true, true); });
            res << ignoreAndDeleteSelectedAction;
        }

        auto *const includeSelectedAction = new QAction(tr("Include checked items (and their children)"), this);
        includeSelectedAction->setIcon(QIcon::fromTheme(QStringLiteral("list-add")));
        connect(includeSelectedAction, &QAction::triggered, this, [this]() { ignoreSelectedItems(false); });
        res << includeSelectedAction;
    }

    auto *const ignoreByDefaultAction
        = new QAction(m_isIgnoringAllByDefault ? tr("Include all items by default") : tr("Ignore all items by default"), this);
    ignoreByDefaultAction->setIcon(QIcon::fromTheme(QStringLiteral("question")));
    connect(ignoreByDefaultAction, &QAction::triggered, this, [this, isIgnoringAllByDefault = m_isIgnoringAllByDefault]() {
        auto &lastLine = m_stagedChanges[m_presentIgnorePatterns.size() - 1];
        if (isIgnoringAllByDefault) {
            // remove all occurrences of "/**"
            auto line = std::size_t();
            lastLine.append.removeAll(m_ignoreAllByDefaultPattern);
            for (auto &pattern : m_presentIgnorePatterns) {
                if (pattern.pattern == m_ignoreAllByDefaultPattern) {
                    m_stagedChanges[line].replace = true;
                }
                ++line;
            }
        } else {
            // append "/**"
            if (m_presentIgnorePatterns.empty() || m_presentIgnorePatterns.back().pattern != m_ignoreAllByDefaultPattern) {
                lastLine.append.append(m_ignoreAllByDefaultPattern);
            } else {
                lastLine.replace = false;
                lastLine.append.removeAll(m_ignoreAllByDefaultPattern);
            }
        }
        m_isIgnoringAllByDefault = !isIgnoringAllByDefault;
        emit hasStagedChangesChanged(hasStagedChanges());
    });
    res << ignoreByDefaultAction;

    if (m_selectionMode) {
        auto *const removeIgnorePatternsAction
            = new QAction(tr("Remove ignore patterns matching checked items (may affect other items as well)"), this);
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
                    m_stagedChanges[item->ignorePattern].replace = true;
                }
                if (item->ignorePattern < m_presentIgnorePatterns.size()) {
                    const auto &pattern = m_presentIgnorePatterns[item->ignorePattern].pattern;
                    for (auto &change : m_stagedChanges) {
                        change.prepend.removeAll(pattern);
                        change.append.removeAll(pattern);
                    }
                }
                return true;
            });
            emit hasStagedChangesChanged(hasStagedChanges());
        });
        res << removeIgnorePatternsAction;
    }

    if (!m_stagedChanges.isEmpty() || !m_stagedLocalFileDeletions.isEmpty()) {
        auto *const applyStagedChangesAction = new RejectableAction(tr("Review and apply staged changes"), this);
        applyStagedChangesAction->setIcon(QIcon::fromTheme(QStringLiteral("dialog-ok-apply")));
        connect(applyStagedChangesAction, &QAction::triggered, this, [this, action = applyStagedChangesAction]() mutable {
            // allow user to review changes before applying them
            if (action->needsConfirmation) {
                action->needsConfirmation = false;
                m_manuallyEditedIgnorePatterns.clear();
                m_manuallyEditedLocalDeletions.reset();
                emit actionNeedsConfirmation(action, tr("Do you want to apply the following changes?"), computeIgnorePatternDiff(), m_stagedLocalFileDeletions);
                return;
            }
            action->needsConfirmation = true;

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
                m_hasIgnorePatterns = false;
                forEachItem(m_root.get(), [](SyncthingItem *item) {
                    item->ignorePattern = SyncthingItem::ignorePatternNotInitialized;
                    return true;
                });
                queryIgnores();

                // delete local files/directories staged for deletion
                const auto &localDeletions = m_manuallyEditedLocalDeletions.value_or(m_stagedLocalFileDeletions);
                auto failedDeletions = QStringList();
                for (const auto &path : localDeletions) {
                    const auto fullPath = QString(m_localPath % m_pathSeparator % path);
                    if (QFile::moveToTrash(fullPath)) {
                        m_stagedLocalFileDeletions.remove(path);
                    } else {
                        failedDeletions.append(fullPath);
                    }
                }

                if (failedDeletions.isEmpty()) {
                    emit notification(QStringLiteral("info"), tr("Ignore patterns have been changed."));
                } else {
                    emit notification(QStringLiteral("info"), tr("Ignore patterns have been changed but the following local files could not be deleted:\n") + failedDeletions.join(QChar('\n')));
                }
                emit hasStagedChangesChanged(hasStagedChanges());
            });
        });
        res << applyStagedChangesAction;
    }

    return res;
}

void SyncthingFileModel::setSelectionModeEnabled(bool selectionModeEnabled)
{
    if (m_selectionMode != selectionModeEnabled) {
        emit selectionModeEnabledChanged(m_selectionMode = selectionModeEnabled);
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
    for (auto &icon : m_statusIcons) {
        icon = QPixmap();
    }
    invalidateAllIndicies(QVector<int>({ Qt::DecorationRole }));
}

void SyncthingFileModel::handleBrightColorsChanged()
{
    invalidateAllIndicies(QVector<int>({ Qt::ForegroundRole }));
}

void SyncthingFileModel::lookupDirLocally(const QDir &dir, SyncthingFileModel::LocalItemMap &items, int depth)
{
    const auto entries = dir.entryInfoList(QDir::AllEntries | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot);
    for (const auto &entry : entries) {
        const auto entryName = entry.fileName();
        auto &item = items[entryName];
        item.name = entryName;
        item.existsLocally = true;
        item.size = static_cast<std::size_t>(entry.size());
        item.modificationTime = DateTime::unixEpochStart() + TimeSpan(TimeSpan::ticksPerMillisecond * entry.lastModified().toMSecsSinceEpoch());
        if (entry.isSymbolicLink()) {
            item.type = SyncthingItemType::Symlink;
        } else if (entry.isDir()) {
            item.type = SyncthingItemType::Directory;
            if (depth > 0) {
                lookupDirLocally(QDir(entry.absoluteFilePath()), item.localChildren, depth - 1);
                item.localChildrenPopulated = true;
            }
        } else {
            item.type = SyncthingItemType::File;
        }
    }
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

    // add/update loading item
    auto *rootItem = reinterpret_cast<SyncthingItem *>(rootIndex.internalPointer());
    const auto populated = rootItem->childrenPopulated;
    if (!rootItem->childrenPopulated) {
        rootItem->childrenPopulated = true;
        switch (rootItem->children.size()) {
        case 0:
            beginInsertRows(rootIndex, 0, 0);
            addLoadingItem(rootItem->children);
            endInsertRows();
            break;
        case 1:
            setLoadingItem(rootItem->children.front().get());
            const auto affectedIndex = index(0, 0, rootIndex);
            emit dataChanged(affectedIndex, affectedIndex, QVector<int>{ Qt::DisplayRole, Qt::DecorationRole });
            break;
        }
    }

    // query directory entries from Syncthing database
    if (rootItem->existsInDb.value_or(false)) {
        m_pendingRequest = m_connection.browse(
            m_dirId, path, 1, [this, populated](std::vector<std::unique_ptr<SyncthingItem>> &&items, QString &&errorMessage) mutable {
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
                    forEachItem(refreshedItem, [](SyncthingItem *item) { return item->existsInDb.emplace(true); });
                    switch (refreshedItem->checked) {
                    case Qt::Checked:
                        if (m_recursiveSelectionEnabled) {
                            setChildrenChecked(refreshedItem, Qt::Checked);
                        }
                        break;
                    case Qt::PartiallyChecked:
                        setCheckState(refreshedIndex, Qt::Unchecked, false);
                        break;
                    default:;
                    }
                    endInsertRows();
                }
                if (!populated || refreshedItem->children.size() != previousChildCount) {
                    const auto sizeIndex = refreshedIndex.siblingAtColumn(1);
                    emit dataChanged(sizeIndex, sizeIndex, QVector<int>{ Qt::DisplayRole });
                    emit dataChanged(refreshedIndex, refreshedIndex, QVector<int>{ SizeRole });
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
        auto items = std::make_shared<LocalItemMap>();
        lookupDirLocally(dir, *items);
        return items;
    });
    if (!rootItem->existsInDb.value_or(false)) {
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
        m_isIgnoringAllByDefault = false;
        m_presentIgnorePatterns.clear();
        m_presentIgnorePatterns.reserve(static_cast<std::size_t>(ignores.ignore.size()));
        for (auto &ignorePattern : ignores.ignore) {
            m_isIgnoringAllByDefault = m_isIgnoringAllByDefault || ignorePattern == m_ignoreAllByDefaultPattern;
            m_presentIgnorePatterns.emplace_back(std::move(ignorePattern));
        }
        resetMatchingIgnorePatterns();
    });
}

void SyncthingFileModel::resetMatchingIgnorePatterns()
{
    forEachItem(m_root.get(), [](SyncthingItem *item) {
        item->ignorePattern = SyncthingItem::ignorePatternNotInitialized;
        return true;
    });
    invalidateAllIndicies(QVector<int>{ Qt::DisplayRole }, 3, QModelIndex());
    invalidateAllIndicies(QVector<int>{ DetailsRole }, 0, QModelIndex());
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

/*!
 * \brief Marks items from the database query as locally existing if they do; marks items from local lookup as existing in the db if they do.
 */
void SyncthingFileModel::markItemsFromDatabaseAsLocallyExisting(
    std::vector<std::unique_ptr<SyncthingItem>> &items, SyncthingFileModel::LocalItemMap &localItems)
{
    for (auto &child : items) {
        const auto localItemIter = localItems.find(child->name);
        const auto itemExists = localItemIter != localItems.end();
        child->existsLocally = itemExists;
        if (!itemExists) {
            static auto noLocalChildren = SyncthingFileModel::LocalItemMap();
            markItemsFromDatabaseAsLocallyExisting(child->children, noLocalChildren);
            continue;
        }
        localItemIter->second.existsInDb = child->existsInDb;
        localItemIter->second.index = child->index;
        markItemsFromDatabaseAsLocallyExisting(child->children, localItemIter->second.localChildren);
    }
}

/*!
 * \brief Inserts items from local lookup that are not already present via the database query (usually ignored files).
 */
void SyncthingFileModel::insertLocalItems(const QModelIndex &refreshedIndex, SyncthingFileModel::LocalItemMap &localItems)
{
    // get refreshed index/item
    auto *const refreshedItem = reinterpret_cast<SyncthingItem *>(refreshedIndex.internalPointer());
    auto &items = refreshedItem->children;
    const auto hasDbItems = !items.empty() && items.front()->type != SyncthingItemType::Error;
    const auto previouslyPopulated = refreshedItem->childrenPopulated;
    const auto previousChildCount = items.size();
    if (!refreshedItem->existsInDb.value_or(false)) {
        refreshedItem->childrenPopulated = true;
    }

    // clear loading item
    if (!refreshedItem->existsInDb.value_or(false) && !items.empty()) {
        const auto last = items.size() - 1;
        beginRemoveRows(refreshedIndex, 0, last < std::numeric_limits<int>::max() ? static_cast<int>(last) : std::numeric_limits<int>::max());
        items.clear();
        endRemoveRows();
    }

    // insert items from local lookup that are not already present via the database query (probably ignored files)
    auto index = items.size();
    auto row = index < std::numeric_limits<int>::max() ? static_cast<int>(index) : std::numeric_limits<int>::max();
    auto firstRow = row;
    for (auto &[localItemName, localItem] : localItems) {
        if (localItem.existsInDb.value_or(false)) {
            continue;
        }
        beginInsertRows(refreshedIndex, row, row);
        auto &item = items.emplace_back(std::make_unique<SyncthingItem>(std::move(localItem)));
        item->parent = refreshedItem;
        item->index = localItem.index = index++;
        if (hasDbItems) {
            item->existsInDb = false;
        }
        switch (refreshedItem->checked) {
        case Qt::Checked:
            if (m_recursiveSelectionEnabled) {
                setChildrenChecked(item.get(), item->checked = Qt::Checked);
            }
            break;
        case Qt::PartiallyChecked:
            setCheckState(refreshedIndex, Qt::Unchecked, false);
            break;
        default:;
        }
        populatePath(item->path = refreshedItem->path.isEmpty() ? item->name : QString(refreshedItem->path % m_pathSeparator % item->name),
            m_pathSeparator, item->children);
        endInsertRows();
        ++row;
    }
    if (!previouslyPopulated || items.size() != previousChildCount) {
        const auto sizeIndex = refreshedIndex.sibling(refreshedIndex.row(), 1);
        emit dataChanged(sizeIndex, sizeIndex, QVector<int>{ Qt::DisplayRole });
        emit dataChanged(refreshedIndex, refreshedIndex, QVector<int>{ SizeRole });
    }

    // insert local child items recursively
    for (auto &[localItemName, localItem] : localItems) {
        if (!localItem.localChildrenPopulated) {
            continue;
        }
        if (const auto childIndex = this->index(static_cast<int>(localItem.index), 0, refreshedIndex); childIndex.isValid()) {
            assert(childIndex.data() == localItemName);
            insertLocalItems(childIndex, localItem.localChildren);
        }
    }

    // update global/local icons and details of items that were already present (as they exist in the database)
    if (firstRow > 0) {
        emit dataChanged(
            this->index(0, 4, refreshedIndex), this->index(firstRow - 1, 4, refreshedIndex), QVector<int>{ Qt::DecorationRole, Qt::ToolTipRole });
        emit dataChanged(this->index(0, 0, refreshedIndex), this->index(firstRow - 1, 0, refreshedIndex), QVector<int>{ DetailsRole });
    }
}

/*!
 * \brief Incorporates data found by the local lookup into the item-tree.
 */
void SyncthingFileModel::handleLocalLookupFinished()
{
    // get refreshed index/item and result from local lookup
    const auto &refreshedIndex = m_pendingRequest.refreshedIndex;
    const auto res = m_pendingRequest.localLookup.result();
    if (!refreshedIndex.isValid() || !res) {
        processFetchQueue(m_pendingRequest.forPath);
        return;
    }

    // update items
    auto &items = reinterpret_cast<SyncthingItem *>(refreshedIndex.internalPointer())->children;
    auto &localItems = *res;
    markItemsFromDatabaseAsLocallyExisting(items, localItems);
    insertLocalItems(refreshedIndex, localItems);
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
