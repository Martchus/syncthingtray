#include "./utils.h"

#include <syncthingconnector/syncthingconnection.h>

namespace QtGui {

void handleRelevantControlsChanged(bool visible, int tabIndex, Data::SyncthingConnection &connection)
{
    auto flags = connection.pollingFlags();
    CppUtilities::modFlagEnum(flags, Data::SyncthingConnection::PollingFlags::DownloadProgress, visible && tabIndex == 2);
    CppUtilities::modFlagEnum(flags, Data::SyncthingConnection::PollingFlags::DiskEvents, visible && tabIndex == 3);
    CppUtilities::modFlagEnum(flags, Data::SyncthingConnection::PollingFlags::TrafficStatistics, visible);
    CppUtilities::modFlagEnum(flags, Data::SyncthingConnection::PollingFlags::DeviceStatistics, visible && tabIndex == 1);
    connection.setPollingFlags(flags);
}

} // namespace QtGui
