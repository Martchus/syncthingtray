#include "./syncthingprocess.h"
#include "./syncthingconnection.h"

#include <QEventLoop>
#include <QStringBuilder>
#include <QTimer>

#ifdef Q_OS_WINDOWS
#include <QCoreApplication>
#endif

// uncomment to enforce stopSyncthing() via REST-API (for testing)
//#define LIB_SYNCTHING_CONNECTOR_ENFORCE_STOP_VIA_API

#ifdef LIB_SYNCTHING_CONNECTOR_BOOST_PROCESS
#include <c++utilities/io/ansiescapecodes.h>

#include <boost/version.hpp>

#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/filesystem/path.hpp>
#if BOOST_VERSION >= 108600
#include <boost/process/v1/async.hpp>
#include <boost/process/v1/async_pipe.hpp>
#include <boost/process/v1/child.hpp>
#include <boost/process/v1/extend.hpp>
#include <boost/process/v1/group.hpp>
#include <boost/process/v1/io.hpp>
#include <boost/process/v1/locale.hpp>
#include <boost/process/v1/search_path.hpp>
#ifdef PLATFORM_WINDOWS
#include <boost/process/v1/windows.hpp>
#endif
#else
#include <boost/process/async.hpp>
#include <boost/process/async_pipe.hpp>
#include <boost/process/child.hpp>
#include <boost/process/extend.hpp>
#include <boost/process/group.hpp>
#include <boost/process/io.hpp>
#include <boost/process/locale.hpp>
#include <boost/process/search_path.hpp>
#ifdef PLATFORM_WINDOWS
#include <boost/process/windows.hpp>
#endif
namespace boost::process {
namespace v1 = boost::process;
}
#endif // BOOST_VERSION >= 108600

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <csignal>
#include <iostream>
#include <limits>
#include <mutex>
#include <system_error>
#include <thread>
#endif // LIB_SYNCTHING_CONNECTOR_BOOST_PROCESS

#ifdef PLATFORM_WINDOWS
// include Windows header directly for SyncthingProcess::nativeEventFilter()
#include <windows.h>
#endif

using namespace CppUtilities;

namespace Data {

#ifdef LIB_SYNCTHING_CONNECTOR_BOOST_PROCESS
#ifdef PLATFORM_WINDOWS
#define LIB_SYNCTHING_CONNECTOR_PLATFORM_ARGS boost::process::v1::windows::create_no_window,
#ifdef Q_CC_MSVC
// fix crashes using MSVC; for some reason using tStdWString() leads to crashes with Qt 6.5.0 and MSVC 2022
#define LIB_SYNCTHING_CONNECTOR_STRING_CONVERSION(s) std::wstring(reinterpret_cast<const wchar_t *>(s.utf16()), s.size())
#else
#define LIB_SYNCTHING_CONNECTOR_STRING_CONVERSION(s) s.toStdWString()
#endif
#else
#define LIB_SYNCTHING_CONNECTOR_PLATFORM_ARGS
#define LIB_SYNCTHING_CONNECTOR_STRING_CONVERSION(s) s.toLocal8Bit().toStdString()
#endif

/// \brief Holds data related to the process execution via Boost.Process.
/// \remarks A new one is created for each process to be started.
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
    boost::process::v1::group group;
    boost::process::v1::child child;
    boost::process::v1::async_pipe pipe;
    std::mutex readMutex;
    std::condition_variable readCondVar;
    char buffer[bufferCapacity];
    std::atomic_size_t bytesBuffered = 0;
    std::size_t bytesRead = 0;
    QProcess::ProcessState state = QProcess::NotRunning;
};

/// \brief Holds the IO context and thread handles for the process execution via Boost.Process.
/// \remarks A new one is created for each SyncthingProcess instance.
struct SyncthingProcessIOHandler {
    explicit SyncthingProcessIOHandler();
    ~SyncthingProcessIOHandler();

    boost::asio::io_context ioc;
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type> guard;
    std::thread t;
};
#endif

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
    , m_manuallyStopped(false)
    , m_fallingAsleep(false)
{
#ifdef Q_OS_WINDOWS
    if (auto *const app = QCoreApplication::instance()) {
        app->installNativeEventFilter(this);
    }
#endif
    m_killTimer.setInterval(3000);
    m_killTimer.setSingleShot(true);
    setProcessChannelMode(QProcess::MergedChannels);
    connect(this, &SyncthingProcess::started, this, &SyncthingProcess::handleStarted);
    connect(this, static_cast<void (SyncthingProcess::*)(int exitCode, QProcess::ExitStatus exitStatus)>(&SyncthingProcess::finished), this,
        &SyncthingProcess::handleFinished, Qt::QueuedConnection);
    connect(&m_killTimer, &QTimer::timeout, this, &SyncthingProcess::confirmKill);
}

