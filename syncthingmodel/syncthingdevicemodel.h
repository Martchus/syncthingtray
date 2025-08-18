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
        DeviceStatus = SyncthingModelUserRole + 1,
        DevicePaused,
        IsThisDevice,
        DeviceStatusString,
        DeviceStatusColor,
        DeviceId,
        DeviceDetail,
        DeviceDetailIcon,
        DeviceNeededItemsCount,
        DeviceDetailTooltip,
    };

    explicit SyncthingDeviceModel(SyncthingConnection &connection, QObject *parent = nullptr);

public:
    QHash<int, QByteArray> roleNames() const override;
    const QVector<int> &colorRoles() const override;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &child) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    int rowCount(const QModelIndex &parent) const override;
    Q_INVOKABLE const SyncthingDev *devInfo(const QModelIndex &index) const;
    Q_INVOKABLE const SyncthingDev *info(const QModelIndex &index) const;

private Q_SLOTS:
    void devStatusChanged(const Data::SyncthingDev &, int index);
    void handleConfigInvalidated() override;
    void handleNewConfigAvailable() override;
    void handleStatusIconsChanged() override;
    void handleForkAwesomeIconsChanged() override;

private:
    QVariant devStatusColor(const SyncthingDev &dev) const;
    void updateRowCount();

    const std::vector<SyncthingDev> &m_devs;
    std::vector<int> m_rowCount;
    mutable QString m_thisDevVersion;
};

inline const SyncthingDev *SyncthingDeviceModel::info(const QModelIndex &index) const
{
    return devInfo(index);
}

} // namespace Data

#endif // DATA_SYNCTHINGDEVICEMODEL_H
