#ifndef DATA_SYNCTHINGPROCESS_H
#define DATA_SYNCTHINGPROCESS_H

#include "./global.h"

#include <c++utilities/chrono/datetime.h>

#include <QProcess>
#include <QTimer>

namespace Data {

class LIB_SYNCTHING_CONNECTOR_EXPORT SyncthingProcess : public QProcess {
    Q_OBJECT
    Q_PROPERTY(bool running READ isRunning)
    Q_PROPERTY(CppUtilities::DateTime activeSince READ activeSince)
    Q_PROPERTY(bool manuallyStopped READ isManuallyStopped)

public:
    explicit SyncthingProcess(QObject *parent = nullptr);
    bool isRunning() const;
    CppUtilities::DateTime activeSince() const;
    bool isActiveFor(unsigned int atLeastSeconds) const;
    bool isManuallyStopped() const;
    static SyncthingProcess *mainInstance();
    static void setMainInstance(SyncthingProcess *mainInstance);
    static QStringList splitArguments(const QString &arguments);

Q_SIGNALS:
    void confirmKill();

public Q_SLOTS:
    void restartSyncthing(const QString &program, const QStringList &arguments);
    void startSyncthing(const QString &program, const QStringList &arguments);
    void stopSyncthing();
    void killSyncthing();

private Q_SLOTS:
    void handleStarted();
    void handleFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void killToRestart();

private:
    QString m_program;
    QStringList m_arguments;
    CppUtilities::DateTime m_activeSince;
    QTimer m_killTimer;
    bool m_manuallyStopped;
    static SyncthingProcess *s_mainInstance;
};

/// \brief Returns whether the process is running.
inline bool SyncthingProcess::isRunning() const
{
    return state() != QProcess::NotRunning;
}

/// \brief Returns the last time when QProcess::started() has been emitted.
inline CppUtilities::DateTime SyncthingProcess::activeSince() const
{
    return m_activeSince;
}

/// \brief Checks whether the process already runs for the specified number of seconds.
inline bool SyncthingProcess::isActiveFor(unsigned int atLeastSeconds) const
{
    return !m_activeSince.isNull() && (CppUtilities::DateTime::gmtNow() - m_activeSince).totalSeconds() > atLeastSeconds;
}

/// \brief Returns whether the process has been manually stopped via SyncthingProcess::stopSyncthing(), SyncthingProcess::killSyncthing()
/// or SyncthingProcess::restartSyncthing().
/// \remarks Resetted on SyncthingProcess::startSyncthing() and SyncthingProcess::restartSyncthing().
inline bool SyncthingProcess::isManuallyStopped() const
{
    return m_manuallyStopped;
}

/*!
 * \brief Returns the "main" instance assigned via SyncthingProcess::setMainInstance().
 */
inline SyncthingProcess *SyncthingProcess::mainInstance()
{
    return s_mainInstance;
}

/*!
 * \brief Sets the "main" instance.
 */
inline void SyncthingProcess::setMainInstance(SyncthingProcess *mainInstance)
{
    s_mainInstance = mainInstance;
}

} // namespace Data

#endif // DATA_SYNCTHINGPROCESS_H
