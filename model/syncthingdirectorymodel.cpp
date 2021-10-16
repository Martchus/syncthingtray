#include "./syncthingdirectorymodel.h"
#include "./colors.h"
#include "./syncthingicons.h"

#include <syncthingconnector/syncthingconnection.h>
#include <syncthingconnector/utils.h>

#include <c++utilities/conversion/stringconversion.h>

#include <QStringBuilder>

using namespace std;
using namespace CppUtilities;

namespace Data {

static int computeDirectoryRowCount(const SyncthingDir &dir)
{
    return dir.paused ? 8 : 10;
}

SyncthingDirectoryModel::SyncthingDirectoryModel(SyncthingConnection &connection, QObject *parent)
    : SyncthingModel(connection, parent)
    , m_dirs(connection.dirInfo())
{
    updateRowCount();
    connect(&m_connection, &SyncthingConnection::dirStatusChanged, this, &SyncthingDirectoryModel::dirStatusChanged);
}

QHash<int, QByteArray> SyncthingDirectoryModel::roleNames() const
{
    const static QHash<int, QByteArray> roles{
        { Qt::DisplayRole, "name" },
        { DirectoryStatus, "status" },
        { Qt::DecorationRole, "statusIcon" },
        { DirectoryStatusString, "statusString" },
        { DirectoryStatusColor, "statusColor" },
        { DirectoryPaused, "paused" },
        { DirectoryId, "dirId" },
        { DirectoryPath, "path" },
        { DirectoryPullErrorCount, "pullErrorCount" },
        { DirectoryDetail, "detail" },
        { DirectoryDetailIcon, "detailIcon" },
    };
    return roles;
}

const QVector<int> &SyncthingDirectoryModel::colorRoles() const
{
    static const QVector<int> colorRoles({ Qt::DecorationRole, Qt::ForegroundRole, DirectoryStatusColor, DirectoryDetailIcon });
    return colorRoles;
}

/*!
 * \brief Returns the directory info for the spcified \a index. The returned object is not persistent.
 */
const SyncthingDir *SyncthingDirectoryModel::dirInfo(const QModelIndex &index) const
{
    return (index.parent().isValid() ? dirInfo(index.parent())
                                     : (static_cast<size_t>(index.row()) < m_dirs.size() ? &m_dirs[static_cast<size_t>(index.row())] : nullptr));
}

QModelIndex SyncthingDirectoryModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!parent.isValid()) {
        // top-level: all dir labels/IDs
        if (row < rowCount(parent)) {
            return createIndex(row, column, static_cast<quintptr>(-1));
        }
    } else if (!parent.parent().isValid()) {
        // dir-level: dir attributes
        if (row < rowCount(parent)) {
            return createIndex(row, column, static_cast<quintptr>(parent.row()));
        }
    }
    return QModelIndex();
}

QModelIndex SyncthingDirectoryModel::parent(const QModelIndex &child) const
{
    return child.internalId() != static_cast<quintptr>(-1) ? index(static_cast<int>(child.internalId()), 0, QModelIndex()) : QModelIndex();
}

QVariant SyncthingDirectoryModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    switch (orientation) {
    case Qt::Horizontal:
        switch (role) {
        case Qt::DisplayRole:
            switch (section) {
            case 0:
                return tr("ID");
            case 1:
                return tr("Status");
            }
            break;
        default:;
        }
        break;
    default:;
    }
    return QVariant();
}

