#include "./internalerror.h"

#include "../settings/settings.h"

#include "../../connector/syncthingconnection.h"
#include "../../connector/syncthingservice.h"
#include "../../connector/utils.h"

#include <QNetworkReply>

using namespace Data;

namespace QtGui {

bool InternalError::isRelevant(const SyncthingConnection &connection, SyncthingErrorCategory category, int networkError)
{
    const auto &settings = Settings::values();
#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
    const SyncthingService &service = syncthingService();
    const bool serviceRelevant = service.isSystemdAvailable() && isLocal(QUrl(connection.syncthingUrl()));
#endif
    return settings.notifyOn.internalErrors && (connection.autoReconnectTries() < 1 || category != SyncthingErrorCategory::OverallConnection)
#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
        && (!settings.systemd.considerForReconnect || !serviceRelevant
               || !(networkError == QNetworkReply::RemoteHostClosedError && service.isManuallyStopped()))
        && (settings.ignoreInavailabilityAfterStart == 0
               || !(networkError == QNetworkReply::ConnectionRefusedError && service.isRunning()
                      && !service.isActiveWithoutSleepFor(settings.ignoreInavailabilityAfterStart)))
#endif
        ;
}
}
