#ifndef DATA_SYNCTHINGDEVICEMODEL_H
#define DATA_SYNCTHINGDEVICEMODEL_H

#include <QAbstractItemModel>
#include <QIcon>

#include <vector>

namespace Data {

class SyncthingConnection;
struct SyncthingDev;

class SyncthingDeviceModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit SyncthingDeviceModel(SyncthingConnection &connection, QObject *parent = nullptr);

public Q_SLOTS:
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex &child) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    QVariant data(const QModelIndex &index, int role) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role);
    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    const SyncthingDev *devInfo(const QModelIndex &index) const;

private:
    Data::SyncthingConnection &m_connection;
    const std::vector<SyncthingDev> &m_devs;
    const QIcon m_unknownIcon;
    const QIcon m_idleIcon;
    const QIcon m_syncIcon;
    const QIcon m_errorIcon;
    const QIcon m_pausedIcon;
    const QIcon m_otherIcon;
};

} // namespace Data

#endif // DATA_SYNCTHINGDEVICEMODEL_H
