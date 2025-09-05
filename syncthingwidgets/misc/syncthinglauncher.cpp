#include "./syncthinglauncher.h"

#include <syncthingconnector/syncthingconnection.h>
#include <syncthingconnector/utils.h>

#if defined(SYNCTHINGWIDGETS_GUI_QTWIDGETS)
#include "../settings/settings.h"
#endif

#include <c++utilities/io/ansiescapecodes.h>

#include <QMetaObject>
#include <QtConcurrentRun>

#ifdef SYNCTHINGCONNECTION_SUPPORT_METERED
#include <QNetworkInformation>
#endif

#include <algorithm>
#include <functional>
#include <iostream>
#include <limits>
#include <string_view>
#include <utility>

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
 * - Using Syncthing as library must be explicitly enabled by setting the CMake variable USE_LIBSYNCTHING.
 * - When using Syncthing as library only one instance of SyncthingLauncher can start Syncthing at a time; trying to start a 2nd Syncthing instance
 *   via another SyncthingLauncher will leads to a failure (but not to undefined behavior).
 * - You must not try to start Syncthing as library from multiple threads at the same time. This will lead to undefined behavior even when using different
 *   SyncthingLauncher instances.
 */

/*!
 * \brief Constructs a new Syncthing launcher.
 */
SyncthingLauncher::SyncthingLauncher(QObject *parent)
    : QObject(parent)
#if defined(SYNCTHINGWIDGETS_GUI_QTWIDGETS)
    , m_lastLauncherSettings(nullptr)
#endif
    , m_relevantConnection(nullptr)
    , m_guiListeningUrlSearch("Access the GUI via the following URL: ", " \n\r", std::string_view(), BufferSearch::CallbackType())
    , m_exitSearch("Syncthing exited", "\n\r", std::string_view(), BufferSearch::CallbackType())
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
#ifdef SYNCTHINGWIDGETS_USE_LIBSYNCTHING
    connect(&m_startWatcher, &QFutureWatcher<std::int64_t>::finished, this, &SyncthingLauncher::handleLibSyncthingFinished);
#endif

    // initialize handling of metered connections
#ifdef SYNCTHINGCONNECTION_SUPPORT_METERED
    if (const auto [networkInformation, isInitiallyMetered] = loadNetworkInformationBackendForMetered(true); networkInformation) {
        connect(networkInformation, &QNetworkInformation::isMeteredChanged, this, [this](bool isMetered) { setNetworkConnectionMetered(isMetered); });
        setNetworkConnectionMetered(isInitiallyMetered);
    }
#endif
}

/*!
 * \brief Ensures the built-in Syncthing instance is stopped if it was started by this SyncthingLauncher instance.
 */
SyncthingLauncher::~SyncthingLauncher()
{
#ifdef SYNCTHINGWIDGETS_USE_LIBSYNCTHING
    if (!m_startFuture.isCanceled()) {
        stopLibSyncthing();
        m_startFuture.waitForFinished();
    }
#endif
}

#ifdef SYNCTHINGWIDGETS_USE_LIBSYNCTHING
/*!
 * \brief Sets whether Syncthing is supposed to run or not.
 * \remarks
 * - This function will takes runtime conditions such as isStoppingOnMeteredConnection() into account.
 * - This function so far only supports launching via the built-in Syncthing library.
 */
void SyncthingLauncher::setRunning(bool running, LibSyncthing::RuntimeOptions &&runtimeOptions)
{
    // check runtime conditions
    auto shouldBeRunning = running;
    if (isStoppingOnMeteredConnection() && isNetworkConnectionMetered().value_or(false)) {
        m_stoppedMetered = running;
        shouldBeRunning = false;
    }
    // start/stop Syncthing depending on \a running
    if (shouldBeRunning) {
        launch(runtimeOptions);
    } else {
        tearDownLibSyncthing();
    }
    // save runtime options so Syncthing can resume in case runtime conditions allow it
#if defined(SYNCTHINGWIDGETS_GUI_QTWIDGETS)
    m_lastLauncherSettings = nullptr;
#endif
    m_lastRuntimeOptions = std::move(runtimeOptions);
    // emit signal in any case (even if there's no change) so runningStatus() is re-evaluated
    emit runningChanged(shouldBeRunning);
}
#endif

