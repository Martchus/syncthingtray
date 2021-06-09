#include "./syncthingprocess.h"

#include <QTimer>

#ifdef LIB_SYNCTHING_CONNECTOR_BOOST_PROCESS
#include <c++utilities/io/ansiescapecodes.h>

#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/process/async.hpp>
#include <boost/process/async_pipe.hpp>
#include <boost/process/child.hpp>
#include <boost/process/extend.hpp>
#include <boost/process/group.hpp>
#include <boost/process/io.hpp>

#include <atomic>
#include <chrono>
#include <csignal>
#include <iostream>
#include <limits>
#include <mutex>
#include <system_error>
#include <thread>
#endif

using namespace CppUtilities;

namespace Data {

/// \cond
#ifdef LIB_SYNCTHING_CONNECTOR_BOOST_PROCESS
struct SyncthingProcessInternalData : std::enable_shared_from_this<SyncthingProcessInternalData> {
    static constexpr std::size_t bufferCapacity = 0x1000;
    static_assert(SyncthingProcessInternalData::bufferCapacity <= std::numeric_limits<qint64>::max());

    explicit SyncthingProcessInternalData(boost::asio::io_context &ioc);
    struct Lock {
        explicit Lock(const std::weak_ptr<SyncthingProcessInternalData> &weak);
        operator bool() const;
        std::shared_ptr<SyncthingProcessInternalData> process;
        std::unique_lock<std::mutex> lock;
    };

    std::mutex mutex;
    QString program;
    QStringList arguments;
    boost::process::group group;
    boost::process::child child;
    boost::process::async_pipe pipe;
    char buffer[bufferCapacity];
    std::atomic_size_t bytesBuffered = 0;
    std::size_t bytesRead = 0;
    QProcess::ProcessState state = QProcess::NotRunning;
};

struct SyncthingProcessIOHandler {
    explicit SyncthingProcessIOHandler();
    ~SyncthingProcessIOHandler();

