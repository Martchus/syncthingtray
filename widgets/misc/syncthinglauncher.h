#ifndef SYNCTHINGWIDGETS_SYNCTHINGLAUNCHER_H
#define SYNCTHINGWIDGETS_SYNCTHINGLAUNCHER_H

#include "../global.h"

#ifdef SYNCTHINGWIDGETS_USE_LIBSYNCTHING
#include <syncthing/interface.h>
#endif

#include <syncthingconnector/syncthingprocess.h>

#include <c++utilities/io/buffersearch.h>

#include <QByteArray>
#include <QFuture>
#include <QUrl>

namespace Settings {
struct Launcher;
}

namespace Data {

class SyncthingConnection;

class SYNCTHINGWIDGETS_EXPORT SyncthingLauncher : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool running READ isRunning NOTIFY runningChanged)
    Q_PROPERTY(CppUtilities::DateTime activeSince READ activeSince)
    Q_PROPERTY(bool manuallyStopped READ isManuallyStopped)
    Q_PROPERTY(bool emittingOutput READ isEmittingOutput WRITE setEmittingOutput)
    Q_PROPERTY(QUrl guiUrl READ guiUrl WRITE guiUrlChanged)
    Q_PROPERTY(SyncthingProcess *process READ process)

public:
    explicit SyncthingLauncher(QObject *parent = nullptr);

    bool isRunning() const;
    CppUtilities::DateTime activeSince() const;
    bool isActiveFor(unsigned int atLeastSeconds) const;
    bool isManuallyStopped() const;
    bool isEmittingOutput() const;
    void setEmittingOutput(bool emittingOutput);
    QString errorString() const;
    QUrl guiUrl() const;
    SyncthingProcess *process();
    const SyncthingProcess *process() const;
#ifdef SYNCTHINGWIDGETS_USE_LIBSYNCTHING
    LibSyncthing::LogLevel libSyncthingLogLevel() const;
    void setLibSyncthingLogLevel(LibSyncthing::LogLevel logLevel);
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
    void outputAvailable(const QByteArray &data);
    void exited(int exitCode, QProcess::ExitStatus exitStatus);
    void errorOccurred(QProcess::ProcessError error);
    void guiUrlChanged(const QUrl &newUrl);

public Q_SLOTS:
    void launch(const QString &program, const QStringList &arguments);
    void launch(const Settings::Launcher &launcherSettings);
    void terminate(SyncthingConnection *relevantConnection = nullptr);
    void kill();
    void tearDownLibSyncthing();

private Q_SLOTS:
    void handleProcessReadyRead();
    void handleProcessStateChanged(QProcess::ProcessState newState);
    void handleProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
#ifdef SYNCTHINGWIDGETS_USE_LIBSYNCTHING
    void runLibSyncthing(const LibSyncthing::RuntimeOptions &runtimeOptions);
    void stopLibSyncthing();
#else
    void showLibSyncthingNotSupported();
#endif

private:
    void resetState();
#ifdef SYNCTHINGWIDGETS_USE_LIBSYNCTHING
    void handleLoggingCallback(LibSyncthing::LogLevel, const char *message, std::size_t messageSize);
#endif
    void handleOutputAvailable(QByteArray &&data);
    void handleGuiListeningUrlFound(CppUtilities::BufferSearch &bufferSearch, std::string &&searchResult);

    SyncthingProcess m_process;
    QUrl m_guiListeningUrl;
    QFuture<void> m_startFuture;
    QFuture<void> m_stopFuture;
    QByteArray m_outputBuffer;
    CppUtilities::BufferSearch m_guiListeningUrlSearch;
    CppUtilities::DateTime m_futureStarted;
#ifdef SYNCTHINGWIDGETS_USE_LIBSYNCTHING
    LibSyncthing::LogLevel m_libsyncthingLogLevel;
#endif
    bool m_manuallyStopped;
    bool m_emittingOutput;
    bool m_useLibSyncthing;
    static SyncthingLauncher *s_mainInstance;
};

/// \brief Returns whether Syncthing is running.
inline bool SyncthingLauncher::isRunning() const
{
    return m_process.isRunning() || m_startFuture.isRunning();
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
    const auto activeSince(this->activeSince());
    return !activeSince.isNull() && (CppUtilities::DateTime::gmtNow() - activeSince).totalSeconds() > atLeastSeconds;
}

/// \brief Returns whether the Syncthing instance has been manually stopped using SyncthingLauncher::terminate()
/// or SyncthingLauncher::kill().
/// \remarks This is reset when calling SyncthingLauncher::launch().
inline bool SyncthingLauncher::isManuallyStopped() const
{
    return m_manuallyStopped;
}

/// \brief Returns whether the output/log should be emitted via outputAvailable() signal.
inline bool SyncthingLauncher::isEmittingOutput() const
{
    return m_emittingOutput;
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
/// \brief Returns the log level used for libsyncthing.
inline LibSyncthing::LogLevel SyncthingLauncher::libSyncthingLogLevel() const
{
    return m_libsyncthingLogLevel;
}

/// \brief Sets the log level used for libsyncthing.
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
