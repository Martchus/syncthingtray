#ifndef SYNCTHING_TRAY_APP_BASE_H
#define SYNCTHING_TRAY_APP_BASE_H

#include <syncthingwidgets/misc/statusinfo.h>

#include <syncthingconnector/syncthingconfig.h>
#include <syncthingconnector/syncthingconnection.h>
#include <syncthingconnector/syncthingconnectionsettings.h>
#include <syncthingconnector/syncthingconnectionstatus.h>
#include <syncthingconnector/syncthingnotifier.h>

#include <QDir>
#include <QFile>
#include <QJsonObject>

#include <optional>

namespace Data {
class IconManager;
}

namespace QtGui {

class AppBase : public QObject {
    Q_OBJECT
    Q_PROPERTY(Data::SyncthingConnection *connection READ connection CONSTANT)
    Q_PROPERTY(Data::SyncthingNotifier *notifier READ notifier CONSTANT)
    Q_PROPERTY(QString status READ status NOTIFY statusChanged)

public:
    explicit AppBase(bool insecure, QObject *parent = nullptr);
    ~AppBase();

    // properties
    Data::SyncthingConnection *connection()
    {
        return &m_connection;
    }
    Data::SyncthingNotifier *notifier()
    {
        return &m_notifier;
    }
    virtual const QString &status();

    Q_INVOKABLE bool loadSettings(bool force = false);
    Q_INVOKABLE void applyConnectionSettings(const QUrl &syncthingUrl);
    Q_INVOKABLE void applySyncthingSettings();

Q_SIGNALS:
    void error(const QString &errorMessage, const QString &details = QString());
    void settingsChanged(const QJsonObject &settingsChanged);
    void logsAvailable(const QString &newLogMessages);
    void statusChanged();

protected Q_SLOTS:
    void handleGuiAddressChanged(const QUrl &newUrl);

protected:
    static QString openSettingFile(QFile &settingsFile, const QString &path);
    static QString readSettingFile(QFile &settingsFile, QJsonObject &settings);
    bool openSettings();
    virtual void invalidateStatus();
    Data::IconManager &initIconManager();

protected:
    Data::SyncthingConnection m_connection;
    Data::SyncthingNotifier m_notifier;
    Data::SyncthingConnectionSettings m_connectionSettingsFromLauncher;
    Data::SyncthingConnectionSettings m_connectionSettingsFromConfig;
    Data::SyncthingConfig m_syncthingConfig;
    QFile m_settingsFile;
    std::optional<QDir> m_settingsDir;
    QJsonObject m_settings;
    QVariantList m_internalErrors;
    std::optional<QString> m_status;
    StatusInfo m_statusInfo;
    QString m_syncthingConfigDir;
    QString m_syncthingDataDir;
    QString m_log;
    bool m_connectToLaunched;
    bool m_insecure;
};

} // namespace QtGui

#endif // SYNCTHING_TRAY_APP_BASE_H
