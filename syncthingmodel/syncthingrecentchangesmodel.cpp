#include "./syncthingrecentchangesmodel.h"
#include "./colors.h"
#include "./syncthingicons.h"

#include <syncthingconnector/syncthingconnection.h>
#include <syncthingconnector/utils.h>

#include <c++utilities/conversion/stringconversion.h>

#include <QStringBuilder>

using namespace std;
using namespace CppUtilities;

namespace Data {

SyncthingRecentChangesModel::SyncthingRecentChangesModel(SyncthingConnection &connection, int maxRows, QObject *parent)
    : SyncthingModel(connection, parent)
    , m_maxRows(maxRows)
{
    connect(&m_connection, &SyncthingConnection::fileChanged, this, &SyncthingRecentChangesModel::fileChanged);
    connect(&m_connection, &SyncthingConnection::statusChanged, this, &SyncthingRecentChangesModel::handleStatusChanged);
}

QHash<int, QByteArray> SyncthingRecentChangesModel::roleNames() const
{
    const static QHash<int, QByteArray> roles{
        { Action, "action" },
        { ActionIcon, "actionIcon" },
        { ModifiedBy, "modifiedBy" },
        { DirectoryId, "directoryId" },
        { DirectoryName, "directoryName" },
        { Path, "path" },
        { EventTime, "eventTime" },
        { ExtendedAction, "extendedAction" },
        { ItemType, "itemType" },
    };
    return roles;
}

const QVector<int> &SyncthingRecentChangesModel::colorRoles() const
{
    static const QVector<int> colorRoles({ Qt::DecorationRole, Qt::ForegroundRole, ActionIcon });
    return colorRoles;
}

QModelIndex SyncthingRecentChangesModel::index(int row, int column, const QModelIndex &parent) const
{
    if (static_cast<size_t>(row) >= m_changes.size() || parent.isValid()) {
        return QModelIndex();
    }
    return createIndex(row, column, static_cast<quintptr>(-1));
}

QModelIndex SyncthingRecentChangesModel::parent(const QModelIndex &child) const
{
    Q_UNUSED(child)
    return QModelIndex();
}

QVariant SyncthingRecentChangesModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    switch (orientation) {
    case Qt::Horizontal:
        switch (role) {
        case Qt::DisplayRole:
            switch (section) {
            case 0:
                return tr("Action");
            case 1:
                return tr("Device");
            case 2:
                return tr("Folder");
            case 3:
                return tr("Path");
            }
            break;
        default:;
        }
        break;
    default:;
    }
    return QVariant();
}

QVariant SyncthingRecentChangesModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.parent().isValid() || static_cast<size_t>(index.row()) >= m_changes.size()) {
        return QVariant();
    }

    const SyncthingRecentChange &change = m_changes[static_cast<size_t>(index.row())];
    switch (role) {
    case Qt::DisplayRole:
    case Qt::EditRole:
        switch (index.column()) {
        case 0:
            return change.fileChange.action;
        case 1:
            return change.deviceName.isEmpty() ? change.fileChange.modifiedBy : change.deviceName;
        case 2:
            return change.directoryName.isEmpty() ? change.directoryId : change.directoryName;
        case 3:
            return change.fileChange.path;
        }
        break;
    case Qt::DecorationRole:
    case ActionIcon:
        switch (index.column()) {
        case 0:
            return change.fileChange.local ? commonForkAwesomeIcons().home : commonForkAwesomeIcons().globe;
        }
        break;
    case Qt::ToolTipRole:
        switch (index.column()) {
        case 0:
            return QString((change.fileChange.local ? tr("Locally") : tr("Remotely")) % QChar(' ') % change.fileChange.action % QStringLiteral(", ")
                % QString::fromStdString(change.fileChange.eventTime.toString(DateTimeOutputFormat::DateAndTime, true)));
        case 1:
            return change.deviceId.isEmpty() ? change.fileChange.modifiedBy : change.deviceId;
        case 2:
            return change.directoryId;
        case 3:
            return change.fileChange.path; // usually too long so add a tooltip
        }
        break;
    case Action:
        return change.fileChange.action;
    case ModifiedBy:
        return change.deviceName.isEmpty() ? change.fileChange.modifiedBy : change.deviceName;
    case DirectoryId:
        return change.directoryId;
    case DirectoryName:
        return change.directoryName;
    case Path:
        return change.fileChange.path;
    case EventTime:
        return QString::fromStdString(change.fileChange.eventTime.toString(DateTimeOutputFormat::DateAndTime, true));
    case ExtendedAction: {
        auto extendedAction = change.fileChange.action;
        extendedAction[0] = extendedAction[0].toUpper();
        return QVariant(std::move(extendedAction));
    }
    case ItemType:
        return change.fileChange.type;
    default:;
    }

    return QVariant();
}

