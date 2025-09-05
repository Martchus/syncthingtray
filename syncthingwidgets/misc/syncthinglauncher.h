#ifndef SYNCTHINGWIDGETS_SYNCTHINGLAUNCHER_H
#define SYNCTHINGWIDGETS_SYNCTHINGLAUNCHER_H

#include "../global.h"

#ifdef SYNCTHINGWIDGETS_USE_LIBSYNCTHING
#include <syncthing/interface.h>
#endif

#include <syncthingconnector/syncthingprocess.h>

#include <c++utilities/io/buffersearch.h>

#include <QByteArray>
#include <QFile>
#include <QFuture>
#include <QFutureWatcher>
#include <QUrl>

#include <cstdint>
#include <functional>
#include <optional>

namespace Settings {
struct Launcher;
}

namespace Data {

class SyncthingConnection;

struct SYNCTHINGWIDGETS_EXPORT SyncthingExitStatus {
    explicit SyncthingExitStatus(int code, QProcess::ExitStatus status)
        : code(code)
        , status(status) {};
    int code = 0;
    QProcess::ExitStatus status = QProcess::NormalExit;
};

class SYNCTHINGWIDGETS_EXPORT SyncthingLauncher : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool running READ isRunning NOTIFY runningChanged)
    Q_PROPERTY(bool starting READ isStarting NOTIFY startingChanged)
    Q_PROPERTY(CppUtilities::DateTime activeSince READ activeSince)
    Q_PROPERTY(bool manuallyStopped READ isManuallyStopped WRITE setManuallyStopped)
    Q_PROPERTY(bool emittingOutput READ isEmittingOutput WRITE setEmittingOutput)
    Q_PROPERTY(std::optional<bool> networkConnectionMetered READ isNetworkConnectionMetered WRITE setNetworkConnectionMetered NOTIFY
            networkConnectionMeteredChanged)
    Q_PROPERTY(QString meteredStatus READ meteredStatus NOTIFY networkConnectionMeteredChanged)
    Q_PROPERTY(QString runningStatus READ runningStatus NOTIFY runningChanged)
    Q_PROPERTY(bool stoppingOnMeteredConnection READ isStoppingOnMeteredConnection WRITE setStoppingOnMeteredConnection)
    Q_PROPERTY(QUrl guiUrl READ guiUrl NOTIFY guiUrlChanged)
    Q_PROPERTY(SyncthingProcess *process READ process)

public:
    explicit SyncthingLauncher(QObject *parent = nullptr);
    ~SyncthingLauncher() override;

    QFile &logFile();
    bool isRunning() const;
    bool isStarting() const;
#ifdef SYNCTHINGWIDGETS_USE_LIBSYNCTHING
    void setRunning(bool running, LibSyncthing::RuntimeOptions &&runtimeOptions);
#endif
    QString runningStatus() const;
    CppUtilities::DateTime activeSince() const;
    bool isActiveFor(unsigned int atLeastSeconds) const;
    bool isActiveWithoutSleepFor(unsigned int atLeastSeconds) const;
    bool isManuallyStopped() const;
    void setManuallyStopped(bool manuallyStopped);
    void setManualStopHandler(std::function<bool(void)> &&handler);
    bool isEmittingOutput() const;
    void setEmittingOutput(bool emittingOutput);
    std::optional<bool> isNetworkConnectionMetered() const;
    QString meteredStatus() const;
    void setNetworkConnectionMetered(std::optional<bool> metered);
    bool isStoppingOnMeteredConnection() const;
    void setStoppingOnMeteredConnection(bool stopOnMeteredConnection);
    bool shouldLaunchAccordingToSettings() const;
    QString errorString() const;
    QUrl guiUrl() const;
    QVariantMap overallStatus() const;
    SyncthingProcess *process();
    const SyncthingProcess *process() const;
#ifdef SYNCTHINGWIDGETS_USE_LIBSYNCTHING
    LibSyncthing::LogLevel libSyncthingLogLevel() const;
    QString libSyncthingLogLevelString() const;
    static QString libSyncthingLogLevelString(LibSyncthing::LogLevel logLevel);
    void setLibSyncthingLogLevel(LibSyncthing::LogLevel logLevel);
    void setLibSyncthingLogLevel(const QString &logLevel, LibSyncthing::LogLevel fallbackLogLevel = LibSyncthing::LogLevel::Info);
#endif
    static bool isLibSyncthingAvailable();
    static SyncthingLauncher *mainInstance();
    static void setMainInstance(SyncthingLauncher *mainInstance);
    static QString libSyncthingVersionInfo();
#ifdef SYNCTHINGWIDGETS_USE_LIBSYNCTHING
    void launch(const LibSyncthing::RuntimeOptions &runtimeOptions);
