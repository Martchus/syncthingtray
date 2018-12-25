#include "./syncthingmodel.h"

#include "../connector/syncthingconnection.h"

namespace Data {

SyncthingModel::SyncthingModel(SyncthingConnection &connection, QObject *parent)
    : QAbstractItemModel(parent)
    , m_connection(connection)
    , m_brightColors(false)
{
    connect(&m_connection, &SyncthingConnection::newConfig, this, &SyncthingModel::handleConfigInvalidated);
    connect(&m_connection, &SyncthingConnection::newConfigApplied, this, &SyncthingModel::handleNewConfigAvailable);
}

const QVector<int> &SyncthingModel::colorRoles() const
{
    static const QVector<int> colorRoles;
    return colorRoles;
}

void SyncthingModel::setBrightColors(bool brightColors)
{
    if (m_brightColors != brightColors) {
        m_brightColors = brightColors;
        const QVector<int> &affectedRoles = colorRoles();
        if (!affectedRoles.isEmpty()) {
            emit dataChanged(index(0, 0), index(rowCount() - 1, columnCount() - 1), affectedRoles);
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

} // namespace Data