bool SyncthingRecentChangesModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    Q_UNUSED(index)
    Q_UNUSED(value)
    Q_UNUSED(role)
    return false;
}

int SyncthingRecentChangesModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid()) {
        return static_cast<int>(m_changes.size());
    } else {
        return 0;
    }
}

int SyncthingRecentChangesModel::columnCount(const QModelIndex &parent) const
{
    if (!parent.isValid()) {
        return 4; // action, device, folder, path
    } else {
        return 0;
    }
}

void SyncthingRecentChangesModel::fileChanged(const SyncthingDir &dir, int index, const SyncthingFileChange &change)
{
    Q_UNUSED(index)

    const SyncthingDev *relatedDev = nullptr;
    for (const SyncthingDev &dev : m_connection.devInfo()) {
        if (dev.id.startsWith(change.modifiedBy)) {
            relatedDev = &dev;
            break;
        }
    }
    if (index >= 0) {
        beginInsertRows(QModelIndex(), 0, 0);
    }
    m_changes.emplace_front(SyncthingRecentChange{
        .directoryId = dir.id,
        .directoryName = dir.displayName(),
        .deviceId = relatedDev ? relatedDev->id : QString(),
        .deviceName = relatedDev ? relatedDev->name : QString(),
        .fileChange = change,
    });
    if (index >= 0) {
        endInsertRows();
    }

    ensureWithinLimit();
}

void SyncthingRecentChangesModel::handleConfigInvalidated()
{
}

void SyncthingRecentChangesModel::handleNewConfigAvailable()
{
}

void SyncthingRecentChangesModel::handleStatusChanged(SyncthingStatus status)
{
    // clear history when re-connecting
    // note: This model is independent of changes to the current configuration (it is about the past and not
    //       the current state) so the re-connect event is used here instead of handleConfigInvalidated()
    //       and handleNewConfigAvailable() as usual.
    if (status != SyncthingStatus::Reconnecting) {
        return;
    }
    beginResetModel();
    m_changes.clear();
    endResetModel();
}

void SyncthingRecentChangesModel::handleForkAwesomeIconsChanged()
{
    invalidateTopLevelIndicies(QVector<int>({ Qt::DecorationRole, ActionIcon }));
}

/*!
 * \brief Sets the maximum number of rows.
 * \remark Specify a negative value for using the disk event limit of the connection.
 */
void SyncthingRecentChangesModel::setMaxRows(int maxRows)
{
    m_maxRows = maxRows;
    ensureWithinLimit();
}

void SyncthingRecentChangesModel::ensureWithinLimit()
{
    const auto maxRows = m_maxRows >= 0 ? m_maxRows : m_connection.diskEventLimit();
    const auto rowsToDelete = static_cast<int>(m_changes.size()) - maxRows;
    if (rowsToDelete <= 0) {
        return;
    }
    beginRemoveRows(QModelIndex(), maxRows, static_cast<int>(m_changes.size()) - 1);
    m_changes.erase(m_changes.begin() + maxRows, m_changes.end());
    endRemoveRows();
}

} // namespace Data
