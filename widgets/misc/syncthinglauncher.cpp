#include "./syncthinglauncher.h"

#include "../settings/settings.h"

#include <QtConcurrentRun>

#include <algorithm>
#include <functional>
#include <limits>
#include <string_view>

using namespace std;
using namespace std::placeholders;
using namespace CppUtilities;

namespace Data {

SyncthingLauncher *SyncthingLauncher::s_mainInstance = nullptr;

/*!
 * \class SyncthingLauncher
 * \brief The SyncthingLauncher class starts a Syncthing instance either as an external process or using a library version of Syncthing.
 * \remarks
 * - This is *not* strictly a singleton class. However, one instance is supposed to be the "main instance" (see SyncthingLauncher::setMainInstance()).
 * - A SyncthingLauncher instance can only launch one Syncthing instance at a time.
 * - Using Syncthing as library is still under development and must be explicitly enabled by setting the CMake variable USE_LIBSYNCTHING.
 */

/*!
 * \brief Constructs a new Syncthing launcher.
 */
SyncthingLauncher::SyncthingLauncher(QObject *parent)
    : QObject(parent)
    , m_guiListeningUrlSearch("Access the GUI via the following URL: ", "\n\r", std::string_view(),
          std::bind(&SyncthingLauncher::handleGuiListeningUrlFound, this, std::placeholders::_1, std::placeholders::_2))
#ifdef SYNCTHINGWIDGETS_USE_LIBSYNCTHING
    , m_libsyncthingLogLevel(LibSyncthing::LogLevel::Info)
#endif
    , m_manuallyStopped(true)
    , m_emittingOutput(false)
{
    connect(&m_process, &SyncthingProcess::readyRead, this, &SyncthingLauncher::handleProcessReadyRead, Qt::QueuedConnection);
    connect(&m_process, static_cast<void (SyncthingProcess::*)(int exitCode, QProcess::ExitStatus exitStatus)>(&SyncthingProcess::finished), this,
        &SyncthingLauncher::handleProcessFinished, Qt::QueuedConnection);
    connect(&m_process, &SyncthingProcess::stateChanged, this, &SyncthingLauncher::handleProcessStateChanged, Qt::QueuedConnection);
    connect(&m_process, &SyncthingProcess::errorOccurred, this, &SyncthingLauncher::errorOccurred, Qt::QueuedConnection);
    connect(&m_process, &SyncthingProcess::confirmKill, this, &SyncthingLauncher::confirmKill);
}

/*!
 * \brief Sets whether the output/log should be emitted via outputAvailable() signal.
 */
void SyncthingLauncher::setEmittingOutput(bool emittingOutput)
{
    if (m_emittingOutput == emittingOutput || !(m_emittingOutput = emittingOutput) || m_outputBuffer.isEmpty()) {
        return;
    }
    QByteArray data;
    m_outputBuffer.swap(data);
    emit outputAvailable(move(data));
}

/*!
 * \brief Returns whether the built-in Syncthing library is available.
 */
bool SyncthingLauncher::isLibSyncthingAvailable()
{
#ifdef SYNCTHINGWIDGETS_USE_LIBSYNCTHING
    return true;
#else
    return false;
#endif
}

/*!
 * \brief Returns the Syncthing version provided by libsyncthing or "Not built with libsyncthing support." if not built with libsyncthing support.
 */
QString SyncthingLauncher::libSyncthingVersionInfo()
{
#ifdef SYNCTHINGWIDGETS_USE_LIBSYNCTHING
    return QString::fromStdString(LibSyncthing::longSyncthingVersion());
#else
    return tr("Not built with libsyncthing support.");
#endif
}

/*!
 * \brief Launches a Syncthing instance using the specified \a arguments.
 *
 * To use the internal library, leave \a program empty. In this case \a arguments are ignored.
 * Otherwise \a program must be the path the external Syncthing executable.
 *
 * \remarks Does nothing if already running an instance.
 */
void SyncthingLauncher::launch(const QString &program, const QStringList &arguments)
{
    if (isRunning() || m_stopFuture.isRunning()) {
        return;
    }
    resetState();

    // start external process
    if (!program.isEmpty()) {
        m_process.startSyncthing(program, arguments);
        return;
    }

#ifdef SYNCTHINGWIDGETS_USE_LIBSYNCTHING
    // use libsyncthing
    m_startFuture = QtConcurrent::run(std::bind(&SyncthingLauncher::runLibSyncthing, this, LibSyncthing::RuntimeOptions{}));
#else
    showLibSyncthingNotSupported();
#endif
}

/*!
 * \brief Launches a Syncthing instance according to the specified \a launcherSettings.
 * \remarks Does nothing if already running an instance.
 */
void SyncthingLauncher::launch(const Settings::Launcher &launcherSettings)
{
    if (isRunning()) {
        return;
    }
    if (!launcherSettings.useLibSyncthing && launcherSettings.syncthingPath.isEmpty()) {
        emit errorOccurred(QProcess::FailedToStart);
        return;
    }
    if (launcherSettings.useLibSyncthing) {
#ifdef SYNCTHINGWIDGETS_USE_LIBSYNCTHING
        LibSyncthing::RuntimeOptions options;
        options.configDir = launcherSettings.libSyncthing.configDir.toStdString();
        options.dataDir = launcherSettings.libSyncthing.configDir.toStdString();
        setLibSyncthingLogLevel(launcherSettings.libSyncthing.logLevel);
        launch(options);
#else
        showLibSyncthingNotSupported();
#endif
    } else {
        launch(launcherSettings.syncthingPath, SyncthingProcess::splitArguments(launcherSettings.syncthingArgs));
    }
}

#ifdef SYNCTHINGWIDGETS_USE_LIBSYNCTHING
/*!
 * \brief Launches a Syncthing instance using the internal library with the specified \a runtimeOptions.
 * \remarks Does nothing if already running an instance.
 */
void SyncthingLauncher::launch(const LibSyncthing::RuntimeOptions &runtimeOptions)
{
    if (isRunning() || m_stopFuture.isRunning()) {
        return;
    }
    resetState();
    m_startFuture = QtConcurrent::run(std::bind(&SyncthingLauncher::runLibSyncthing, this, runtimeOptions));
}
#endif

void SyncthingLauncher::terminate(SyncthingConnection *relevantConnection)
{
    if (m_process.isRunning()) {
        m_manuallyStopped = true;
        m_process.stopSyncthing(relevantConnection);
    } else {
        tearDownLibSyncthing();
    }
}

void SyncthingLauncher::kill()
{
    if (m_process.isRunning()) {
        m_manuallyStopped = true;
        m_process.killSyncthing();
    } else {
        tearDownLibSyncthing();
    }
}

void SyncthingLauncher::tearDownLibSyncthing()
{
#ifdef SYNCTHINGWIDGETS_USE_LIBSYNCTHING
    if (!m_startFuture.isRunning() || m_stopFuture.isRunning()) {
        return;
    }
    m_manuallyStopped = true;
    m_stopFuture = QtConcurrent::run(std::bind(&SyncthingLauncher::stopLibSyncthing, this));
#endif
}

void SyncthingLauncher::handleProcessReadyRead()
{
    handleOutputAvailable(m_process.readAll());
}

void SyncthingLauncher::handleProcessStateChanged(QProcess::ProcessState newState)
{
    switch (newState) {
    case QProcess::NotRunning:
        emit runningChanged(false);
        break;
    case QProcess::Starting:
        emit runningChanged(true);
        break;
    default:;
    }
}

void SyncthingLauncher::handleProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    emit exited(exitCode, exitStatus);
}

