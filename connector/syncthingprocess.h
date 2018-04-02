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
    Q_PROPERTY(ChronoUtilities::DateTime activeSince READ activeSince)
    Q_PROPERTY(bool manuallyStopped READ isManuallyStopped)

public:
    explicit SyncthingProcess(QObject *parent = nullptr);
    bool isRunning() const;
    ChronoUtilities::DateTime activeSince() const;
    bool isActiveFor(unsigned int atLeastSeconds) const;
    bool isManuallyStopped() const;

Q_SIGNALS:
    void confirmKill();

public Q_SLOTS:
    void restartSyncthing(const QString &cmd);
    void startSyncthing(const QString &cmd);
    void stopSyncthing();
    void killSyncthing();

private Q_SLOTS:
    void handleStarted();
    void handleFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void killToRestart();

private:
    QString m_cmd;
    ChronoUtilities::DateTime m_activeSince;
    QTimer m_killTimer;
    bool m_manuallyStopped;
};

inline bool SyncthingProcess::isRunning() const
{
    return state() != QProcess::NotRunning;
}

inline ChronoUtilities::DateTime SyncthingProcess::activeSince() const
{
    return m_activeSince;
}

inline bool SyncthingProcess::isActiveFor(unsigned int atLeastSeconds) const
{
    return !m_activeSince.isNull() && (ChronoUtilities::DateTime::gmtNow() - m_activeSince).totalSeconds() > atLeastSeconds;
}

inline bool SyncthingProcess::isManuallyStopped() const
{
    return m_manuallyStopped;
}

SyncthingProcess LIB_SYNCTHING_CONNECTOR_EXPORT &syncthingProcess();

} // namespace Data

#endif // DATA_SYNCTHINGPROCESS_H
