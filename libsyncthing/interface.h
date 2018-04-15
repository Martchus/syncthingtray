#ifndef LIBSYNCTHING_INTERFACE_H
#define LIBSYNCTHING_INTERFACE_H

#include "./global.h"

#include <functional>
#include <string>

namespace LibSyncthing {

struct RuntimeOptions {
    std::string configDir;
    std::string guiAddress;
    std::string guiApiKey;
    std::string logFile;
    bool verbose = false;
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

void LIB_SYNCTHING_EXPORT setLoggingCallback(const LoggingCallback &callback);
void LIB_SYNCTHING_EXPORT setLoggingCallback(LoggingCallback &&callback);
long long LIB_SYNCTHING_EXPORT runSyncthing(const RuntimeOptions &options);
long long LIB_SYNCTHING_EXPORT runSyncthing(const std::string &configDir);
bool LIB_SYNCTHING_EXPORT isSyncthingRunning();
void LIB_SYNCTHING_EXPORT stopSyncthing();
void LIB_SYNCTHING_EXPORT restartSyncthing();
void LIB_SYNCTHING_EXPORT generateCertFiles(const std::string &generateDir);
void LIB_SYNCTHING_EXPORT openGUI();
std::string LIB_SYNCTHING_EXPORT syncthingVersion();
std::string LIB_SYNCTHING_EXPORT longSyncthingVersion();

} // namespace LibSyncthing

#endif // LIBSYNCTHING_INTERFACE_H
