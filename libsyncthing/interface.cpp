#include "./interface.h"

// avoid exporting symbols for internal functions declared within libsyncthinginternal.h as we
// are building a static library here
// note: cgo uses `__declspec` despite `-buildmode c-archive`; bug or feature?
#ifdef PLATFORM_WINDOWS
#ifdef __declspec
#undef __declspec
#endif
#define __declspec(foo)
#endif

#include "libsyncthinginternal.h"

#include <atomic>
#include <cstring>

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
 * \brief Converts the specified null-terminated C-string to a "GoString".
 */
inline _GoString_ gostr(const char *str)
{
    return _GoString_{ str, static_cast<ptrdiff_t>(std::strlen(str)) };
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
 *   Use e.g. std::thread(runSyncthing, options) to run it in another thread.
 */
std::int64_t runSyncthing(const RuntimeOptions &options)
{
    const RunningState runningState;
    if (!runningState) {
        return -1;
    }
    return ::libst_run_syncthing(gostr(options.configDir), gostr(options.dataDir), gostr(options.guiAddress), gostr(options.guiApiKey),
        options.flags & RuntimeFlags::Verbose, options.flags & RuntimeFlags::AllowNewerConfig, options.flags & RuntimeFlags::NoDefaultConfig,
        options.flags & RuntimeFlags::SkipPortProbing, options.flags & RuntimeFlags::EnsureConfigDirExists,
        options.flags & RuntimeFlags::EnsureDataDirExists);
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
 * \returns Returns the Syncthing exit code (as usual, zero means no error).
 * \remarks
 * - Might be called from any thread.
 * - Blocks the current thread until the instance has been stopped.
 *   Use e.g. std::thread(stopSyncthing) to run it in another thread.
 */
std::int64_t stopSyncthing()
{
    return ::libst_stop_syncthing();
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

/*!
 * \brief Runs Syncthing's CLI with the specified \a args.
 */
long long runCli(const std::vector<const char *> &args)
{
    ::libst_clear_cli_args();
    for (const auto *const arg : args) {
        ::libst_append_cli_arg(gostr(arg));
    }
    return ::libst_run_cli();
}

} // namespace LibSyncthing
