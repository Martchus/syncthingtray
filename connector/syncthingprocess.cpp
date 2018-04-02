#include "./syncthingprocess.h"

#include <QTimer>

using namespace ChronoUtilities;

namespace Data {

SyncthingProcess::SyncthingProcess(QObject *parent)
    : QProcess(parent)
    , m_manuallyStopped(false)
{
    setProcessChannelMode(QProcess::MergedChannels);
    connect(this, &SyncthingProcess::started, this, &SyncthingProcess::handleStarted);
    connect(this, static_cast<void (SyncthingProcess::*)(int exitCode, QProcess::ExitStatus exitStatus)>(&SyncthingProcess::finished), this,
        &SyncthingProcess::handleFinished);
}

void SyncthingProcess::restartSyncthing(const QString &cmd)
{
    if (!isRunning()) {
        startSyncthing(cmd);
        return;
    }

    m_cmd = cmd;
    m_manuallyStopped = true;
    // give Syncthing 5 seconds to terminate, otherwise kill it
    QTimer::singleShot(5000, this, &SyncthingProcess::killToRestart);
    terminate();
}

void SyncthingProcess::startSyncthing(const QString &cmd)
{
    if (isRunning()) {
        return;
    }
    m_manuallyStopped = false;
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
    // give Syncthing 5 seconds to terminate, otherwise kill it
    QTimer::singleShot(5000, this, &SyncthingProcess::kill);
    terminate();
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
