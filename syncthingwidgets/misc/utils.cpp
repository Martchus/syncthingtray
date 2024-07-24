#include "./utils.h"

#include <syncthingconnector/syncthingconnection.h>

namespace QtGui {

void handleCurrentTabChanged(int tabIndex, Data::SyncthingConnection &connection)
{
    auto flags = connection.pollingFlags();
    CppUtilities::modFlagEnum(flags, Data::SyncthingConnection::PollingFlags::DiskEvents, tabIndex == 3);
    connection.setPollingFlags(flags);
}

} // namespace QtGui
