#include "./syncthingdirectorymodel.h"
#include "./syncthingicons.h"
#include "./colors.h"

#include "../connector/syncthingconnection.h"
#include "../connector/utils.h"

#include <QStringBuilder>

using namespace ChronoUtilities;

namespace Data {

SyncthingDirectoryModel::SyncthingDirectoryModel(SyncthingConnection &connection, QObject *parent) :
    SyncthingModel(connection, parent),
    m_dirs(connection.dirInfo())
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
    return (index.parent().isValid() ? dirInfo(index.parent()) : (static_cast<size_t>(index.row()) < m_dirs.size() ? &m_dirs[static_cast<size_t>(index.row())] : nullptr));
}

QModelIndex SyncthingDirectoryModel::index(int row, int column, const QModelIndex &parent) const
{
    if(!parent.isValid()) {
        // top-level: all dir labels/IDs
        if(row < rowCount(parent)) {
            return createIndex(row, column, static_cast<quintptr>(-1));
        }
    } else if(!parent.parent().isValid()) {
        // dir-level: dir attributes
        if(row < rowCount(parent)) {
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
            if(static_cast<size_t>(index.parent().row()) < m_dirs.size()) {
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
                        case 7: return tr("Errors");
                        }
                        break;
                    case 1: // attribute values
                        const SyncthingDir &dir = m_dirs[static_cast<size_t>(index.parent().row())];
                        switch(index.row()) {
                        case 0: return dir.id;
                        case 1: return dir.path;
                        case 2: return dir.devices.join(QStringLiteral(", "));
                        case 3: return dir.readOnly ? tr("yes") : tr("no");
                        case 4: return QString::fromLatin1(TimeSpan::fromSeconds(dir.rescanInterval).toString(TimeSpanOutputFormat::WithMeasures, true).data());
                        case 5: return dir.lastScanTime.isNull() ? tr("unknown") : QString::fromLatin1(dir.lastScanTime.toString(DateTimeOutputFormat::DateAndTime, true).data());
                        case 6: return dir.lastFileName.isEmpty() ? tr("unknown") : dir.lastFileName;
                        case 7: return dir.errors.empty() ? tr("none") : tr("%1 item(s) out of sync", nullptr, static_cast<int>(dir.errors.size())).arg(dir.errors.size());
                        }
                        break;
                    }
                    break;
                case Qt::ForegroundRole:
                    switch(index.column()) {
                    case 1:
                        const SyncthingDir &dir = m_dirs[static_cast<size_t>(index.parent().row())];
                        switch(index.row()) {
                        case 5:
                            if(dir.lastScanTime.isNull()) {
                                return Colors::gray(m_brightColors);
                            }
                            break;
                        case 6:
                            return dir.lastFileName.isEmpty() ? Colors::gray(m_brightColors) : (dir.lastFileDeleted ? Colors::red(m_brightColors) : QVariant());
                        case 7:
                            return dir.errors.empty() ? Colors::gray(m_brightColors) : Colors::red(m_brightColors);
                        }
                    }
                    break;
                case Qt::ToolTipRole:
                    switch(index.column()) {
                    case 1:
                        const SyncthingDir &dir = m_dirs[static_cast<size_t>(index.parent().row())];
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
                        case 7:
                            if(!dir.errors.empty()) {
                                QStringList errors;
                                errors.reserve(static_cast<int>(dir.errors.size()));
                                for(const auto &error : dir.errors) {
                                    errors << error.path;
                                }
                                return QVariant(QStringLiteral("<b>") % tr("Failed items") % QStringLiteral("</b><ul><li>") % errors.join(QString()) % QStringLiteral("</li></ul>") % tr("Click for details"));
                            }
                        }
                    }
                default:
                    ;
                }
            }
        } else if(static_cast<size_t>(index.row()) < m_dirs.size()) {
            // dir IDs and status
            const SyncthingDir &dir = m_dirs[static_cast<size_t>(index.row())];
            switch(role) {
            case Qt::DisplayRole:
            case Qt::EditRole:
                switch(index.column()) {
                case 0: return dir.label.isEmpty() ? dir.id : dir.label;
                case 1:
                    if(dir.paused && dir.status != SyncthingDirStatus::OutOfSync) {
                        return tr("Paused");
                    } else {
                        switch(dir.status) {
                        case SyncthingDirStatus::Unknown: return tr("Unknown status");
                        case SyncthingDirStatus::Unshared: return tr("Unshared");
                        case SyncthingDirStatus::Idle: return tr("Idle");
                        case SyncthingDirStatus::Scanning: return dir.progressPercentage > 0 ? tr("Scanning (%1 %)").arg(dir.progressPercentage) : tr("Scanning");
                        case SyncthingDirStatus::Synchronizing: return dir.progressPercentage > 0 ? tr("Synchronizing (%1 %)").arg(dir.progressPercentage) : tr("Synchronizing");
                        case SyncthingDirStatus::OutOfSync: return tr("Out of sync");
                        }
                    }
                    break;
                }
                break;
            case Qt::DecorationRole:
                switch(index.column()) {
                case 0:
                    if(dir.paused && dir.status != SyncthingDirStatus::OutOfSync) {
                        return statusIcons().pause;
                    } else {
                        switch(dir.status) {
                        case SyncthingDirStatus::Unknown:
                        case SyncthingDirStatus::Unshared: return statusIcons().disconnected;
                        case SyncthingDirStatus::Idle: return statusIcons().idling;
                        case SyncthingDirStatus::Scanning: return statusIcons().scanninig;
                        case SyncthingDirStatus::Synchronizing: return statusIcons().sync;
                        case SyncthingDirStatus::OutOfSync: return statusIcons().error;
                        }
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
                    if(dir.paused && dir.status != SyncthingDirStatus::OutOfSync) {
                        break;
                    } else {
                        switch(dir.status) {
                        case SyncthingDirStatus::Unknown: break;
                        case SyncthingDirStatus::Idle: return Colors::green(m_brightColors);
                        case SyncthingDirStatus::Unshared: return Colors::orange(m_brightColors);
                        case SyncthingDirStatus::Scanning:
                        case SyncthingDirStatus::Synchronizing: return Colors::blue(m_brightColors);
                        case SyncthingDirStatus::OutOfSync: return Colors::red(m_brightColors);
                        }
                    }
                    break;
                }
                break;
            case DirectoryStatus:
                return static_cast<int>(dir.status);
            case DirectoryPaused:
                return dir.paused;
            default:
                ;
            }
        }
    }
    return QVariant();
}

bool SyncthingDirectoryModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    Q_UNUSED(index) Q_UNUSED(value) Q_UNUSED(role)
    return false;
}

int SyncthingDirectoryModel::rowCount(const QModelIndex &parent) const
{
    if(!parent.isValid()) {
        return static_cast<int>(m_dirs.size());
    } else if(!parent.parent().isValid()) {
        return 8;
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
    emit dataChanged(this->index(0, 1, modelIndex1), this->index(7, 1, modelIndex1), QVector<int>() << Qt::DisplayRole);
}

} // namespace Data
