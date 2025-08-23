#include "./syncthingdevicemodel.h"
#include "./colors.h"
#include "./syncthingicons.h"

#include <syncthingconnector/syncthingconnection.h>
#include <syncthingconnector/utils.h>

#include <c++utilities/conversion/stringconversion.h>

#include <QRegularExpression>
#include <QStringBuilder>

using namespace std;
using namespace CppUtilities;

namespace Data {

static int computeDeviceRowCount(const SyncthingDev &dev)
{
    // hide connection type, last seen and everything after introducer (eg. traffic) unless connected
    return dev.status == SyncthingDevStatus::ThisDevice ? 7 : (dev.isConnected() ? 11 : 6);
}

SyncthingDeviceModel::SyncthingDeviceModel(SyncthingConnection &connection, QObject *parent)
    : SyncthingModel(connection, parent)
    , m_devs(connection.devInfo())
{
    updateRowCount();
    connect(&m_connection, &SyncthingConnection::devStatusChanged, this, &SyncthingDeviceModel::devStatusChanged);
}

QHash<int, QByteArray> SyncthingDeviceModel::roleNames() const
{
    const static QHash<int, QByteArray> roles{
        { Qt::DisplayRole, "name" },
        { DeviceStatus, "status" },
        { Qt::DecorationRole, "statusIcon" },
        { DevicePaused, "paused" },
        { IsThisDevice, "isThisDevice" },
        { IsPinned, "isPinned" },
        { DeviceStatusString, "statusString" },
        { DeviceStatusColor, "statusColor" },
        { DeviceId, "devId" },
        { DeviceDetail, "detail" },
        { DeviceDetailIcon, "detailIcon" },
        { DeviceDetailTooltip, "detailTooltip" },
        { DeviceNeededItemsCount, "neededItemsCount" },
    };
    return roles;
}

const QVector<int> &SyncthingDeviceModel::colorRoles() const
{
    static const QVector<int> colorRoles({ Qt::DecorationRole, Qt::ForegroundRole, DeviceStatusColor, DeviceDetailIcon });
    return colorRoles;
}

/*!
 * \brief Returns the device info for the specified \a index. The returned object is not persistent.
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
        const auto &dev = m_devs[static_cast<size_t>(index.parent().row())];
        const auto skipRows = !dev.isConnected();
        auto row = skipRows && index.row() >= 2 ? index.row() + 2 : index.row();
        if (skipRows && row >= 5) {
            row += 2;
        }
        switch (role) {
        case Qt::DisplayRole:
        case Qt::EditRole:
            if (index.column() == 0) {
                // attribute names
                switch (row) {
                case 0:
                    return tr("ID");
                case 1:
                    return tr("Out of Sync items");
                case 2:
                    return tr("Incoming traffic");
                case 3:
                    return tr("Outgoing traffic");
                case 4:
                    return tr("Address");
                case 5:
                    return tr("Connection type");
                case 6:
                    return tr("Last seen");
                case 7:
                    return tr("Compression");
                case 8:
                    return tr("Certificate");
                case 9:
                    return tr("Introducer");
                case 10:
                    return tr("Version");
                }
                break;
            }
            [[fallthrough]];
        case DeviceDetail:
            if (index.column() == 1 || role == DeviceDetail) {
                // attribute values
                switch (row) {
                case 0:
                    return dev.id;
                case 1:
                    if (dev.overallCompletion.needed.isNull()) {
                        return tr("none");
                    }
                    return tr("%1 item(s), ~ %2", nullptr, trQuandity(dev.overallCompletion.needed.items))
                        .arg(dev.overallCompletion.needed.items)
                        .arg(dataSizeToString(dev.overallCompletion.needed.bytes).data());
                case 2:
                    return QString::fromStdString(dataSizeToString(dev.totalIncomingTraffic));
                case 3:
                    return QString::fromStdString(dataSizeToString(dev.totalOutgoingTraffic));
                case 4:
                    if (dev.connectionAddress.isEmpty()) {
                        return dev.addresses.join(QStringLiteral(", "));
                    } else {
                        return QVariant(
                            dev.connectionAddress % QStringLiteral(" (") % dev.addresses.join(QStringLiteral(", ")) % QStringLiteral(")"));
                    }
                case 5:
                    if (!dev.connectionType.isEmpty()) {
                        return QVariant(
                            dev.connectionType % QStringLiteral(" (") % (dev.connectionLocal ? tr("local") : tr("remote")) % QStringLiteral(")"));
                    } else {
                        return QVariant();
                    }
                case 6:
                    return dev.lastSeen.isNull() ? tr("unknown or this device")
                                                 : QString::fromLatin1(dev.lastSeen.toString(DateTimeOutputFormat::DateAndTime, true).data());
                case 7:
                    return dev.compression;
                case 8:
                    return dev.certName.isEmpty() ? tr("none") : dev.certName;
                case 9:
                    return dev.introducer ? tr("yes") : tr("no");
                case 10: {
                    const QString *version = nullptr;
                    if (dev.status == SyncthingDevStatus::ThisDevice) {
                        if (m_thisDevVersion.isEmpty()) {
                            static const auto versionRegex
                                = QRegularExpression(QStringLiteral("(syncthing )?(v[^\\(\\)\\s]*)([^\\(\\)]*)(\\(.*(\\))).*"));
                            const auto versionMatch = versionRegex.match(m_connection.syncthingVersion());
                            if (versionMatch.hasMatch()) {
                                m_thisDevVersion = versionMatch.captured(2) % QChar(' ') % versionMatch.captured(4);
                            }
                        }
                        if (!m_thisDevVersion.isEmpty()) {
                            version = &m_thisDevVersion;
                        }
                    } else if (!dev.clientVersion.isEmpty()) {
                        version = &dev.clientVersion;
                    }
                    return version ? *version : tr("unknown");
                }
                }
            }
            break;
        case Qt::DecorationRole:
        case DeviceDetailIcon:
            if (index.column() == 0) {
                // attribute icons
                const auto &icons = commonForkAwesomeIcons();
                switch (row) {
                case 0:
                    return icons.hashtag;
                case 1:
                    return icons.exchange;
                case 2:
                    return icons.cloudDownload;
                case 3:
                    return icons.cloudUpload;
                case 4:
                    return icons.link;
                case 5:
                    return icons.signal;
                case 6:
                    return icons.eye;
                case 7:
                    return icons.fileArchive;
                case 8:
                    return icons.certificate;
                case 9:
                    return icons.networkWired;
                case 10:
                    return icons.tag;
                }
            }
            break;
        case Qt::ForegroundRole:
            switch (index.column()) {
            case 1:
                switch (row) {
                case 1:
                    if (dev.overallCompletion.needed.isNull()) {
                        return Colors::gray(m_brightColors);
                    }
                    break;
                case 6:
                    if (dev.lastSeen.isNull()) {
                        return Colors::gray(m_brightColors);
                    }
                    break;
                case 8:
                    if (dev.certName.isEmpty()) {
                        return Colors::gray(m_brightColors);
                    }
                    break;
                }
            }
            break;
        case Qt::ToolTipRole:
        case DeviceDetailTooltip:
            switch ((m_singleColumnMode || role == DeviceDetailTooltip) ? 1 : index.column()) {
            case 1:
                switch (row) {
                case 4:
                    if (!dev.connectionType.isEmpty()) {
                        return dev.connectionType;
                    }
                    break;
                case 6:
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
            return dev.statusString();
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
                case SyncthingDevStatus::ThisDevice:
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
    case IsPinned:
    case IsThisDevice:
        return dev.status == SyncthingDevStatus::ThisDevice;
    case DeviceStatus:
        return static_cast<int>(dev.status);
    case DevicePaused:
        return dev.paused;
    case DeviceStatusString:
        return dev.statusString();
    case DeviceStatusColor:
        return devStatusColor(dev);
    case DeviceId:
        return dev.id;
    case DeviceNeededItemsCount:
        return dev.overallCompletion.needed.items;
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
    } else if (!parent.parent().isValid() && static_cast<std::size_t>(parent.row()) < m_rowCount.size()) {
        return m_rowCount[static_cast<std::size_t>(parent.row())];
    } else {
        return 0;
    }
}

void SyncthingDeviceModel::devStatusChanged(const SyncthingDev &dev, int index)
{
    if (index < 0 || static_cast<std::size_t>(index) >= m_rowCount.size()) {
        return;
    }

    // update top-level indices
    const QModelIndex modelIndex1(this->index(index, 0, QModelIndex()));
    static const QVector<int> modelRoles1({ Qt::DisplayRole, Qt::EditRole, Qt::DecorationRole, Qt::ForegroundRole, DevicePaused, DeviceStatus,
        DeviceStatusString, DeviceStatusColor, DeviceId, IsThisDevice, IsPinned, DeviceNeededItemsCount });
    emit dataChanged(modelIndex1, modelIndex1, modelRoles1);
    const QModelIndex modelIndex2(this->index(index, 1, QModelIndex()));
    static const QVector<int> modelRoles2({ Qt::DisplayRole, Qt::EditRole, Qt::ForegroundRole });
    emit dataChanged(modelIndex2, modelIndex2, modelRoles2);

    // remove/insert detail rows
    const auto oldRowCount = m_rowCount[static_cast<std::size_t>(index)];
    const auto newRowCount = computeDeviceRowCount(dev);
    const auto newLastRow = newRowCount - 1;
    if (newRowCount < oldRowCount) {
        // remove surplus rows
        beginRemoveRows(modelIndex1, newRowCount, oldRowCount - 1);
        m_rowCount[static_cast<std::size_t>(index)] = newRowCount;
        endRemoveRows();
    } else if (oldRowCount < newRowCount) {
        // insert additional rows
        beginInsertRows(modelIndex1, oldRowCount, newLastRow);
        m_rowCount[static_cast<std::size_t>(index)] = newRowCount;
        endInsertRows();
    }

    // update detail rows
    static const QVector<int> modelRoles3({ Qt::DisplayRole, Qt::EditRole, Qt::ToolTipRole, DeviceDetailTooltip });
    emit dataChanged(this->index(0, 1, modelIndex1), this->index(newLastRow, 1, modelIndex1), modelRoles3);
    static const QVector<int> modelRoles4({ Qt::DisplayRole, Qt::EditRole, DeviceDetail, DeviceDetailIcon });
    emit dataChanged(this->index(0, 0, modelIndex1), this->index(newLastRow, 0, modelIndex1), modelRoles4);
}

void SyncthingDeviceModel::handleConfigInvalidated()
{
    beginResetModel();
    m_thisDevVersion.clear();
}

void SyncthingDeviceModel::handleNewConfigAvailable()
{
    updateRowCount();
    endResetModel();
}

void SyncthingDeviceModel::handleStatusIconsChanged()
{
    invalidateTopLevelIndicies(QVector<int>({ Qt::DecorationRole }));
}

void SyncthingDeviceModel::handleForkAwesomeIconsChanged()
{
    invalidateNestedIndicies(QVector<int>({ Qt::DecorationRole, DeviceDetailIcon }));
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
    case SyncthingDevStatus::ThisDevice:
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

void SyncthingDeviceModel::updateRowCount()
{
    m_rowCount.clear();
    m_rowCount.reserve(m_devs.size());
    for (const auto &dev : m_devs) {
        m_rowCount.emplace_back(computeDeviceRowCount(dev));
    }
}

} // namespace Data
