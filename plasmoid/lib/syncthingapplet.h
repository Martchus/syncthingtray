#ifndef SYNCTHINGAPPLET_H
#define SYNCTHINGAPPLET_H

#include "../../widgets/misc/dbusstatusnotifier.h"
#include "../../widgets/misc/statusinfo.h"

#include "../../model/syncthingdevicemodel.h"
#include "../../model/syncthingdirectorymodel.h"
#include "../../model/syncthingdownloadmodel.h"

#include "../../connector/syncthingconnection.h"
#include "../../connector/syncthingservice.h"

#include <qtutilities/aboutdialog/aboutdialog.h>

#include <Plasma/Applet>

namespace Dialogs {
class SettingsDialog;
}

namespace Data {
class SyncthingConnection;
struct SyncthingConnectionSettings;
class SyncthingDirectoryModel;
class SyncthingDeviceModel;
class SyncthingDownloadModel;
class SyncthingService;
enum class SyncthingErrorCategory;
}

namespace QtGui {
class WebViewDialog;
}

namespace Plasmoid {

class SyncthingApplet : public Plasma::Applet {
    Q_OBJECT
    Q_PROPERTY(Data::SyncthingConnection *connection READ connection NOTIFY connectionChanged)
    Q_PROPERTY(Data::SyncthingDirectoryModel *dirModel READ dirModel NOTIFY dirModelChanged)
    Q_PROPERTY(Data::SyncthingDeviceModel *devModel READ devModel NOTIFY devModelChanged)
    Q_PROPERTY(Data::SyncthingDownloadModel *downloadModel READ downloadModel NOTIFY downloadModelChanged)
    Q_PROPERTY(Data::SyncthingService *service READ service NOTIFY serviceChanged)
    Q_PROPERTY(bool local READ isLocal NOTIFY localChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY connectionStatusChanged)
    Q_PROPERTY(QString additionalStatusText READ additionalStatusText NOTIFY connectionStatusChanged)
    Q_PROPERTY(QIcon statusIcon READ statusIcon NOTIFY connectionStatusChanged)
    Q_PROPERTY(QString incomingTraffic READ incomingTraffic NOTIFY trafficChanged)
    Q_PROPERTY(QString outgoingTraffic READ outgoingTraffic NOTIFY trafficChanged)
    Q_PROPERTY(QStringList connectionConfigNames READ connectionConfigNames NOTIFY settingsChanged)
    Q_PROPERTY(QString currentConnectionConfigName READ currentConnectionConfigName NOTIFY currentConnectionConfigIndexChanged)
    Q_PROPERTY(int currentConnectionConfigIndex READ currentConnectionConfigIndex WRITE setCurrentConnectionConfigIndex NOTIFY
            currentConnectionConfigIndexChanged)
    Q_PROPERTY(bool startStopForServiceEnabled READ isStartStopForServiceEnabled NOTIFY settingsChanged)

public:
    SyncthingApplet(QObject *parent, const QVariantList &data);
    ~SyncthingApplet() override;

public:
    Data::SyncthingConnection *connection() const;
    Data::SyncthingDirectoryModel *dirModel() const;
    Data::SyncthingDeviceModel *devModel() const;
    Data::SyncthingDownloadModel *downloadModel() const;
    Data::SyncthingService *service() const;
    bool isLocal() const;
    QString statusText() const;
    QString additionalStatusText() const;
    QIcon statusIcon() const;
    QString incomingTraffic() const;
    QString outgoingTraffic() const;
    QStringList connectionConfigNames() const;
    QString currentConnectionConfigName() const;
    int currentConnectionConfigIndex() const;
    Data::SyncthingConnectionSettings *currentConnectionConfig();
    Data::SyncthingConnectionSettings *connectionConfig(int index);
    void setCurrentConnectionConfigIndex(int index);
    bool isStartStopForServiceEnabled() const;

public Q_SLOTS:
    void init() Q_DECL_OVERRIDE;
    void showSettingsDlg();
    void showWebUI();
    void showLog();
    void showOwnDeviceId();
    void showAboutDialog();
    void showNotificationsDialog();
    void dismissNotifications();
    void showInternalErrorsDialog();
    void showDirectoryErrors(unsigned int directoryIndex) const;

Q_SIGNALS:
    /// \remarks Never emitted, just to silence "... depends on non-NOTIFYable ..."
    void connectionChanged();
    /// \remarks Never emitted, just to silence "... depends on non-NOTIFYable ..."
    void dirModelChanged();
    /// \remarks Never emitted, just to silence "... depends on non-NOTIFYable ..."
    void devModelChanged();
    /// \remarks Never emitted, just to silence "... depends on non-NOTIFYable ..."
    void downloadModelChanged();
    /// \remarks Never emitted, just to silence "... depends on non-NOTIFYable ..."
    void serviceChanged();
    void localChanged();
    void connectionStatusChanged();
    void trafficChanged();
    void settingsChanged();
    void currentConnectionConfigIndexChanged(int index);

private Q_SLOTS:
    void handleSettingsChanged();
    void handleConnectionStatusChanged(Data::SyncthingStatus status);
    void handleInternalError(
        const QString &errorMsg, Data::SyncthingErrorCategory category, int networkError, const QNetworkRequest &request, const QByteArray &response);
    void handleErrorsCleared();
    void handleAboutDialogDeleted();
#ifndef SYNCTHINGWIDGETS_NO_WEBVIEW
    void handleWebViewDeleted();
#endif
    void handleNewNotification(ChronoUtilities::DateTime when, const QString &msg);
    void handleSystemdServiceError(const QString &context, const QString &name, const QString &message);

private:
    Dialogs::AboutDialog *m_aboutDlg;
    Data::SyncthingConnection m_connection;
    QtGui::StatusInfo m_statusInfo;
    Data::SyncthingDirectoryModel m_dirModel;
    Data::SyncthingDeviceModel m_devModel;
    Data::SyncthingDownloadModel m_downloadModel;
    Dialogs::SettingsDialog *m_settingsDlg;
    QtGui::DBusStatusNotifier m_dbusNotifier;
    std::vector<Data::SyncthingLogEntry> m_notifications;
#ifndef SYNCTHINGWIDGETS_NO_WEBVIEW
    QtGui::WebViewDialog *m_webViewDlg;
#endif
    int m_currentConnectionConfig;
    Data::SyncthingStatus m_status;
    bool m_initialized;
};

inline Data::SyncthingConnection *SyncthingApplet::connection() const
{
    return const_cast<Data::SyncthingConnection *>(&m_connection);
}

inline Data::SyncthingDirectoryModel *SyncthingApplet::dirModel() const
{
    return const_cast<Data::SyncthingDirectoryModel *>(&m_dirModel);
}

inline Data::SyncthingDeviceModel *SyncthingApplet::devModel() const
{
    return const_cast<Data::SyncthingDeviceModel *>(&m_devModel);
}

inline Data::SyncthingDownloadModel *SyncthingApplet::downloadModel() const
{
    return const_cast<Data::SyncthingDownloadModel *>(&m_downloadModel);
}

inline Data::SyncthingService *SyncthingApplet::service() const
{
#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
    return &Data::syncthingService();
#else
    return nullptr;
#endif
}

inline QString SyncthingApplet::statusText() const
{
    return m_statusInfo.statusText();
}

inline QString SyncthingApplet::additionalStatusText() const
{
    return m_statusInfo.additionalStatusText();
}

inline bool SyncthingApplet::isLocal() const
{
    return m_connection.isLocal();
}

inline int SyncthingApplet::currentConnectionConfigIndex() const
{
    return m_currentConnectionConfig;
}

inline Data::SyncthingConnectionSettings *SyncthingApplet::currentConnectionConfig()
{
    return connectionConfig(m_currentConnectionConfig);
}
}

#endif // SYNCTHINGAPPLET_H
