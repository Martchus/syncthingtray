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
    SingleInstance(int argc, const char *const *argv, QObject *parent = nullptr);

Q_SIGNALS:
    void newInstance(int argc, const char *const *argv);

private Q_SLOTS:
    void handleNewConnection();
    void readArgs();

private:
    QLocalServer *m_server;
};
}

#endif // SINGLEINSTANCE_H
