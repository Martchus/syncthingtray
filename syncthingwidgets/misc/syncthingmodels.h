#ifndef SYNCTHING_DATA_MODELS_H
#define SYNCTHING_DATA_MODELS_H

#include <syncthingmodel/syncthingdevicemodel.h>
#include <syncthingmodel/syncthingdirectorymodel.h>
#include <syncthingmodel/syncthingrecentchangesmodel.h>
#include <syncthingmodel/syncthingsortfiltermodel.h>

#include <syncthingconnector/syncthingconnection.h>
#include <syncthingconnector/syncthingnotifier.h>

#include <QObject>

#ifdef SYNCTHINGWIDGETS_GUI_QTQUICK
#include <QtQmlIntegration/qqmlintegration.h>
#endif

namespace QtGui {

class SYNCTHINGWIDGETS_EXPORT SyncthingData : public QObject {
    Q_OBJECT
    Q_PROPERTY(Data::SyncthingConnection *connection READ connection CONSTANT)
    Q_PROPERTY(Data::SyncthingNotifier *notifier READ notifier CONSTANT)
#ifdef SYNCTHINGWIDGETS_GUI_QTQUICK
    QML_ELEMENT
    QML_SINGLETON
#endif

public:
    explicit SyncthingData(QObject *parent = nullptr);
    ~SyncthingData() override;
#ifdef SYNCTHINGWIDGETS_GUI_QTQUICK
    static SyncthingData *create(QQmlEngine *, QJSEngine *engine);
#endif

    Data::SyncthingConnection *connection()
    {
        return &m_connection;
    }
    Data::SyncthingNotifier *notifier()
    {
        return &m_notifier;
    }

private:
    Data::SyncthingConnection m_connection;
    Data::SyncthingNotifier m_notifier;
};

class SYNCTHINGWIDGETS_EXPORT SyncthingModels : public QObject {
    Q_OBJECT
    Q_PROPERTY(Data::SyncthingDirectoryModel *dirModel READ dirModel CONSTANT)
    Q_PROPERTY(Data::SyncthingSortFilterModel *sortFilterDirModel READ sortFilterDirModel CONSTANT)
    Q_PROPERTY(Data::SyncthingDeviceModel *devModel READ devModel CONSTANT)
    Q_PROPERTY(Data::SyncthingSortFilterModel *sortFilterDevModel READ sortFilterDevModel CONSTANT)
    Q_PROPERTY(Data::SyncthingRecentChangesModel *changesModel READ changesModel CONSTANT)
#ifdef SYNCTHINGWIDGETS_GUI_QTQUICK
    QML_ELEMENT
    QML_SINGLETON
#endif

public:
    explicit SyncthingModels(SyncthingData &data, QObject *parent = nullptr);
    ~SyncthingModels() override;
#ifdef SYNCTHINGWIDGETS_GUI_QTQUICK
    static SyncthingModels *create(QQmlEngine *, QJSEngine *engine);
#endif

    Data::SyncthingDirectoryModel *dirModel()
    {
        return &m_dirModel;
    }
    Data::SyncthingSortFilterModel *sortFilterDirModel()
    {
        return &m_sortFilterDirModel;
    }
    Data::SyncthingDeviceModel *devModel()
    {
        return &m_devModel;
    }
    Data::SyncthingSortFilterModel *sortFilterDevModel()
    {
        return &m_sortFilterDevModel;
    }
    Data::SyncthingRecentChangesModel *changesModel()
    {
        return &m_changesModel;
    }

private:
    Data::SyncthingDirectoryModel m_dirModel;
    Data::SyncthingSortFilterModel m_sortFilterDirModel;
    Data::SyncthingDeviceModel m_devModel;
    Data::SyncthingSortFilterModel m_sortFilterDevModel;
    Data::SyncthingRecentChangesModel m_recentChangesModel;
};

} // namespace QtGui

#endif // SYNCTHING_DATA_MODELS_H