#endif

Q_SIGNALS:
    void confirmKill();
    void runningChanged(bool isRunning);
    void startingChanged();
    void outputAvailable(const QByteArray &data);
    void exitLogged(std::string_view exitMessage);
    void exited(int exitCode, QProcess::ExitStatus exitStatus);
    void errorOccurred(QProcess::ProcessError error);
    void guiUrlChanged(const QUrl &newUrl);
    void networkConnectionMeteredChanged(std::optional<bool> isMetered);

public Q_SLOTS:
    void launch(const QString &program, const QStringList &arguments);
#if defined(SYNCTHINGWIDGETS_GUI_QTWIDGETS)
    void launch(const Settings::Launcher &launcherSettings);
#endif
    void terminate(Data::SyncthingConnection *relevantConnection = nullptr);
    void kill();
    void tearDownLibSyncthing();
    void stopLibSyncthing();

private Q_SLOTS:
    void handleProcessReadyRead();
    void handleProcessStateChanged(QProcess::ProcessState newState);
    void handleProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void handleOutputAvailable(int logLevel, const QByteArray &data);

private:
    void resetState();
#ifdef SYNCTHINGWIDGETS_USE_LIBSYNCTHING
    void handleLoggingCallback(LibSyncthing::LogLevel, const char *message, std::size_t messageSize);
    void runLibSyncthing(const LibSyncthing::RuntimeOptions &runtimeOptions);
    void handleLibSyncthingFinished();
#endif
    void terminateDueToMeteredConnection();
    void showLibSyncthingNotSupported(QByteArray &&reason = QByteArrayLiteral("libsyncthing support not enabled"));

    SyncthingProcess m_process;
    QFile m_logFile;
    QUrl m_guiListeningUrl;
#if defined(SYNCTHINGWIDGETS_GUI_QTWIDGETS)
    const Settings::Launcher *m_lastLauncherSettings;
#endif
#ifdef SYNCTHINGWIDGETS_USE_LIBSYNCTHING
    std::optional<LibSyncthing::RuntimeOptions> m_lastRuntimeOptions;
#endif
    SyncthingConnection *m_relevantConnection;
    QFuture<std::int64_t> m_startFuture;
    QFuture<void> m_stopFuture;
#ifdef SYNCTHINGWIDGETS_USE_LIBSYNCTHING
    QFutureWatcher<std::int64_t> m_startWatcher;
#endif
    QByteArray m_outputBuffer;
    CppUtilities::BufferSearch m_guiListeningUrlSearch;
    CppUtilities::BufferSearch m_exitSearch;
    CppUtilities::DateTime m_futureStarted;
#ifdef SYNCTHINGWIDGETS_USE_LIBSYNCTHING
    LibSyncthing::LogLevel m_libsyncthingLogLevel;
#endif
    std::function<bool(void)> m_manualStopHandler;
    bool m_manuallyStopped;
    bool m_stoppedMetered;
    bool m_emittingOutput;
    bool m_useLibSyncthing;
    bool m_stopOnMeteredConnection;
    std::optional<bool> m_metered;
    std::optional<SyncthingExitStatus> m_lastExitStatus;
    static SyncthingLauncher *s_mainInstance;
};

/// \brief Returns the log file.
/// \remarks
/// - No log file is written by default. Open the QFile returned by this function to enable logging to this file.
/// - You must not use the QFile object returned by this function while Syncthing is running.
inline QFile &SyncthingLauncher::logFile()
{
    return m_logFile;
}

/// \brief Returns whether Syncthing is running.
inline bool SyncthingLauncher::isRunning() const
{
    return m_process.isRunning() || m_startFuture.isRunning();
}

/// \brief Returns whether Syncthing is starting (it is running but the GUI/API is not available yet).
inline bool SyncthingLauncher::isStarting() const
{
    return isRunning() && m_guiListeningUrl.isEmpty();
}

/// \brief Returns when the Syncthing instance has been started.
inline CppUtilities::DateTime SyncthingLauncher::activeSince() const
{
    if (m_process.isRunning()) {
        return m_process.activeSince();
    } else if (m_startFuture.isRunning()) {
        return m_futureStarted;
    }
    return CppUtilities::DateTime();
}

/// \brief Checks whether Syncthing is already running for the specified number of seconds.
inline bool SyncthingLauncher::isActiveFor(unsigned int atLeastSeconds) const
{
    return SyncthingProcess::isActiveFor(activeSince(), atLeastSeconds);
}

/// \brief Checks whether Syncthing is already running for the specified number of seconds.
/// \remarks Considers only the time since the last wakeup if known; otherwise it is identical to isActiveFor().
inline bool SyncthingLauncher::isActiveWithoutSleepFor(unsigned int atLeastSeconds) const
{
    return m_process.isActiveWithoutSleepFor(activeSince(), atLeastSeconds);
}

