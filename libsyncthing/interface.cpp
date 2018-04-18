#include "./interface.h"

#include "libsyncthinginternal.h"

#include <atomic>

using namespace std;

namespace LibSyncthing {

///! \cond

/*!
 * \brief Holds the user provided logging callback which is assigned via setLoggingCallback().
 */
static LoggingCallback loggingCallback;

/*!
 * \brief An indication whether Syncthing is running. Accessible via isSyncthingRunning().
 */
static atomic_bool syncthingRunning;

/*!
 * \brief Converts the specified std::string to a "GoString".
 */
inline _GoString_ gostr(const string &str)
{
    return _GoString_{ str.data(), static_cast<ptrdiff_t>(str.size()) };
}

/*!
 * \brief Converts the specified C-string to a std::string. Takes care of freeing \a str.
 */
inline string stdstr(char *str)
{
    const string copy(str);
    free(reinterpret_cast<void *>(str));
    return copy;
}

/*!
 * \brief Converts the specified "GoString" to a std::string.
 */
inline string stdstr(_GoString_ gostr)
{
    return string(gostr.p, static_cast<size_t>(gostr.n));
}

/*!
 * \brief Internal callback for logging. Calls the user-provided callback.
 */
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

class RunningState {
public:
    RunningState();
    ~RunningState();
    operator bool() const;

private:
    bool m_invalid;
};

inline RunningState::RunningState()
{
    if ((m_invalid = syncthingRunning.load())) {
        return;
    }
    ::libst_loggingCallbackFunction = handleLoggingCallback;
    syncthingRunning.store(true);
}

inline RunningState::~RunningState()
{
    ::libst_loggingCallbackFunction = nullptr;
    syncthingRunning.store(false);
}

inline RunningState::operator bool() const
{
    return !m_invalid;
}

///! \endcond

/*!
 * \brief Sets the callback function for logging. It will be called when a new log message becomes available.
 * \remarks The callback is not necessarily invoked in the same thread it was registered.
 */
void setLoggingCallback(const LoggingCallback &callback)
{
    loggingCallback = callback;
}

/*!
 * \brief Sets the callback function for logging. It will be called when a new log message becomes available.
 * \remarks The callback is not necessarily invoked in the same thread it was registered.
 */
void setLoggingCallback(LoggingCallback &&callback)
{
    loggingCallback = callback;
}

/*!
 * \brief Runs a Syncthing instance using the specified \a options.
 * \return Returns the exit code (as usual, zero means no error).
 * \remark
 * - Does nothing if Syncthing is already running.
 * - Blocks the current thread as long as the instance is running.
 *   Use eg. std::thread(runSyncthing, options) to run it in another thread.
 */
long long runSyncthing(const RuntimeOptions &options)
{
    const RunningState runningState;
    if (!runningState) {
        return -1;
    }
    return ::libst_runSyncthingWithConfig(
        gostr(options.configDir), gostr(options.guiAddress), gostr(options.guiApiKey), gostr(options.logFile), options.verbose);
}

/*!
 * \brief Runs a Syncthing instance using the default options and the specified \a configDir.
 * \return Returns the exit code (as usual, zero means no error).
 * \remark
 * - Does nothing if Syncthing is already running.
 * - Blocks the current thread as long as the instance is running.
 *   Use eg. std::thread(runSyncthing, options) to run it in another thread.
 */
long long runSyncthing(const std::string &configDir)
{
    const RunningState runningState;
    if (!runningState) {
        return -1;
    }
    const string empty;
    return ::libst_runSyncthingWithConfig(gostr(configDir), gostr(empty), gostr(empty), gostr(empty), false);
}

/*!
 * \brief Runs a Syncthing instance using the specified raw \a cliArguments.
 * \return Returns the exit code (as usual, zero means no error).
 * \remark
 * - Does nothing if Syncthing is already running.
 * - Blocks the current thread as long as the instance is running.
 *   Use eg. std::thread(runSyncthing, options) to run it in another thread.
 */
long long runSyncthing(const std::vector<string> &cliArguments)
{
    const RunningState runningState;
    if (!runningState) {
        return -1;
    }
    vector<const char *> argsAsGoStrings;
    argsAsGoStrings.reserve(cliArguments.size());
    for (const auto &arg : cliArguments) {
        argsAsGoStrings.emplace_back(arg.data());
    }
    const GoSlice slice{ argsAsGoStrings.data(), static_cast<GoInt>(argsAsGoStrings.size()), static_cast<GoInt>(argsAsGoStrings.capacity()) };
    return ::libst_runSyncthingWithArgs(slice);
}

/*!
 * \brief Returns whether Syncthing is already running.
 * \returns Might be called from any thread.
 */
bool isSyncthingRunning()
{
    return syncthingRunning.load();
}

/*!
 * \brief Stops Syncthing if it is running; otherwise does nothing.
 * \returns Might be called from any thread.
 * \todo Make this actually work. Currently crashes happen after stopping Syncthing.
 * \sa https://github.com/syncthing/syncthing/issues/4085
 */
void stopSyncthing()
{
    if (!syncthingRunning.load()) {
        return;
    }
    ::libst_stopSyncthing();
}

/*!
 * \brief Restarts Syncthing if it is running; otherwise does nothing.
 * \returns Might be called from any thread.
 * \todo Make this actually work. Currently crashes happen after stopping Syncthing.
 * \sa https://github.com/syncthing/syncthing/issues/4085
 */
void restartSyncthing()
{
    if (!syncthingRunning.load()) {
        return;
    }
    ::libst_restartSyncthing();
}

/*!
 * \brief Generates certificated in the specified directory.
 */
void generateCertFiles(const std::string &generateDir)
{
    ::libst_generateCertFiles(gostr(generateDir));
}

/*!
 * \brief Opens the Syncthing GUI.
 */
void openGUI()
{
    ::libst_openGUI();
}

/*!
 * \brief Returns the Syncthing version.
 */
string syncthingVersion()
{
    return stdstr(::libst_syncthingVersion());
}

/*!
 * \brief Returns a long form of the Syncthing version, including the codename.
 */
string longSyncthingVersion()
{
    return stdstr(::libst_longSyncthingVersion());
}

} // namespace LibSyncthing
