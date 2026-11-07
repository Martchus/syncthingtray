#ifndef SYNCTHINGTESTHELPER_SYNCTHINGTESTINSTANCE_H
#define SYNCTHINGTESTHELPER_SYNCTHINGTESTINSTANCE_H

#include "./global.h"
#include "./helper.h"

#include <syncthingconnector/syncthingprocess.h>

#include <QCoreApplication>
#include <QProcess>
#include <QTemporaryDir>

using namespace std;

namespace CppUtilities {

/*!
 * \brief The SyncthingTestInstance class provides running a test instance of Syncthing.
 *
 * The class is meant to be subclassed by tests requiring a running Syncthing instance.
 */
class SYNCTHINGTESTHELPER_EXPORT SyncthingTestInstance {
public:
    SyncthingTestInstance();

    inline const QString &apiKey() const;
    inline const QString &syncthingPort() const;
    inline QCoreApplication &application();
    inline const QTemporaryDir &dataDir() const;
    inline Data::SyncthingProcess &syncthingProcess();

public:
    void start();
    void stop();
    inline bool isInterleavedOutputEnabled() const;
    void setInterleavedOutputEnabled(bool interleavedOutputEnabled);
    void setInterleavedOutputEnabledFromEnv();

private:
    void handleError(QProcess::ProcessError error);
    void handleFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    QString m_apiKey;
    QString m_syncthingPort;
    QCoreApplication m_app;
    QTemporaryDir m_dataDir;
    Data::SyncthingProcess m_syncthingProcess;
    bool m_interleavedOutput;
    bool m_processSupposedToRun;
};

inline const QString &SyncthingTestInstance::apiKey() const
{
    return m_apiKey;
}

inline const QString &SyncthingTestInstance::syncthingPort() const
{
    return m_syncthingPort;
}

inline QCoreApplication &SyncthingTestInstance::application()
{
    return m_app;
}

inline const QTemporaryDir &SyncthingTestInstance::dataDir() const
{
    return m_dataDir;
}

inline Data::SyncthingProcess &SyncthingTestInstance::syncthingProcess()
{
    return m_syncthingProcess;
}

/*!
 * \brief Whether Syncthing's output should be forwarded to see what Syncthing and the test is doing at the same time.
 */
inline bool SyncthingTestInstance::isInterleavedOutputEnabled() const
{
    return m_interleavedOutput;
}
} // namespace CppUtilities

#endif // SYNCTHINGTESTHELPER_SYNCTHINGTESTINSTANCE_H
