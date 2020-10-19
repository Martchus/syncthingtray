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
    SingleInstance(int argc, const char *const *argv, bool newInstance = false, QObject *parent = nullptr);

Q_SIGNALS:
    void newInstance(int argc, const char *const *argv);

private Q_SLOTS:
    void handleNewConnection();
    void readArgs();

private:
    void passArgsToRunningInstance(int argc, const char *const *argv, const QString &appId);

    QLocalServer *m_server;
};
} // namespace QtGui

#endif // SINGLEINSTANCE_H
