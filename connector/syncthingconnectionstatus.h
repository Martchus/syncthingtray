#ifndef SYNCTHINGCONNECTION_STATUS_H
#define SYNCTHINGCONNECTION_STATUS_H

#include "./global.h"

#include <QObject>

namespace Data {
#undef Q_NAMESPACE
#define Q_NAMESPACE
Q_NAMESPACE
extern LIB_SYNCTHING_CONNECTOR_EXPORT const QMetaObject staticMetaObject;
QT_ANNOTATE_CLASS(qt_qnamespace, "") /*end*/

enum class SyncthingStatus { Disconnected, Reconnecting, Idle, Scanning, Paused, Synchronizing, OutOfSync, BeingDestroyed };
#if (QT_VERSION >= QT_VERSION_CHECK(5, 8, 0))
Q_ENUM_NS(SyncthingStatus)
#endif

/*!
 * \brief The SyncthingErrorCategory enum classifies different errors related to the SyncthingConnection class.
 */
enum class SyncthingErrorCategory {
    OverallConnection, /**< an error affecting the overall connection */
    SpecificRequest, /**< an error only affecting a specific request */
    Parsing, /**< an error when parsing Syncthing's response as a JSON document */
};
#if (QT_VERSION >= QT_VERSION_CHECK(5, 8, 0))
Q_ENUM_NS(SyncthingErrorCategory)
#endif

} // namespace Data

#endif // SYNCTHINGCONNECTION_STATUS_H