/*!
 * \brief Destroys the process.
 */
SyncthingProcess::~SyncthingProcess()
{
#ifdef LIB_SYNCTHING_CONNECTOR_BOOST_PROCESS
    // block until all callbacks have been processed
    const auto lock = std::lock_guard<std::mutex>(m_processMutex);
    m_process.reset();
#endif
}

/*!
 * \brief Splits the given arguments similar to how a shell would split it. So whitespaces are considered separators unless quotes are used.
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
 * \brief Emits errorOccurred() as if the specified \a error had occurred and sets the specified \a errorString.
 * \remarks May be called if a precondition for starting the process (that is checked outside of the SyncthingProcess
 *          class itself) has failed (instead of starting the process).
 */
void SyncthingProcess::reportError(QProcess::ProcessError error, const QString &errorString)
{
    setErrorString(errorString);
    emit errorOccurred(error);
}

/*!
 * \brief Stops the currently running process gracefully. If it has been stopped, starts the specified \a program with the specified \a arguments.
 * \sa See stopSyncthing() for details about \a currentConnection.
 */
void SyncthingProcess::restartSyncthing(const QString &program, const QStringList &arguments, SyncthingConnection *currentConnection)
{
    if (!isRunning()) {
        startSyncthing(program, arguments);
        return;
    }
    m_program = program;
    m_arguments = arguments;
    stopSyncthing(currentConnection);
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
 * \remarks
 * If graceful stopping is not possible (under Windows) the process is attempted to be stopped via the REST-API using \a currentConnection.
 * This is however only considered, if \a currentConnection is configured and a local connection.
 */
void SyncthingProcess::stopSyncthing(SyncthingConnection *currentConnection)
{
    m_manuallyStopped = true;
    m_killTimer.start();
#if defined(PLATFORM_UNIX) && !defined(LIB_SYNCTHING_CONNECTOR_ENFORCE_STOP_VIA_API)
    Q_UNUSED(currentConnection)
#else
    if (currentConnection && !currentConnection->syncthingUrl().isEmpty() && !currentConnection->apiKey().isEmpty() && currentConnection->isLocal()) {
        currentConnection->shutdown();
        return;
    }
#endif
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

/*!
 * \brief Keeps track of when the process has been started.
 */
void SyncthingProcess::handleStarted()
{
    m_activeSince = DateTime::gmtNow();
}

/*!
 * \brief Resets the state after the process has finished; restart it again if requested.
 */
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

/*!
 * \brief Kills the process if it is supposed to be restarted.
 */
void SyncthingProcess::killToRestart()
{
    if (!m_program.isEmpty()) {
        kill();
    }
}

#ifdef LIB_SYNCTHING_CONNECTOR_PROCESS_IO_DEV_BASED

// The functions below are for using Boost.Process (instead of QProcess) to be able to
// terminate the process better by using a group.
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

Data::SyncthingProcessInternalData::Lock::Lock(const std::weak_ptr<SyncthingProcessInternalData> &weak)
    : process(weak.lock())
    , lock(process ? decltype(lock)(process->mutex) : decltype(lock)())
{
}

Data::SyncthingProcessInternalData::Lock::operator bool() const
{
    return process && lock;
}
#endif

/*!
 * \brief Internally handles an error.
 */
void SyncthingProcess::handleError(int error, const QString &errorMessage, bool closed)
{
    setErrorString(errorMessage);
    emit errorOccurred(static_cast<QProcess::ProcessError>(error));
    if (closed) {
        setOpenMode(QIODevice::NotOpen);
    }
}

/*!
 * \brief Returns the process' state.
 * \remarks The process is only considered QProcess::NotRunning if all sub processes it has created have been
 *          terminated as well.
 */
QProcess::ProcessState SyncthingProcess::state() const
{
#ifdef LIB_SYNCTHING_CONNECTOR_BOOST_PROCESS
    return !m_process ? QProcess::NotRunning : m_process->state;
#else
    return QProcess::NotRunning;
#endif
}

/*!
 * \brief Starts a new process within a new process group (or job object under Windows) with the specified parameters.
 * \remarks
 * - The specified \a openMode must be QIODevice::ReadOnly.
 * - Only one process can be started at a time. Starting another process while the previous one or any child
 *   process of it is still running is an error.
 * - In case of an error the error strinng is set and errorOccurred() is emitted.
 * - If \a program is a relative path, it is tried to be located in PATH.
 */
void SyncthingProcess::start(const QString &program, const QStringList &arguments, QIODevice::OpenMode openMode)
{
    start(QStringList{ program }, arguments, openMode);
}

/*!
 * \brief Starts a new process within a new process group (or job object under Windows) with the specified parameters.
 *
 * This function is generally the same as the overload that just takes a single program (see its remarks for details).
 * The difference is that it allows one to specify fallback \a programs which will be tried to be located in PATH in
 * case the first program cannot be located in PATH.
 *
 * \remarks
 * If an absolute path is given, no lookup via PATH will happen. Instead, the absolute path is used as-is and further
 * fallbacks are not considered. This is even the case when the absolute path does not exist.
 *
 */
void SyncthingProcess::start(const QStringList &programs, const QStringList &arguments, OpenMode openMode)
{
#ifdef LIB_SYNCTHING_CONNECTOR_BOOST_PROCESS
    // get Boost.Process' code converter to provoke and handle a possible error when it is setting up its default locale via e.g. `std::locale("")`
    try {
        boost::process::v1::codecvt();
    } catch (const std::runtime_error &e) {
        setErrorString(QStringLiteral("unable to initialize codecvt/locale: ") % QString::fromLocal8Bit(e.what())
            % QStringLiteral("\nBe sure your locale setup is correct, see https://wiki.archlinux.org/title/Locale for details."));
        std::cerr << EscapeCodes::Phrases::Error << "Unable to initialize locale for launching process" << EscapeCodes::Phrases::End;
        std::cerr << EscapeCodes::Phrases::SubError << "Unable to initialize boost::process::v1::codecvt/std::locale: " << e.what()
                  << EscapeCodes::Phrases::End;
        std::cerr << EscapeCodes::Phrases::SubMessage
                  << "Be sure the locale is setup correctly in your environment (see https://wiki.archlinux.org/title/Locale)"
                  << EscapeCodes::Phrases::End;
        emit errorOccurred(QProcess::FailedToStart);
        return;
    }

    // ensure the dev is only opened  in read-only mode because writing is not implemented here
    if (openMode != QIODevice::ReadOnly) {
        setErrorString(QStringLiteral("only read-only openmode supported"));
        std::cerr << EscapeCodes::Phrases::Error << "Unable to launch process: only read-only openmode supported" << EscapeCodes::Phrases::End;
        emit errorOccurred(QProcess::FailedToStart);
        return;
    }

    // handle current/previous process
    auto processLock = std::unique_lock<std::mutex>(m_processMutex);
    if (m_process) {
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
    processLock.unlock(); // the critical section where m_process is reset or re-assigned is over
    emit stateChanged(m_process->state = QProcess::Starting);

    // define handler
    auto successHandler = boost::process::v1::extend::on_success = [this, maybeProcess = m_process->weak_from_this()](auto &executor) {
        std::cerr << EscapeCodes::Phrases::Info << "Launched process, PID: "
                  << executor
#ifdef PLATFORM_WINDOWS
                         .proc_info.dwProcessId
#else
                         .pid
#endif
                  << EscapeCodes::Phrases::End;
        const auto overallProcessLock = std::lock_guard<std::mutex>(m_processMutex);
        if (const auto lock = SyncthingProcessInternalData::Lock(maybeProcess)) {
            emit stateChanged(m_process->state = QProcess::Running);
            emit started();
        }
    };
    auto exitHandler = boost::process::v1::on_exit = [this, maybeProcess = m_process->weak_from_this()](int rc, const std::error_code &ec) {
        const auto overallProcessLock = std::lock_guard<std::mutex>(m_processMutex);
        const auto lock = SyncthingProcessInternalData::Lock(maybeProcess);
        if (!lock) {
            return;
        }
        handleLeftoverProcesses();
        emit stateChanged(m_process->state = QProcess::NotRunning);
        emit finished(rc, rc != 0 ? QProcess::CrashExit : QProcess::NormalExit);
        if (ec) {
            const auto msg = ec.message();
            std::cerr << EscapeCodes::Phrases::Info << "Launched process " << m_process->child.native_handle() << " exited with error: " << msg
                      << EscapeCodes::Phrases::End;
            QMetaObject::invokeMethod(this, "handleError", Qt::QueuedConnection, Q_ARG(int, QProcess::Crashed),
                Q_ARG(QString, QString::fromStdString(msg)), Q_ARG(bool, false));
        }
    };
    auto errorHandler = boost::process::v1::extend::on_error = [this, maybeProcess = m_process->weak_from_this()](auto &, const std::error_code &ec) {
        const auto overallProcessLock = std::lock_guard<std::mutex>(m_processMutex);
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
        const auto error = ec == std::errc::timed_out
#ifdef ETIME
                || ec == std::errc::stream_timeout
#endif
            ? QProcess::Timedout
            : QProcess::Crashed;
        const auto msg = ec.message();
        std::cerr << EscapeCodes::Phrases::Error << "Unable to launch process: " << msg << EscapeCodes::Phrases::End;
        QMetaObject::invokeMethod(
            this, "handleError", Qt::QueuedConnection, Q_ARG(int, error), Q_ARG(QString, QString::fromStdString(msg)), Q_ARG(bool, false));
    };

    // convert args
    m_process->arguments = arguments;
    auto args = std::vector<decltype(LIB_SYNCTHING_CONNECTOR_STRING_CONVERSION(arguments.front()))>();
    for (const auto &arg : arguments) {
        args.emplace_back(LIB_SYNCTHING_CONNECTOR_STRING_CONVERSION(arg));
    }

    // locate program
    auto hasProgram = false;
    auto path = boost::filesystem::path();
    for (const auto &program : programs) {
        m_process->program = program;
        path = boost::filesystem::path(LIB_SYNCTHING_CONNECTOR_STRING_CONVERSION(program));
        if (path.empty()) {
            continue;
        }
        hasProgram = true;
        if (path.is_relative()) {
            path = boost::process::v1::search_path(path);
        }
        if (!path.empty()) {
            break;
        }
    }
    if (!hasProgram) {
        std::cerr << EscapeCodes::Phrases::Error << "Unable to launch process: no executable specified" << EscapeCodes::Phrases::End;
        emit stateChanged(m_process->state = QProcess::NotRunning);
        handleError(QProcess::FailedToStart, QStringLiteral("no executable specified"), false);
        return;
    }

    // start the process within a new process group (or job object under Windows)
    try {
        if (path.empty()) {
            throw boost::process::v1::process_error(
                std::make_error_code(std::errc::no_such_file_or_directory), "unable to find the specified executable in the search paths");
        }
        switch (m_mode) {
        case QProcess::MergedChannels:
            m_process->child = boost::process::v1::child(LIB_SYNCTHING_CONNECTOR_PLATFORM_ARGS m_handler->ioc, m_process->group, path, args,
                (boost::process::v1::std_out & boost::process::v1::std_err) > m_process->pipe, std::move(successHandler), std::move(exitHandler),
                std::move(errorHandler));
            break;
        case QProcess::ForwardedChannels:
            m_process->child = boost::process::v1::child(LIB_SYNCTHING_CONNECTOR_PLATFORM_ARGS m_handler->ioc, m_process->group, path, args,
                std::move(successHandler), std::move(exitHandler), std::move(errorHandler));
            break;
        default:
            std::cerr << EscapeCodes::Phrases::Error << "Unable to launch process: specified channel mode not supported" << EscapeCodes::Phrases::End;
            emit stateChanged(m_process->state = QProcess::NotRunning);
            handleError(QProcess::FailedToStart, QStringLiteral("specified channel mode not supported"), false);
            return;
        }
    } catch (const boost::process::v1::process_error &e) {
        std::cerr << EscapeCodes::Phrases::Error << "Unable to launch process: " << e.what() << EscapeCodes::Phrases::End;
        emit stateChanged(m_process->state = QProcess::NotRunning);
        handleError(QProcess::FailedToStart, QString::fromUtf8(e.what()), false);
        return;
    }

    // start reading the process' output
    open(QIODevice::ReadOnly);
    bufferOutput();
#else
    Q_UNUSED(programs)
    Q_UNUSED(arguments)
    Q_UNUSED(openMode)
    emit stateChanged(QProcess::NotRunning);
    handleError(QProcess::FailedToStart, QStringLiteral("process launching is not supported"), false);
#endif
}

/*!
 * \brief Terminates the launched process and all child processes gracefully if possible.
 * \remarks Sends SIGTERM to the process group under POSIX and invokes TerminateJobObject() under Windows.
 */
void SyncthingProcess::terminate()
{
#ifdef LIB_SYNCTHING_CONNECTOR_BOOST_PROCESS
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
        if (const auto ec = boost::process::v1::detail::get_last_error(); ec != std::errc::no_such_process) {
            const auto msg = ec.message();
            std::cerr << EscapeCodes::Phrases::Error << "Unable to kill process group " << groupId << ": " << msg << EscapeCodes::Phrases::End;
            setErrorString(QString::fromStdString(msg));
            emit errorOccurred(QProcess::UnknownError);
        }
    }
#else
    // there seems no way to stop the process group gracefully under Windows so just kill it
    // note: Posting a WM_CLOSE message like QProcess would attempt doesn't work for Syncthing.
    kill();
#endif
#endif
}

/*!
 * \brief Terminates the launched process and all child processes forcefully.
 * \remarks Sends SIGKILL to the process group under POSIX and invokes TerminateJobObject() under Windows.
 */
void SyncthingProcess::kill()
{
#ifdef LIB_SYNCTHING_CONNECTOR_BOOST_PROCESS
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
        emit errorOccurred(QProcess::UnknownError);
    }
    // note: No need to emit finished() signal here, the on_exit handler will fire
    //       also in case of a forceful termination.
#endif
}

/*!
 * \brief Reads data from the pipe into the internal buffer.
 */
void SyncthingProcess::bufferOutput()
{
#ifdef LIB_SYNCTHING_CONNECTOR_BOOST_PROCESS
    m_process->pipe.async_read_some(boost::asio::buffer(m_process->buffer, m_process->bufferCapacity),
        [this, maybeProcess = m_process->weak_from_this()](const boost::system::error_code &ec, auto bytesRead) {
            const auto processLock = std::lock_guard<std::mutex>(m_processMutex);
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
                QMetaObject::invokeMethod(this, "handleError", Qt::QueuedConnection, Q_ARG(int, QProcess::ReadError),
                    Q_ARG(QString, QString::fromStdString(msg)), Q_ARG(bool, true));
            }
            if (!ec || bytesRead) {
                emit readyRead();
                m_process->readCondVar.notify_all();
            }
        });
#endif
}

