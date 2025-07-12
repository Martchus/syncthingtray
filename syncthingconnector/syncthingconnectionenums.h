#ifndef SYNCTHINGCONNECTION_ENUMS_H
#define SYNCTHINGCONNECTION_ENUMS_H

#include <c++utilities/misc/flagenumclass.h>

#include <QtGlobal>

namespace Data {
enum class SyncthingStatusComputionFlags : quint64;

enum class SyncthingConnectionLoggingFlags : quint64 {
    None, /**< loggingn is disabled */
    FromEnvironment = (1 << 0), /**< environment variables are checked to pull in any of the other flags dynamically */
    ApiCalls = (1 << 1), /**< log calls to Syncthing's REST-API and responses */
    ApiReplies = (1 << 2), /**< log replies from Syncthing's REST-API */
    Events = (1 << 3), /**< log events received via Syncthing's event API */
    DirsOrDevsResetted = (1 << 4), /**< log list of directories/devices when list is reset */
    CertLoading = (1 << 5), /**< log loading of the certificate */
    All = ApiCalls | ApiReplies | Events | DirsOrDevsResetted | CertLoading, /** log as much as possible */
};

} // namespace Data

CPP_UTILITIES_MARK_FLAG_ENUM_CLASS(Data, Data::SyncthingConnectionLoggingFlags)

#endif // SYNCTHINGCONNECTION_ENUMS_H
