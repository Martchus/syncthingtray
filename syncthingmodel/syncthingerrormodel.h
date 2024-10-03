#ifndef DATA_SYNCTHINGERRORMODEL_H
#define DATA_SYNCTHINGERRORMODEL_H

#include "./syncthingmodel.h"

namespace Data {

struct SyncthingError;

class LIB_SYNCTHING_MODEL_EXPORT SyncthingErrorModel : public SyncthingModel {
    Q_OBJECT

public:
    enum SyncthingErrorModelRole {
        When = SyncthingModelUserRole + 1,
        Message,
    };

    explicit SyncthingErrorModel(SyncthingConnection &connection, QObject *parent = nullptr);

    QHash<int, QByteArray> roleNames() const override;
    const QVector<int> &colorRoles() const override;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &child) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;

Q_SIGNALS:
    void requestResizeColumns();

private Q_SLOTS:
    void handleConfigInvalidated() override;
    void handleNewConfigAvailable() override;
    void handleBeforeNewErrors(const std::vector<Data::SyncthingError> &oldErrors, const std::vector<Data::SyncthingError> &newErrors);
    void handleNewErrors(const std::vector<Data::SyncthingError> &newErrors);
    void handleForkAwesomeIconsChanged() override;

private:
    enum class Change { None, Reset, Append };
    Change m_currentChange;
    int m_currentAppendCount;
};

} // namespace Data

Q_DECLARE_METATYPE(Data::SyncthingErrorModel)

#endif // DATA_SYNCTHINGERRORMODEL_H
