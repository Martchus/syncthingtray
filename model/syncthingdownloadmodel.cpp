#include "./syncthingdownloadmodel.h"

#include <syncthingconnector/syncthingconnection.h>
#include <syncthingconnector/utils.h>

#include <QStringBuilder>

using namespace std;
using namespace CppUtilities;

namespace Data {

SyncthingDownloadModel::PendingDir::PendingDir(const SyncthingDir *syncthingDir, unsigned int pendingItems)
    : syncthingDir(syncthingDir)
    , pendingItems(pendingItems)
{
}

bool SyncthingDownloadModel::PendingDir::operator==(const SyncthingDir *dir) const
{
    return syncthingDir == dir;
}

SyncthingDownloadModel::SyncthingDownloadModel(SyncthingConnection &connection, QObject *parent)
    : SyncthingModel(connection, parent)
    , m_dirs(connection.dirInfo())
    , m_unknownIcon(
          QIcon::fromTheme(QStringLiteral("text-x-generic"), QIcon(QStringLiteral(":/icons/hicolor/scalable/mimetypes/text-x-generic.svg"))))
    , m_pendingDownloads(0)
    , m_singleColumnMode(true)
{
    connect(&m_connection, &SyncthingConnection::downloadProgressChanged, this, &SyncthingDownloadModel::downloadProgressChanged);
}

QHash<int, QByteArray> SyncthingDownloadModel::roleNames() const
{
    const static QHash<int, QByteArray> roles{
        { Qt::DisplayRole, "name" },
        { Qt::DecorationRole, "fileIcon" },
        { ItemPercentage, "percentage" },
        { ItemProgressLabel, "progressLabel" },
        { ItemPath, "path" },
    };
    return roles;
}

/*!
 * \brief Returns the directory info for the spcified \a index. The returned object is not persistent.
 */
const SyncthingDir *SyncthingDownloadModel::dirInfo(const QModelIndex &index) const
{
    return (index.parent().isValid()
            ? dirInfo(index.parent())
            : (static_cast<size_t>(index.row()) < m_pendingDirs.size() ? m_pendingDirs[static_cast<size_t>(index.row())].syncthingDir : nullptr));
}

const SyncthingItemDownloadProgress *SyncthingDownloadModel::progressInfo(const QModelIndex &index) const
{
    if (index.parent().isValid() && static_cast<size_t>(index.parent().row()) < m_pendingDirs.size()
        && static_cast<size_t>(index.row()) < m_pendingDirs[static_cast<size_t>(index.parent().row())].syncthingDir->downloadingItems.size()) {
        return &(m_pendingDirs[static_cast<size_t>(index.parent().row())].syncthingDir->downloadingItems[static_cast<size_t>(index.row())]);
    } else {
        return nullptr;
    }
}

QModelIndex SyncthingDownloadModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!parent.isValid()) {
        // top-level: all pending dir labels/IDs
        if (row < rowCount(parent)) {
            return createIndex(row, column, static_cast<quintptr>(-1));
        }
    } else if (!parent.parent().isValid()) {
        // dir-level: pending downloads
        if (row < rowCount(parent)) {
            return createIndex(row, column, static_cast<quintptr>(parent.row()));
        }
    }
    return QModelIndex();
}

QModelIndex SyncthingDownloadModel::parent(const QModelIndex &child) const
{
    return child.internalId() != static_cast<quintptr>(-1) ? index(static_cast<int>(child.internalId()), 0, QModelIndex()) : QModelIndex();
}

QVariant SyncthingDownloadModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    switch (orientation) {
    case Qt::Horizontal:
        switch (role) {
        case Qt::DisplayRole:
            switch (section) {
            case 0:
                return tr("Dir/item");
            case 1:
                return tr("Progress");
            }
            break;
        default:;
        }
        break;
    default:;
    }
    return QVariant();
}

QVariant SyncthingDownloadModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    if (index.parent().isValid()) {
        // downloading items (of dir)
        if (static_cast<size_t>(index.parent().row()) >= m_pendingDirs.size()) {
            return QVariant();
        }
        const SyncthingDir &dir = *m_pendingDirs[static_cast<size_t>(index.parent().row())].syncthingDir;
        if (static_cast<size_t>(index.row()) < dir.downloadingItems.size()) {
            const SyncthingItemDownloadProgress &progress = dir.downloadingItems[static_cast<size_t>(index.row())];
            switch (role) {
            case Qt::DisplayRole:
            case Qt::EditRole:
                switch (index.column()) {
                case 0: // file names
                    return progress.relativePath;
                case 1: // progress information
                    return progress.label;
                }
                break;
            case Qt::ToolTipRole:
                break;
            case Qt::DecorationRole:
                switch (index.column()) {
                case 0: // file icon
                    return progress.fileInfo.exists() ? m_fileIconProvider.icon(progress.fileInfo) : m_unknownIcon;
                default:;
                }
                break;
            case ItemPercentage:
                return progress.downloadPercentage;
            case ItemProgressLabel:
                return progress.label;
            case ItemPath:
                return dir.path + progress.relativePath;
            default:;
            }
        }
        return QVariant();
    }

    if (static_cast<size_t>(index.row()) >= m_pendingDirs.size()) {
        return QVariant();
    }

    // dir IDs and overall dir progress
    const SyncthingDir &dir = *m_pendingDirs[static_cast<size_t>(index.row())].syncthingDir;
    switch (role) {
    case Qt::DisplayRole:
    case Qt::EditRole:
        switch (index.column()) {
        case 0:
            return QVariant(
                (dir.label.isEmpty() ? dir.id : dir.label) % QChar(' ') % QChar('(') % QString::number(dir.downloadingItems.size()) % QChar(')'));
        case 1:
            return dir.downloadLabel;
        }
        break;
    case Qt::TextAlignmentRole:
        switch (index.column()) {
        case 0:
            break;
        case 1:
            return static_cast<int>(Qt::AlignRight | Qt::AlignVCenter);
        }
        break;
    case ItemPercentage:
        return dir.downloadPercentage;
    case ItemProgressLabel:
        return dir.downloadLabel;
    case ItemPath:
        return dir.path;
    default:;
    }
    return QVariant();
}

