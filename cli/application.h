#ifndef CLI_APPLICATION_H
#define CLI_APPLICATION_H

#include "./args.h"

#include "../connector/syncthingconnection.h"
#include "../connector/syncthingconnectionsettings.h"

#include <QObject>

namespace Cli {

class Application : public QObject
{
    Q_OBJECT

public:
    Application();
    ~Application();

    int exec(int argc, const char *const *argv);

private slots:
    void handleStatusChanged(Data::SyncthingStatus newStatus);
    void handleResponse();
    void handleError(const QString &message);

private:
    void requestLog(const ArgumentOccurrence &);
    void requestShutdown(const ArgumentOccurrence &);
    void requestRestart(const ArgumentOccurrence &);
    void requestRescan(const ArgumentOccurrence &occurrence);
    void requestRescanAll(const ArgumentOccurrence &);
    void requestPause(const ArgumentOccurrence &occurrence);
    void requestPauseAll(const ArgumentOccurrence &);
    void requestResume(const ArgumentOccurrence &);
    void requestResumeAll(const ArgumentOccurrence &);
    void printStatus(const ArgumentOccurrence &);
    void printLog(const std::vector<Data::SyncthingLogEntry> &logEntries);

    Args m_args;
    Data::SyncthingConnectionSettings m_settings;
    Data::SyncthingConnection m_connection;
    size_t m_expectedResponse;
};

} // namespace Cli

#endif // CLI_APPLICATION_H
