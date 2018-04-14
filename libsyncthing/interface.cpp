#include "./interface.h"

#include "libsyncthinginternal.h"

#include <iostream>

using namespace std;

namespace LibSyncthing {

static LoggingCallback loggingCallback;

inline _GoString_ gostr(const string &str)
{
    return _GoString_{ str.data(), static_cast<ptrdiff_t>(str.size()) };
}

inline string stdstr(char *str)
{
    const string copy(str);
    free(reinterpret_cast<void *>(str));
    return copy;
}

inline string stdstr(_GoString_ gostr)
{
    return string(gostr.p, static_cast<size_t>(gostr.n));
}

void handleLoggingCallback(int logLevelInt, const char *msg, size_t msgSize)
{
    // ignore callback when no callback registered
    if (!loggingCallback) {
        return;
    }

    // ignore invalid/unknown log level
    const auto logLevel = static_cast<LogLevel>(logLevelInt);
    if (logLevel < lowestLogLevel || logLevel > highestLogLevel) {
        return;
    }

    loggingCallback(logLevel, msg, msgSize);
}

void setLoggingCallback(const LoggingCallback &callback)
{
    loggingCallback = callback;
}

void setLoggingCallback(LoggingCallback &&callback)
{
    loggingCallback = callback;
}

long long runSyncthing(const RuntimeOptions &options)
{
    ::libst_loggingCallbackFunction = handleLoggingCallback;
    return ::libst_runSyncthing(
        gostr(options.configDir), gostr(options.guiAddress), gostr(options.guiApiKey), gostr(options.logFile), options.verbose);
}

void stopSyncthing()
{
    ::libst_stopSyncthing();
}

void restartSyncthing()
{
    ::libst_restartSyncthing();
}

void generateCertFiles(const std::string &generateDir)
{
    ::libst_generateCertFiles(gostr(generateDir));
}

void openGUI()
{
    ::libst_openGUI();
}

string syncthingVersion()
{
    return stdstr(::libst_syncthingVersion());
}

string longSyncthingVersion()
{
    return stdstr(::libst_longSyncthingVersion());
}

} // namespace LibSyncthing
