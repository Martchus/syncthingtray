#include "./utils.h"

// use meta-data of syncthingtray application here
#include "resources/../../tray/resources/config.h"

#include <syncthingconnector/syncthingconnection.h>

#include <string_view>

namespace QtGui {

void handleRelevantControlsChanged(bool visible, int tabIndex, Data::SyncthingConnection &connection)
{
    auto flags = connection.pollingFlags();
    CppUtilities::modFlagEnum(flags, Data::SyncthingConnection::PollingFlags::DownloadProgress, visible && tabIndex == 3);
    CppUtilities::modFlagEnum(flags, Data::SyncthingConnection::PollingFlags::DiskEvents, visible && tabIndex == 2);
    CppUtilities::modFlagEnum(flags, Data::SyncthingConnection::PollingFlags::TrafficStatistics, visible);
    CppUtilities::modFlagEnum(flags, Data::SyncthingConnection::PollingFlags::DeviceStatistics, visible && tabIndex == 1);
    connection.setPollingFlags(flags);
}

QString readmeUrl()
{
    if constexpr (std::string_view(APP_VERSION).find('-') == std::string_view::npos) {
        return QStringLiteral("https://github.com/" APP_AUTHOR "/" PROJECT_NAME "/blob/v" APP_VERSION "/README.md");
    } else {
        return QStringLiteral("https://github.com/" APP_AUTHOR "/" PROJECT_NAME "/blob/master/README.md");
    }
}

QString documentationUrl()
{
#ifdef Q_OS_ANDROID
    return QStringLiteral(APP_URL "/../doc/" PROJECT_VARNAME "/docs/android.html");
#else
    return QStringLiteral(APP_URL "/../doc/" PROJECT_VARNAME "/README.html");
#endif
}

} // namespace QtGui
