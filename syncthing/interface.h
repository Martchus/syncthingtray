#ifndef LIBSYNCTHING_INTERFACE_H
#define LIBSYNCTHING_INTERFACE_H

#include "./global.h"

#include <chrono>
#include <cstdint>
#include <functional>
#include <string>
#include <type_traits>
#include <vector>

namespace LibSyncthing {

enum class RuntimeFlags : std::uint64_t {
    None = 0,
    Verbose = (1 << 0),
    AllowNewerConfig = (1 << 1),
    ResetDeltaIndexes = (1 << 2),
    EnsureConfigDirExists = (1 << 3),
    EnsureDataDirExists = (1 << 4),
    SkipPortProbing = (1 << 5),
    ExpandPathsFromEnv = (1 << 6),
};

constexpr bool operator&(RuntimeFlags lhs, RuntimeFlags rhs)
{
    return static_cast<std::underlying_type_t<RuntimeFlags>>(lhs) & static_cast<std::underlying_type_t<RuntimeFlags>>(rhs);
}

constexpr RuntimeFlags operator|(RuntimeFlags lhs, RuntimeFlags rhs)
{
    return static_cast<RuntimeFlags>(static_cast<std::underlying_type_t<RuntimeFlags>>(lhs) | static_cast<std::underlying_type_t<RuntimeFlags>>(rhs));
}

struct RuntimeOptions {
    std::string configDir;
    std::string dataDir;
    std::string guiAddress;
    std::string guiApiKey;
    std::string profilerAddress;
    std::chrono::nanoseconds dbMaintenanceInterval = std::chrono::hours(8);
    std::chrono::nanoseconds dbDeleteRetentionInterval = std::chrono::hours(4320);
    RuntimeFlags flags = RuntimeFlags::AllowNewerConfig | RuntimeFlags::EnsureConfigDirExists | RuntimeFlags::EnsureDataDirExists;
};

enum class LogLevel : int {
    Debug = -4,
    Info = 0,
    Default = Info,
    Warning = 4,
    Error = 8,
};
constexpr auto lowestLogLevel = LogLevel::Debug;
constexpr auto highestLogLevel = LogLevel::Error;

using LoggingCallback = std::function<void(LogLevel, const char *message, std::size_t messageSize)>;

LIB_SYNCTHING_EXPORT bool hasLoggingCallback();
LIB_SYNCTHING_EXPORT void setLoggingCallback(const LoggingCallback &callback);
LIB_SYNCTHING_EXPORT void setLoggingCallback(LoggingCallback &&callback);
LIB_SYNCTHING_EXPORT std::int64_t runSyncthing(const RuntimeOptions &options = RuntimeOptions{});
LIB_SYNCTHING_EXPORT bool isSyncthingRunning();
LIB_SYNCTHING_EXPORT std::int64_t stopSyncthing();
LIB_SYNCTHING_EXPORT std::string ownDeviceId();
LIB_SYNCTHING_EXPORT std::string syncthingVersion();
LIB_SYNCTHING_EXPORT std::string longSyncthingVersion();
LIB_SYNCTHING_EXPORT long long runCli(const std::vector<const char *> &arguments);
LIB_SYNCTHING_EXPORT long long runCommand(const std::vector<const char *> &arguments);
LIB_SYNCTHING_EXPORT std::string verify(std::string_view pubKeyPEM, std::string_view signature, std::string_view data);

} // namespace LibSyncthing

#endif // LIBSYNCTHING_INTERFACE_H
