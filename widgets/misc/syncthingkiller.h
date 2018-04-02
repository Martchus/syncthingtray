#ifndef SYNCTHINGWIDGETS_SYNCTHINGKILLER_H
#define SYNCTHINGWIDGETS_SYNCTHINGKILLER_H

#include "../global.h"

#include <QObject>

#include <vector>

namespace Data {
class SyncthingProcess;
}

namespace QtGui {

class SYNCTHINGWIDGETS_EXPORT SyncthingKiller : public QObject {
    Q_OBJECT
public:
    SyncthingKiller(std::vector<Data::SyncthingProcess *> &&processes);

Q_SIGNALS:
    void ignored();

public Q_SLOTS:
    void waitForFinished();

private Q_SLOTS:
    void confirmKill() const;

private:
    std::vector<Data::SyncthingProcess *> m_processes;
};

} // namespace QtGui

#endif // SYNCTHINGWIDGETS_SYNCTHINGKILLER_H
