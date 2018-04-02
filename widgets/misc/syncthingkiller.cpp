#include "./syncthingkiller.h"

#include "../../connector/syncthingprocess.h"
#define SYNCTHINGTESTHELPER_FOR_CLI
#include "../../testhelper/helper.h"

#include <QCoreApplication>
#include <QMessageBox>

using namespace std;
using namespace Data;
using namespace TestUtilities;

namespace QtGui {

SyncthingKiller::SyncthingKiller(std::vector<SyncthingProcess *> &&processes)
    : m_processes(processes)
{
    for (auto *process : m_processes) {
        process->stopSyncthing();
        connect(process, &SyncthingProcess::confirmKill, this, &SyncthingKiller::confirmKill);
    }
}

void SyncthingKiller::waitForFinished()
{
    for (auto *process : m_processes) {
        if (!process->isRunning()) {
            continue;
        }
        if (!waitForSignalsOrFail(noop, 0, signalInfo(this, &SyncthingKiller::ignored),
                signalInfo(process, static_cast<void (SyncthingProcess::*)(int)>(&SyncthingProcess::finished)))) {
            return;
        }
    }
}

void SyncthingKiller::confirmKill() const
{
    auto *const process = static_cast<SyncthingProcess *>(sender());
    if (!process->isRunning()) {
        return;
    }

    const auto msg(tr("The process %1 (PID: %2) has been requested to terminate but hasn't reacted yet. "
                      "Kill the process?\n\n"
                      "This dialog closes automatically when the process finally terminates.")
                       .arg(process->program(), QString::number(process->processId())));
    auto *const msgBox = new QMessageBox(QMessageBox::Critical, QCoreApplication::applicationName(), msg);
    msgBox->setAttribute(Qt::WA_DeleteOnClose);
    msgBox->addButton(tr("Keep running"), QMessageBox::RejectRole);
    msgBox->addButton(tr("Kill process"), QMessageBox::AcceptRole);
    connect(process, static_cast<void (QProcess::*)(int)>(&SyncthingProcess::finished), msgBox, &QMessageBox::close);
    connect(msgBox, &QMessageBox::accepted, process, &SyncthingProcess::killSyncthing);
    // FIXME: can not really ignore, just keep the process running
    //connect(msgBox, &QMessageBox::rejected, this, &SyncthingKiller::ignored);
    msgBox->show();
}

} // namespace QtGui