    boost::asio::io_context ioc;
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type> guard;
    std::thread t;
};
#endif
/// \endcond

SyncthingProcess *SyncthingProcess::s_mainInstance = nullptr;

/*!
 * \class SyncthingProcess
 * \brief The SyncthingProcess class starts a Syncthing instance or additional tools as an external process.
 *
 * This class is actually not Syncthing-specific. It is just an extension of QProcess for some use-cases within
 * Syncthing Tray.
 */

/*!
 * \brief Constructs a new Syncthing process.
 */
SyncthingProcess::SyncthingProcess(QObject *parent)
    : SyncthingProcessBase(parent)
    , m_manuallyStopped(true)
{
    m_killTimer.setInterval(3000);
    m_killTimer.setSingleShot(true);
#ifndef LIB_SYNCTHING_CONNECTOR_BOOST_PROCESS
    setProcessChannelMode(QProcess::MergedChannels);
#endif
    connect(this, &SyncthingProcess::started, this, &SyncthingProcess::handleStarted);
    connect(this, static_cast<void (SyncthingProcess::*)(int exitCode, QProcess::ExitStatus exitStatus)>(&SyncthingProcess::finished), this,
        &SyncthingProcess::handleFinished);
    connect(&m_killTimer, &QTimer::timeout, this, &SyncthingProcess::confirmKill);
}

/*!
 * \brief Destroys the process.
 */
SyncthingProcess::~SyncthingProcess()
{
#ifdef LIB_SYNCTHING_CONNECTOR_BOOST_PROCESS
    // block until all callbacks have been processed
    if (!m_process) {
        return;
    }
    const auto lock = std::lock_guard<std::mutex>(m_process->mutex);
    m_process.reset();
#endif
}

/*!
 * \brief Splits the given arguments similar to how a shell would split it. So whitespaces are considered seperators unless quotes are used.
 */
QStringList SyncthingProcess::splitArguments(const QString &arguments)
{
    enum { Any, Quote, Slash, Space } lastInput = Any;
    bool inQuotes = false;
    QStringList result;
    QString currentArg;
    for (const auto c : arguments) {
        switch (c.unicode()) {
        case '\"':
        case '\'':
            switch (lastInput) {
            case Slash:
                currentArg += c;
                lastInput = Any;
                break;
            default:
                inQuotes = !inQuotes;
                lastInput = Quote;
            }
            break;
        case '\\':
            switch (lastInput) {
            case Slash:
                currentArg += c;
                lastInput = Any;
                break;
            default:
                lastInput = Slash;
            }
            break;
        case ' ':
            switch (lastInput) {
            case Slash:
                if (!currentArg.isEmpty()) {
                    currentArg += c;
                }
                lastInput = Any;
                break;
            case Space:
                if (inQuotes) {
                    currentArg += c;
                    lastInput = Any;
                }
                break;
            default:
                if (inQuotes) {
                    currentArg += c;
                    lastInput = Any;
                } else {
                    if (!currentArg.isEmpty()) {
                        result << currentArg;
                        currentArg.clear();
                    }
                    lastInput = Space;
                }
            }
            break;
        default:
            currentArg += c;
            lastInput = Any;
        }
    }
    if (!currentArg.isEmpty()) {
        result << currentArg;
    }
    return result;
}

/*!
 * \brief Stops the currently running process. If it has been stopped, starts the specified \a program with the specified \a arguments.
 */
void SyncthingProcess::restartSyncthing(const QString &program, const QStringList &arguments)
{
    if (!isRunning()) {
        startSyncthing(program, arguments);
        return;
    }
    m_program = program;
    m_arguments = arguments;
    m_manuallyStopped = true;
    m_killTimer.start();
    terminate();
}

/*!
 * \brief Starts the specified \a program with the specified \a arguments.
 */
void SyncthingProcess::startSyncthing(const QString &program, const QStringList &arguments)
{
    if (isRunning()) {
        return;
    }
    m_manuallyStopped = false;
    m_killTimer.stop();
    start(program, arguments, QIODevice::ReadOnly);
}

/*!
 * \brief Stops the currently running process gracefully. If it doesn't stop after 3 seconds, attempts to kill the process.
 */
void SyncthingProcess::stopSyncthing()
{
    m_manuallyStopped = true;
    m_killTimer.start();
    terminate();
}

/*!
 * \brief Kills the currently running process.
 */
void SyncthingProcess::killSyncthing()
{
    m_manuallyStopped = true;
    m_killTimer.stop();
    kill();
}

void SyncthingProcess::handleStarted()
{
    m_activeSince = DateTime::gmtNow();
}

void SyncthingProcess::handleFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitCode)
    Q_UNUSED(exitStatus)
    m_activeSince = DateTime();
    m_killTimer.stop();
    if (!m_program.isEmpty()) {
        startSyncthing(m_program, m_arguments);
        m_program.clear();
        m_arguments.clear();
    }
}

void SyncthingProcess::killToRestart()
{
    if (!m_program.isEmpty()) {
        kill();
    }
}

/// \cond
/// \remarks The functions below are for using Boost.Process (instead of QProcess) to be able to
///          terminate the process better by using a group.
#ifdef LIB_SYNCTHING_CONNECTOR_BOOST_PROCESS
SyncthingProcessIOHandler::SyncthingProcessIOHandler()
    : ioc()
    , guard(boost::asio::make_work_guard(ioc))
    , t([this] { ioc.run(); })
{
}

SyncthingProcessIOHandler::~SyncthingProcessIOHandler()
{
    ioc.stop();
    guard.reset();
    t.join();
}

Data::SyncthingProcessInternalData::SyncthingProcessInternalData(boost::asio::io_context &ioc)
    : pipe(ioc)
{
}

inline Data::SyncthingProcessInternalData::Lock::Lock(const std::weak_ptr<SyncthingProcessInternalData> &weak)
    : process(weak.lock())
    , lock(process ? decltype(lock)(process->mutex, std::try_to_lock) : decltype(lock)())
{
}

