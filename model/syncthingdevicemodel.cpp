#include "./syncthingdevicemodel.h"
#include "./colors.h"
#include "./syncthingicons.h"

#include <syncthingconnector/syncthingconnection.h>
#include <syncthingconnector/utils.h>

#include <c++utilities/conversion/stringconversion.h>

#include <QStringBuilder>

using namespace std;
using namespace CppUtilities;

namespace Data {

SyncthingDeviceModel::SyncthingDeviceModel(SyncthingConnection &connection, QObject *parent)
    : SyncthingModel(connection, parent)
    , m_devs(connection.devInfo())
{
    connect(&m_connection, &SyncthingConnection::devStatusChanged, this, &SyncthingDeviceModel::devStatusChanged);
}

QHash<int, QByteArray> SyncthingDeviceModel::roleNames() const
{
    const static QHash<int, QByteArray> roles{
        { Qt::DisplayRole, "name" },
        { DeviceStatus, "status" },
        { Qt::DecorationRole, "statusIcon" },
        { DevicePaused, "paused" },
        { IsOwnDevice, "isOwnDevice" },
        { DeviceStatusString, "statusString" },
        { DeviceStatusColor, "statusColor" },
        { DeviceId, "devId" },
        { DeviceDetail, "detail" },
        { DeviceDetailIcon, "detailIcon" },
    };
    return roles;
}

const QVector<int> &SyncthingDeviceModel::colorRoles() const
{
    static const QVector<int> colorRoles({ Qt::DecorationRole, Qt::ForegroundRole, DeviceStatusColor, DeviceDetailIcon });
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
    if (!index.isValid()) {
        return QVariant();
    }
    if (index.parent().isValid()) {
        // dir attributes
        if (static_cast<size_t>(index.parent().row()) >= m_devs.size()) {
            return QVariant();
        }
        switch (role) {
        case Qt::DisplayRole:
        case Qt::EditRole:
            if (index.column() == 0) {
                // attribute names
                switch (index.row()) {
                case 0:
                    return tr("ID");
                case 1:
                    return tr("Address");
                case 2:
                    return tr("Last seen");
                case 3:
                    return tr("Compression");
                case 4:
                    return tr("Certificate");
                case 5:
                    return tr("Introducer");
                case 6:
                    return tr("Incoming traffic");
                case 7:
                    return tr("Outgoing traffic");
                case 8:
                    return tr("Version");
                }
                break;
            }
            [[fallthrough]];
        case DeviceDetail:
            if (index.column() == 1 || role == DeviceDetail) {
                // attribute values
                const SyncthingDev &dev = m_devs[static_cast<size_t>(index.parent().row())];
                switch (index.row()) {
                case 0:
                    return dev.id;
                case 1:
                    if (dev.connectionAddress.isEmpty()) {
                        return dev.addresses.join(QStringLiteral(", "));
                    } else {
                        return QVariant(
                            dev.connectionAddress % QStringLiteral(" (") % dev.addresses.join(QStringLiteral(", ")) % QStringLiteral(")"));
                    }
                case 2:
                    return dev.lastSeen.isNull() ? tr("unknown or own device")
                                                 : QString::fromLatin1(dev.lastSeen.toString(DateTimeOutputFormat::DateAndTime, true).data());
                case 3:
                    return dev.compression;
                case 4:
                    return dev.certName.isEmpty() ? tr("none") : dev.certName;
                case 5:
                    return dev.introducer ? tr("yes") : tr("no");
                case 6:
                    return QString::fromStdString(dataSizeToString(dev.totalIncomingTraffic));
                case 7:
                    return QString::fromStdString(dataSizeToString(dev.totalOutgoingTraffic));
                case 8:
                    return dev.clientVersion;
                }
            }
            break;
        case Qt::DecorationRole:
        case DeviceDetailIcon:
            if (index.column() == 0) {
                // attribute icons
                const auto &icons = commonForkAwesomeIcons();
                switch (index.row()) {
                case 0:
                    return icons.hashtag;
                case 1:
                    return icons.link;
                case 2:
                    return icons.eye;
                case 3:
                    return icons.fileArchive;
                case 4:
                    return icons.certificate;
                case 5:
                    return icons.networkWired;
                case 6:
                    return icons.cloudDownloadAlt;
                case 7:
                    return icons.cloudUploadAlt;
                case 8:
                    return icons.tag;
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
                const SyncthingDev &dev = m_devs[static_cast<size_t>(index.parent().row())];
                switch (index.row()) {
                case 1:
                    if (!dev.connectionType.isEmpty()) {
                        return dev.connectionType;
                    }
                    break;
                case 2:
                    if (!dev.lastSeen.isNull()) {
                        return agoString(dev.lastSeen);
                    }
                    break;
                }
            }
            break;
        default:;
        }
        return QVariant();
    }

    if (static_cast<size_t>(index.row()) >= m_devs.size()) {
        return QVariant();
    }

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
        break;
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
    return QVariant();
}

bool SyncthingDeviceModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    Q_UNUSED(index)
    Q_UNUSED(value)
    Q_UNUSED(role)
    return false;
}

int SyncthingDeviceModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid()) {
        return static_cast<int>(m_devs.size());
    } else if (!parent.parent().isValid()) {
        // hide everything after introducer (eg. traffic) unless connected
        const auto *const dev(devInfo(parent));
        return dev && dev->isConnected() ? 9 : 6;
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

void SyncthingDeviceModel::handleStatusIconsChanged()
{
    invalidateTopLevelIndicies(QVector<int>({ Qt::DecorationRole }));
}

void SyncthingDeviceModel::handleForkAwesomeIconsChanged()
{
    invalidateNestedIndicies(QVector<int>({ Qt::DecorationRole, DeviceDetailIcon }));
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
        return dev.overallCompletion.needed.bytes > 0 ? tr("Synchronizing (%1 %, %2)")
                                                            .arg(static_cast<int>(dev.overallCompletion.percentage))
                                                            .arg(QString::fromStdString(dataSizeToString(dev.overallCompletion.needed.bytes)))
                                                      : tr("Synchronizing");
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
