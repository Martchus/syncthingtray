#include "./syncthingdevicemodel.h"

#include "../connector/syncthingconnection.h"
#include "../connector/utils.h"

using namespace ChronoUtilities;

namespace Data {

SyncthingDeviceModel::SyncthingDeviceModel(SyncthingConnection &connection, QObject *parent) :
    QAbstractItemModel(parent),
    m_connection(connection),
    m_devs(connection.devInfo()),
    m_unknownIcon(QIcon(QStringLiteral(":/icons/hicolor/scalable/status/syncthing-disconnected.svg"))),
    m_idleIcon(QIcon(QStringLiteral(":/icons/hicolor/scalable/status/syncthing-ok.svg"))),
    m_syncIcon(QIcon(QStringLiteral(":/icons/hicolor/scalable/status/syncthing-sync.svg"))),
    m_errorIcon(QIcon(QStringLiteral(":/icons/hicolor/scalable/status/syncthing-error.svg"))),
    m_pausedIcon(QIcon(QStringLiteral(":/icons/hicolor/scalable/status/syncthing-pause.svg"))),
    m_otherIcon(QIcon(QStringLiteral(":/icons/hicolor/scalable/status/syncthing-default.svg")))
{
    connect(&m_connection, &SyncthingConnection::newConfig, this, &SyncthingDeviceModel::newConfig);
    connect(&m_connection, &SyncthingConnection::newDevices, this, &SyncthingDeviceModel::newDevices);
    connect(&m_connection, &SyncthingConnection::devStatusChanged, this, &SyncthingDeviceModel::devStatusChanged);
}

/*!
 * \brief Returns the device info for the spcified \a index. The returned object is not persistent.
 */
const SyncthingDev *SyncthingDeviceModel::devInfo(const QModelIndex &index) const
{
    return (index.parent().isValid() ? devInfo(index.parent()) : (static_cast<size_t>(index.row()) < m_devs.size() ? &m_devs[static_cast<size_t>(index.row())] : nullptr));
}

QModelIndex SyncthingDeviceModel::index(int row, int column, const QModelIndex &parent) const
{
    if(!parent.isValid()) {
        // top-level: all dev IDs
        if(row < rowCount(parent)) {
            return createIndex(row, column, static_cast<quintptr>(-1));
        }
    } else if(!parent.parent().isValid()) {
        // dev-level: dev attributes
        if(row < rowCount(parent)) {
            return createIndex(row, column, static_cast<quintptr>(parent.row()));
        }
    }
    return QModelIndex();
}

QModelIndex SyncthingDeviceModel::parent(const QModelIndex &child) const
{
    return child.internalId() != static_cast<quintptr>(-1) ? index(static_cast<int>(child.internalId()), 0, QModelIndex()) : QModelIndex();
}

QVariant SyncthingDeviceModel::headerData(int section, Qt::Orientation orientation, int role) const
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

QVariant SyncthingDeviceModel::data(const QModelIndex &index, int role) const
{
    if(index.isValid()) {
        if(index.parent().isValid()) {
            // dir attributes
            if(static_cast<size_t>(index.parent().row()) < m_devs.size()) {
                switch(role) {
                case Qt::DisplayRole:
                case Qt::EditRole:
                    switch(index.column()) {
                    case 0: // attribute names
                        switch(index.row()) {
                        case 0: return tr("ID");
                        case 1: return tr("Addresses");
                        case 2: return tr("Last seen");
                        case 3: return tr("Compression");
                        case 4: return tr("Certificate");
                        case 5: return tr("Introducer");
                        }
                        break;
                    case 1: // attribute values
                        const SyncthingDev &dev = m_devs[static_cast<size_t>(index.parent().row())];
                        switch(index.row()) {
                        case 0: return dev.id;
                        case 1: return dev.addresses.join(QStringLiteral(", "));
                        case 2: return dev.lastSeen.isNull() ? tr("unknown or own device") : QString::fromLatin1(dev.lastSeen.toString(DateTimeOutputFormat::DateAndTime, true).data());
                        case 3: return dev.compression;
                        case 4: return dev.certName.isEmpty() ? tr("none") : dev.certName;
                        case 5: return dev.introducer ? tr("yes") : tr("no");
                        }
                        break;
                    }
                    break;
                case Qt::ForegroundRole:
                    switch(index.column()) {
                    case 1:
                        const SyncthingDev &dev = m_devs[static_cast<size_t>(index.parent().row())];
                        switch(index.row()) {
                        case 2:
                            if(dev.lastSeen.isNull()) {
                                return QColor(Qt::gray);
                            }
                            break;
                        case 4:
                            if(dev.certName.isEmpty()) {
                                return QColor(Qt::gray);
                            }
                            break;
                        }
                    }
                    break;
                case Qt::ToolTipRole:
                    switch(index.column()) {
                    case 1:
                        switch(index.row()) {
                        case 2:
                            const SyncthingDev &dev = m_devs[static_cast<size_t>(index.parent().row())];
                            if(!dev.lastSeen.isNull()) {
                                return agoString(dev.lastSeen);
                            }
                            break;
                        }
                    }
                default:
                    ;
                }
            }
        } else if(static_cast<size_t>(index.row()) < m_devs.size()) {
            // dir IDs and status
            const SyncthingDev &dev = m_devs[static_cast<size_t>(index.row())];
            switch(role) {
            case Qt::DisplayRole:
            case Qt::EditRole:
                switch(index.column()) {
                case 0: return dev.name.isEmpty() ? dev.id : dev.name;
                case 1:
                    if(dev.paused) {
                        return tr("Paused");
                    } else {
                        switch(dev.status) {
                        case DevStatus::Unknown: return tr("Unknown status");
                        case DevStatus::OwnDevice: return tr("Own device");
                        case DevStatus::Idle: return tr("Idle");
                        case DevStatus::Disconnected: return tr("Disconnected");
                        case DevStatus::Synchronizing: return dev.progressPercentage > 0 ? tr("Synchronizing (%1 %)").arg(dev.progressPercentage) : tr("Synchronizing");
                        case DevStatus::OutOfSync: return tr("Out of sync");
                        case DevStatus::Rejected: return tr("Rejected");
                        }
                    }
                    break;
                }
                break;
            case Qt::DecorationRole:
                switch(index.column()) {
                case 0:
                    if(dev.paused) {
                        return m_pausedIcon;
                    } else {
                        switch(dev.status) {
                        case DevStatus::Unknown:
                        case DevStatus::Disconnected: return m_unknownIcon;
                        case DevStatus::OwnDevice:
                        case DevStatus::Idle: return m_idleIcon;
                        case DevStatus::Synchronizing: return m_syncIcon;
                        case DevStatus::OutOfSync:
                        case DevStatus::Rejected: return m_errorIcon;
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
                    if(!dev.paused) {
                        switch(dev.status) {
                        case DevStatus::Unknown: break;
                        case DevStatus::Disconnected: break;
                        case DevStatus::OwnDevice:
                        case DevStatus::Idle: return QColor(Qt::darkGreen);
                        case DevStatus::Synchronizing: return QColor(Qt::darkBlue);
                        case DevStatus::OutOfSync:
                        case DevStatus::Rejected: return QColor(Qt::red);
                        }
                    }
                    break;
                }
                break;
            case DeviceStatus:
                return static_cast<int>(dev.status);
            case DevicePaused:
                return dev.paused;
            case IsOwnDevice:
                return dev.status == DevStatus::OwnDevice;
            default:
                ;
            }
        }
    }
    return QVariant();
}

bool SyncthingDeviceModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    Q_UNUSED(index) Q_UNUSED(value) Q_UNUSED(role)
    return false;
}

int SyncthingDeviceModel::rowCount(const QModelIndex &parent) const
{
    if(!parent.isValid()) {
        return static_cast<int>(m_devs.size());
    } else if(!parent.parent().isValid()) {
        return 6;
    } else {
        return 0;
    }
}

int SyncthingDeviceModel::columnCount(const QModelIndex &parent) const
{
    if(!parent.isValid()) {
        return 2; // name/id, status
    } else if(!parent.parent().isValid()) {
        return 2; // field name and value
    } else {
        return 0;
    }
}

void SyncthingDeviceModel::newConfig()
{
    beginResetModel();
}

void SyncthingDeviceModel::newDevices()
{
    endResetModel();
}

void SyncthingDeviceModel::devStatusChanged(const SyncthingDev &, int index)
{
    const QModelIndex modelIndex1(this->index(index, 0, QModelIndex()));
    emit dataChanged(modelIndex1, modelIndex1, QVector<int>() << Qt::DecorationRole);
    const QModelIndex modelIndex2(this->index(index, 1, QModelIndex()));
    emit dataChanged(modelIndex2, modelIndex2, QVector<int>() << Qt::DisplayRole << Qt::ForegroundRole << DeviceStatus);
}

} // namespace Data
