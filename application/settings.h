#ifndef SETTINGS_H
#define SETTINGS_H

#include <c++utilities/conversion/types.h>

#include <QtGlobal>

QT_FORWARD_DECLARE_CLASS(QByteArray)
QT_FORWARD_DECLARE_CLASS(QString)
QT_FORWARD_DECLARE_CLASS(QSize)

namespace Media {
enum class TagUsage;
enum class ElementPosition;
}

namespace Dialogs {
class QtSettings;
}

namespace Settings {

bool &firstLaunch();

// connection
QString &syncthingUrl();
bool &authEnabled();
QString &userName();
QString &password();
QByteArray &apiKey();

// notifications
bool &notifyOnDisconnect();
bool &notifyOnInternalErrors();
bool &notifyOnSyncComplete();
bool &showSyncthingNotifications();

// apprearance
bool &showTraffic();
QSize &trayMenuSize();
int &frameStyle();

// autostart/launcher
bool &launchSynchting();
QString &syncthingPath();
QString &syncthingArgs();

// web view
#if defined(SYNCTHINGTRAY_USE_WEBENGINE) || defined(SYNCTHINGTRAY_USE_WEBKIT)
bool &webViewDisabled();
double &webViewZoomFactor();
QByteArray &webViewGeometry();
bool &webViewKeepRunning();
#endif

// Qt settings
Dialogs::QtSettings &qtSettings();

void restore();
void save();

}

#endif // SETTINGS_H