/*!
 * \brief Returns a short message about whether Syncthing is running.
 */
QString SyncthingLauncher::runningStatus() const
{
    if (isRunning()) {
        return tr("Syncthing is running");
    } else if (m_stoppedMetered) {
        return tr("Syncthing is temporarily stopped due to metered connection");
    } else if (m_lastExitStatus.has_value()) {
        return tr("Syncthing exited with status %1").arg(m_lastExitStatus.value().code);
    } else {
        return tr("Syncthing is not running");
    }
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
 * \brief Returns a short status message about whether the network connection is metered.
 */
QString SyncthingLauncher::meteredStatus() const
{
    if (m_metered.has_value()) {
        return m_metered.value() ? tr("Network connection is metered") : tr("Network connection is not metered");
    } else {
        return tr("State of network connection cannot be determined");
    }
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
            } else if (!metered.value_or(true) && m_stoppedMetered) {
#if defined(SYNCTHINGWIDGETS_GUI_QTWIDGETS)
                if (m_lastLauncherSettings) {
                    launch(*m_lastLauncherSettings);
                }
#endif
#if defined(SYNCTHINGWIDGETS_GUI_QTWIDGETS) && defined(SYNCTHINGWIDGETS_USE_LIBSYNCTHING)
                else
#endif
#if defined(SYNCTHINGWIDGETS_USE_LIBSYNCTHING)
                    if (m_lastRuntimeOptions) {
                    launch(*m_lastRuntimeOptions);
                }
#endif
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
    if ((stopOnMeteredConnection != m_stopOnMeteredConnection) && (m_stopOnMeteredConnection = stopOnMeteredConnection)
        && m_metered.value_or(false)) {
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
    runLibSyncthing(LibSyncthing::RuntimeOptions{});
#else
    showLibSyncthingNotSupported();
#endif
}

#if defined(SYNCTHINGWIDGETS_GUI_QTWIDGETS)
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
#ifdef SYNCTHINGWIDGETS_USE_LIBSYNCTHING
    m_lastRuntimeOptions.reset();
#endif
}
#endif

#ifdef SYNCTHINGWIDGETS_USE_LIBSYNCTHING
/*!
 * \brief Returns the specified \a logLevel for the built-in Syncthing instance as string.
 */
QString SyncthingLauncher::libSyncthingLogLevelString(LibSyncthing::LogLevel logLevel)
{
    switch (logLevel) {
    case LibSyncthing::LogLevel::Debug:
        return QStringLiteral("debug");
    case LibSyncthing::LogLevel::Warning:
        return QStringLiteral("warning");
    case LibSyncthing::LogLevel::Error:
        return QStringLiteral("error");
    default:
        return QStringLiteral("info");
    }
}

/*!
 * \brief Sets the log level for the built-in Syncthing instance from the specified string.
 * \remarks Assigns \a fallbackLogLevel if \a logLevel is not valid.
 */
void SyncthingLauncher::setLibSyncthingLogLevel(const QString &logLevel, LibSyncthing::LogLevel fallbackLogLevel)
{
    if (logLevel.compare(QLatin1String("debug"), Qt::CaseInsensitive) == 0) {
        m_libsyncthingLogLevel = LibSyncthing::LogLevel::Debug;
    } else if (logLevel.compare(QLatin1String("info"), Qt::CaseInsensitive) == 0) {
        m_libsyncthingLogLevel = LibSyncthing::LogLevel::Info;
    } else if (logLevel.compare(QLatin1String("warning"), Qt::CaseInsensitive) == 0) {
        m_libsyncthingLogLevel = LibSyncthing::LogLevel::Warning;
    } else if (logLevel.compare(QLatin1String("error"), Qt::CaseInsensitive) == 0) {
        m_libsyncthingLogLevel = LibSyncthing::LogLevel::Error;
    } else {
        m_libsyncthingLogLevel = fallbackLogLevel;
    }
}

/*!
 * \brief Launches a Syncthing instance using the internal library with the specified \a runtimeOptions.
 * \remarks
 * - Does nothing if already running an instance.
 * - In contrast to other overloads and setRunning() this function does *not* keep track of the last launcher
 *   settings or runtime options. Hence Syncthing will not be able to automatically resume in case runtime options
 *   allow it.
 */
void SyncthingLauncher::launch(const LibSyncthing::RuntimeOptions &runtimeOptions)
{
    if (isRunning() || m_stopFuture.isRunning()) {
        return;
    }
    resetState();
    runLibSyncthing(runtimeOptions);
}
#endif

void SyncthingLauncher::terminate(SyncthingConnection *relevantConnection)
{
    if (m_process.isRunning()) {
        setManuallyStopped(true);
        m_process.stopSyncthing(relevantConnection);
    } else {
        tearDownLibSyncthing();
    }
}

void SyncthingLauncher::kill()
{
    if (m_process.isRunning()) {
        setManuallyStopped(true);
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
    setManuallyStopped(true);
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
    // no need to emit exited/runningChanged here; that is already done after
    // m_startFuture has finished
#endif
}

void SyncthingLauncher::handleProcessReadyRead()
{
    const auto data = m_process.readAll();
    if (m_logFile.isOpen()) {
        m_logFile.write(data);
    }
    handleOutputAvailable(std::numeric_limits<int>::max(), data);
}

void SyncthingLauncher::handleProcessStateChanged(QProcess::ProcessState newState)
{
    switch (newState) {
    case QProcess::NotRunning:
        emit runningChanged(false);
        emit startingChanged();
        break;
    case QProcess::Starting:
        emit runningChanged(true);
        emit startingChanged();
        break;
    default:;
    }
}

void SyncthingLauncher::handleProcessFinished(int code, QProcess::ExitStatus status)
{
    const auto &exitStatus = m_lastExitStatus.emplace(code, status);
    emit exited(exitStatus.code, exitStatus.status);
    if (m_logFile.isOpen()) {
        m_logFile.flush();
    }
}

void SyncthingLauncher::resetState()
{
    m_manuallyStopped = false;
    m_stoppedMetered = false;
    delete m_relevantConnection;
    m_relevantConnection = nullptr;
    m_guiListeningUrlSearch.reset();
    m_exitSearch.reset();
    m_lastExitStatus.reset();
    if (!m_guiListeningUrl.isEmpty()) {
        m_guiListeningUrl.clear();
        emit guiUrlChanged(m_guiListeningUrl);
    }
}

#ifdef SYNCTHINGWIDGETS_USE_LIBSYNCTHING
constexpr std::string_view logLevelPrefix(LibSyncthing::LogLevel logLevel)
{
    if (logLevel <= LibSyncthing::LogLevel::Debug) {
        return "[DBG] ";
    } else if (logLevel <= LibSyncthing::LogLevel::Info) {
        return "[INF] ";
    } else if (logLevel <= LibSyncthing::LogLevel::Warning) {
        return "[WRN] ";
    } else {
        return "[ERR] ";
    }
}

void SyncthingLauncher::handleLoggingCallback(LibSyncthing::LogLevel level, const char *message, size_t messageSize)
{
    auto messageData = QByteArray();
    messageSize = min<size_t>(numeric_limits<int>::max() - 20, messageSize);
    messageData.reserve(static_cast<int>(messageSize) + 20);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    messageData.append(logLevelPrefix(level));
#else
    const auto prefix = logLevelPrefix(level);
    messageData.append(prefix.data(), static_cast<qsizetype>(prefix.size()));
#endif
    messageData.append(message, static_cast<int>(messageSize));
    messageData.append('\n');
    if (level >= m_libsyncthingLogLevel && m_logFile.isOpen()) {
        m_logFile.write(messageData);
    }
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    QMetaObject::invokeMethod(this, &SyncthingLauncher::handleOutputAvailable, Qt::QueuedConnection, static_cast<int>(level), std::move(messageData));
#else
    QMetaObject::invokeMethod(
        this, "handleOutputAvailable", Qt::QueuedConnection, Q_ARG(int, static_cast<int>(level)), Q_ARG(QByteArray, messageData));
#endif
}
#endif

void SyncthingLauncher::handleOutputAvailable(int logLevel, const QByteArray &data)
{
    const auto *const exitOffset = m_exitSearch.process(data.data(), static_cast<std::size_t>(data.size()));
    const auto *const guiAddressOffset = m_guiListeningUrlSearch.process(data.data(), static_cast<std::size_t>(data.size()));
    if (exitOffset) {
        auto res = std::string_view(m_exitSearch.result());
        while (res.starts_with(' ') || res.starts_with(':')) {
            res.remove_prefix(1);
        }
        std::cerr << EscapeCodes::Phrases::Info << "Syncthing exited: " << res << EscapeCodes::Phrases::End;
        emit exitLogged(res);
        m_exitSearch.reset();
    }
    if (guiAddressOffset > exitOffset) {
        m_guiListeningUrl.setUrl(QString::fromStdString(m_guiListeningUrlSearch.result()));
        std::cerr << EscapeCodes::Phrases::Info << "Syncthing GUI available: " << m_guiListeningUrlSearch.result() << EscapeCodes::Phrases::End;
        m_guiListeningUrlSearch.reset();
        emit guiUrlChanged(m_guiListeningUrl);
        emit startingChanged();
    } else if (exitOffset) {
        m_guiListeningUrl.clear();
        emit guiUrlChanged(m_guiListeningUrl);
    }
#ifdef SYNCTHINGWIDGETS_USE_LIBSYNCTHING
    if (logLevel < static_cast<int>(m_libsyncthingLogLevel)) {
        return;
    }
#else
    Q_UNUSED(logLevel)
#endif
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
#if defined(SYNCTHINGWIDGETS_GUI_QTWIDGETS)
    if (m_lastLauncherSettings && !m_relevantConnection) {
        m_relevantConnection = m_lastLauncherSettings->connectionForLauncher(this);
    }
#endif
    terminate(m_relevantConnection);
    m_stoppedMetered = true;
}

#ifdef SYNCTHINGWIDGETS_USE_LIBSYNCTHING
/*!
 * \brief Starts the built-in Syncthing instance.
 * \remarks Returns immediately as Syncthing is started in a different thread.
 */
void SyncthingLauncher::runLibSyncthing(const LibSyncthing::RuntimeOptions &runtimeOptions)
{
    if (LibSyncthing::hasLoggingCallback()) {
        showLibSyncthingNotSupported(QByteArrayLiteral("libsyncthing has already been started"));
        return;
    }
    LibSyncthing::setLoggingCallback(std::bind(&SyncthingLauncher::handleLoggingCallback, this, _1, _2, _3));
    emit runningChanged(true);
    emit startingChanged();
    m_startWatcher.setFuture(m_startFuture = QtConcurrent::run(std::bind(&LibSyncthing::runSyncthing, runtimeOptions)));
}

void SyncthingLauncher::handleLibSyncthingFinished()
{
    const auto exitCode = m_startFuture.result();
    const auto &exitStatus = m_lastExitStatus.emplace(static_cast<int>(exitCode), exitCode == 0 ? QProcess::NormalExit : QProcess::CrashExit);
    LibSyncthing::setLoggingCallback(LibSyncthing::LoggingCallback());
    m_guiListeningUrl.clear();
    emit guiUrlChanged(m_guiListeningUrl);
    emit exited(exitStatus.code, exitStatus.status);
    emit runningChanged(false);
    emit startingChanged();
    if (m_logFile.isOpen()) {
        m_logFile.flush();
    }
}
#endif

/*!
 * \brief Shows that launching Syncthing is not supported by emitting a non-zero exit status and logging \a reason.
 */
void SyncthingLauncher::showLibSyncthingNotSupported(QByteArray &&reason)
{
    handleOutputAvailable(std::numeric_limits<int>::max(), std::move(reason));
    const auto &exitStatus = m_lastExitStatus.emplace(-1, QProcess::CrashExit);
    emit exited(exitStatus.code, exitStatus.status);
}

QVariantMap SyncthingLauncher::overallStatus() const
{
    const auto isMetered = isNetworkConnectionMetered();
    return QVariantMap{
        { QStringLiteral("isRunning"), isRunning() },
        { QStringLiteral("isStarting"), isStarting() },
        { QStringLiteral("isManuallyStopped"), isManuallyStopped() },
        { QStringLiteral("guiUrl"), guiUrl() },
        { QStringLiteral("errorString"), errorString() },
        { QStringLiteral("runningStatus"), runningStatus() },
        { QStringLiteral("isMetered"), isMetered.has_value() ? QVariant(isMetered.value()) : QVariant() },
    };
}

} // namespace Data