/*!
 * \brief Terminates all processes in the group forcefully and waits until they're gone.
 */
void SyncthingProcess::handleLeftoverProcesses()
{
#ifdef LIB_SYNCTHING_CONNECTOR_BOOST_PROCESS
    if (!m_process->group.valid()) {
        return;
    }
    auto ec = std::error_code();
    m_process->group.terminate(ec);
    if (ec && ec != std::errc::no_such_process) {
        std::cerr << EscapeCodes::Phrases::Error << "Unable to kill leftover processes in group " << m_process->group.native_handle() << ": "
                  << ec.message() << EscapeCodes::Phrases::End;
    }
    if (!m_process->group.valid()) {
        return;
    }
    m_process->group.wait(ec); // wait until group has terminated: Is this ever required?
    if (ec && ec != std::errc::no_such_process) {
        std::cerr << EscapeCodes::Phrases::Error << "Unable to wait for leftover processes in group " << m_process->group.native_handle() << ": "
                  << ec.message() << EscapeCodes::Phrases::End;
    }
#endif
}

qint64 SyncthingProcess::bytesAvailable() const
{
#ifdef LIB_SYNCTHING_CONNECTOR_BOOST_PROCESS
    return (m_process ? static_cast<qint64>(m_process->bytesBuffered) : 0) + QIODevice::bytesAvailable();
#else
    return 0;
#endif
}