QVariant SyncthingDirectoryModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }
    if (index.parent().isValid()) {
        // dir attributes
        if (static_cast<size_t>(index.parent().row()) >= m_dirs.size()) {
            return QVariant();
        }
        const SyncthingDir &dir = m_dirs[static_cast<size_t>(index.parent().row())];
        const auto row = dir.paused && index.row() > 1 ? index.row() + 2 : index.row();
        switch (role) {
        case Qt::DisplayRole:
        case Qt::EditRole:
            if (index.column() == 0) {
                // attribute names
                switch (row) {
                case 0:
                    return tr("ID");
                case 1:
                    return tr("Path");
                case 2:
                    return tr("Global status");
                case 3:
                    return tr("Local status");
                case 4:
                    return tr("Shared with");
                case 5:
                    return tr("Type");
                case 6:
                    return tr("Rescan interval");
                case 7:
                    return tr("Last scan");
                case 8:
                    return tr("Last file");
                case 9:
                    return tr("Errors");
                }
                break;
            }
            [[fallthrough]];
        case DirectoryDetail:
            if (index.column() == 1 || role == DirectoryDetail) {
                // attribute values
                switch (row) {
                case 0:
                    return dir.id;
                case 1:
                    return dir.path;
                case 2:
                    return directoryStatusString(dir.globalStats);
                case 3:
                    return directoryStatusString(dir.localStats);
                case 4:
                    if (!dir.deviceNames.isEmpty()) {
                        return dir.deviceNames.join(QStringLiteral(", "));
                    } else if (!dir.deviceIds.isEmpty()) {
                        return dir.deviceIds.join(QStringLiteral(", "));
                    } else {
                        return tr("not shared");
                    }
                case 5:
                    return dir.dirTypeString();
                case 6:
                    return rescanIntervalString(dir.rescanInterval, dir.fileSystemWatcherEnabled);
                case 7:
                    return dir.lastScanTime.isNull() ? tr("unknown")
                                                     : QString::fromLatin1(dir.lastScanTime.toString(DateTimeOutputFormat::DateAndTime, true).data());
                case 8:
                    return dir.lastFileName.isEmpty() ? tr("unknown") : dir.lastFileName;
                case 9:
                    if (dir.globalError.isEmpty() && !dir.pullErrorCount) {
                        return tr("none");
                    }
                    if (!dir.pullErrorCount) {
                        return dir.globalError;
                    }
                    if (dir.globalError.isEmpty()) {
                        return tr("%1 item(s) out of sync", nullptr, trQuandity(dir.pullErrorCount)).arg(dir.pullErrorCount);
                    }
                    return tr("%1 and %2 item(s) out of sync", nullptr, trQuandity(dir.pullErrorCount)).arg(dir.globalError).arg(dir.pullErrorCount);
                }
            }
            break;
        case Qt::DecorationRole:
        case DirectoryDetailIcon:
            if (index.column() == 0) {
                // attribute icons
                const auto &icons = commonForkAwesomeIcons();
                switch (row) {
                case 0:
                    return icons.hashtag;
                case 1:
                    return icons.folderOpen;
                case 2:
                    return icons.globe;
                case 3:
                    return icons.home;
                case 4:
                    return icons.shareAlt;
                case 5:
                    return icons.cogs;
                case 6:
                    return icons.refresh;
                case 7:
                    return icons.clock;
                case 8:
                    return icons.exchangeAlt;
                case 9:
                    return icons.exclamationTriangle;
                }
            }
            break;
        case Qt::ForegroundRole:
            switch (index.column()) {
            case 1:
                switch (row) {
                case 4:
                    if (dir.deviceIds.isEmpty()) {
                        return Colors::gray(m_brightColors);
                    }
                    break;
                case 7:
                    if (dir.lastScanTime.isNull()) {
                        return Colors::gray(m_brightColors);
                    }
                    break;
                case 8:
                    return dir.lastFileName.isEmpty() ? Colors::gray(m_brightColors)
                                                      : (dir.lastFileDeleted ? Colors::red(m_brightColors) : QVariant());
                case 9:
                    return dir.globalError.isEmpty() && !dir.pullErrorCount ? Colors::gray(m_brightColors) : Colors::red(m_brightColors);
                }
            }
            break;
        case Qt::ToolTipRole:
            switch (index.column()) {
            case 1:
                switch (row) {
                case 3:
                    if (dir.deviceNames.isEmpty()) {
                        return dir.deviceIds.join(QChar('\n'));
                    } else {
                        return QString(
                            dir.deviceNames.join(QStringLiteral(", ")) % QChar('\n') % QChar('(') % dir.deviceIds.join(QChar('\n')) % QChar(')'));
                    }
                case 7:
                    if (!dir.lastScanTime.isNull()) {
                        return agoString(dir.lastScanTime);
                    }
                    break;
                case 8:
                    if (!dir.lastFileTime.isNull()) {
                        if (dir.lastFileDeleted) {
                            return tr("Deleted at %1")
                                .arg(QString::fromStdString(dir.lastFileTime.toString(DateTimeOutputFormat::DateAndTime, true)));
                        } else {
                            return tr("Updated at %1")
                                .arg(QString::fromStdString(dir.lastFileTime.toString(DateTimeOutputFormat::DateAndTime, true)));
                        }
                    }
                    break;
                case 9:
                    if (!dir.itemErrors.empty()) {
                        QStringList errors;
                        errors.reserve(static_cast<int>(dir.itemErrors.size()));
                        for (const auto &error : dir.itemErrors) {
                            errors << error.path;
                        }
                        return QVariant(QStringLiteral("<b>") % tr("Failed items") % QStringLiteral("</b><ul><li>")
                            % errors.join(QStringLiteral("</li><li>")) % QStringLiteral("</li></ul>") % tr("Click for details"));
                    }
                }
            }
            break;
        default:;
        }
        return QVariant();
    }

    if (static_cast<size_t>(index.row()) >= m_dirs.size()) {
        return QVariant();
    }

    // dir IDs and status
    const SyncthingDir &dir = m_dirs[static_cast<size_t>(index.row())];
    switch (role) {
    case Qt::DisplayRole:
    case Qt::EditRole:
        switch (index.column()) {
        case 0:
            return dir.label.isEmpty() ? dir.id : dir.label;
        case 1:
            return dirStatusString(dir);
        }
        break;
    case Qt::DecorationRole:
        switch (index.column()) {
        case 0:
            if (dir.paused && dir.status != SyncthingDirStatus::OutOfSync) {
                return statusIcons().pause;
            } else if (dir.deviceIds.empty()) {
                return statusIcons().disconnected; // "unshared" status
            } else {
                switch (dir.status) {
                case SyncthingDirStatus::Unknown:
                    return statusIcons().disconnected;
                case SyncthingDirStatus::Idle:
                case SyncthingDirStatus::Cleaning:
                case SyncthingDirStatus::WaitingToClean:
                    return statusIcons().idling;
                case SyncthingDirStatus::WaitingToScan:
                case SyncthingDirStatus::Scanning:
                    return statusIcons().scanninig;
                case SyncthingDirStatus::WaitingToSync:
                case SyncthingDirStatus::PreparingToSync:
                case SyncthingDirStatus::Synchronizing:
                    return statusIcons().sync;
                case SyncthingDirStatus::OutOfSync:
                    return statusIcons().error;
                }
            }
            break;
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
    case Qt::ForegroundRole:
        switch (index.column()) {
        case 0:
            break;
        case 1:
            return dirStatusColor(dir);
        }
        break;
    case DirectoryStatus:
        return static_cast<int>(dir.status);
    case DirectoryPaused:
        return dir.paused;
    case DirectoryStatusString:
        return dirStatusString(dir);
    case DirectoryStatusColor:
        return dirStatusColor(dir);
    case DirectoryId:
        return dir.id;
    case DirectoryPath:
        return dir.path;
    case DirectoryPullErrorCount:
        return dir.pullErrorCount;
    default:;
    }

    return QVariant();
}

