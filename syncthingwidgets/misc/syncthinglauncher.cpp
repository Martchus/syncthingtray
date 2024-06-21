#include "./syncthinglauncher.h"

#include <syncthingconnector/syncthingconnection.h>
#include <syncthingconnector/utils.h>

#include "../settings/settings.h"

#include <c++utilities/io/ansiescapecodes.h>

#include <QtConcurrentRun>

#ifdef SYNCTHINGCONNECTION_SUPPORT_METERED
#include <QNetworkInformation>
#endif

#include <algorithm>
#include <functional>
#include <iostream>
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
    , m_lastLauncherSettings(nullptr)
    , m_relevantConnection(nullptr)
    , m_guiListeningUrlSearch("Access the GUI via the following URL: ", "\n\r", std::string_view(), BufferSearch::CallbackType())
    , m_exitSearch("Syncthing exited: ", "\n\r", std::string_view(), BufferSearch::CallbackType())
#ifdef SYNCTHINGWIDGETS_USE_LIBSYNCTHING
    , m_libsyncthingLogLevel(LibSyncthing::LogLevel::Info)
#endif
    , m_manuallyStopped(true)
    , m_stoppedMetered(false)
    , m_emittingOutput(false)
    , m_useLibSyncthing(false)
    , m_stopOnMeteredConnection(false)
{
    connect(&m_process, &SyncthingProcess::readyRead, this, &SyncthingLauncher::handleProcessReadyRead, Qt::QueuedConnection);
    connect(&m_process, static_cast<void (SyncthingProcess::*)(int exitCode, QProcess::ExitStatus exitStatus)>(&SyncthingProcess::finished), this,
        &SyncthingLauncher::handleProcessFinished, Qt::QueuedConnection);
    connect(&m_process, &SyncthingProcess::stateChanged, this, &SyncthingLauncher::handleProcessStateChanged, Qt::QueuedConnection);
    connect(&m_process, &SyncthingProcess::errorOccurred, this, &SyncthingLauncher::errorOccurred, Qt::QueuedConnection);
    connect(&m_process, &SyncthingProcess::confirmKill, this, &SyncthingLauncher::confirmKill);

    // initialize handling of metered connections
#ifdef SYNCTHINGCONNECTION_SUPPORT_METERED
    if (const auto *const networkInformation = loadNetworkInformationBackendForMetered()) {
        connect(networkInformation, &QNetworkInformation::isMeteredChanged, this, [this](bool isMetered) { setNetworkConnectionMetered(isMetered); });
        setNetworkConnectionMetered(networkInformation->isMetered());
    }
#endif
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
    emit outputAvailable(std::move(data));
}

/*!
 * \brief Sets whether the current network connection is metered and stops/starts Syncthing accordingly as needed.
 * \remarks
 * - This is detected and monitored automatically. A manually set value will be overridden again on the next change.
 * - One may set this manually for testing purposes or in case the automatic detection is not supported (then
 *   isNetworkConnectionMetered() returns a std::optional<bool> without value).
 */
void SyncthingLauncher::setNetworkConnectionMetered(std::optional<bool> metered)
{
    if (metered != m_metered) {
        m_metered = metered;
        if (m_stopOnMeteredConnection) {
            if (metered.value_or(false)) {
                terminateDueToMeteredConnection();
            } else if (!metered.value_or(true) && m_stoppedMetered && m_lastLauncherSettings) {
                launch(*m_lastLauncherSettings);
            }
        }
        emit networkConnectionMeteredChanged(metered);
    }
}

/*!
 * \brief Sets whether Syncthing should automatically be stopped as long as the network connection is metered.
 */
void SyncthingLauncher::setStoppingOnMeteredConnection(bool stopOnMeteredConnection)
{
    if ((stopOnMeteredConnection != m_stopOnMeteredConnection) && (m_stopOnMeteredConnection = stopOnMeteredConnection) && m_metered) {
        terminateDueToMeteredConnection();
    }
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
    return QString::fromStdString(LibSyncthing::longSyncthingVersion()).replace(QStringLiteral(" 1970-01-01 00:00:00 UTC"), QString());
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
        m_process.reportError(QProcess::FailedToStart, QStringLiteral("executable path is empty"));
        return;
    }
    if (launcherSettings.useLibSyncthing) {
#ifdef SYNCTHINGWIDGETS_USE_LIBSYNCTHING
        const auto &libSyncthingSettings = launcherSettings.libSyncthing;
        auto options = LibSyncthing::RuntimeOptions();
        options.configDir = libSyncthingSettings.configDir.toStdString();
        options.dataDir = libSyncthingSettings.dataDir.isEmpty() ? options.configDir : libSyncthingSettings.dataDir.toStdString();
        if (libSyncthingSettings.expandPaths) {
            options.flags = options.flags | LibSyncthing::RuntimeFlags::ExpandPathsFromEnv;
        }
        setLibSyncthingLogLevel(libSyncthingSettings.logLevel);
        launch(options);
#else
        showLibSyncthingNotSupported();
#endif
    } else {
        launch(launcherSettings.syncthingPath, SyncthingProcess::splitArguments(launcherSettings.syncthingArgs));
    }
    m_stopOnMeteredConnection = launcherSettings.stopOnMeteredConnection;
    m_lastLauncherSettings = &launcherSettings;
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