void SyncthingProcess::close()
{
#ifdef LIB_SYNCTHING_CONNECTOR_BOOST_PROCESS
    emit aboutToClose();
    if (m_process) {
        const auto lock = std::lock_guard<std::mutex>(m_process->mutex);
        m_process->pipe.async_close();
        kill();
    }
#endif
    setOpenMode(QIODevice::NotOpen);
}

/*!
 * \brief Returns the exit code of the last process which has been started.
 * \remarks Only valid if the process has already terminated.
 */
int SyncthingProcess::exitCode() const
{
#ifdef LIB_SYNCTHING_CONNECTOR_BOOST_PROCESS
    return m_process ? m_process->child.exit_code() : 0;
#else
    return 0;
#endif
}

/*!
 * \brief Waits until the current process and all its child processes have been terminated.
 * \param msecs Specifies the number of milliseconds to wait at most or a negative integer to wait infinitely.
 */
bool SyncthingProcess::waitForFinished(int msecs)
{
#ifdef LIB_SYNCTHING_CONNECTOR_BOOST_PROCESS
    if (!m_process || !m_process->group.valid()) {
        return false;
    }
    auto ec = std::error_code();
    if (msecs < 0) {
        m_process->group.wait(ec);
    } else {
        // disable warning about deprecated/unreliable function `wait_for`; it is good enough for now
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif
        m_process->group.wait_for(std::chrono::milliseconds(msecs), ec);
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
    }
    return !ec || ec == std::errc::no_such_process || ec == std::errc::no_child_process;
#else
    Q_UNUSED(msecs)
    return true;
#endif
}

