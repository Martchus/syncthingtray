#include "./syncthingprocess.h"

#include <QTimer>

using namespace ChronoUtilities;

namespace Data {

SyncthingProcess *SyncthingProcess::s_mainInstance = nullptr;

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

QStringList SyncthingProcess::splitArguments(const QString &arguments)
{
    enum { Any, Quote, Slash, Space } lastInput = Any;
    bool inQuotes = false;
    QStringList result;
    QString currentArg;
    for (const auto c : arguments) {
        switch (c.unicode()) {
        case '\"':
        case '\'':
            switch (lastInput) {
            case Slash:
                currentArg += c;
                lastInput = Any;
                break;
            default:
                inQuotes = !inQuotes;
                lastInput = Quote;
            }
            break;
        case '\\':
            switch (lastInput) {
            case Slash:
                currentArg += c;
                lastInput = Any;
                break;
            default:
                lastInput = Slash;
            }
            break;
        case ' ':
            switch (lastInput) {
            case Slash:
                currentArg += c;
                lastInput = Any;
                break;
            case Space:
                if (inQuotes) {
                    currentArg += c;
                    lastInput = Any;
                }
                break;
            default:
                if (inQuotes) {
                    currentArg += c;
                    lastInput = Any;
                } else {
                    result << currentArg;
                    currentArg.clear();
                    lastInput = Space;
                }
            }
            break;
        default:
            currentArg += c;
            lastInput = Any;
        }
    }
    if (!currentArg.isEmpty()) {
        result << currentArg;
    }
    return result;
}

void SyncthingProcess::restartSyncthing(const QString &program, const QStringList &arguments)
{
    if (!isRunning()) {
        startSyncthing(program, arguments);
        return;
    }
    m_program = program;
    m_arguments = arguments;
    m_manuallyStopped = true;
    m_killTimer.start();
    terminate();
}

void SyncthingProcess::startSyncthing(const QString &program, const QStringList &arguments)
{
    if (isRunning()) {
        return;
    }
    m_manuallyStopped = false;
    m_killTimer.stop();
    start(program, arguments, QProcess::ReadOnly);
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
    if (!m_program.isEmpty()) {
        startSyncthing(m_program, m_arguments);
        m_program.clear();
        m_arguments.clear();
    }
}

void SyncthingProcess::killToRestart()
{
    if (!m_program.isEmpty()) {
        kill();
    }
}

} // namespace Data
