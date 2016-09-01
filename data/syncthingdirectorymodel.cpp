#include "./syncthingdirectorymodel.h"
#include "./syncthingconnection.h"
#include "./utils.h"

using namespace ChronoUtilities;

namespace Data {

SyncthingDirectoryModel::SyncthingDirectoryModel(SyncthingConnection &connection, QObject *parent) :
    QAbstractItemModel(parent),
    m_connection(connection),
    m_dirs(connection.dirInfo()),
    m_unknownIcon(QIcon(QStringLiteral(":/icons/hicolor/scalable/status/syncthing-disconnected.svg"))),
    m_idleIcon(QIcon(QStringLiteral(":/icons/hicolor/scalable/status/syncthing-ok.svg"))),
    m_syncIcon(QIcon(QStringLiteral(":/icons/hicolor/scalable/status/syncthing-sync.svg"))),
    m_errorIcon(QIcon(QStringLiteral(":/icons/hicolor/scalable/status/syncthing-error.svg"))),
    m_pausedIcon(QIcon(QStringLiteral(":/icons/hicolor/scalable/status/syncthing-pause.svg"))),
    m_otherIcon(QIcon(QStringLiteral(":/icons/hicolor/scalable/status/syncthing-default.svg")))
{
    connect(&m_connection, &SyncthingConnection::newConfig, this, &SyncthingDirectoryModel::newConfig);
    connect(&m_connection, &SyncthingConnection::newDirs, this, &SyncthingDirectoryModel::newDirs);
    connect(&m_connection, &SyncthingConnection::dirStatusChanged, this, &SyncthingDirectoryModel::dirStatusChanged);
}

/*!
 * \brief Returns the directory info for the spcified \a index. The returned object is not persistent.
 */
const SyncthingDir *SyncthingDirectoryModel::dirInfo(const QModelIndex &index) const
{
    return (index.parent().isValid() ? dirInfo(index.parent()) : (index.row() < m_dirs.size() ? &m_dirs[index.row()] : nullptr));
}

QModelIndex SyncthingDirectoryModel::index(int row, int column, const QModelIndex &parent) const
{
    if(!parent.isValid()) {
        // top-level: all dir labels/IDs
        if(row < rowCount(parent)) {
            return createIndex(row, column, -1);
        }
    } else if(!parent.parent().isValid()) {
        // dir-level: dir attributes
        if(row < rowCount(parent)) {
            return createIndex(row, column, parent.row());
        }
    }
    return QModelIndex();
}

QModelIndex SyncthingDirectoryModel::parent(const QModelIndex &child) const
{
    return child.internalId() != static_cast<quintptr>(-1) ? index(child.internalId(), 0, QModelIndex()) : QModelIndex();
}

QVariant SyncthingDirectoryModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    switch(orientation) {
    case Qt::Horizontal:
        switch(role) {
        case Qt::DisplayRole:
            switch(section) {
            case 0: return tr("ID");
            case 1: return tr("Status");
            }
            break;
        default:
            ;
        }
        break;
    default:
        ;
    }
    return QVariant();
}