bool SyncthingDirectoryModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    Q_UNUSED(index)
    Q_UNUSED(value)
    Q_UNUSED(role)
    return false;
}

int SyncthingDirectoryModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid()) {
        return static_cast<int>(m_dirs.size());
    } else if (!parent.parent().isValid() && static_cast<size_t>(parent.row()) < m_rowCount.size()) {
        return m_rowCount[static_cast<size_t>(parent.row())];
    } else {
        return 0;
    }
}

int SyncthingDirectoryModel::columnCount(const QModelIndex &parent) const
{
    if (!parent.isValid()) {
        return 2; // label/ID, status/buttons
    } else if (!parent.parent().isValid()) {
        return 2; // field name and value
    } else {
        return 0;
    }
}

void SyncthingDirectoryModel::dirStatusChanged(const SyncthingDir &dir, int index)
{
    if (index < 0 || static_cast<size_t>(index) >= m_rowCount.size()) {
        return;
    }

    // update top-level indizes
    const QModelIndex modelIndex1(this->index(index, 0, QModelIndex()));
    static const QVector<int> modelRoles1({ Qt::DisplayRole, Qt::EditRole, Qt::DecorationRole, DirectoryPaused, DirectoryStatus,
        DirectoryStatusString, DirectoryStatusColor, DirectoryId, DirectoryPath, DirectoryPullErrorCount });
    emit dataChanged(modelIndex1, modelIndex1, modelRoles1);
    const QModelIndex modelIndex2(this->index(index, 1, QModelIndex()));
    static const QVector<int> modelRoles2({ Qt::DisplayRole, Qt::EditRole, Qt::ForegroundRole });
    emit dataChanged(modelIndex2, modelIndex2, modelRoles2);

    // remove/insert detail rows
    const auto oldRowCount = m_rowCount[static_cast<size_t>(index)];
    const auto newRowCount = computeDirectoryRowCount(dir);
    const auto newLastRow = newRowCount - 1;
    if (oldRowCount > newRowCount) {
        // begin removing rows for statistics
        beginRemoveRows(modelIndex1, 2, 3);
        m_rowCount[static_cast<size_t>(index)] = newRowCount;
        endRemoveRows();
    } else if (newRowCount > oldRowCount) {
        // begin inserting rows for statistics
        beginInsertRows(modelIndex1, 2, 3);
        m_rowCount[static_cast<size_t>(index)] = newRowCount;
        endInsertRows();
    }

    // update detail rows
    static const QVector<int> modelRoles3({ Qt::DisplayRole, Qt::EditRole, Qt::ToolTipRole });
    emit dataChanged(this->index(0, 1, modelIndex1), this->index(newLastRow, 1, modelIndex1), modelRoles3);
    static const QVector<int> modelRoles4({ Qt::DisplayRole, Qt::EditRole, DirectoryDetail });
    emit dataChanged(this->index(0, 0, modelIndex1), this->index(newLastRow, 0, modelIndex1), modelRoles4);
}