inline Data::SyncthingProcessInternalData::Lock::operator bool() const
{
    return process && lock;
}

void SyncthingProcess::handleError(QProcess::ProcessError error, const QString &errorMessage, bool closed)
{
    setErrorString(errorMessage);
    errorOccurred(error);
    if (closed) {
        setOpenMode(QIODevice::NotOpen);
    }
}

QProcess::ProcessState SyncthingProcess::state() const
{
    return !m_process ? QProcess::NotRunning : m_process->state;
}

void SyncthingProcess::start(const QString &program, const QStringList &arguments, QIODevice::OpenMode openMode)
{
    // ensure the dev is only opened  in read-only mode because writing is not implemented here
    if (openMode != QIODevice::ReadOnly) {
        setErrorString(QStringLiteral("only read-only openmode supported"));
        std::cerr << EscapeCodes::Phrases::Error << "Unable to launch process: only read-only openmode supported" << EscapeCodes::Phrases::End;
        emit errorOccurred(QProcess::FailedToStart);
        return;
    }

    // handle current/previous process
    if (m_process) {
        const auto lock = std::lock_guard<std::mutex>(m_process->mutex);
        if (m_process->state != QProcess::NotRunning) {
            setErrorString(QStringLiteral("process is still running"));
            std::cerr << EscapeCodes::Phrases::Error << "Unable to launch process: previous process still running" << EscapeCodes::Phrases::End;
            emit errorOccurred(QProcess::FailedToStart);
            return;
        }
        m_process.reset();
    }

    // setup new handler/process
    if (!m_handler) {
        m_handler = std::make_unique<SyncthingProcessIOHandler>();
    }
    m_process = std::make_shared<SyncthingProcessInternalData>(m_handler->ioc);
    emit stateChanged(m_process->state = QProcess::Starting);

    // convert args
    auto args = std::vector<std::string>();
    args.reserve(1 + static_cast<std::size_t>(arguments.size()));
    args.emplace_back(program.toStdString());
    for (const auto &arg : arguments) {
        args.emplace_back(arg.toStdString());
    }
    m_process->program = program;
    m_process->arguments = arguments;

    // start the process within a new process group (or job object under Windows)
    try {
        m_process->child = boost::process::child(
            m_handler->ioc, m_process->group, args, (boost::process::std_out & boost::process::std_err) > m_process->pipe,
            boost::process::extend::on_success =
                [this, maybeProcess = m_process->weak_from_this()](auto &executor) {
                    std::cerr << EscapeCodes::Phrases::Info << "Launched process, PID: "
                              << executor
#ifdef PLATFORM_WINDOWS
                                     .proc_info.dwProcessId
#else
                                     .pid
#endif
                              << EscapeCodes::Phrases::End;
                    if (const auto lock = SyncthingProcessInternalData::Lock(maybeProcess)) {
                        emit stateChanged(m_process->state = QProcess::Running);
                        emit started();
                    }
                },
            boost::process::on_exit =
                [this, maybeProcess = m_process->weak_from_this()](int rc, const std::error_code &ec) {
                    const auto lock = SyncthingProcessInternalData::Lock(maybeProcess);
                    if (!lock) {
                        return;
                    }
                    handleLeftoverProcesses();
                    emit stateChanged(m_process->state = QProcess::NotRunning);
                    emit finished(rc, rc != 0 ? QProcess::CrashExit : QProcess::NormalExit);
                    if (ec) {
                        const auto msg = ec.message();
                        std::cerr << EscapeCodes::Phrases::Error << "Launched process " << m_process->child.native_handle()
                                  << " exited with error: " << msg << EscapeCodes::Phrases::End;
                        QMetaObject::invokeMethod(this, "handleError", Qt::QueuedConnection, Q_ARG(QProcess::ProcessError, QProcess::Crashed),
                            Q_ARG(QString, QString::fromStdString(msg)), Q_ARG(bool, false));
                    }
                },
            boost::process::extend::on_error =
                [this, maybeProcess = m_process->weak_from_this()](auto &, const std::error_code &ec) {
                    const auto lock = SyncthingProcessInternalData::Lock(maybeProcess);
                    if (!lock) {
                        return;
                    }
                    handleLeftoverProcesses();
                    const auto started = m_process->state == QProcess::Running;
                    if (m_process->state != QProcess::NotRunning) {
                        emit stateChanged(m_process->state = QProcess::NotRunning);
                    }
                    if (started) {
                        emit finished(0, QProcess::CrashExit);
                    }
                    const auto error = ec == std::errc::timed_out || ec == std::errc::stream_timeout ? QProcess::Timedout : QProcess::Crashed;
                    const auto msg = ec.message();
                    std::cerr << EscapeCodes::Phrases::Error << "Unable to launch process: " << msg << EscapeCodes::Phrases::End;
                    QMetaObject::invokeMethod(this, "handleError", Qt::QueuedConnection, Q_ARG(QProcess::ProcessError, error),
                        Q_ARG(QString, QString::fromStdString(msg)), Q_ARG(bool, false));
                });
    } catch (const boost::process::process_error &e) {
        std::cerr << EscapeCodes::Phrases::Error << "Unable to launch process: " << e.what() << EscapeCodes::Phrases::End;
        emit stateChanged(m_process->state = QProcess::NotRunning);
        handleError(QProcess::FailedToStart, QString::fromUtf8(e.what()), false);
        return;
    }

    // start reading the process' output
    open(QIODevice::ReadOnly);
    bufferOutput();
}

