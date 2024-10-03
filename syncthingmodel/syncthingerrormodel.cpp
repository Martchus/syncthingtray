#include "./syncthingerrormodel.h"
#include "./colors.h"
#include "./syncthingicons.h"

#include <syncthingconnector/syncthingconnection.h>
#include <syncthingconnector/utils.h>

#include <c++utilities/conversion/stringconversion.h>

#include <utility>

using namespace std;
using namespace CppUtilities;

namespace Data {

SyncthingErrorModel::SyncthingErrorModel(SyncthingConnection &connection, QObject *parent)
    : SyncthingModel(connection, parent)
    , m_currentChange(Change::None)
{
    connect(&m_connection, &SyncthingConnection::beforeNewErrors, this, &SyncthingErrorModel::handleBeforeNewErrors);
    connect(&m_connection, &SyncthingConnection::newErrors, this, &SyncthingErrorModel::handleNewErrors);
}

QHash<int, QByteArray> SyncthingErrorModel::roleNames() const
{
    const static QHash<int, QByteArray> roles{
        { When, "when" },
        { Message, "message" },
    };
    return roles;
}

const QVector<int> &SyncthingErrorModel::colorRoles() const
{
    static const QVector<int> colorRoles({ Qt::DecorationRole });
    return colorRoles;
}

QModelIndex SyncthingErrorModel::index(int row, int column, const QModelIndex &parent) const
{
    if (parent.isValid() || static_cast<std::size_t>(row) >= m_connection.errors().size()) {
        return QModelIndex();
    }
    return createIndex(row, column, static_cast<quintptr>(-1));
}

QModelIndex SyncthingErrorModel::parent(const QModelIndex &child) const
{
    Q_UNUSED(child)
    return QModelIndex();
}

QVariant SyncthingErrorModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    switch (orientation) {
    case Qt::Horizontal:
        switch (role) {
        case Qt::DisplayRole:
            switch (section) {
            case 0:
                return tr("Message");
            case 1:
                return tr("Time");
            }
            break;
        default:;
        }
        break;
    default:;
    }
    return QVariant();
}

QVariant SyncthingErrorModel::data(const QModelIndex &index, int role) const
{
    const auto errors = m_connection.errors();
    if (!index.isValid() || index.parent().isValid() || static_cast<std::size_t>(index.row()) >= errors.size()) {
        return QVariant();
    }

    const auto &error = errors[static_cast<std::size_t>(index.row())];
    switch (role) {
    case Qt::DisplayRole:
    case Qt::EditRole:
        switch (index.column()) {
        case 0:
            return error.message;
        case 1:
            return QString::fromStdString(error.when.toString());
        }
        break;
    case Qt::DecorationRole:
        switch (index.column()) {
        case 0:
            return commonForkAwesomeIcons().exclamationTriangle;
        case 1:
            return commonForkAwesomeIcons().clock;
        }
        break;
    case When:
        return QString::fromStdString(error.when.toString());
    case Message:
        return error.message;
    default:;
    }

    return QVariant();
}

bool SyncthingErrorModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    Q_UNUSED(index)
    Q_UNUSED(value)
    Q_UNUSED(role)
    return false;
}

int SyncthingErrorModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : static_cast<int>(m_connection.errors().size());
}

int SyncthingErrorModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : 2; // time, message
}

void SyncthingErrorModel::handleConfigInvalidated()
{
}

void SyncthingErrorModel::handleNewConfigAvailable()
{
}

void SyncthingErrorModel::handleBeforeNewErrors(const std::vector<SyncthingError> &oldErrors, const std::vector<SyncthingError> &newErrors)
{
    if (newErrors.empty() || oldErrors.size() > newErrors.size()) {
        m_currentChange = SyncthingErrorModel::Change::Reset;
        beginResetModel();
    } else if (newErrors.size() > oldErrors.size()) {
        m_currentChange = SyncthingErrorModel::Change::Append;
        m_currentAppendCount = static_cast<int>(newErrors.size() - oldErrors.size());
        beginInsertRows(QModelIndex(), static_cast<int>(oldErrors.size()), static_cast<int>(oldErrors.size()) + m_currentAppendCount - 1);
    }
}

void SyncthingErrorModel::handleNewErrors(const std::vector<Data::SyncthingError> &errors)
{
    switch (std::exchange(m_currentChange, SyncthingErrorModel::Change::None)) {
    case SyncthingErrorModel::Change::None:
        if (!errors.empty()) {
            emit dataChanged(
                index(0, 0), index(static_cast<int>(errors.size() - 1), 2), QVector<int>({ Qt::DisplayRole, Qt::EditRole, When, Message }));
        }
        break;
    case SyncthingErrorModel::Change::Reset:
        endResetModel();
        break;
    case SyncthingErrorModel::Change::Append:
        endInsertRows();
        if (auto changedRows = static_cast<int>(errors.size()) - 1 - m_currentAppendCount; changedRows >= 0) {
            emit dataChanged(index(0, 0), index(changedRows, 2), QVector<int>({ Qt::DisplayRole, Qt::EditRole, When, Message }));
        }
        break;
    }
    emit requestResizeColumns();
}

void SyncthingErrorModel::handleForkAwesomeIconsChanged()
{
    invalidateTopLevelIndicies(QVector<int>({ Qt::DecorationRole }));
}

} // namespace Data
