#include "./syncthingkiller.h"

#include <syncthingconnector/syncthingprocess.h>
#define SYNCTHINGTESTHELPER_FOR_CLI
#include "../../testhelper/helper.h"

#include <QCoreApplication>
#include <QMessageBox>

using namespace std;
using namespace Data;
using namespace CppUtilities;

namespace QtGui {

SyncthingKiller::SyncthingKiller(std::vector<ProcessWithConnection> &&processes)
    : m_processes(processes)
{
    for (const auto [process, connection] : m_processes) {
        process->stopSyncthing(connection);
        connect(process, &SyncthingProcess::confirmKill, this, &SyncthingKiller::confirmKill);
    }
}

void SyncthingKiller::waitForFinished()
{
    for (const auto [process, connection] : m_processes) {
        if (!process->isRunning()) {
            continue;
        }
        if (!waitForSignalsOrFail(noop, 0, signalInfo(this, &SyncthingKiller::ignored),
                signalInfo(process,
#if QT_VERSION < QT_VERSION_CHECK(5, 13, 0) || QT_DEPRECATED_SINCE(5, 13)
                    static_cast<void (SyncthingProcess::*)(int, QProcess::ExitStatus)>(
#endif
                        &SyncthingProcess::finished
#if QT_VERSION < QT_VERSION_CHECK(5, 13, 0) || QT_DEPRECATED_SINCE(5, 13)
                        )
#endif
                        ))) {
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
    connect(process,
#if !defined(LIB_SYNCTHING_CONNECTOR_BOOST_PROCESS) && (QT_VERSION < QT_VERSION_CHECK(5, 13, 0) || QT_DEPRECATED_SINCE(5, 13))
        static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(
#endif
            &SyncthingProcess::finished
#if !defined(LIB_SYNCTHING_CONNECTOR_BOOST_PROCESS) && (QT_VERSION < QT_VERSION_CHECK(5, 13, 0) || QT_DEPRECATED_SINCE(5, 13))
            )
#endif
            ,
        msgBox, &QMessageBox::close);
    connect(msgBox, &QMessageBox::accepted, process, &SyncthingProcess::killSyncthing);
    // FIXME: can not really ignore, just keep the process running
    //connect(msgBox, &QMessageBox::rejected, this, &SyncthingKiller::ignored);
    msgBox->show();
}

} // namespace QtGui
