#ifndef DATA_SYNCTHINGMODEL_H
#define DATA_SYNCTHINGMODEL_H

#include "./global.h"

#include <QAbstractItemModel>

#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
Q_MOC_INCLUDE("../connector/syncthingconnection.h")
#endif

namespace Data {

class SyncthingConnection;

class LIB_SYNCTHING_MODEL_EXPORT SyncthingModel : public QAbstractItemModel {
    Q_OBJECT
    Q_PROPERTY(SyncthingConnection *connection READ connection)
    Q_PROPERTY(bool brightColors READ brightColors WRITE setBrightColors)

public:
    explicit SyncthingModel(SyncthingConnection &connection, QObject *parent = nullptr);
    Data::SyncthingConnection *connection();
    const Data::SyncthingConnection *connection() const;
    bool brightColors() const;
    void setBrightColors(bool brightColors);

protected:
    virtual const QVector<int> &colorRoles() const;
    void invalidateTopLevelIndicies(const QVector<int> &affectedRoles);
    void invalidateNestedIndicies(const QVector<int> &affectedRoles);

private Q_SLOTS:
    virtual void handleConfigInvalidated();
    virtual void handleNewConfigAvailable();
    virtual void handleStatusIconsChanged();
    virtual void handleForkAwesomeIconsChanged();

protected:
    Data::SyncthingConnection &m_connection;
    bool m_brightColors;
};

inline SyncthingConnection *SyncthingModel::connection()
{
    return &m_connection;
}

inline const SyncthingConnection *SyncthingModel::connection() const
{
    return &m_connection;
}

inline bool SyncthingModel::brightColors() const
{
    return m_brightColors;
}

} // namespace Data

#endif // DATA_SYNCTHINGMODEL_H
