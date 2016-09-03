#ifndef DATA_SYNCTHINGPROCESS_H
#define DATA_SYNCTHINGPROCESS_H

#include <QProcess>

namespace Data {

class SyncthingProcess : public QProcess
{
    Q_OBJECT
public:
    SyncthingProcess(QObject *parent = nullptr);

public Q_SLOTS:
    void restartSyncthing();
    void startSyncthing();

private Q_SLOTS:
    void handleFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void killToRestart();

private:
    bool m_restarting;
};

SyncthingProcess &syncthingProcess();

} // namespace Data

#endif // DATA_SYNCTHINGPROCESS_H
