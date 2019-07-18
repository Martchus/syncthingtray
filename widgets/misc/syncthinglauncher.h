#ifndef SYNCTHINGWIDGETS_SYNCTHINGLAUNCHER_H
#define SYNCTHINGWIDGETS_SYNCTHINGLAUNCHER_H

#include "../global.h"

#include "../../connector/syncthingprocess.h"
#include "../../libsyncthing/interface.h"

#include <QFuture>

namespace LibSyncthing {
struct RuntimeOptions;
}

namespace Settings {
struct Launcher;
}

namespace Data {

class SYNCTHINGWIDGETS_EXPORT SyncthingLauncher : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool running READ isRunning NOTIFY runningChanged)
    Q_PROPERTY(CppUtilities::DateTime activeSince READ activeSince)
    Q_PROPERTY(bool manuallyStopped READ isManuallyStopped)

public:
    explicit SyncthingLauncher(QObject *parent = nullptr);

    bool isRunning() const;
    CppUtilities::DateTime activeSince() const;
    bool isActiveFor(unsigned int atLeastSeconds) const;
    bool isManuallyStopped() const;
    static bool isLibSyncthingAvailable();
    static SyncthingLauncher *mainInstance();
    static void setMainInstance(SyncthingLauncher *mainInstance);

Q_SIGNALS:
    void confirmKill();
    void runningChanged(bool isRunning);
    void outputAvailable(const QByteArray &data);
    void exited(int exitCode, QProcess::ExitStatus exitStatus);
    void errorOccurred(QProcess::ProcessError error);

public Q_SLOTS:
    void launch(const QString &program, const QStringList &arguments);
    void launch(const Settings::Launcher &launcherSettings);
    void launch(const LibSyncthing::RuntimeOptions &runtimeOptions);
    void terminate();
    void kill();

private Q_SLOTS:
    void handleProcessReadyRead();
    void handleProcessStateChanged(QProcess::ProcessState newState);
    void handleProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void handleLoggingCallback(LibSyncthing::LogLevel, const char *message, std::size_t messageSize);
    void runLibSyncthing(const LibSyncthing::RuntimeOptions &runtimeOptions);
    void stopLibSyncthing();

private:
    SyncthingProcess m_process;
    QFuture<void> m_future;
    CppUtilities::DateTime m_futureStarted;
    bool m_manuallyStopped;
    bool m_useLibSyncthing;
    static SyncthingLauncher *s_mainInstance;
};

/// \brief Returns whether Syncthing is running.
inline bool SyncthingLauncher::isRunning() const
{
    return m_process.isRunning() || m_future.isRunning();
}

/// \brief Returns when the Syncthing instance has been started.
inline CppUtilities::DateTime SyncthingLauncher::activeSince() const
{
    if (m_process.isRunning()) {
        return m_process.activeSince();
    } else if (m_future.isRunning()) {
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
/// \remarks This is resetted when calling SyncthingLauncher::launch().
inline bool SyncthingLauncher::isManuallyStopped() const
{
    return m_manuallyStopped;
}

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
