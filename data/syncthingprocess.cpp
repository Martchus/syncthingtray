#include "./syncthingprocess.h"

#include "../application/settings.h"

#include <QTimer>
#include <QStringBuilder>

namespace Data {

SyncthingProcess::SyncthingProcess(QObject *parent) :
    QProcess(parent),
    m_restarting(false)
{
    setProcessChannelMode(QProcess::MergedChannels);
    connect(this, static_cast<void(SyncthingProcess::*)(int exitCode, QProcess::ExitStatus exitStatus)>(&SyncthingProcess::finished), this, &SyncthingProcess::handleFinished);
}

void SyncthingProcess::restartSyncthing()
{
    if(state() == QProcess::Running) {
        m_restarting = true;
        // give Syncthing 5 seconds to terminate, otherwise kill it
        QTimer::singleShot(5000, this, SLOT(killToRestart()));
        terminate();
    } else {
        startSyncthing();
    }
}

void SyncthingProcess::startSyncthing()
{
    if(state() == QProcess::NotRunning) {
        start(Settings::syncthingPath() % QChar(' ') % Settings::syncthingArgs(), QProcess::ReadOnly);
    }
}

void SyncthingProcess::handleFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitCode)
    Q_UNUSED(exitStatus)
    if(m_restarting) {
        m_restarting = false;
        startSyncthing();
    }
}

void SyncthingProcess::killToRestart()
{
    if(m_restarting) {
        kill();
    }
}

SyncthingProcess &syncthingProcess()
{
    static SyncthingProcess process;
    return process;
}

} // namespace Data
