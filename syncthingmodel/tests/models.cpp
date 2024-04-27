#include "../syncthingdirectorymodel.h"
#include "../syncthingdevicemodel.h"

#include <syncthingconnector/syncthingconnection.h>

#include <QtTest/QtTest>

#include <QEventLoop>
#include <QTimer>
#include <QLocale>

#include <qtutilities/misc/compat.h>

class ModelTests : public QObject {
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();

    void testDirectoryModel();
    void testDevicesModel();

private:
    QTimer m_timeout;
    QEventLoop m_loop;
    Data::SyncthingConnection m_connection;
};

void ModelTests::initTestCase()
{
    // ensure all text is English
    QLocale::setDefault(QLocale(QLocale::English));

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
    const auto model = Data::SyncthingDirectoryModel(m_connection);
    QCOMPARE(model.rowCount(QModelIndex()), 3);
    QCOMPARE(model.index(0, 0).data(), QStringLiteral("A folder"));
    QCOMPARE(model.index(1, 0).data(), QStringLiteral("Yet another folder"));
    QCOMPARE(model.index(2, 0).data(), QStringLiteral("A folder which is not shared"));
    const auto folder1Idx = model.index(0, 0);
    QCOMPARE(model.rowCount(folder1Idx), 10);
    QCOMPARE(model.index(0, 0, folder1Idx).data(), QStringLiteral("ID"));
    QCOMPARE(model.index(0, 1, folder1Idx).data(), QStringLiteral("GXWxf-3zgnU"));
    QCOMPARE(model.index(1, 0, folder1Idx).data(), QStringLiteral("Path"));
    QCOMPARE(model.index(1, 1, folder1Idx).data(), QStringLiteral("..."));
    const auto folder2Idx = model.index(1, 0);
    QCOMPARE(model.rowCount(folder2Idx), 10);
    QCOMPARE(model.index(0, 1, folder2Idx).data(), QStringLiteral("zX8xfl3ygn-"));
    QCOMPARE(model.index(1, 1, folder2Idx).data(), QStringLiteral("..."));
    const auto folder3Idx = model.index(2, 0);
    QCOMPARE(model.rowCount(folder2Idx), 10);
    QCOMPARE(model.index(0, 1, folder3Idx).data(), QStringLiteral("forever-alone"));
    QCOMPARE(model.index(1, 1, folder3Idx).data(), QStringLiteral("..."));
}

void ModelTests::testDevicesModel()
{
    const auto model = Data::SyncthingDeviceModel(m_connection);
    QCOMPARE(model.rowCount(QModelIndex()), 2);
    QCOMPARE(model.index(0, 0).data(), QStringLiteral("Myself"));
    QCOMPARE(model.index(0, 1).data(), QStringLiteral("This device"));
    QCOMPARE(model.index(1, 0).data(), QStringLiteral("Other instance"));
    QCOMPARE(model.index(1, 1).data(), QStringLiteral("Unknown status"));
    const auto dev1Idx = model.index(0, 0);
    QCOMPARE(model.rowCount(dev1Idx), 5);
    QCOMPARE(model.index(0, 0, dev1Idx).data(), QStringLiteral("ID"));
    QCOMPARE(model.index(0, 1, dev1Idx).data(), QStringLiteral("P56IOI7-MZJNU2Y-IQGDREY-DM2MGTI-MGL3BXN-PQ6W5BM-TBBZ4TJ-XZWICQ2"));
    QCOMPARE(model.index(1, 0, dev1Idx).data(), QStringLiteral("Address"));
    QCOMPARE(model.index(1, 1, dev1Idx).data(), QStringLiteral("dynamic, tcp://192.168.1.2:22000"));
    QCOMPARE(model.index(4, 0, dev1Idx).data(), QStringLiteral("Introducer"));
    QCOMPARE(model.index(4, 1, dev1Idx).data(), QStringLiteral("no"));
    const auto dev2Idx = model.index(1, 0);
    QCOMPARE(model.rowCount(dev2Idx), 5);
    QCOMPARE(model.index(0, 1, dev2Idx).data(), QStringLiteral("53STGR7-YBM6FCX-PAZ2RHM-YPY6OEJ-WYHVZO7-PCKQRCK-PZLTP7T"));
    QCOMPARE(model.index(1, 1, dev2Idx).data(), QStringLiteral("dynamic, tcp://192.168.1.3:22000"));
}

QTEST_MAIN(ModelTests)
#include "models.moc"
