#include "./syncthingdevicemodel.h"
#include "./colors.h"
#include "./syncthingicons.h"

#include "../connector/syncthingconnection.h"
#include "../connector/utils.h"

using namespace std;
using namespace ChronoUtilities;

namespace Data {

SyncthingDeviceModel::SyncthingDeviceModel(SyncthingConnection &connection, QObject *parent)
    : SyncthingModel(connection, parent)
    , m_devs(connection.devInfo())
{
    connect(&m_connection, &SyncthingConnection::newConfig, this, &SyncthingDeviceModel::newConfig);
    connect(&m_connection, &SyncthingConnection::newDevices, this, &SyncthingDeviceModel::newDevices);
    connect(&m_connection, &SyncthingConnection::devStatusChanged, this, &SyncthingDeviceModel::devStatusChanged);
}

QHash<int, QByteArray> SyncthingDeviceModel::roleNames() const
{
    const static auto roles([] {
        QHash<int, QByteArray> roles;
        roles[Qt::DisplayRole] = "name";
        roles[DeviceStatus] = "status";
        roles[Qt::DecorationRole] = "statusIcon";
        roles[DevicePaused] = "paused";
        roles[DeviceStatusString] = "statusString";
        roles[DeviceStatusColor] = "statusColor";
        roles[DeviceId] = "devId";
        roles[DeviceDetail] = "detail";
        return roles;
    }());
    return roles;
}

const QVector<int> &SyncthingDeviceModel::colorRoles() const
{
    static const QVector<int> colorRoles({ Qt::ForegroundRole, DeviceStatusColor });
    return colorRoles;
}

/*!
 * \brief Returns the device info for the spcified \a index. The returned object is not persistent.
 */
const SyncthingDev *SyncthingDeviceModel::devInfo(const QModelIndex &index) const
{
    return (index.parent().isValid() ? devInfo(index.parent())
                                     : (static_cast<size_t>(index.row()) < m_devs.size() ? &m_devs[static_cast<size_t>(index.row())] : nullptr));
}

QModelIndex SyncthingDeviceModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!parent.isValid()) {
        // top-level: all dev IDs
        if (row < rowCount(parent)) {
            return createIndex(row, column, static_cast<quintptr>(-1));
        }
    } else if (!parent.parent().isValid()) {
        // dev-level: dev attributes
        if (row < rowCount(parent)) {
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

QVariant SyncthingDeviceModel::data(const QModelIndex &index, int role) const
{
    if (index.isValid()) {
        if (index.parent().isValid()) {
            // dir attributes
            if (static_cast<size_t>(index.parent().row()) < m_devs.size()) {
                switch (role) {
                case Qt::DisplayRole:
                case Qt::EditRole:
                    if (index.column() == 0) {
                        // attribute names
                        switch (index.row()) {
                        case 0:
                            return tr("ID");
                        case 1:
                            return tr("Addresses");
                        case 2:
                            return tr("Last seen");
                        case 3:
                            return tr("Compression");
                        case 4:
                            return tr("Certificate");
                        case 5:
                            return tr("Introducer");
                        }
                        break;
                    }
                    FALLTHROUGH;
                case DeviceDetail:
                    if (index.column() == 1 || role == DeviceDetail) {
                        // attribute values
                        const SyncthingDev &dev = m_devs[static_cast<size_t>(index.parent().row())];
                        switch (index.row()) {
                        case 0:
                            return dev.id;
                        case 1:
                            return dev.addresses.join(QStringLiteral(", "));
                        case 2:
                            return dev.lastSeen.isNull() ? tr("unknown or own device")
                                                         : QString::fromLatin1(dev.lastSeen.toString(DateTimeOutputFormat::DateAndTime, true).data());
                        case 3:
                            return dev.compression;
                        case 4:
                            return dev.certName.isEmpty() ? tr("none") : dev.certName;
                        case 5:
                            return dev.introducer ? tr("yes") : tr("no");
                        }
                    }
                    break;
                case Qt::ForegroundRole:
                    switch (index.column()) {
                    case 1:
                        const SyncthingDev &dev = m_devs[static_cast<size_t>(index.parent().row())];
                        switch (index.row()) {
                        case 2:
                            if (dev.lastSeen.isNull()) {
                                return Colors::gray(m_brightColors);
                            }
                            break;
                        case 4:
                            if (dev.certName.isEmpty()) {
                                return Colors::gray(m_brightColors);
                            }
                            break;
                        }
                    }
                    break;
                case Qt::ToolTipRole:
                    switch (index.column()) {
                    case 1:
                        switch (index.row()) {
                        case 2:
                            const SyncthingDev &dev = m_devs[static_cast<size_t>(index.parent().row())];
                            if (!dev.lastSeen.isNull()) {
                                return agoString(dev.lastSeen);
                            }
                            break;
                        }
                    }
                    break;
                default:;
                }
            }
        } else if (static_cast<size_t>(index.row()) < m_devs.size()) {
            // dir IDs and status
            const SyncthingDev &dev = m_devs[static_cast<size_t>(index.row())];
            switch (role) {
            case Qt::DisplayRole:
            case Qt::EditRole:
                switch (index.column()) {
                case 0:
                    return dev.name.isEmpty() ? dev.id : dev.name;
                case 1:
                    return devStatusString(dev);
                }
            case Qt::DecorationRole:
                switch (index.column()) {
                case 0:
                    if (dev.paused) {
                        return statusIcons().pause;
                    } else {
                        switch (dev.status) {
                        case SyncthingDevStatus::Unknown:
                        case SyncthingDevStatus::Disconnected:
                            return statusIcons().disconnected;
                        case SyncthingDevStatus::OwnDevice:
                        case SyncthingDevStatus::Idle:
                            return statusIcons().idling;
                        case SyncthingDevStatus::Synchronizing:
                            return statusIcons().sync;
                        case SyncthingDevStatus::OutOfSync:
                        case SyncthingDevStatus::Rejected:
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
                    return devStatusColor(dev);
                }
                break;
            case DeviceStatus:
                return static_cast<int>(dev.status);
            case DevicePaused:
                return dev.paused;
            case IsOwnDevice:
                return dev.status == SyncthingDevStatus::OwnDevice;
            case DeviceStatusString:
                return devStatusString(dev);
            case DeviceStatusColor:
                return devStatusColor(dev);
            case DeviceId:
                return dev.id;
            default:;
            }
        }
    }
    return QVariant();
}

bool SyncthingDeviceModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    Q_UNUSED(index) Q_UNUSED(value) Q_UNUSED(role) return false;
}

int SyncthingDeviceModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid()) {
        return static_cast<int>(m_devs.size());
    } else if (!parent.parent().isValid()) {
        return 6;
    } else {
        return 0;
    }
}

int SyncthingDeviceModel::columnCount(const QModelIndex &parent) const
{
    if (!parent.isValid()) {
        return 2; // name/id, status
    } else if (!parent.parent().isValid()) {
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
    static const QVector<int> modelRoles1(
        { Qt::DisplayRole, Qt::EditRole, Qt::DecorationRole, DevicePaused, DeviceStatus, DeviceStatusString, DeviceStatusColor, DeviceId });
    emit dataChanged(modelIndex1, modelIndex1, modelRoles1);
    const QModelIndex modelIndex2(this->index(index, 1, QModelIndex()));
    static const QVector<int> modelRoles2({ Qt::DisplayRole, Qt::EditRole, Qt::ForegroundRole });
    emit dataChanged(modelIndex2, modelIndex2, modelRoles2);
    static const QVector<int> modelRoles3({ Qt::DisplayRole, Qt::EditRole, Qt::ToolTipRole });
    emit dataChanged(this->index(0, 1, modelIndex1), this->index(5, 1, modelIndex1), modelRoles3);
    static const QVector<int> modelRoles4({ Qt::DisplayRole, Qt::EditRole, DeviceDetail });
    emit dataChanged(this->index(0, 0, modelIndex1), this->index(5, 0, modelIndex1), modelRoles4);
}

QString SyncthingDeviceModel::devStatusString(const SyncthingDev &dev)
{
    if (dev.paused) {
        return tr("Paused");
    }
    switch (dev.status) {
    case SyncthingDevStatus::Unknown:
        return tr("Unknown status");
    case SyncthingDevStatus::OwnDevice:
        return tr("Own device");
    case SyncthingDevStatus::Idle:
        return tr("Idle");
    case SyncthingDevStatus::Disconnected:
        return tr("Disconnected");
    case SyncthingDevStatus::Synchronizing:
        return dev.progressPercentage > 0 ? tr("Synchronizing (%1 %)").arg(dev.progressPercentage) : tr("Synchronizing");
    case SyncthingDevStatus::OutOfSync:
        return tr("Out of sync");
    case SyncthingDevStatus::Rejected:
        return tr("Rejected");
    }
    return QString();
}

QVariant SyncthingDeviceModel::devStatusColor(const SyncthingDev &dev) const
{
    if (dev.paused) {
        return QVariant();
    }
    switch (dev.status) {
    case SyncthingDevStatus::Unknown:
        break;
    case SyncthingDevStatus::Disconnected:
        break;
    case SyncthingDevStatus::OwnDevice:
    case SyncthingDevStatus::Idle:
        return Colors::green(m_brightColors);
    case SyncthingDevStatus::Synchronizing:
        return Colors::blue(m_brightColors);
    case SyncthingDevStatus::OutOfSync:
    case SyncthingDevStatus::Rejected:
        return Colors::red(m_brightColors);
    }
    return QVariant();
}

} // namespace Data
