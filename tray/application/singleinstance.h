#ifndef SINGLEINSTANCE_H
#define SINGLEINSTANCE_H

#include <QObject>

QT_FORWARD_DECLARE_CLASS(QLocalServer)

namespace Data {
struct SyncthingDir;
}

namespace QtGui {

class SingleInstance : public QObject {
    Q_OBJECT
public:
    explicit SingleInstance(
        int argc, const char *const *argv, bool skipSingleInstanceBehavior = false, bool skipPassing = false, QObject *parent = nullptr);

Q_SIGNALS:
    void newInstance(int argc, const char *const *argv);

public:
    static const QString &applicationId();
    static bool passArgsToRunningInstance(int argc, const char *const *argv, const QString &appId, bool waitUntilGone = false);

private Q_SLOTS:
    void handleNewConnection();
    void readArgs();

private:
    QLocalServer *m_server;
};
} // namespace QtGui

#endif // SINGLEINSTANCE_H
