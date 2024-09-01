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

class AppSettings : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString syncthingUrl MEMBER syncthingUrl NOTIFY settingsChanged)
    Q_PROPERTY(QString apiKey READ apiKeyAsString WRITE setApiKeyFromString NOTIFY settingsChanged)

public:
    explicit AppSettings(Data::SyncthingConnectionSettings &connectionSettings, QObject *parent = nullptr);

    QString apiKeyAsString() const;
    void setApiKeyFromString(const QString &apiKeyAsString);

Q_SIGNALS:
    void settingsChanged();

public:
    QString &syncthingUrl;
    QByteArray &apiKey;
};

class App : public QObject {
    Q_OBJECT
    Q_PROPERTY(Data::SyncthingConnection *connection READ connection CONSTANT)
    Q_PROPERTY(Data::SyncthingDirectoryModel *dirModel READ dirModel CONSTANT)
    Q_PROPERTY(Data::SyncthingDeviceModel *devModel READ devModel CONSTANT)
    Q_PROPERTY(Data::SyncthingRecentChangesModel *changesModel READ changesModel CONSTANT)
    Q_PROPERTY(AppSettings *settings READ settings CONSTANT)
    Q_PROPERTY(QString faUrlBase READ faUrlBase CONSTANT)

public:
    explicit App(QObject *parent = nullptr);

    // properties
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
    AppSettings *settings()
    {
        return &m_settings;
    }

    // helper functions invoked from QML
    Q_INVOKABLE bool applySettings();
    Q_INVOKABLE bool openPath(const QString &path);
    Q_INVOKABLE bool openPath(const QString &dirId, const QString &relativePath);
    Q_INVOKABLE bool copyText(const QString &text);
    Q_INVOKABLE bool copyPath(const QString &dirId, const QString &relativePath);
    Q_INVOKABLE bool loadIgnorePatterns(const QString &dirId, QObject *textArea);
    Q_INVOKABLE bool saveIgnorePatterns(const QString &dirId, QObject *textArea);
    Q_INVOKABLE Data::SyncthingFileModel *createFileModel(const QString &dirId, QObject *parent);

protected:
    bool event(QEvent *event) override;

private:
    void setBrightColorsOfModelsAccordingToPalette();

    QQmlApplicationEngine m_engine;
    Data::SyncthingConnection m_connection;
    Data::SyncthingDirectoryModel m_dirModel;
    Data::SyncthingDeviceModel m_devModel;
    Data::SyncthingRecentChangesModel m_changesModel;
    AppSettings m_settings;
    QString m_faUrlBase;
};
} // namespace QtGui

#endif // SYNCTHING_TRAY_APP_H
