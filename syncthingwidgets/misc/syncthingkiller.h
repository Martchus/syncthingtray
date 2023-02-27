#ifndef SYNCTHINGWIDGETS_SYNCTHINGKILLER_H
#define SYNCTHINGWIDGETS_SYNCTHINGKILLER_H

#include "../global.h"

#include <QObject>

#include <vector>

namespace Data {
class SyncthingConnection;
class SyncthingProcess;
} // namespace Data

namespace QtGui {

struct ProcessWithConnection {
    explicit ProcessWithConnection(Data::SyncthingProcess *process, Data::SyncthingConnection *connection = nullptr);
    Data::SyncthingProcess *const process;
    Data::SyncthingConnection *const connection;
};

inline ProcessWithConnection::ProcessWithConnection(Data::SyncthingProcess *process, Data::SyncthingConnection *connection)
    : process(process)
    , connection(connection)
{
}

class SYNCTHINGWIDGETS_EXPORT SyncthingKiller : public QObject {
    Q_OBJECT
public:
    explicit SyncthingKiller(std::vector<ProcessWithConnection> &&processes);

Q_SIGNALS:
    void ignored();

public Q_SLOTS:
    void waitForFinished();

private Q_SLOTS:
    void confirmKill() const;

private:
    std::vector<ProcessWithConnection> m_processes;
};

} // namespace QtGui

#endif // SYNCTHINGWIDGETS_SYNCTHINGKILLER_H
