#ifndef SYNCTHINGTESTHELPER_SYNCTHINGTESTINSTANCE_H
#define SYNCTHINGTESTHELPER_SYNCTHINGTESTINSTANCE_H

#include "./global.h"
#include "./helper.h"

#include <QCoreApplication>
#include <QProcess>

using namespace std;

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

public Q_SLOTS:
    void start();
    void stop();

protected:
    QCoreApplication &application();
    QProcess &syncthingProcess();

private:
    QString m_apiKey;
    QString m_syncthingPort;
    QCoreApplication m_app;
    QProcess m_syncthingProcess;
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

inline QProcess &SyncthingTestInstance::syncthingProcess()
{
    return m_syncthingProcess;
}

#endif // SYNCTHINGTESTHELPER_SYNCTHINGTESTINSTANCE_H
