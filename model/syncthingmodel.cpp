#include "./syncthingmodel.h"

namespace Data {

SyncthingModel::SyncthingModel(SyncthingConnection &connection, QObject *parent)
    : QAbstractItemModel(parent)
    , m_connection(connection)
    , m_brightColors(false)
{
}

void SyncthingModel::setBrightColors(bool brightColors)
{
    if (m_brightColors != brightColors) {
        m_brightColors = brightColors;
        emit dataChanged(index(0, 0), index(rowCount() - 1, columnCount() - 1), QVector<int>() << Qt::ForegroundRole);
    }
}

} // namespace Data
