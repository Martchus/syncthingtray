#ifndef DATA_SYNCTHINGDEVICEMODEL_H
#define DATA_SYNCTHINGDEVICEMODEL_H

#include "./syncthingmodel.h"

#include <QIcon>

#include <vector>

namespace Data {

struct SyncthingDev;

class LIB_SYNCTHING_MODEL_EXPORT SyncthingDeviceModel : public SyncthingModel {
    Q_OBJECT
public:
    enum SyncthingDeviceModelRole {
        DeviceStatus = Qt::UserRole + 1,
        DevicePaused,
        IsOwnDevice,
        DeviceStatusString,
        DeviceStatusColor,
        DeviceId,
        DeviceDetail
    };

    explicit SyncthingDeviceModel(SyncthingConnection &connection, QObject *parent = nullptr);

public Q_SLOTS:
    QHash<int, QByteArray> roleNames() const;
    const QVector<int> &colorRoles() const;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex &child) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    QVariant data(const QModelIndex &index, int role) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role);
    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    const SyncthingDev *devInfo(const QModelIndex &index) const;

private Q_SLOTS:
    void newConfig();
    void newDevices();
    void devStatusChanged(const SyncthingDev &, int index);

private:
    static QString devStatusString(const SyncthingDev &dev);
    QVariant devStatusColor(const SyncthingDev &dev) const;

    const std::vector<SyncthingDev> &m_devs;
};

} // namespace Data

#endif // DATA_SYNCTHINGDEVICEMODEL_H
