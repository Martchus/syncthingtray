#include "./syncthingmodel.h"

namespace Data {

SyncthingModel::SyncthingModel(SyncthingConnection &connection, QObject *parent)
    : QAbstractItemModel(parent)
    , m_connection(connection)
    , m_brightColors(false)
{
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

} // namespace Data