void SyncthingProcess::terminate()
{
    if (!m_process) {
        return;
    }
#ifdef PLATFORM_UNIX
    auto lock = std::unique_lock<std::mutex>(m_process->mutex);
    if (!m_process->group.valid()) {
        return;
    }
    const auto groupId = m_process->group.native_handle();
    lock.unlock();
    if (::killpg(groupId, SIGTERM) == -1) {
        if (const auto ec = boost::process::detail::get_last_error(); ec != std::errc::no_such_process) {
            const auto msg = ec.message();
            std::cerr << EscapeCodes::Phrases::Error << "Unable to kill process group " << groupId << ": " << msg << EscapeCodes::Phrases::End;
            setErrorString(QString::fromStdString(msg));
            errorOccurred(QProcess::UnknownError);
        }
    }
#else
    // there seems no way to stop the process group gracefully under Windows so just kill it
    // note: Posting a WM_CLOSE message like QProcess would attempt doesn't work for Syncthing.
    kill();
#endif
}

void SyncthingProcess::kill()
{
    if (!m_process) {
        return;
    }
    auto ec = std::error_code();
    auto lock = std::unique_lock<std::mutex>(m_process->mutex);
    if (!m_process->group.valid()) {
        return;
    }
    const auto groupId = m_process->group.native_handle();
    m_process->group.terminate(ec);
    lock.unlock();
    if (ec && ec != std::errc::no_such_process) {
        const auto msg = ec.message();
        std::cerr << EscapeCodes::Phrases::Error << "Unable to kill process group " << groupId << ": " << msg << EscapeCodes::Phrases::End;
        setErrorString(QString::fromStdString(msg));
        errorOccurred(QProcess::UnknownError);
    }
    // note: No need to emit finished() signal here, the on_exit handler will fire
    //       also in case of a forceful termination.
}