/*!
 * \brief Stops the built-in Syncthing instance.
 * \remarks
 * - Does nothing if Syncthing is starting or stopping or has already been stopped or never been started.
 * - Returns immediately. The stopping is performed asynchronously.
 */
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

/*!
 * \brief Stops the built-in Syncthing instance.
 * \remarks
 * - Does nothing if Syncthing has already been stopped or never been started.
 * - Blocks the current thread until Syncthing has been stopped.
 */
void SyncthingLauncher::stopLibSyncthing()
{
#ifdef SYNCTHINGWIDGETS_USE_LIBSYNCTHING
    LibSyncthing::stopSyncthing();
    // no need to emit exited/runningChanged here; that is already done in runLibSyncthing()
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
    m_stoppedMetered = false;
    delete m_relevantConnection;
    m_relevantConnection = nullptr;
    m_guiListeningUrlSearch.reset();
    m_exitSearch.reset();
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

    handleOutputAvailable(std::move(messageData));
}
#endif

void SyncthingLauncher::handleOutputAvailable(QByteArray &&data)
{
    const auto *const exitOffset = m_exitSearch.process(data.data(), static_cast<std::size_t>(data.size()));
    const auto *const guiAddressOffset = m_guiListeningUrlSearch.process(data.data(), static_cast<std::size_t>(data.size()));
    if (exitOffset) {
        std::cerr << EscapeCodes::Phrases::Info << "Syncthing exited: " << m_exitSearch.result() << EscapeCodes::Phrases::End;
        emit exitLogged(std::move(m_exitSearch.result()));
        m_exitSearch.reset();
    }
    if (guiAddressOffset > exitOffset) {
        m_guiListeningUrl.setUrl(QString::fromStdString(m_guiListeningUrlSearch.result()));
        std::cerr << EscapeCodes::Phrases::Info << "Syncthing GUI available: " << m_guiListeningUrlSearch.result() << EscapeCodes::Phrases::End;
        m_guiListeningUrlSearch.reset();
        emit guiUrlChanged(m_guiListeningUrl);
    } else if (exitOffset) {
        m_guiListeningUrl.clear();
        emit guiUrlChanged(m_guiListeningUrl);
    }
    if (isEmittingOutput()) {
        emit outputAvailable(data);
    } else {
        m_outputBuffer += data;
    }
}

void SyncthingLauncher::terminateDueToMeteredConnection()
{
    if (!isRunning()) {
        // do not set m_stoppedMetered (and basically don't do anything) if not running anyway; otherwise we'd
        // always start Syncthing once the connection is not metered anymore (even if Syncthing has not even been
        // running before)
        return;
    }
    if (m_lastLauncherSettings && !m_relevantConnection) {
        m_relevantConnection = m_lastLauncherSettings->connectionForLauncher(this);
    }
    terminate(m_relevantConnection);
    m_stoppedMetered = true;
}

#ifdef SYNCTHINGWIDGETS_USE_LIBSYNCTHING
void SyncthingLauncher::runLibSyncthing(const LibSyncthing::RuntimeOptions &runtimeOptions)
{
    LibSyncthing::setLoggingCallback(bind(&SyncthingLauncher::handleLoggingCallback, this, _1, _2, _3));
    emit runningChanged(true);
    const auto exitCode = LibSyncthing::runSyncthing(runtimeOptions);
    m_guiListeningUrl.clear();
    emit guiUrlChanged(m_guiListeningUrl);
    emit exited(static_cast<int>(exitCode), exitCode == 0 ? QProcess::NormalExit : QProcess::CrashExit);
    emit runningChanged(false);
}
#else
void SyncthingLauncher::showLibSyncthingNotSupported()
{
    handleOutputAvailable(QByteArrayLiteral("libsyncthing support not enabled"));
    emit exited(-1, QProcess::CrashExit);
}
#endif

} // namespace Data
