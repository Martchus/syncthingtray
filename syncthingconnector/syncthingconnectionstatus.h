#ifndef SYNCTHINGCONNECTION_STATUS_H
#define SYNCTHINGCONNECTION_STATUS_H

#include "./global.h"

#include <QObject>

namespace Data {

/*!
 * \brief The SyncthingStatus enum specifies the overall status of the connection to Syncthing via its REST-API.
 *
 * Scanning, Paused, Synchronizing, RemoteNotInSync and NoRemoteConnected are only computed if the SyncthingStatusComputionFlags
 * are set for these.
 *
 * This is *not* a flag enum even though the "connected" states are not exclusive. That's because only one icon can be
 * shown at the same time anyway. Check out SyncthingConnection::setStatus() for the precedence.
 */
enum class SyncthingStatus {
    Disconnected = 0, /**< disconnected, possibly currently connecting */
    Reconnecting = 1, /**< disconnected, currently re-connnecting */
    BeingDestroyed = 7, /**< status is unknown; the SyncthingConnnection object is being destroyed anyway */
    Idle = 2, /**< connected, no special status information available/determined */
    Scanning = 3, /**< connected, at least one directory is scanning */
    Paused = 4, /**< connected, at least one device is paused */
    Synchronizing = 5, /**< connected, at least one local directory is waiting to sync, preparing to sync or synchronizing */
    RemoteNotInSync = 8, /**< connected, at least one directory of a connected remote device is not in sync (still synchronizing, error, …) */
    NoRemoteConnected = 9, /**< connected to Syncthing but Syncthing is not connected to any remote device */
};

/*!
 * \brief The SyncthingErrorCategory enum classifies different errors related to the SyncthingConnection class.
 */
enum class SyncthingErrorCategory {
    OverallConnection, /**< an error affecting the overall connection */
    SpecificRequest, /**< an error only affecting a specific request */
    Parsing, /**< an error when parsing Syncthing's response as a JSON document */
    TLS, /**< a TLS error occurred */
};

} // namespace Data

#endif // SYNCTHINGCONNECTION_STATUS_H
