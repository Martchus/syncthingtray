#include "./syncthingprocess.h"

#include <QTimer>

using namespace CppUtilities;

namespace Data {

SyncthingProcess *SyncthingProcess::s_mainInstance = nullptr;

/*!
 * \class SyncthingProcess
 * \brief The SyncthingProcess class starts a Syncthing instance or additional tools as an external process.
 *
 * This class is actually not Syncthing-specific. It is just an extension of QProcess for some use-cases within
 * Syncthing Tray.
 */

/*!
 * \brief Constructs a new Syncthing process.
 */
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

/*!
 * \brief Splits the given arguments similar to how a shell would split it. So whitespaces are considered seperators unless quotes are used.
 */
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
                if (!currentArg.isEmpty()) {
                    currentArg += c;
                }
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
                    if (!currentArg.isEmpty()) {
                        result << currentArg;
                        currentArg.clear();
                    }
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

/*!
 * \brief Stops the currently running process. If it has been stopped, starts the specified \a program with the specified \a arguments.
 */
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

/*!
 * \brief Starts the specified \a program with the specified \a arguments.
 */
void SyncthingProcess::startSyncthing(const QString &program, const QStringList &arguments)
{
    if (isRunning()) {
        return;
    }
    m_manuallyStopped = false;
    m_killTimer.stop();
    start(program, arguments, QProcess::ReadOnly);
}

/*!
 * \brief Stops the currently running process gracefully. If it doesn't stop after 3 seconds, attempts to kill the process.
 */
void SyncthingProcess::stopSyncthing()
{
    if (!isRunning()) {
        return;
    }
    m_manuallyStopped = true;
    m_killTimer.start();
    terminate();
}

/*!
 * \brief Kills the currently running process.
 */
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