bool SyncthingProcess::waitForReadyRead(int msecs)
{
#ifdef LIB_SYNCTHING_CONNECTOR_BOOST_PROCESS
    if (!m_process) {
        return false;
    }
    if (m_process->bytesBuffered) {
        return true;
    }
    QEventLoop().processEvents(
        QEventLoop::ExcludeUserInputEvents | QEventLoop::ExcludeSocketNotifiers); // ensure a possibly pending bufferOutput() invocation is processed
    auto lock = std::unique_lock<std::mutex>(m_process->readMutex);
    if (msecs < 0) {
        m_process->readCondVar.wait(lock);
    } else {
        m_process->readCondVar.wait_for(lock, std::chrono::milliseconds(msecs));
    }
    return m_process->bytesBuffered;
#else
    Q_UNUSED(msecs)
    return false;
#endif
}

/*!
 * \brief Returns the process ID of the last process which has been started.
 */
qint64 SyncthingProcess::processId() const
{
#ifdef LIB_SYNCTHING_CONNECTOR_BOOST_PROCESS
    return m_process ? static_cast<qint64>(m_process->child.id()) : static_cast<qint64>(-1);
#else
    return 0;
#endif
}

/*!
 * \brief Returns the path/executable of the last process which has been started.
 */
QString SyncthingProcess::program() const
{
#ifdef LIB_SYNCTHING_CONNECTOR_BOOST_PROCESS
    return m_process ? m_process->program : QString();
#else
    return QString();
#endif
}

