#include "./syncthingmodel.h"
#include "./syncthingicons.h"

#include <syncthingconnector/syncthingconnection.h>

namespace Data {

SyncthingModel::SyncthingModel(SyncthingConnection &connection, QObject *parent)
    : QAbstractItemModel(parent)
    , m_connection(connection)
    , m_brightColors(false)
{
    connect(&m_connection, &SyncthingConnection::newConfig, this, &SyncthingModel::handleConfigInvalidated);
    connect(&m_connection, &SyncthingConnection::newConfigApplied, this, &SyncthingModel::handleNewConfigAvailable);
    connect(&IconManager::instance(), &IconManager::statusIconsChanged, this, &SyncthingModel::handleStatusIconsChanged);
}

const QVector<int> &SyncthingModel::colorRoles() const
{
    static const QVector<int> colorRoles;
    return colorRoles;
}

void SyncthingModel::setBrightColors(bool brightColors)
{
    if (m_brightColors == brightColors) {
        return;
    }
    m_brightColors = brightColors;

    const QVector<int> &affectedRoles = colorRoles();
    if (affectedRoles.isEmpty()) {
        return;
    }

    // update top-level indices
    const auto rows = rowCount();
    emit dataChanged(index(0, 0), index(rows - 1, columnCount() - 1), affectedRoles);

    // update nested indices
    for (auto i = 0; i != rows; ++i) {
        const auto parentIndex = index(i, 0);
        const auto childRows = rowCount(parentIndex);
        if (childRows > 0) {
            emit dataChanged(index(0, 0, parentIndex), index(childRows - 1, columnCount(parentIndex) - 1), affectedRoles);
        }
    }
}

void SyncthingModel::handleConfigInvalidated()
{
    beginResetModel();
}

void SyncthingModel::handleNewConfigAvailable()
{
    endResetModel();
}

void SyncthingModel::handleStatusIconsChanged()
{
}

} // namespace Data
