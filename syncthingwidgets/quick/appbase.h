#ifndef SYNCTHING_TRAY_APP_BASE_H
#define SYNCTHING_TRAY_APP_BASE_H

#include "../misc/statusinfo.h"
#include "../misc/syncthingmodels.h"

#include <syncthingconnector/syncthingconfig.h>
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

class SYNCTHINGWIDGETS_EXPORT AppBase : public QObject {
    Q_OBJECT
    Q_PROPERTY(SyncthingData *data READ data CONSTANT)
    Q_PROPERTY(QString status READ status NOTIFY statusChanged)

public:
    explicit AppBase(bool insecure, bool textOnly = false, bool clickToConnect = false, QObject *parent = nullptr);
    ~AppBase();

    // properties
    SyncthingData *data();
    virtual const QString &status();
    virtual bool isSyncthingRunning() const = 0;
    virtual bool mayPauseDevicesOnMeteredNetworkConnection() const = 0;

    Q_INVOKABLE QDir &settingsDir();
    Q_INVOKABLE bool loadSettings(bool force = false);
    Q_INVOKABLE void applyConnectionSettings(const QUrl &syncthingUrl);
    Q_INVOKABLE void applySyncthingSettings();

Q_SIGNALS:
    void error(const QString &errorMessage, const QString &details = QString());
    void settingsChanged(const QJsonObject &settingsChanged);
    void statusChanged();

protected Q_SLOTS:
    void handleGuiUrlChanged(const QUrl &newUrl);

protected:
    static QString openSettingFile(QFile &settingsFile, const QString &path);
    static QString readSettingFile(QFile &settingsFile, QJsonObject &settings);
    QString syncthingLogFilePath() const;
    bool openSettings();
    virtual void invalidateStatus();
    Data::IconManager &initIconManager();

protected:
    SyncthingData m_data;
    Data::SyncthingConnectionSettings m_connectionSettingsFromLauncher;
    Data::SyncthingConnectionSettings m_connectionSettingsFromConfig;
    Data::SyncthingConfig m_syncthingConfig;
    QFile m_settingsFile;
    std::optional<QDir> m_settingsDir;
    QJsonObject m_settings;
    QVariantList m_internalErrors;
    std::optional<QString> m_status;
    QString m_syncthingConfigDir;
    QString m_syncthingDataDir;
    QString m_syncthingUnixSocketPath;
    bool m_connectToLaunched;
    bool m_insecure;
};

inline SyncthingData *AppBase::data()
{
    return &m_data;
}

/*
 * \brief Returns the path of the Syncthing log file.
 */
inline QString AppBase::syncthingLogFilePath() const
{
    return m_settingsDir->path() + QStringLiteral("/syncthing.log");
}

} // namespace QtGui

#endif // SYNCTHING_TRAY_APP_BASE_H
