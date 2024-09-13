#ifndef SYNCTHING_TRAY_APP_H
#define SYNCTHING_TRAY_APP_H

#include <QQmlApplicationEngine>

#include <syncthingmodel/syncthingdevicemodel.h>
#include <syncthingmodel/syncthingdirectorymodel.h>
#include <syncthingmodel/syncthingrecentchangesmodel.h>

#include <syncthingconnector/syncthingconnection.h>
#include <syncthingconnector/syncthingconnectionsettings.h>
#include <syncthingconnector/syncthingnotifier.h>

#include <qtutilities/settingsdialog/qtsettings.h>

#include <QJsonObject>
#include <QFile>

namespace Data {
class SyncthingFileModel;
}

namespace QtGui {

class App : public QObject {
    Q_OBJECT
    Q_PROPERTY(Data::SyncthingConnection *connection READ connection CONSTANT)
    Q_PROPERTY(Data::SyncthingNotifier *notifier READ notifier CONSTANT)
    Q_PROPERTY(Data::SyncthingDirectoryModel *dirModel READ dirModel CONSTANT)
    Q_PROPERTY(Data::SyncthingDeviceModel *devModel READ devModel CONSTANT)
    Q_PROPERTY(Data::SyncthingRecentChangesModel *changesModel READ changesModel CONSTANT)
    Q_PROPERTY(QJsonObject settings READ settings WRITE setSettings NOTIFY settingsChanged)
    Q_PROPERTY(QString faUrlBase READ faUrlBase CONSTANT)
    Q_PROPERTY(bool darkmodeEnabled READ isDarkmodeEnabled NOTIFY darkmodeEnabledChanged)
    Q_PROPERTY(int iconSize READ iconSize CONSTANT)

public:
    explicit App(bool insecure = false, QObject *parent = nullptr);

    // properties
    Data::SyncthingConnection *connection()
    {
        return &m_connection;
    }
    Data::SyncthingNotifier *notifier()
    {
        return &m_notifier;
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
    QJsonObject settings() const
    {
        return m_settings;
    }
    void setSettings(const QJsonObject &settings)
    {
        m_settings = settings;
        applySettings();
        storeSettings();
        emit settingsChanged(m_settings);
    }
    /*!
     * \brief Returns whether darkmode is enabled.
     * \remarks
     * The QML code could just use "Qt.styleHints.colorScheme === Qt.Dark" but this would not be consistent with the approach in
     * QtSettings that also takes the palette into account for platforms without darkmode flag.
     */
    bool isDarkmodeEnabled() const
    {
        return m_darkmodeEnabled;
    }
    int iconSize() const
    {
        return m_iconSize;
    }

    // helper functions invoked from QML
    Q_INVOKABLE bool loadSettings();
    Q_INVOKABLE bool storeSettings();
    Q_INVOKABLE bool applySettings();
    Q_INVOKABLE bool openPath(const QString &path);
    Q_INVOKABLE bool openPath(const QString &dirId, const QString &relativePath);
    Q_INVOKABLE bool copyText(const QString &text);
    Q_INVOKABLE bool copyPath(const QString &dirId, const QString &relativePath);
    Q_INVOKABLE bool loadIgnorePatterns(const QString &dirId, QObject *textArea);
    Q_INVOKABLE bool saveIgnorePatterns(const QString &dirId, QObject *textArea);
    Q_INVOKABLE bool loadDirErrors(const QString &dirId, QObject *view);
    Q_INVOKABLE Data::SyncthingFileModel *createFileModel(const QString &dirId, QObject *parent);

Q_SIGNALS:
    void darkmodeEnabledChanged(bool darkmodeEnabled);
    void settingsChanged(const QJsonObject &settingsChanged);
    void error(const QString &errorMessage);

protected:
    bool event(QEvent *event) override;

private Q_SLOTS:
    void handleConnectionError(const QString &errorMessage, Data::SyncthingErrorCategory category, int networkError, const QNetworkRequest &request,
        const QByteArray &response);

private:
    void applyDarkmodeChange(bool isDarkColorSchemeEnabled, bool isDarkPaletteEnabled);
    bool openSettings();

    QQmlApplicationEngine m_engine;
    Data::SyncthingConnection m_connection;
    Data::SyncthingNotifier m_notifier;
    Data::SyncthingDirectoryModel m_dirModel;
    Data::SyncthingDeviceModel m_devModel;
    Data::SyncthingRecentChangesModel m_changesModel;
    Data::SyncthingConnectionSettings m_connectionSettings;
    QtUtilities::QtSettings m_qtSettings;
    QFile m_settingsFile;
    QJsonObject m_settings;
    QString m_faUrlBase;
    int m_iconSize;
    bool m_insecure;
    bool m_darkmodeEnabled;
    bool m_darkColorScheme;
    bool m_darkPalette;
};
} // namespace QtGui

#endif // SYNCTHING_TRAY_APP_H
