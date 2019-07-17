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
    static bool s_loggingInitialized;
};

bool RunningState::s_loggingInitialized = false;

inline RunningState::RunningState()
{
    // prevent running multiple Syncthing instances at the same time (for now)
    if ((m_invalid = syncthingRunning.load())) {
        return;
    }

    // initialize logging callback
    if (!s_loggingInitialized) {
        ::libst_init_logging();
        s_loggingInitialized = true;
    }
    ::libst_logging_callback_function = handleLoggingCallback;

    syncthingRunning.store(true);
}

inline RunningState::~RunningState()
{
    ::libst_logging_callback_function = nullptr;
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
    return ::libst_run_syncthing(
        gostr(options.configDir), gostr(options.guiAddress), gostr(options.guiApiKey), options.verbose);
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
    ::libst_stop_syncthing();
}

/*!
 * \brief Returns the ID of the own device.
 * \remarks The own device ID is initialized within runSyncthing().
 */
string ownDeviceId()
{
    return stdstr(::libst_own_device_id());
}

/*!
 * \brief Returns the Syncthing version.
 */
string syncthingVersion()
{
    return stdstr(::libst_syncthing_version());
}

/*!
 * \brief Returns a long form of the Syncthing version, including the codename.
 */
string longSyncthingVersion()
{
    return stdstr(::libst_long_syncthing_version());
}

} // namespace LibSyncthing
