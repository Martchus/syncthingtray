#include "./syncthingprocess.h"

#include <QTimer>

using namespace ChronoUtilities;

namespace Data {

SyncthingProcess::SyncthingProcess(QObject *parent)
    : QProcess(parent)
    , m_manuallyStopped(false)
{
    m_killTimer.setInterval(3000);
    m_killTimer.setSingleShot(true);
    setProcessChannelMode(QProcess::MergedChannels);
    connect(this, &SyncthingProcess::started, this, &SyncthingProcess::handleStarted);
    connect(this, static_cast<void (SyncthingProcess::*)(int exitCode, QProcess::ExitStatus exitStatus)>(&SyncthingProcess::finished), this,
        &SyncthingProcess::handleFinished);
    connect(&m_killTimer, &QTimer::timeout, this, &SyncthingProcess::confirmKill);
}

void SyncthingProcess::restartSyncthing(const QString &cmd)
{
    if (!isRunning()) {
        startSyncthing(cmd);
        return;
    }
    m_cmd = cmd;
    m_manuallyStopped = true;
    m_killTimer.start();
    terminate();
}

void SyncthingProcess::startSyncthing(const QString &cmd)
{
    if (isRunning()) {
        return;
    }
    m_manuallyStopped = false;
    m_killTimer.stop();
    if (cmd.isEmpty()) {
        start(QProcess::ReadOnly);
    } else {
        start(cmd, QProcess::ReadOnly);
    }
}

void SyncthingProcess::stopSyncthing()
{
    if (!isRunning()) {
        return;
    }
    m_manuallyStopped = true;
    m_killTimer.start();
    terminate();
}

void SyncthingProcess::killSyncthing()
{
    if (!isRunning()) {
        return;
    }
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
    if (!m_cmd.isEmpty()) {
        startSyncthing(m_cmd);
        m_cmd.clear();
    }
}

void SyncthingProcess::killToRestart()
{
    if (!m_cmd.isEmpty()) {
        kill();
    }
}

SyncthingProcess &syncthingProcess()
{
    static SyncthingProcess process;
    return process;
}

} // namespace Data