bool SyncthingDownloadModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    Q_UNUSED(index)
    Q_UNUSED(value)
    Q_UNUSED(role)
    return false;
}

int SyncthingDownloadModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid()) {
        return static_cast<int>(m_pendingDirs.size());
    } else if (!parent.parent().isValid() && parent.row() >= 0 && static_cast<size_t>(parent.row()) < m_pendingDirs.size()) {
        return static_cast<int>(m_pendingDirs[static_cast<size_t>(parent.row())].pendingItems);
    } else {
        return 0;
    }
}

int SyncthingDownloadModel::columnCount(const QModelIndex &parent) const
{
    if (!parent.isValid()) {
        return singleColumnMode() ? 1 : 2; // label/ID, status/progress
    } else if (!parent.parent().isValid()) {
        return singleColumnMode() ? 1 : 2; // file, progress
    } else {
        return 0;
    }
}

void SyncthingDownloadModel::handleConfigInvalidated()
{
    beginResetModel();
    m_pendingDirs.clear();
    endResetModel();
}

void SyncthingDownloadModel::handleNewConfigAvailable()
{
    m_pendingDirs.reserve(m_connection.dirInfo().size());
}

void SyncthingDownloadModel::downloadProgressChanged()
{
    int row = 0;
    // iterate through all directories ...
    for (const SyncthingDir &dirInfo : m_connection.dirInfo()) {
        // ... and check whether the directory has been pending before
        const auto pendingIterator = find(m_pendingDirs.begin(), m_pendingDirs.end(), &dirInfo);

        // check whether the directory has downloading items ("is pending")
        if (dirInfo.downloadingItems.empty()) {
            // check whether the non-pending directory was pending before
            if (pendingIterator != m_pendingDirs.end()) {
                // remove the row for the previously pending directory
                beginRemoveRows(QModelIndex(), row, row);
                m_pendingDirs.erase(pendingIterator);
                endRemoveRows();
            }

        } else {
            // check whether the pending directory was pending before
            if (pendingIterator != m_pendingDirs.end()) {
                // get parent index and number of previous/new pending items
                const auto parentIndex(index(row, 0));
                const auto pendingItemCount(pendingIterator->pendingItems);
                const auto newPendingItemCount(dirInfo.downloadingItems.size());

                // insert/remove rows missing/surplus rows
                if (pendingItemCount < newPendingItemCount) {
                    beginInsertRows(parentIndex, static_cast<int>(pendingItemCount), static_cast<int>(newPendingItemCount - 1));
                    pendingIterator->pendingItems = newPendingItemCount;
                    endInsertRows();
                } else if (pendingItemCount > newPendingItemCount) {
                    beginRemoveRows(parentIndex, static_cast<int>(newPendingItemCount), static_cast<int>(pendingItemCount - 1));
                    pendingIterator->pendingItems = newPendingItemCount;
                    endRemoveRows();
                }

                // update the present items
                if (newPendingItemCount) {
                    static const QVector<int> roles({ Qt::DisplayRole, Qt::EditRole, Qt::DecorationRole, Qt::ForegroundRole, Qt::ToolTipRole,
                        ItemPercentage, ItemProgressLabel, ItemPath });
                    emit dataChanged(parentIndex, index(row, 1), roles);
                    emit dataChanged(index(0, 0, parentIndex), index(static_cast<int>(newPendingItemCount - 1), 1, parentIndex), roles);
                }

            } else {
                // add the new directory
                beginInsertRows(QModelIndex(), row, row);
                m_pendingDirs.emplace_back(&dirInfo, 0);
                endInsertRows();
                // add new pending items
                beginInsertRows(index(row, row), 0, static_cast<int>(dirInfo.downloadingItems.size() - 1));
                m_pendingDirs.back().pendingItems = dirInfo.downloadingItems.size();
                endInsertRows();
            }
            ++row;
        }
    }
}

void SyncthingDownloadModel::setSingleColumnMode(bool singleColumnModeEnabled)
{
    if (m_singleColumnMode != singleColumnModeEnabled) {
        if (m_singleColumnMode) {
            beginInsertColumns(QModelIndex(), 1, 1);
            m_singleColumnMode = true;
            endInsertColumns();
        } else {
            beginRemoveColumns(QModelIndex(), 1, 1);
            m_singleColumnMode = false;
            endRemoveColumns();
        }
    }
}

} // namespace Data
