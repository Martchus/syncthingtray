#include "./syncthingprocess.h"

#include <QTimer>

namespace Data {

SyncthingProcess::SyncthingProcess(QObject *parent)
    : QProcess(parent)
{
    setProcessChannelMode(QProcess::MergedChannels);
    connect(this, static_cast<void (SyncthingProcess::*)(int exitCode, QProcess::ExitStatus exitStatus)>(&SyncthingProcess::finished), this,
        &SyncthingProcess::handleFinished);
}

void SyncthingProcess::restartSyncthing(const QString &cmd)
{
    if (state() == QProcess::Running) {
        m_cmd = cmd;
        // give Syncthing 5 seconds to terminate, otherwise kill it
        QTimer::singleShot(5000, this, &SyncthingProcess::killToRestart);
        terminate();
    } else {
        startSyncthing(cmd);
    }
}

void SyncthingProcess::startSyncthing(const QString &cmd)
{
    if (state() == QProcess::NotRunning) {
        if (cmd.isEmpty()) {
            start(QProcess::ReadOnly);
        } else {
            start(cmd, QProcess::ReadOnly);
        }
    }
}

void SyncthingProcess::stopSyncthing()
{
    if (state() == QProcess::Running) {
        // give Syncthing 5 seconds to terminate, otherwise kill it
        QTimer::singleShot(5000, this, &SyncthingProcess::kill);
        terminate();
    }
}

void SyncthingProcess::handleFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitCode)
    Q_UNUSED(exitStatus)
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
