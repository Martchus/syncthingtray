#ifndef SYNCTHINGTESTHELPER_SYNCTHINGTESTINSTANCE_H
#define SYNCTHINGTESTHELPER_SYNCTHINGTESTINSTANCE_H

#include "./global.h"
#include "./helper.h"

#include <syncthingconnector/syncthingprocess.h>

#include <QCoreApplication>
#include <QProcess>

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

    const QString &apiKey() const;
    const QString &syncthingPort() const;
    QCoreApplication &application();
    Data::SyncthingProcess &syncthingProcess();

public Q_SLOTS:
    void start();
    void stop();
    bool isInterleavedOutputEnabled() const;
    void setInterleavedOutputEnabled(bool interleavedOutputEnabled);
    void setInterleavedOutputEnabledFromEnv();

private:
    QString m_apiKey;
    QString m_syncthingPort;
    QCoreApplication m_app;
    Data::SyncthingProcess m_syncthingProcess;
    bool m_interleavedOutput;
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