QVariant SyncthingDirectoryModel::data(const QModelIndex &index, int role) const
{
    if(index.isValid()) {
        if(index.parent().isValid()) {
            // dir attributes
            if(index.parent().row() < m_dirs.size()) {
                switch(role) {
                case Qt::DisplayRole:
                case Qt::EditRole:
                    switch(index.column()) {
                    case 0: // attribute names
                        switch(index.row()) {
                        case 0: return tr("ID");
                        case 1: return tr("Path");
                        case 2: return tr("Devices");
                        case 3: return tr("Read-only");
                        case 4: return tr("Rescan interval");
                        case 5: return tr("Last scan");
                        case 6: return tr("Last file");
                        }
                        break;
                    case 1: // attribute values
                        const SyncthingDir &dir = m_dirs[index.parent().row()];
                        switch(index.row()) {
                        case 0: return dir.id;
                        case 1: return dir.path;
                        case 2: return dir.devices.join(QStringLiteral(", "));
                        case 3: return dir.readOnly ? tr("yes") : tr("no");
                        case 4: return QStringLiteral("%1 s").arg(dir.rescanInterval);
                        case 5: return dir.lastScanTime.isNull() ? tr("unknown") : QString::fromLatin1(dir.lastScanTime.toString(DateTimeOutputFormat::DateAndTime, true).data());
                        case 6: return dir.lastFileName.isEmpty() ? tr("unknown") : dir.lastFileName;
                        }
                        break;
                    }
                    break;
                case Qt::ForegroundRole:
                    switch(index.column()) {
                    case 1:
                        const SyncthingDir &dir = m_dirs[index.parent().row()];
                        switch(index.row()) {
                        case 5:
                            if(dir.lastScanTime.isNull()) {
                                return QColor(Qt::gray);
                            }
                            break;
                        case 6:
                            if(dir.lastFileName.isEmpty()) {
                                return QColor(Qt::gray);
                            } else if(dir.lastFileDeleted) {
                                return QColor(Qt::red);
                            }
                            break;
                        }
                    }
                    break;
                case Qt::ToolTipRole:
                    switch(index.column()) {
                    case 1:
                        const SyncthingDir &dir = m_dirs[index.parent().row()];
                        switch(index.row()) {
                        case 5:
                            if(!dir.lastScanTime.isNull()) {
                                return agoString(dir.lastScanTime);
                            }
                            break;
                        case 6:
                            if(!dir.lastFileTime.isNull()) {
                                if(dir.lastFileDeleted) {
                                    return tr("Deleted at %1").arg(QString::fromLatin1(dir.lastFileTime.toString(DateTimeOutputFormat::DateAndTime, true).data()));
                                } else {
                                    return tr("Updated at %1").arg(QString::fromLatin1(dir.lastFileTime.toString(DateTimeOutputFormat::DateAndTime, true).data()));
                                }
                            }
                            break;
                        }
                    }
                default:
                    ;
                }
            }
        } else if(index.row() < m_dirs.size()) {
            // dir IDs and status
            const SyncthingDir &dir = m_dirs[index.row()];
            switch(role) {
            case Qt::DisplayRole:
            case Qt::EditRole:
                switch(index.column()) {
                case 0: return dir.label.isEmpty() ? dir.id : dir.label;
                case 1:
                    switch(dir.status) {
                    case DirStatus::Unknown: return tr("Unknown status");
                    case DirStatus::Idle: return tr("Idle");
                    case DirStatus::Scanning: return dir.progressPercentage > 0 ? tr("Scanning (%1 %)").arg(dir.progressPercentage) : tr("Scanning");
                    case DirStatus::Synchronizing: return dir.progressPercentage > 0 ? tr("Synchronizing (%1 %)").arg(dir.progressPercentage) : tr("Synchronizing");
                    case DirStatus::Paused: return tr("Paused");
                    case DirStatus::OutOfSync: return tr("Out of sync");
                    }
                    break;
                }
                break;
            case Qt::DecorationRole:
                switch(index.column()) {
                case 0:
                    switch(dir.status) {
                    case DirStatus::Unknown: return m_unknownIcon;
                    case DirStatus::Idle: return m_idleIcon;
                    case DirStatus::Scanning: return m_otherIcon;
                    case DirStatus::Synchronizing: return m_syncIcon;
                    case DirStatus::Paused: return m_pausedIcon;
                    case DirStatus::OutOfSync: return m_errorIcon;
                    }
                    break;
                }
                break;
            case Qt::TextAlignmentRole:
                switch(index.column()) {
                case 0: break;
                case 1: return static_cast<int>(Qt::AlignRight | Qt::AlignVCenter);
                }
                break;
            case Qt::ForegroundRole:
                switch(index.column()) {
                case 0: break;
                case 1:
                    switch(dir.status) {
                    case DirStatus::Unknown: break;
                    case DirStatus::Idle: return QColor(Qt::darkGreen);
                    case DirStatus::Scanning: return QColor(Qt::blue);
                    case DirStatus::Synchronizing: return QColor(Qt::blue);
                    case DirStatus::Paused: break;
                    case DirStatus::OutOfSync: return QColor(Qt::red);
                    }
                    break;
                }
                break;
            default:
                ;
            }
        }
    }
    return QVariant();
}

bool SyncthingDirectoryModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    return false;
}

int SyncthingDirectoryModel::rowCount(const QModelIndex &parent) const
{
    if(!parent.isValid()) {
        return m_dirs.size();
    } else if(!parent.parent().isValid()) {
        return 7;
    } else {
        return 0;
    }
}

int SyncthingDirectoryModel::columnCount(const QModelIndex &parent) const
{
    if(!parent.isValid()) {
        return 2; // label/ID, status/buttons
    } else if(!parent.parent().isValid()) {
        return 2; // field name and value
    } else {
        return 0;
    }
}

void SyncthingDirectoryModel::newConfig()
{
    beginResetModel();
}

void SyncthingDirectoryModel::newDirs()
{
    endResetModel();
}

void SyncthingDirectoryModel::dirStatusChanged(const SyncthingDir &, int index)
{
    const QModelIndex modelIndex1(this->index(index, 0, QModelIndex()));
    emit dataChanged(modelIndex1, modelIndex1, QVector<int>() << Qt::DecorationRole);
    const QModelIndex modelIndex2(this->index(index, 1, QModelIndex()));
    emit dataChanged(modelIndex2, modelIndex2, QVector<int>() << Qt::DisplayRole << Qt::ForegroundRole);
}

} // namespace Data