void SyncthingLauncher::resetState()
{
    m_manuallyStopped = false;
    m_guiListeningUrlSearch.reset();
    if (!m_guiListeningUrl.isEmpty()) {
        m_guiListeningUrl.clear();
        emit guiUrlChanged(m_guiListeningUrl);
    }
}

#ifdef SYNCTHINGWIDGETS_USE_LIBSYNCTHING
static const char *const logLevelStrings[] = {
    "[DEBUG]   ",
    "[VERBOSE] ",
    "[INFO]    ",
    "[WARNING] ",
    "[FATAL]   ",
};

void SyncthingLauncher::handleLoggingCallback(LibSyncthing::LogLevel level, const char *message, size_t messageSize)
{
    if (level < m_libsyncthingLogLevel) {
        return;
    }
    QByteArray messageData;
    messageSize = min<size_t>(numeric_limits<int>::max() - 20, messageSize);
    messageData.reserve(static_cast<int>(messageSize) + 20);
    messageData.append(logLevelStrings[static_cast<int>(level)]);
    messageData.append(message, static_cast<int>(messageSize));
    messageData.append('\n');

    handleOutputAvailable(move(messageData));
}
#endif

void SyncthingLauncher::handleOutputAvailable(QByteArray &&data)
{
    m_guiListeningUrlSearch(data.data(), static_cast<std::size_t>(data.size()));
    if (isEmittingOutput()) {
        emit outputAvailable(data);
    } else {
        m_outputBuffer += data;
    }
}

void SyncthingLauncher::handleGuiListeningUrlFound(CppUtilities::BufferSearch &, std::string &&searchResult)
{
    m_guiListeningUrl.setUrl(QString::fromStdString(searchResult));
    emit guiUrlChanged(m_guiListeningUrl);
}

#ifdef SYNCTHINGWIDGETS_USE_LIBSYNCTHING
void SyncthingLauncher::runLibSyncthing(const LibSyncthing::RuntimeOptions &runtimeOptions)
{
    LibSyncthing::setLoggingCallback(bind(&SyncthingLauncher::handleLoggingCallback, this, _1, _2, _3));
    emit runningChanged(true);
    const auto exitCode = LibSyncthing::runSyncthing(runtimeOptions);
    emit exited(static_cast<int>(exitCode), exitCode == 0 ? QProcess::NormalExit : QProcess::CrashExit);
    emit runningChanged(false);
}

void SyncthingLauncher::stopLibSyncthing()
{
    LibSyncthing::stopSyncthing();
    // no need to emit exited/runningChanged here; that is already done in runLibSyncthing()
}

#else
void SyncthingLauncher::showLibSyncthingNotSupported()
{
    handleOutputAvailable(QByteArray("libsyncthing support not enabled"));
    emit exited(-1, QProcess::CrashExit);
}
#endif

} // namespace Data
