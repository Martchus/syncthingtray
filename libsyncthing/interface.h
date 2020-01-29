#ifndef LIBSYNCTHING_INTERFACE_H
#define LIBSYNCTHING_INTERFACE_H

#include "./global.h"

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace LibSyncthing {

struct RuntimeOptions {
    std::string configDir;
    std::string guiAddress;
    std::string guiApiKey;
    bool verbose = false;
    bool allowNewerConfig = true;
    bool noDefaultConfig = false;
    bool ensureConfigDirectoryExists = true;
};

enum class LogLevel : int {
    Debug,
    Verbose,
    Info,
    Warning,
    Fatal,
};
constexpr auto lowestLogLevel = LogLevel::Debug;
constexpr auto highestLogLevel = LogLevel::Fatal;

using LoggingCallback = std::function<void(LogLevel, const char *message, std::size_t messageSize)>;

LIB_SYNCTHING_EXPORT void setLoggingCallback(const LoggingCallback &callback);
LIB_SYNCTHING_EXPORT void setLoggingCallback(LoggingCallback &&callback);
LIB_SYNCTHING_EXPORT std::int64_t runSyncthing(const RuntimeOptions &options = RuntimeOptions{});
LIB_SYNCTHING_EXPORT bool isSyncthingRunning();
LIB_SYNCTHING_EXPORT std::int64_t stopSyncthing();
LIB_SYNCTHING_EXPORT std::string ownDeviceId();
LIB_SYNCTHING_EXPORT std::string syncthingVersion();
LIB_SYNCTHING_EXPORT std::string longSyncthingVersion();

} // namespace LibSyncthing

#endif // LIBSYNCTHING_INTERFACE_H
