#ifndef DATA_SYNCTHINGPROCESS_H
#define DATA_SYNCTHINGPROCESS_H

#include "./global.h"

#include <c++utilities/chrono/datetime.h>

#include <QProcess>
#include <QStringList>
#include <QTimer>

#ifdef LIB_SYNCTHING_CONNECTOR_BOOST_PROCESS
#include <memory>
#endif

#if !QT_CONFIG(process) && !defined(LIB_SYNCTHING_CONNECTOR_BOOST_PROCESS)
namespace QProcess {
enum ProcessError { FailedToStart, Crashed, Timedout, ReadError, WriteError, UnknownError };
enum ProcessState { NotRunning, Starting, Running };
enum ProcessChannel { StandardOutput, StandardError };
enum ProcessChannelMode { SeparateChannels, MergedChannels, ForwardedChannels, ForwardedOutputChannel, ForwardedErrorChannel };
enum InputChannelMode { ManagedInputChannel, ForwardedInputChannel };
enum ExitStatus { NormalExit, CrashExit };
} // namespace QProcess
#endif

namespace Data {

class SyncthingConnection;
#ifdef LIB_SYNCTHING_CONNECTOR_BOOST_PROCESS
#define LIB_SYNCTHING_CONNECTOR_PROCESS_IO_DEV_BASED
struct SyncthingProcessInternalData;
struct SyncthingProcessIOHandler;
#elif !QT_CONFIG(process)
#define LIB_SYNCTHING_CONNECTOR_PROCESS_IO_DEV_BASED
#define LIB_SYNCTHING_CONNECTOR_NOOP_PROCESS
#endif

#ifdef LIB_SYNCTHING_CONNECTOR_PROCESS_IO_DEV_BASED
using SyncthingProcessBase = QIODevice;
#else
using SyncthingProcessBase = QProcess;
#endif

class LIB_SYNCTHING_CONNECTOR_EXPORT SyncthingProcess : public SyncthingProcessBase {
    Q_OBJECT
    Q_PROPERTY(bool running READ isRunning)
    Q_PROPERTY(CppUtilities::DateTime activeSince READ activeSince)
    Q_PROPERTY(bool manuallyStopped READ isManuallyStopped)

public:
    explicit SyncthingProcess(QObject *parent = nullptr);
    ~SyncthingProcess() override;
    bool isRunning() const;
    CppUtilities::DateTime activeSince() const;
    bool isActiveFor(unsigned int atLeastSeconds) const;
    bool isManuallyStopped() const;
    static SyncthingProcess *mainInstance();
    static void setMainInstance(SyncthingProcess *mainInstance);
    static QStringList splitArguments(const QString &arguments);
    void reportError(QProcess::ProcessError error, const QString &errorString);
#ifdef LIB_SYNCTHING_CONNECTOR_PROCESS_IO_DEV_BASED
    QProcess::ProcessState state() const;
    void start(const QString &program, const QStringList &arguments, QIODevice::OpenMode openMode = QIODevice::ReadOnly);
    void start(const QStringList &program, const QStringList &arguments, QIODevice::OpenMode openMode = QIODevice::ReadOnly);
    qint64 bytesAvailable() const override;
    void close() override;
    int exitCode() const;
    bool waitForFinished(int msecs = 30000);
    bool waitForReadyRead(int msecs) override;
    qint64 processId() const;
    QString program() const;
    QStringList arguments() const;
    QProcess::ProcessChannelMode processChannelMode() const;
    void setProcessChannelMode(QProcess::ProcessChannelMode mode);
#endif

public Q_SLOTS:
    void restartSyncthing(const QString &program, const QStringList &arguments, Data::SyncthingConnection *currentConnection = nullptr);
    void startSyncthing(const QString &program, const QStringList &arguments);
    void stopSyncthing(Data::SyncthingConnection *currentConnection = nullptr);
    void killSyncthing();
#ifdef LIB_SYNCTHING_CONNECTOR_PROCESS_IO_DEV_BASED
    void terminate();
    void kill();
#endif

Q_SIGNALS:
#ifdef LIB_SYNCTHING_CONNECTOR_PROCESS_IO_DEV_BASED
    void started();
    void finished(int exitCode, QProcess::ExitStatus exitStatus);
    void errorOccurred(QProcess::ProcessError error);
    void stateChanged(QProcess::ProcessState newState);
#endif
    void confirmKill();

#ifdef LIB_SYNCTHING_CONNECTOR_PROCESS_IO_DEV_BASED
protected:
    qint64 readData(char *data, qint64 maxSize) override;
    qint64 writeData(const char *data, qint64 len) override;
#endif

private Q_SLOTS:
    void handleStarted();
    void handleFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void killToRestart();
#ifdef LIB_SYNCTHING_CONNECTOR_PROCESS_IO_DEV_BASED
    void handleError(int error, const QString &errorMessage, bool closed);
    void bufferOutput();
    void handleLeftoverProcesses();
#endif

private:
    QString m_program;
    QStringList m_arguments;
    CppUtilities::DateTime m_activeSince;
    QTimer m_killTimer;
#ifdef LIB_SYNCTHING_CONNECTOR_BOOST_PROCESS
    std::shared_ptr<SyncthingProcessInternalData> m_process;
    std::unique_ptr<SyncthingProcessIOHandler> m_handler;
#endif
#ifdef LIB_SYNCTHING_CONNECTOR_PROCESS_IO_DEV_BASED
    QProcess::ProcessChannelMode m_mode;
#endif
    bool m_manuallyStopped;
    static SyncthingProcess *s_mainInstance;
};

/// \brief Returns whether the process is running.
inline bool SyncthingProcess::isRunning() const
{
    return state() != QProcess::NotRunning;
}

/// \brief Returns the last time when QProcess::started() has been emitted.
inline CppUtilities::DateTime SyncthingProcess::activeSince() const
{
    return m_activeSince;
}

/// \brief Checks whether the process already runs for the specified number of seconds.
inline bool SyncthingProcess::isActiveFor(unsigned int atLeastSeconds) const
{
    return !m_activeSince.isNull() && (CppUtilities::DateTime::gmtNow() - m_activeSince).totalSeconds() > atLeastSeconds;
}

/// \brief Returns whether the process has been manually stopped via SyncthingProcess::stopSyncthing(), SyncthingProcess::killSyncthing()
/// or SyncthingProcess::restartSyncthing().
/// \remarks Reset on SyncthingProcess::startSyncthing() and SyncthingProcess::restartSyncthing().
inline bool SyncthingProcess::isManuallyStopped() const
{
    return m_manuallyStopped;
}

/*!
 * \brief Returns the "main" instance assigned via SyncthingProcess::setMainInstance().
 */
inline SyncthingProcess *SyncthingProcess::mainInstance()
{
    return s_mainInstance;
}

/*!
 * \brief Sets the "main" instance.
 */
inline void SyncthingProcess::setMainInstance(SyncthingProcess *mainInstance)
{
    s_mainInstance = mainInstance;
}

#ifdef LIB_SYNCTHING_CONNECTOR_PROCESS_IO_DEV_BASED
/*!
 * \brief Returns the QProcess::ProcessChannelMode like QProcess::processChannelMode().
 */
inline QProcess::ProcessChannelMode SyncthingProcess::processChannelMode() const
{
    return m_mode;
}

/*!
 * \brief Sets the QProcess::ProcessChannelMode like QProcess::setProcessChannelMode().
 * \remarks
 * - Does not affect an already running process.
 * - Supports only QProcess::MergedChannels, QProcess::SeparateChannels and QProcess::ForwardedChannels.
 */
inline void SyncthingProcess::setProcessChannelMode(QProcess::ProcessChannelMode mode)
{
    m_mode = mode;
}
#endif

} // namespace Data

#endif // DATA_SYNCTHINGPROCESS_H
