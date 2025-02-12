#include "./syncthingmodel.h"
#include "./syncthingicons.h"

#include <syncthingconnector/syncthingconnection.h>

namespace Data {

SyncthingModel::SyncthingModel(SyncthingConnection &connection, QObject *parent)
    : QAbstractItemModel(parent)
    , m_connection(connection)
    , m_brightColors(false)
    , m_singleColumnMode(true)
{
    connect(&m_connection, &SyncthingConnection::newConfig, this, &SyncthingModel::handleConfigInvalidated);
    connect(&m_connection, &SyncthingConnection::newConfigApplied, this, &SyncthingModel::handleNewConfigAvailable);

    const auto &iconManager = IconManager::instance();
    connect(&iconManager, &IconManager::statusIconsChanged, this, &SyncthingModel::handleStatusIconsChanged);
    connect(&iconManager, &IconManager::forkAwesomeIconsChanged, this, &SyncthingModel::handleForkAwesomeIconsChanged);
}

const QVector<int> &SyncthingModel::colorRoles() const
{
    static const QVector<int> colorRoles;
    return colorRoles;
}

void SyncthingModel::invalidateTopLevelIndicies(const QVector<int> &affectedRoles)
{
    emit dataChanged(index(0, 0), index(rowCount() - 1, columnCount() - 1), affectedRoles);
}

void SyncthingModel::invalidateNestedIndicies(const QVector<int> &affectedRoles)
{
    for (auto i = 0, rows = rowCount(); i != rows; ++i) {
        const auto parentIndex = index(i, 0);
        const auto childRows = rowCount(parentIndex);
        if (childRows > 0) {
            emit dataChanged(index(0, 0, parentIndex), index(childRows - 1, columnCount(parentIndex) - 1, parentIndex), affectedRoles);
        }
    }
}

void SyncthingModel::invalidateAllIndicies(const QVector<int> &affectedRoles, const QModelIndex &parentIndex)
{
    const auto rows = rowCount(parentIndex);
    const auto columns = columnCount(parentIndex);
    if (rows <= 0 || columns <= 0) {
        return;
    }
    const auto topLeftIndex = index(0, 0, parentIndex);
    const auto bottomRightIndex = index(rows - 1, columns - 1, parentIndex);
    emit dataChanged(topLeftIndex, bottomRightIndex, affectedRoles);
    for (auto row = 0; row != rows; ++row) {
        if (const auto idx = index(row, 0, parentIndex); idx.isValid()) {
            invalidateAllIndicies(affectedRoles, idx);
        }
    }
}

void SyncthingModel::invalidateAllIndicies(const QVector<int> &affectedRoles, int column, const QModelIndex &parentIndex)
{
    const auto rows = rowCount(parentIndex);
    const auto columns = columnCount(parentIndex);
    if (rows <= 0 || column >= columns) {
        return;
    }
    const auto topLeftIndex = index(0, column, parentIndex);
    const auto bottomRightIndex = index(rows - 1, column, parentIndex);
    emit dataChanged(topLeftIndex, bottomRightIndex, affectedRoles);
    for (auto row = 0; row != rows; ++row) {
        if (const auto idx = index(row, 0, parentIndex); idx.isValid()) {
            invalidateAllIndicies(affectedRoles, column, idx);
        }
    }
}

void SyncthingModel::setBrightColors(bool brightColors)
{
    if (m_brightColors == brightColors) {
        return;
    }
    m_brightColors = brightColors;
    handleBrightColorsChanged();
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

void SyncthingModel::handleForkAwesomeIconsChanged()
{
}

void SyncthingModel::handleBrightColorsChanged()
{
    if (const QVector<int> &affectedRoles = colorRoles(); !affectedRoles.isEmpty()) {
        invalidateTopLevelIndicies(affectedRoles);
    }
}

/*!
 * \brief Sets whether the model should only have a single column.
 * \remarks
 * - This is the default but ignored by some derived models.
 * - This is used so all the data can be rendered as a single column avoiding the inflexible behavior of QTreeView
 *   when it comes to sizing columns.
 */
void SyncthingModel::setSingleColumnMode(bool singleColumnModeEnabled)
{
    if (m_singleColumnMode != singleColumnModeEnabled) {
        if (m_singleColumnMode) {
            beginInsertColumns(QModelIndex(), 1, 1);
            m_singleColumnMode = true;
            endInsertColumns();
        } else {
            beginRemoveColumns(QModelIndex(), 1, 1);
            m_singleColumnMode = false;
            endRemoveColumns();
        }
    }
}

/*!
 * \brief Implements columnCount() in a way that makes sense for most derived classes taking singleColumnMode() into
 *        account.
 */
int SyncthingModel::columnCount(const QModelIndex &parent) const
{
    if (!parent.isValid()) {
        return singleColumnMode() ? 1 : 2; // label/name/ID, status/buttons
    } else if (!parent.parent().isValid()) {
        return singleColumnMode() ? 1 : 2; // field name and value (or file and progress)
    } else {
        return 0;
    }
}

} // namespace Data
