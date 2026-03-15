#ifndef SYNCTHINGWIDGETS_DATA_H
#define SYNCTHINGWIDGETS_DATA_H

#include "./statusinfo.h"
#include "./syncthinglauncher.h"
#include "./utils.h"

#include <syncthingconnector/syncthingconnection.h>
#include <syncthingconnector/syncthingnotifier.h>

#include <QObject>
#include <QtVersion>

#ifdef SYNCTHINGWIDGETS_GUI_QTQUICK
#include <QJSValue>
#include <QtQmlIntegration/qqmlintegration.h>

QT_FORWARD_DECLARE_CLASS(QQmlEngine)
QT_FORWARD_DECLARE_CLASS(QJSEngine)
#endif

namespace QtGui {

class SYNCTHINGWIDGETS_EXPORT SyncthingData : public QObject {
    Q_OBJECT
    Q_PROPERTY(Data::SyncthingConnection *connection READ connection CONSTANT)
    Q_PROPERTY(Data::SyncthingNotifier *notifier READ notifier CONSTANT)
    Q_PROPERTY(StatusInfo *statusInfo READ statusInfo CONSTANT)
    Q_PROPERTY(QString syncthingVersion READ syncthingVersion CONSTANT)
    Q_PROPERTY(QString qtVersion READ qtVersion CONSTANT)
    Q_PROPERTY(QString readmeUrl READ readmeUrl CONSTANT)
    Q_PROPERTY(QString documentationUrl READ documentationUrl CONSTANT)
    Q_PROPERTY(QString website READ website CONSTANT)
#ifdef SYNCTHINGWIDGETS_GUI_QTQUICK
    QML_ELEMENT
    QML_SINGLETON
#endif

public:
    explicit SyncthingData(QObject *parent, bool textOnly = false,
        bool clickToConnect = false); // avoid "parent = nullptr" to make it non-default constructable so create() is used
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
    StatusInfo *statusInfo()
    {
        return &m_statusInfo;
    }
    QString syncthingVersion() const
    {
#ifdef SYNCTHINGWIDGETS_USE_LIBSYNCTHING
        return Data::SyncthingLauncher::libSyncthingVersionInfo().remove(0, 11);
#else
        return tr("not available");
#endif
    }
    QString qtVersion() const
    {
        return QString::fromUtf8(qVersion());
    }
    QString readmeUrl() const
    {
        return QtGui::readmeUrl();
    }
    QString documentationUrl() const
    {
        return QtGui::documentationUrl();
    }
    QString website() const;

public Q_SLOTS:
    void updateStatusInfo(const QString &configurationName = QString());
    void updateDeviceInfo();

private:
    Data::SyncthingConnection m_connection;
    Data::SyncthingNotifier m_notifier;
    StatusInfo m_statusInfo;
};

inline void SyncthingData::updateStatusInfo(const QString &configurationName)
{
    m_statusInfo.updateConnectionStatus(m_connection, configurationName);
}

inline void SyncthingData::updateDeviceInfo()
{
    m_statusInfo.updateConnectedDevices(m_connection);
}

} // namespace QtGui

#endif // SYNCTHINGWIDGETS_DATA_H