/*!
 * \brief Returns the arguments the last process has been started with.
 */
QStringList SyncthingProcess::arguments() const
{
#ifdef LIB_SYNCTHING_CONNECTOR_BOOST_PROCESS
    return m_process ? m_process->arguments : QStringList();
#else
    return QStringList();
#endif
}

qint64 SyncthingProcess::readData(char *data, qint64 maxSize)
{
#ifdef LIB_SYNCTHING_CONNECTOR_BOOST_PROCESS
    if (!m_process) {
        return -1;
    }
    if (maxSize < 1) {
        return 0;
    }
    if (!m_process->bytesBuffered) {
        return 0; // do *not* invoke bufferOutput() here; an async read operation is already pending
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
        QMetaObject::invokeMethod(this, "bufferOutput", Qt::QueuedConnection);
        return static_cast<qint64>(bytesAvailable);
    }
#else
    Q_UNUSED(data)
    Q_UNUSED(maxSize)
    return -1;
#endif
}

qint64 SyncthingProcess::writeData(const char *data, qint64 len)
{
    Q_UNUSED(data)
    Q_UNUSED(len)
    return -1;
}
#endif

#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
bool SyncthingProcess::nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result)
#else
bool SyncthingProcess::nativeEventFilter(const QByteArray &eventType, void *message, long *result)
#endif
{
    Q_UNUSED(result)
#ifdef Q_OS_WINDOWS
    if (eventType == "windows_generic_MSG") {
        const auto *const msg = static_cast<MSG *>(message);
        switch (msg->message) {
        case WM_POWERBROADCAST:
            switch (msg->wParam) {
            case PBT_APMSUSPEND:
                m_fallingAsleep = true;
                break;
            case PBT_APMRESUMEAUTOMATIC:
                m_fallingAsleep = false;
                m_lastWakeUp = DateTime::gmtNow();
                break;
            default:;
            }
            break;
        default:;
        }
    }
#else
    Q_UNUSED(eventType)
    Q_UNUSED(message)
#endif
    return false;
}

} // namespace Data