void SyncthingDirectoryModel::handleConfigInvalidated()
{
    beginResetModel();
}

void SyncthingDirectoryModel::handleNewConfigAvailable()
{
    updateRowCount();
    endResetModel();
}

void SyncthingDirectoryModel::handleStatusIconsChanged()
{
    invalidateTopLevelIndicies(QVector<int>({ Qt::DecorationRole }));
}

void SyncthingDirectoryModel::handleForkAwesomeIconsChanged()
{
    invalidateNestedIndicies(QVector<int>({ Qt::DecorationRole, DirectoryDetailIcon }));
}

QString SyncthingDirectoryModel::dirStatusString(const SyncthingDir &dir)
{
    if (dir.paused && dir.status != SyncthingDirStatus::OutOfSync) {
        return tr("Paused");
    }
    if (dir.isUnshared()) {
        return tr("Unshared");
    }
    switch (dir.status) {
    case SyncthingDirStatus::Unknown:
        return dir.rawStatus.isEmpty() ? tr("Unknown status") : QString(dir.rawStatus);
    case SyncthingDirStatus::Idle:
        return tr("Idle");
    case SyncthingDirStatus::WaitingToScan:
        return tr("Waiting to scan");
    case SyncthingDirStatus::Scanning:
        if (dir.scanningPercentage > 0) {
            if (dir.scanningRate != 0.0) {
                return tr("Scanning (%1 %, %2)").arg(dir.scanningPercentage).arg(bitrateToString(dir.scanningRate * 0.008, true).data());
            }
            return tr("Scanning (%1 %)").arg(dir.scanningPercentage);
        }
        return tr("Scanning");
    case SyncthingDirStatus::WaitingToSync:
        return tr("Waiting to sync");
    case SyncthingDirStatus::PreparingToSync:
        return tr("Preparing to sync");
    case SyncthingDirStatus::Synchronizing:
        return dir.completionPercentage > 0 ? tr("Synchronizing (%1 %)").arg(dir.completionPercentage) : tr("Synchronizing");
    case SyncthingDirStatus::Cleaning:
        return tr("Cleaning");
    case SyncthingDirStatus::WaitingToClean:
        return tr("Waiting to clean");
    case SyncthingDirStatus::OutOfSync:
        return tr("Out of sync");
    }
    return QString();
}

QVariant SyncthingDirectoryModel::dirStatusColor(const SyncthingDir &dir) const
{
    if (dir.paused && dir.status != SyncthingDirStatus::OutOfSync) {
        return QVariant();
    }
    if (dir.isUnshared()) {
        return Colors::orange(m_brightColors);
    }
    switch (dir.status) {
    case SyncthingDirStatus::Unknown:
        break;
    case SyncthingDirStatus::Idle:
        return Colors::green(m_brightColors);
    case SyncthingDirStatus::WaitingToScan:
    case SyncthingDirStatus::WaitingToSync:
    case SyncthingDirStatus::WaitingToClean:
        return Colors::orange(m_brightColors);
    case SyncthingDirStatus::Scanning:
    case SyncthingDirStatus::PreparingToSync:
    case SyncthingDirStatus::Synchronizing:
    case SyncthingDirStatus::Cleaning:
        return Colors::blue(m_brightColors);
    case SyncthingDirStatus::OutOfSync:
        return Colors::red(m_brightColors);
    }
    return QVariant();
}

void SyncthingDirectoryModel::updateRowCount()
{
    m_rowCount.clear();
    m_rowCount.reserve(m_dirs.size());
    for (const auto &dir : m_dirs) {
        m_rowCount.emplace_back(computeDirectoryRowCount(dir));
    }
}

} // namespace Data
