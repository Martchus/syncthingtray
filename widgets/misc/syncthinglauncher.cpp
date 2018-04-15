#include "./syncthinglauncher.h"

#include <QtConcurrentRun>

#include <algorithm>
#include <limits>

using namespace std;
using namespace std::placeholders;
using namespace ChronoUtilities;

namespace Data {

SyncthingLauncher *SyncthingLauncher::s_mainInstance = nullptr;

SyncthingLauncher::SyncthingLauncher(QObject *parent)
    : QObject(parent)
{
    connect(&m_process, &SyncthingProcess::readyRead, this, &SyncthingLauncher::handleProcessReadyRead);
    connect(&m_process, static_cast<void (SyncthingProcess::*)(int exitCode, QProcess::ExitStatus exitStatus)>(&SyncthingProcess::finished), this,
        &SyncthingLauncher::handleProcessFinished);
    connect(&m_process, &SyncthingProcess::confirmKill, this, &SyncthingLauncher::confirmKill);
}

bool SyncthingLauncher::isLibSyncthingAvailable()
{
#ifdef SYNCTHING_WIDGETS_USE_LIBSYNCTHING
    return true;
#else
    return false;
#endif
}

void SyncthingLauncher::launch(const QString &cmd)
{
    if (isRunning()) {
        return;
    }
    m_manuallyStopped = false;
    m_process.startSyncthing(cmd);
}

void SyncthingLauncher::launch(const LibSyncthing::RuntimeOptions &runtimeOptions)
{
    if (isRunning()) {
        return;
    }
    m_manuallyStopped = false;
    m_future = QtConcurrent::run(this, &SyncthingLauncher::runLibSyncthing, runtimeOptions);
}

void SyncthingLauncher::terminate()
{
    if (m_process.isRunning()) {
        m_manuallyStopped = true;
        m_process.stopSyncthing();
    } else if (m_future.isRunning()) {
        m_manuallyStopped = true;
#ifdef SYNCTHING_WIDGETS_USE_LIBSYNCTHING
        LibSyncthing::stopSyncthing();
#endif
    }
}

void SyncthingLauncher::kill()
{
    if (m_process.isRunning()) {
        m_manuallyStopped = true;
        m_process.stopSyncthing();
    } else if (m_future.isRunning()) {
        m_manuallyStopped = true;
#ifdef SYNCTHING_WIDGETS_USE_LIBSYNCTHING
        // FIXME: any change to try harder?
        LibSyncthing::stopSyncthing();
#endif
    }
}

void SyncthingLauncher::handleProcessReadyRead()
{
    emit outputAvailable(m_process.readAll());
}

void SyncthingLauncher::handleProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    emit runningChanged(false);
    emit exited(exitCode, exitStatus);
}

void SyncthingLauncher::handleLoggingCallback(LibSyncthing::LogLevel level, const char *message, size_t messageSize)
{
#ifdef SYNCTHING_WIDGETS_USE_LIBSYNCTHING
    if (level < LibSyncthing::LogLevel::Info) {
        return;
    }
    emit outputAvailable(QByteArray(message, static_cast<int>(max<size_t>(numeric_limits<int>::max(), messageSize))));
#else
    VAR_UNUSED(level)
    VAR_UNUSED(message)
    VAR_UNUSED(messageSize)
#endif
}

void SyncthingLauncher::runLibSyncthing(const LibSyncthing::RuntimeOptions &runtimeOptions)
{
#ifdef SYNCTHING_WIDGETS_USE_LIBSYNCTHING
    LibSyncthing::setLoggingCallback(bind(&SyncthingLauncher::handleLoggingCallback, this, _1, _2, _3));
    const auto exitCode = LibSyncthing::runSyncthing(runtimeOptions);
    emit exited(static_cast<int>(exitCode), exitCode == 0 ? QProcess::NormalExit : QProcess::CrashExit);
#else
    VAR_UNUSED(runtimeOptions)
    emit outputAvailable("libsyncthing support not enabled");
    emit exited(-1, QProcess::CrashExit);
#endif
}

SyncthingLauncher &syncthingLauncher()
{
    static SyncthingLauncher launcher;
    return launcher;
}

} // namespace Data
