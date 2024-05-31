#include "../syncthingdevicemodel.h"
#include "../syncthingdirectorymodel.h"
#include "../syncthingfilemodel.h"

#include <syncthingconnector/syncthingconnection.h>

#include <QtTest/QtTest>

#include <QEventLoop>
#include <QLocale>
#include <QTimer>

#include <qtutilities/misc/compat.h>

class ModelTests : public QObject {
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();

    void testDirectoryModel();
    void testDevicesModel();
    void testFileModel();

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
        QFAIL("Timeout exceeded");
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

void ModelTests::testFileModel()
{
    auto row = 0;
    const auto dirId = QStringLiteral("GXWxf-3zgnU");
    const auto *dirInfo = m_connection.findDirInfo(dirId, row);
    QVERIFY(dirInfo);
    QCOMPARE(dirInfo->displayName(), QStringLiteral("A folder"));

    // test behavior of empty/unpopulated model
    auto model = Data::SyncthingFileModel(m_connection, *dirInfo);
    QCOMPARE(model.rowCount(QModelIndex()), 1);
    const auto rootIdx = QPersistentModelIndex(model.index(0, 0));
    QVERIFY(rootIdx.isValid());
    QCOMPARE(rootIdx.data(Data::SyncthingFileModel::NameRole).toString(), dirInfo->displayName());
    QVERIFY(!model.index(1, 0).isValid());
    QCOMPARE(model.rowCount(rootIdx), 1);
    QCOMPARE(model.index(0, 0, rootIdx).data(), QStringLiteral("Loading…"));
    QCOMPARE(model.index(1, 0, rootIdx).data(), QVariant());
    QVERIFY(model.canFetchMore(rootIdx));

    // wait until the root has been updated
    connect(&model, &Data::SyncthingFileModel::fetchQueueEmpty, this, [this]() {
        m_timeout.stop();
        m_loop.quit();
    });
    m_timeout.start();
    m_loop.exec();

    QVERIFY(rootIdx.isValid());
    QCOMPARE(model.rowCount(rootIdx), 2);

    // test access to nested folders
    const auto androidIdx = QPersistentModelIndex(model.index(0, 0, rootIdx));
    const auto cameraIdx = QPersistentModelIndex(model.index(1, 0, rootIdx));
    const auto nestedIdx = QPersistentModelIndex(model.index(0, 0, cameraIdx));
    const auto initialAndroidPtr = androidIdx.internalPointer();
    const auto initialCameraPtr = cameraIdx.internalPointer();
    QVERIFY(androidIdx.isValid());
    QVERIFY(cameraIdx.isValid());
    QCOMPARE(androidIdx.parent(), rootIdx);
    QCOMPARE(cameraIdx.parent(), rootIdx);
    QCOMPARE(nestedIdx.parent(), cameraIdx);
    QCOMPARE(model.rowCount(androidIdx), 0);
    QCOMPARE(model.rowCount(cameraIdx), 5);
    QCOMPARE(androidIdx.data(), QStringLiteral("100ANDRO"));
    QCOMPARE(cameraIdx.data(), QStringLiteral("Camera"));
    QCOMPARE(model.index(0, 0, cameraIdx).data(), QStringLiteral("IMG_20201114_124821.jpg"));
    QCOMPARE(model.index(0, 1, cameraIdx).data(), QStringLiteral("10.19 MiB"));
    QCOMPARE(model.index(0, 2, cameraIdx).data(), QStringLiteral("2020-12-16 22:31:34.500"));
    QCOMPARE(model.index(1, 0, cameraIdx).data(), QStringLiteral("IMG_20201213_122451.jpg"));
    QCOMPARE(model.index(2, 0, cameraIdx).data(), QStringLiteral("IMG_20201213_122504.jpg"));
    QCOMPARE(model.index(3, 0, cameraIdx).data(), QStringLiteral("IMG_20201213_122505.jpg"));
    QCOMPARE(model.index(4, 0, cameraIdx).data(), QStringLiteral("IMG_20201213_125329.jpg"));
    QCOMPARE(model.index(5, 0, cameraIdx).data(), QVariant());
    QCOMPARE(model.index(5, 1, cameraIdx).data(), QVariant());
    QCOMPARE(model.index(5, 2, cameraIdx).data(), QVariant());
    QCOMPARE(model.index(5, 3, cameraIdx).data(), QVariant());

    // test conversion of indexes to/from paths
    const auto testPath = QStringLiteral("Camera/IMG_20201213_122504.jpg");
    const auto testPathIdx = model.index(2, 0, cameraIdx);
    QCOMPARE(model.path(testPathIdx), testPath);
    QCOMPARE(testPathIdx.data(Data::SyncthingFileModel::PathRole), testPath);
    QCOMPARE(model.index(testPath), testPathIdx);

    // re-load the data again and wait for the update
    model.fetchMore(rootIdx);
    m_timeout.start();
    m_loop.exec();

    // verify that only the root index is still valid (all other indexes have been invalidated)
    QVERIFY(rootIdx.isValid());
    QCOMPARE(model.rowCount(rootIdx), 2);
    QVERIFY(androidIdx.internalPointer() != initialAndroidPtr);
    QVERIFY(!androidIdx.isValid());
    QVERIFY(cameraIdx.internalPointer() != initialCameraPtr);
    QVERIFY(!cameraIdx.isValid());
    QVERIFY(!nestedIdx.isValid());

    // verify that data was re-loaded
    const auto androidIdx2 = QPersistentModelIndex(model.index(0, 0, rootIdx));
    const auto cameraIdx2 = QPersistentModelIndex(model.index(1, 0, rootIdx));
    QCOMPARE(androidIdx2.data(), QStringLiteral("100ANDRO"));
    QCOMPARE(cameraIdx2.data(), QStringLiteral("Camera"));

    // item actions
    QCOMPARE(androidIdx2.data(Data::SyncthingFileModel::Actions).toStringList(),
        QStringList({ QStringLiteral("refresh"), QStringLiteral("toggle-selection-recursively"), QStringLiteral("toggle-selection-single"), QStringLiteral("open"), QStringLiteral("copy-path") }));
    QCOMPARE(androidIdx2.data(Data::SyncthingFileModel::ActionNames).toStringList(),
        QStringList({ QStringLiteral("Refresh"), QStringLiteral("Select recursively"), QStringLiteral("Select single item"), QStringLiteral("Browse locally"), QStringLiteral("Copy local path") }));
    QCOMPARE(androidIdx2.data(Data::SyncthingFileModel::ActionIcons).toList().size(), 5);

    // selection actions when selection mode disabled
    auto actions = model.selectionActions();
    QVERIFY(!model.isSelectionModeEnabled());
    QCOMPARE(actions.size(), 0);
    qDeleteAll(actions);

    // selecting items recursively
    QCOMPARE(rootIdx.data(Qt::CheckStateRole).toInt(), Qt::Unchecked);
    QCOMPARE(androidIdx2.data(Qt::CheckStateRole).toInt(), Qt::Unchecked);
    QVERIFY(model.setData(androidIdx2, Qt::Checked, Qt::CheckStateRole));
    QCOMPARE(rootIdx.data(Qt::CheckStateRole).toInt(), Qt::Unchecked);
    QCOMPARE(androidIdx2.data(Qt::CheckStateRole).toInt(), Qt::Unchecked);
    model.setSelectionModeEnabled(true);
    QCOMPARE(rootIdx.data(Qt::CheckStateRole).toInt(), Qt::PartiallyChecked);
    QCOMPARE(androidIdx2.data(Qt::CheckStateRole).toInt(), Qt::Checked);

    // selection actions when selection mode enabled
    actions = model.selectionActions();
    QCOMPARE(actions.size(), 2);
    QCOMPARE(actions.at(0)->text(), QStringLiteral("Discard selection and staged changes"));
    QCOMPARE(actions.at(1)->text(), QStringLiteral("Remove related ignore patterns"));
    actions.at(1)->trigger(); // this won't do much in the test setup
    actions.at(0)->trigger(); // disables selection mode
    qDeleteAll(actions);
    QVERIFY(!model.isSelectionModeEnabled());
    QCOMPARE(rootIdx.data(Qt::CheckStateRole).toInt(), Qt::Unchecked);
    QCOMPARE(androidIdx2.data(Qt::CheckStateRole).toInt(), Qt::Unchecked);

    // selecting single items
    model.triggerAction(QStringLiteral("toggle-selection-single"), androidIdx2);
    QVERIFY(model.isSelectionModeEnabled());
    QCOMPARE(rootIdx.data(Qt::CheckStateRole).toInt(), Qt::Unchecked);
    QCOMPARE(androidIdx2.data(Qt::CheckStateRole).toInt(), Qt::Checked);
    QCOMPARE(cameraIdx2.data(Qt::CheckStateRole).toInt(), Qt::Unchecked);

    // select actions recursively via triggerAction() on sibling
    model.triggerAction(QStringLiteral("toggle-selection-recursively"), cameraIdx2);
    QCOMPARE(rootIdx.data(Qt::CheckStateRole).toInt(), Qt::Checked);
    QCOMPARE(androidIdx2.data(Qt::CheckStateRole).toInt(), Qt::Checked);
    QCOMPARE(cameraIdx2.data(Qt::CheckStateRole).toInt(), Qt::Checked);

    // deselecting actions recursively
    model.triggerAction(QStringLiteral("toggle-selection-recursively"), rootIdx);
    QCOMPARE(rootIdx.data(Qt::CheckStateRole).toInt(), Qt::Unchecked);
    QCOMPARE(androidIdx2.data(Qt::CheckStateRole).toInt(), Qt::Unchecked);
    QCOMPARE(cameraIdx2.data(Qt::CheckStateRole).toInt(), Qt::Unchecked);
}

QTEST_MAIN(ModelTests)
#include "models.moc"
