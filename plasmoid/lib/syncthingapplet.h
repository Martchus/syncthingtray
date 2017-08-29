#ifndef SYNCTHINGAPPLET_H
#define SYNCTHINGAPPLET_H

#include "../../widgets/misc/statusinfo.h"

#include "../../model/syncthingdevicemodel.h"
#include "../../model/syncthingdirectorymodel.h"
#include "../../model/syncthingdownloadmodel.h"

#include "../../connector/syncthingconnection.h"

#include <qtutilities/aboutdialog/aboutdialog.h>

#include <Plasma/Applet>

namespace Dialogs {
class SettingsDialog;
}

namespace Data {
class SyncthingConnection;
class SyncthingDirectoryModel;
class SyncthingDeviceModel;
class SyncthingDownloadModel;
}

namespace QtGui {
class WebViewDialog;
}

class SyncthingApplet : public Plasma::Applet {
    Q_OBJECT
    Q_PROPERTY(Data::SyncthingConnection *connection READ connection NOTIFY connectionChanged)
    Q_PROPERTY(Data::SyncthingDirectoryModel *dirModel READ dirModel NOTIFY dirModelChanged)
    Q_PROPERTY(Data::SyncthingDeviceModel *devModel READ devModel NOTIFY devModelChanged)
    Q_PROPERTY(Data::SyncthingDownloadModel *downloadModel READ downloadModel NOTIFY downloadModelChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY connectionStatusChanged)
    Q_PROPERTY(QString additionalStatusText READ additionalStatusText NOTIFY connectionStatusChanged)
    Q_PROPERTY(QIcon statusIcon READ statusIcon NOTIFY connectionStatusChanged)
    Q_PROPERTY(QString incomingTraffic READ incomingTraffic NOTIFY trafficChanged)
    Q_PROPERTY(QString outgoingTraffic READ outgoingTraffic NOTIFY trafficChanged)

public:
    SyncthingApplet(QObject *parent, const QVariantList &data);
    ~SyncthingApplet() override;

public:
    Data::SyncthingConnection *connection() const;
    Data::SyncthingDirectoryModel *dirModel() const;
    Data::SyncthingDeviceModel *devModel() const;
    Data::SyncthingDownloadModel *downloadModel() const;
    QString statusText() const;
    QString additionalStatusText() const;
    QIcon statusIcon() const;
    QString incomingTraffic() const;
    QString outgoingTraffic() const;

public Q_SLOTS:
    void init() Q_DECL_OVERRIDE;
    void showConnectionSettingsDlg();
    void showWebUI();
    void showLog();
    void showOwnDeviceId();
    void showAboutDialog();

Q_SIGNALS:
    /// \remarks Never emitted, just to silence "... depends on non-NOTIFYable ..."
    void connectionChanged();
    /// \remarks Never emitted, just to silence "... depends on non-NOTIFYable ..."
    void dirModelChanged();
    /// \remarks Never emitted, just to silence "... depends on non-NOTIFYable ..."
    void devModelChanged();
    /// \remarks Never emitted, just to silence "... depends on non-NOTIFYable ..."
    void downloadModelChanged();
    void connectionStatusChanged();
    void trafficChanged();

private Q_SLOTS:
    void applyConnectionSettings();
    void handleConnectionStatusChanged();
    void handleAboutDialogDeleted();
#ifndef SYNCTHINGWIDGETS_NO_WEBVIEW
    void handleWebViewDeleted();
#endif

private:
    Dialogs::AboutDialog *m_aboutDlg;
    Data::SyncthingConnection m_connection;
    QtGui::StatusInfo m_statusInfo;
    Data::SyncthingDirectoryModel m_dirModel;
    Data::SyncthingDeviceModel m_devModel;
    Data::SyncthingDownloadModel m_downloadModel;
    Dialogs::SettingsDialog *m_connectionSettingsDlg;
#ifndef SYNCTHINGWIDGETS_NO_WEBVIEW
    QtGui::WebViewDialog *m_webViewDlg;
#endif
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

#endif // SYNCTHINGAPPLET_H
