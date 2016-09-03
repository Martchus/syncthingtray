#include "./settings.h"

#include <qtutilities/settingsdialog/qtsettings.h>

#include <QString>
#include <QByteArray>
#include <QApplication>
#include <QSettings>
#include <QFrame>
#include <QSslCertificate>
#include <QSslError>
#include <QMessageBox>

using namespace std;
using namespace Media;

namespace Settings {

bool &firstLaunch()
{
    static bool v = false;
    return v;
}

// connection
ConnectionSettings &primaryConnectionSettings()
{
    static ConnectionSettings v;
    return v;
}

std::vector<ConnectionSettings> &secondaryConnectionSettings()
{
    static vector<ConnectionSettings> v;
    return v;
}

// notifications
bool &notifyOnDisconnect()
{
    static bool v = true;
    return v;
}
bool &notifyOnInternalErrors()
{
    static bool v = true;
    return v;
}
bool &notifyOnSyncComplete()
{
    static bool v = true;
    return v;
}
bool &showSyncthingNotifications()
{
    static bool v = true;
    return v;
}

// appearance
bool &showTraffic()
{
    static bool v = true;
    return v;
}
QSize &trayMenuSize()
{
    static QSize v(350, 300);
    return v;
}
int &frameStyle()
{
    static int v = QFrame::StyledPanel | QFrame::Sunken;
    return v;
}

// autostart/launcher
bool &launchSynchting()
{
    static bool v = false;
    return v;
}
QString &syncthingPath()
{
#ifdef PLATFORM_WINDOWS
    static QString v(QStringLiteral("syncthing.exe"));
#else
    static QString v(QStringLiteral("syncthing"));
#endif
    return v;
}
QString &syncthingArgs()
{
    static QString v;
    return v;
}

// web view
#if defined(SYNCTHINGTRAY_USE_WEBENGINE) || defined(SYNCTHINGTRAY_USE_WEBKIT)
bool &webViewDisabled()
{
    static bool v = false;
    return v;
}
double &webViewZoomFactor()
{
    static double v = 1.0;
    return v;
}
QByteArray &webViewGeometry()
{
    static QByteArray v;
    return v;
}
bool &webViewKeepRunning()
{
    static bool v = true;
    return v;
}
#endif

// Qt settings
Dialogs::QtSettings &qtSettings()
{
    static Dialogs::QtSettings v;
    return v;
}

void restore()
{
    QSettings settings(QSettings::IniFormat, QSettings::UserScope,  QApplication::organizationName(), QApplication::applicationName());

    settings.beginGroup(QStringLiteral("tray"));

    const int connectionCount = settings.beginReadArray(QStringLiteral("connections"));
    if(connectionCount > 0) {
        secondaryConnectionSettings().clear();
        secondaryConnectionSettings().reserve(static_cast<size_t>(connectionCount));
        for(int i = 0; i < connectionCount; ++i) {
            ConnectionSettings *connectionSettings;
            if(i == 0) {
                connectionSettings = &primaryConnectionSettings();
            } else {
                secondaryConnectionSettings().emplace_back();
                connectionSettings = &secondaryConnectionSettings().back();
            }
            settings.setArrayIndex(i);
            connectionSettings->label = settings.value(QStringLiteral("label")).toString();
            if(connectionSettings->label.isEmpty()) {
                connectionSettings->label = (i == 0 ? QStringLiteral("Primary instance") : QStringLiteral("Secondary instance %1").arg(i));
            }
            connectionSettings->syncthingUrl = settings.value(QStringLiteral("syncthingUrl"), connectionSettings->syncthingUrl).toString();
            connectionSettings->authEnabled = settings.value(QStringLiteral("authEnabled"), connectionSettings->authEnabled).toBool();
            connectionSettings->userName = settings.value(QStringLiteral("userName")).toString();
            connectionSettings->password = settings.value(QStringLiteral("password")).toString();
            connectionSettings->apiKey = settings.value(QStringLiteral("apiKey")).toByteArray();
            connectionSettings->httpsCertPath = settings.value(QStringLiteral("httpsCertPath")).toString();
            if(!connectionSettings->loadHttpsCert()) {
                QMessageBox::critical(nullptr, QCoreApplication::applicationName(), QCoreApplication::translate("Settings::restore", "Unable to load certificate \"%1\" when restoring settings.").arg(connectionSettings->httpsCertPath));
            }
        }
    } else {
        firstLaunch() = true;
        primaryConnectionSettings().label = QStringLiteral("Primary instance");
    }
    settings.endArray();

    notifyOnDisconnect() = settings.value(QStringLiteral("notifyOnDisconnect"), notifyOnDisconnect()).toBool();
    notifyOnInternalErrors() = settings.value(QStringLiteral("notifyOnErrors"), notifyOnInternalErrors()).toBool();
    notifyOnSyncComplete() = settings.value(QStringLiteral("notifyOnSyncComplete"), notifyOnSyncComplete()).toBool();
    showSyncthingNotifications() = settings.value(QStringLiteral("showSyncthingNotifications"), showSyncthingNotifications()).toBool();
    showTraffic() = settings.value(QStringLiteral("showTraffic"), showTraffic()).toBool();
    trayMenuSize() = settings.value(QStringLiteral("trayMenuSize"), trayMenuSize()).toSize();
    frameStyle() = settings.value(QStringLiteral("frameStyle"), frameStyle()).toInt();
    settings.endGroup();

    settings.beginGroup(QStringLiteral("startup"));
    launchSynchting() = settings.value(QStringLiteral("launchSynchting"), false).toBool();
    syncthingPath() = settings.value(QStringLiteral("syncthingPath"), syncthingPath()).toString();
    syncthingArgs() = settings.value(QStringLiteral("syncthingArgs"), syncthingArgs()).toString();
    settings.endGroup();

#if defined(SYNCTHINGTRAY_USE_WEBENGINE) || defined(SYNCTHINGTRAY_USE_WEBKIT)
    settings.beginGroup(QStringLiteral("webview"));
    webViewDisabled() = settings.value(QStringLiteral("isabled"), false).toBool();
    webViewZoomFactor() = settings.value(QStringLiteral("zoomFactor"), 1.0).toDouble();
    webViewGeometry() = settings.value(QStringLiteral("geometry")).toByteArray();
    webViewKeepRunning() = settings.value(QStringLiteral("keepRunning"), true).toBool();
    settings.endGroup();
#endif

    qtSettings().restore(settings);
}

void save()
{
    QSettings settings(QSettings::IniFormat, QSettings::UserScope,  QApplication::organizationName(), QApplication::applicationName());

    settings.beginGroup(QStringLiteral("tray"));
    const int connectionCount = static_cast<int>(1 + secondaryConnectionSettings().size());
    settings.beginWriteArray(QStringLiteral("connections"), connectionCount);
    for(int i = 0; i < connectionCount; ++i) {
        const ConnectionSettings *connectionSettings = (i == 0 ? &primaryConnectionSettings() : &secondaryConnectionSettings()[static_cast<size_t>(i - 1)]);
        settings.setArrayIndex(i);
        settings.setValue(QStringLiteral("label"), connectionSettings->label);
        settings.setValue(QStringLiteral("syncthingUrl"), connectionSettings->syncthingUrl);
        settings.setValue(QStringLiteral("authEnabled"), connectionSettings->authEnabled);
        settings.setValue(QStringLiteral("userName"), connectionSettings->userName);
        settings.setValue(QStringLiteral("password"), connectionSettings->password);
        settings.setValue(QStringLiteral("apiKey"), connectionSettings->apiKey);
        settings.setValue(QStringLiteral("httpsCertPath"), connectionSettings->httpsCertPath);
    }
    settings.endArray();

    settings.setValue(QStringLiteral("notifyOnDisconnect"), notifyOnDisconnect());
    settings.setValue(QStringLiteral("notifyOnErrors"), notifyOnInternalErrors());
    settings.setValue(QStringLiteral("notifyOnSyncComplete"), notifyOnSyncComplete());
    settings.setValue(QStringLiteral("showSyncthingNotifications"), showSyncthingNotifications());
    settings.setValue(QStringLiteral("showTraffic"), showTraffic());
    settings.setValue(QStringLiteral("trayMenuSize"), trayMenuSize());
    settings.setValue(QStringLiteral("frameStyle"), frameStyle());
    settings.endGroup();

    settings.beginGroup(QStringLiteral("startup"));
    settings.setValue(QStringLiteral("launchSynchting"), launchSynchting());
    settings.setValue(QStringLiteral("syncthingPath"), syncthingPath());
    settings.setValue(QStringLiteral("syncthingArgs"), syncthingArgs());
    settings.endGroup();

#if defined(SYNCTHINGTRAY_USE_WEBENGINE) || defined(SYNCTHINGTRAY_USE_WEBKIT)
    settings.beginGroup(QStringLiteral("webview"));
    settings.setValue(QStringLiteral("disabled"), webViewDisabled());
    settings.setValue(QStringLiteral("zoomFactor"), webViewZoomFactor());
    settings.setValue(QStringLiteral("geometry"), webViewGeometry());
    settings.setValue(QStringLiteral("keepRunning"), webViewKeepRunning());
    settings.endGroup();
#endif

    qtSettings().save(settings);
}

bool ConnectionSettings::loadHttpsCert()
{
    if(!httpsCertPath.isEmpty()) {
        const QList<QSslCertificate> cert = QSslCertificate::fromPath(httpsCertPath);
        if(cert.isEmpty()) {
            return false;
        }
        expectedSslErrors.clear();
        expectedSslErrors.reserve(4);
        expectedSslErrors << QSslError(QSslError::UnableToGetLocalIssuerCertificate, cert.at(0));
        expectedSslErrors << QSslError(QSslError::UnableToVerifyFirstCertificate, cert.at(0));
        expectedSslErrors << QSslError(QSslError::SelfSignedCertificate, cert.at(0));
        expectedSslErrors << QSslError(QSslError::HostNameMismatch, cert.at(0));
    }
    return true;
}

}