/// \brief Returns whether the Syncthing instance has been manually stopped using SyncthingLauncher::terminate()
/// or SyncthingLauncher::kill() or by other means communicated via SyncthingLauncher::setManuallyStopped().
/// \remarks This is reset when calling SyncthingLauncher::launch().
inline bool SyncthingLauncher::isManuallyStopped() const
{
    return m_manuallyStopped;
}

/// \brief Sets whether the Syncthing instance has been manually stopped.
/// \remarks Set this before stopping Syncthing by other means than the launcher (e.g. when using the shutdown API)
///          to have SyncthingLauncher::isManuallyStopped() reflect that as well (e.g. to suppress notifications).
inline void SyncthingLauncher::setManuallyStopped(bool manuallyStopped)
{
    if (!manuallyStopped || !m_manualStopHandler || !m_manualStopHandler()) {
        m_manuallyStopped = manuallyStopped;
    }
}

/// \brief Sets a handler which is executed *before* the process is manually stopped.
/// \remarks
// - When the handler returns true isManuallyStopped() will *not* be set. The stopping is *not* intercepted
//   in any case, though.
// - Used under Android to inform the UI process that Syncthing is going to stop *before* stopping it so it
//   can avoid showing "connection refused" errors.
inline void SyncthingLauncher::setManualStopHandler(std::function<bool(void)> &&handler)
{
    m_manualStopHandler = std::move(handler);
}

/// \brief Returns whether the output/log should be emitted via outputAvailable() signal.
inline bool SyncthingLauncher::isEmittingOutput() const
{
    return m_emittingOutput;
}

/// \brief Returns whether the current network connection is metered.
/// \remarks Returns an std::optional<bool> without value if it is unknown whether the network connection is metered.
inline std::optional<bool> SyncthingLauncher::isNetworkConnectionMetered() const
{
    return m_metered;
}

/// \brief Returns whether Syncthing should automatically be stopped as long as the network connection is metered.
inline bool SyncthingLauncher::isStoppingOnMeteredConnection() const
{
    return m_stopOnMeteredConnection;
}

/// \brief Returns whether Syncthing is supposed to be launched according to settings.
/// \remarks
/// - The only relevant setting so far is isStoppingOnMeteredConnection().
/// - One can still launch Syncthing via the launch() functions despite shouldLaunchAccordingToSettings() returning true.
inline bool SyncthingLauncher::shouldLaunchAccordingToSettings() const
{
    return !isStoppingOnMeteredConnection() || !isNetworkConnectionMetered().value_or(false);
}

/// \brief Returns the last error message.
inline QString SyncthingLauncher::errorString() const
{
    return m_process.errorString();
}

/// \brief Returns the GUI listening URL determined from Syncthing's log.
inline QUrl SyncthingLauncher::guiUrl() const
{
    return m_guiListeningUrl;
}

/// \brief Returns the underlying SyncthingProcess.
inline SyncthingProcess *SyncthingLauncher::process()
{
    return &m_process;
}

/// \brief Returns the underlying SyncthingProcess.
inline const SyncthingProcess *SyncthingLauncher::process() const
{
    return &m_process;
}

#ifdef SYNCTHINGWIDGETS_USE_LIBSYNCTHING
/// \brief Returns the log level for the built-in Syncthing instance.
inline LibSyncthing::LogLevel SyncthingLauncher::libSyncthingLogLevel() const
{
    return m_libsyncthingLogLevel;
}

/// \brief Returns the log level for the built-in Syncthing instance as string.
inline QString SyncthingLauncher::libSyncthingLogLevelString() const
{
    return libSyncthingLogLevelString(m_libsyncthingLogLevel);
}

/// \brief Sets the log level for the built-in Syncthing instance.
inline void SyncthingLauncher::setLibSyncthingLogLevel(LibSyncthing::LogLevel logLevel)
{
    m_libsyncthingLogLevel = logLevel;
}
#endif

/// \brief Returns the SyncthingLauncher instance previously assigned via SyncthingLauncher::setMainInstance().
inline SyncthingLauncher *SyncthingLauncher::mainInstance()
{
    return s_mainInstance;
}

/// \brief Sets the "main" SyncthingLauncher instance and SyncthingProcess::mainInstance() if not already assigned.
inline void SyncthingLauncher::setMainInstance(SyncthingLauncher *mainInstance)
{
    if ((s_mainInstance = mainInstance) && !SyncthingProcess::mainInstance()) {
        SyncthingProcess::setMainInstance(&mainInstance->m_process);
    }
}

} // namespace Data

#endif // SYNCTHINGWIDGETS_SYNCTHINGLAUNCHER_H
