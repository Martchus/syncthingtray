#ifndef DATA_SYNCTHINGPROCESS_H
#define DATA_SYNCTHINGPROCESS_H

#include "./global.h"

#include <QProcess>

namespace Data {

class LIB_SYNCTHING_CONNECTOR_EXPORT SyncthingProcess : public QProcess
{
    Q_OBJECT
public:
    explicit SyncthingProcess(QObject *parent = nullptr);

public Q_SLOTS:
    void restartSyncthing(const QString &cmd);
    void startSyncthing(const QString &cmd);
    void stopSyncthing();

private Q_SLOTS:
    void handleFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void killToRestart();

private:
    QString m_cmd;
};

SyncthingProcess LIB_SYNCTHING_CONNECTOR_EXPORT &syncthingProcess();

} // namespace Data

#endif // DATA_SYNCTHINGPROCESS_H
