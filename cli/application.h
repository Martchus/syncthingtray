#ifndef CLI_APPLICATION_H
#define CLI_APPLICATION_H

#include "./args.h"

#include "../connector/syncthingconnection.h"
#include "../connector/syncthingconnectionsettings.h"

#include <QObject>

#include <tuple>

namespace Cli {

enum class OperationType
{
    Status,
    PauseResume
};

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
    void findRelevantDirsAndDevs();
    void findRelevantDirsAndDevs(OperationType operationType);

private:
    void requestLog(const ArgumentOccurrence &);
    void requestShutdown(const ArgumentOccurrence &);
    void requestRestart(const ArgumentOccurrence &);
    void requestRescan(const ArgumentOccurrence &occurrence);
    void requestRescanAll(const ArgumentOccurrence &);
    void requestPauseResume(bool pause);
    void requestPauseAllDevs(const ArgumentOccurrence &);
    void requestPauseAllDirs(const ArgumentOccurrence &);
    void requestResumeAllDevs(const ArgumentOccurrence &);
    void requestResumeAllDirs(const ArgumentOccurrence &);
    void printStatus(const ArgumentOccurrence &);
    void printLog(const std::vector<Data::SyncthingLogEntry> &logEntries);
    void initWaitForIdle(const ArgumentOccurrence &);
    void waitForIdle();

    Args m_args;
    Data::SyncthingConnectionSettings m_settings;
    Data::SyncthingConnection m_connection;
    size_t m_expectedResponse;
    std::vector<const Data::SyncthingDir *> m_relevantDirs;
    std::vector<const Data::SyncthingDev *> m_relevantDevs;

};

} // namespace Cli

#endif // CLI_APPLICATION_H
