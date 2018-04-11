#include "./syncthinglauncher.h"

#ifdef SYNCTHING_WIDGETS_USE_LIBSYNCTHING
#include "../../libsyncthing/interface.h"
#include <QtConcurrentRun>
#endif

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
#ifdef SYNCTHING_WIDGETS_USE_LIBSYNCTHING
    m_future = QtConcurrent::run(this, &SyncthingLauncher::runLibSyncthing, runtimeOptions);
#else
    VAR_UNUSED(runtimeOptions)
    emit outputAvailable("libsyncthing support not enabled");
    emit exited(-1, QProcess::CrashExit);
#endif
}

void SyncthingLauncher::terminate()
{
    if (m_process.isRunning()) {
        m_manuallyStopped = true;
        m_process.stopSyncthing();
    } else if (m_future.isRunning()) {
        m_manuallyStopped = true;
        m_future.cancel(); // FIXME: this will not work of course
    }
}

void SyncthingLauncher::kill()
{
    if (m_process.isRunning()) {
        m_manuallyStopped = true;
        m_process.stopSyncthing();
    } else if (m_future.isRunning()) {
        m_manuallyStopped = true;
        // FIXME
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

void SyncthingLauncher::runLibSyncthing(const LibSyncthing::RuntimeOptions &runtimeOptions)
{
    LibSyncthing::runSyncthing(runtimeOptions);
}

SyncthingLauncher &syncthingLauncher()
{
    static SyncthingLauncher launcher;
    return launcher;
}

} // namespace Data
