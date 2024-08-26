#ifndef SYNCTHING_TRAY_APP_H
#define SYNCTHING_TRAY_APP_H

#include <QQmlApplicationEngine>

#include <syncthingmodel/syncthingdevicemodel.h>
#include <syncthingmodel/syncthingdirectorymodel.h>
#include <syncthingmodel/syncthingrecentchangesmodel.h>

#include <syncthingconnector/syncthingconnection.h>

namespace Data {
class SyncthingFileModel;
}

namespace QtGui {

class App : public QObject {
    Q_OBJECT
    Q_PROPERTY(Data::SyncthingConnection *connection READ connection CONSTANT)
    Q_PROPERTY(Data::SyncthingDirectoryModel *dirModel READ dirModel CONSTANT)
    Q_PROPERTY(Data::SyncthingDeviceModel *devModel READ devModel CONSTANT)
    Q_PROPERTY(Data::SyncthingRecentChangesModel *changesModel READ changesModel CONSTANT)
    Q_PROPERTY(QString faUrlBase READ faUrlBase CONSTANT)

public:
    explicit App(QObject *parent = nullptr);
    Data::SyncthingConnection *connection()
    {
        return &m_connection;
    }
    Data::SyncthingDirectoryModel *dirModel()
    {
        return &m_dirModel;
    }
    Data::SyncthingDeviceModel *devModel()
    {
        return &m_devModel;
    }
    Data::SyncthingRecentChangesModel *changesModel()
    {
        return &m_changesModel;
    }
    const QString &faUrlBase()
    {
        return m_faUrlBase;
    }
    Q_INVOKABLE bool openDir(const QString &path);
    Q_INVOKABLE bool copy(const QString &text);
    Q_INVOKABLE Data::SyncthingFileModel *createFileModel(const QString &dirId);

Q_SIGNALS:

protected:
private Q_SLOTS:

private:
    QQmlApplicationEngine m_engine;
    Data::SyncthingConnection m_connection;
    Data::SyncthingDirectoryModel m_dirModel;
    Data::SyncthingDeviceModel m_devModel;
    Data::SyncthingRecentChangesModel m_changesModel;
    QString m_faUrlBase;
};
} // namespace QtGui

#endif // SYNCTHING_TRAY_APP_H