void SyncthingProcess::bufferOutput()
{
    m_process->pipe.async_read_some(boost::asio::buffer(m_process->buffer, m_process->bufferCapacity),
        [this, maybeProcess = m_process->weak_from_this()](const boost::system::error_code &ec, auto bytesRead) {
            const auto lock = SyncthingProcessInternalData::Lock(maybeProcess);
            if (!lock) {
                return;
            }
            m_process->bytesBuffered = bytesRead;
            if (ec == boost::asio::error::eof
#ifdef PLATFORM_WINDOWS // looks like we're getting broken pipe (and not just eof) under Windows when stopping the process
                || ec == boost::asio::error::broken_pipe
#endif
            ) {
                m_process->pipe.async_close();
                setOpenMode(QIODevice::NotOpen);
            } else if (ec) {
                const auto msg = ec.message();
                std::cerr << EscapeCodes::Phrases::Error << "Unable to read output of process " << m_process->child.native_handle() << ": " << msg
                          << EscapeCodes::Phrases::End;
                QMetaObject::invokeMethod(this, "handleError", Qt::QueuedConnection, Q_ARG(QProcess::ProcessError, QProcess::ReadError),
                    Q_ARG(QString, QString::fromStdString(msg)), Q_ARG(bool, true));
            }
            if (!ec || bytesRead) {
                emit readyRead();
            }
        });
}

void SyncthingProcess::handleLeftoverProcesses()
{
    if (!m_process->group.valid()) {
        return;
    }
    auto ec = std::error_code();
    m_process->group.terminate(ec);
    if (ec && ec != std::errc::no_such_process) {
        std::cerr << EscapeCodes::Phrases::Error << "Unable to kill leftover processes in group " << m_process->group.native_handle() << ": "
                  << ec.message() << EscapeCodes::Phrases::End;
    }
    m_process->group.wait(ec);
    if (ec && ec != std::errc::no_such_process) {
        std::cerr << EscapeCodes::Phrases::Error << "Unable to wait for leftover processes in group " << m_process->group.native_handle() << ": "
                  << ec.message() << EscapeCodes::Phrases::End;
    }
}

qint64 SyncthingProcess::bytesAvailable() const
{
    return (m_process ? static_cast<qint64>(m_process->bytesBuffered) : 0) + QIODevice::bytesAvailable();
}

void SyncthingProcess::close()
{
    emit aboutToClose();
    if (m_process) {
        const auto lock = std::lock_guard<std::mutex>(m_process->mutex);
        m_process->pipe.async_close();
        kill();
    }
    setOpenMode(QIODevice::NotOpen);
}

int SyncthingProcess::exitCode() const
{
    return m_process ? m_process->child.exit_code() : 0;
}

bool SyncthingProcess::waitForFinished(int msecs)
{
    if (!m_process) {
        return false;
    }
    auto ec = std::error_code();
    if (msecs < 0) {
        m_process->group.wait(ec);
    } else {
        m_process->group.wait_for(std::chrono::milliseconds(msecs), ec);
    }
    return !ec || ec == std::errc::no_such_process || ec == std::errc::no_child_process;
}

qint64 SyncthingProcess::processId() const
{
    return m_process ? m_process->child.id() : -1;
}

QString SyncthingProcess::program() const
{
    return m_process ? m_process->program : QString();
}

QStringList SyncthingProcess::arguments() const
{
    return m_process ? m_process->arguments : QStringList();
}

qint64 SyncthingProcess::readData(char *data, qint64 maxSize)
{
    if (!m_process) {
        return -1;
    }
    if (maxSize < 1) {
        return 0;
    }
    if (!m_process->bytesBuffered) {
        bufferOutput();
        return 0;
    }

    const auto bytesAvailable = m_process->bytesBuffered - m_process->bytesRead;
    if (bytesAvailable > static_cast<std::size_t>(maxSize)) {
        std::memcpy(data, m_process->buffer + m_process->bytesRead, static_cast<std::size_t>(maxSize));
        m_process->bytesRead += static_cast<std::size_t>(maxSize);
        return maxSize;
    } else {
        std::memcpy(data, m_process->buffer + m_process->bytesRead, bytesAvailable);
        m_process->bytesBuffered = 0;
        m_process->bytesRead = 0;
        bufferOutput();
        return static_cast<qint64>(bytesAvailable);
    }
}

qint64 SyncthingProcess::writeData(const char *data, qint64 len)
{
    Q_UNUSED(data)
    Q_UNUSED(len)
    return -1;
}
#endif
/// \endcond

} // namespace Data
