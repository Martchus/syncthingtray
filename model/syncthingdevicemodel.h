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
        DeviceDetail,
        DeviceDetailIcon,
    };

    explicit SyncthingDeviceModel(SyncthingConnection &connection, QObject *parent = nullptr);

public Q_SLOTS:
    QHash<int, QByteArray> roleNames() const override;
    const QVector<int> &colorRoles() const override;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &child) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;
    const SyncthingDev *devInfo(const QModelIndex &index) const;
    const SyncthingDev *info(const QModelIndex &index) const;

private Q_SLOTS:
    void devStatusChanged(const SyncthingDev &, int index);
    void handleStatusIconsChanged() override;
    void handleForkAwesomeIconsChanged() override;

private:
    static QString devStatusString(const SyncthingDev &dev);
    QVariant devStatusColor(const SyncthingDev &dev) const;

    const std::vector<SyncthingDev> &m_devs;
};

inline const SyncthingDev *SyncthingDeviceModel::info(const QModelIndex &index) const
{
    return devInfo(index);
}

} // namespace Data

#endif // DATA_SYNCTHINGDEVICEMODEL_H
