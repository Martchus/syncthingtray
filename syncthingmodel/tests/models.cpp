#include "../syncthingdirectorymodel.h"

#include <syncthingconnector/syncthingconnection.h>

#include <QtTest/QtTest>

#include <QEventLoop>
#include <QTimer>

#include <qtutilities/misc/compat.h>

class ModelTests : public QObject {
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();

    void testDirectoryModel();

private:
    QTimer m_timeout;
    QEventLoop m_loop;
    Data::SyncthingConnection m_connection;
};

void ModelTests::initTestCase()
{
    // setup timeout
    m_timeout.setSingleShot(true);
    m_timeout.setInterval(5000);
    m_timeout.start();
    connect(&m_timeout, &QTimer::timeout, this, [this] {
        m_loop.quit();
        QFAIL("Timeout exceeded when loading mocked config/status for test");
    });

    // request config and status and wait until available
    connect(&m_connection, &Data::SyncthingConnection::newConfigApplied, &m_loop, &QEventLoop::quit);
    m_connection.requestConfigAndStatus();
    m_loop.exec();
    m_timeout.stop();
}

void ModelTests::cleanupTestCase()
{
}

void ModelTests::testDirectoryModel()
{
    auto model = Data::SyncthingDirectoryModel(m_connection);
    QCOMPARE(model.rowCount(QModelIndex()), 3);
    QCOMPARE(model.index(0, 0).data(), QStringLiteral("A folder"));
    QCOMPARE(model.index(1, 0).data(), QStringLiteral("Yet another folder"));
    QCOMPARE(model.index(2, 0).data(), QStringLiteral("A folder which is not shared"));
}

QTEST_MAIN(ModelTests)
#include "models.moc"
