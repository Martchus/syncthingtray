#ifndef CLI_APPLICATION_H
#define CLI_APPLICATION_H

#include "./args.h"

#include "../connector/syncthingconnection.h"
#include "../connector/syncthingconnectionsettings.h"

#include <QObject>

#include <tuple>

namespace Cli {

enum class OperationType { Status, PauseResume };

class Application : public QObject {
    Q_OBJECT

public:
    Application();
    ~Application();

    int exec(int argc, const char *const *argv);

private slots:
    void handleStatusChanged(Data::SyncthingStatus newStatus);
    void handleResponse();
    void handleError(
        const QString &message, Data::SyncthingErrorCategory category, int networkError, const QNetworkRequest &request, const QByteArray &response);
    void findRelevantDirsAndDevs();
    void findRelevantDirsAndDevs(OperationType operationType);
    bool findPwd();

private:
    int loadConfig();
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
    static void printDir(const Data::SyncthingDir *dir);
    static void printDev(const Data::SyncthingDev *dev);
    void printStatus(const ArgumentOccurrence &);
    static void printLog(const std::vector<Data::SyncthingLogEntry> &logEntries);
    void initWaitForIdle(const ArgumentOccurrence &);
    void waitForIdle();
    void checkPwdOperationPresent(const ArgumentOccurrence &occurrence);
    void printPwdStatus(const ArgumentOccurrence &occurrence);
    void requestRescanPwd(const ArgumentOccurrence &occurrence);
    void requestPausePwd(const ArgumentOccurrence &occurrence);
    void requestResumePwd(const ArgumentOccurrence &occurrence);
    void initDirCompletion(Argument &arg, const ArgumentOccurrence &);
    void initDevCompletion(Argument &arg, const ArgumentOccurrence &);

    Args m_args;
    Data::SyncthingConnectionSettings m_settings;
    Data::SyncthingConnection m_connection;
    size_t m_expectedResponse;
    bool m_preventDisconnect;
    bool m_callbacksInvoked;
    std::vector<const Data::SyncthingDir *> m_relevantDirs;
    std::vector<const Data::SyncthingDev *> m_relevantDevs;
    const Data::SyncthingDir *m_pwd;
    QString m_relativePath;
    bool m_argsRead;
};

} // namespace Cli

#endif // CLI_APPLICATION_H
