#include "../syncthingdevicemodel.h"
#include "../syncthingdirectorymodel.h"
#include "../syncthingfilemodel.h"

#include <syncthingconnector/syncthingconnection.h>

#include <QtTest/QtTest>

#include <QAction>
#include <QDebug>
#include <QEventLoop>
#include <QLocale>
#include <QStringBuilder>
#include <QTimer>

#include <qtutilities/misc/compat.h>

#include <limits>

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

    // setup error handling
    connect(&m_connection, &Data::SyncthingConnection::error, this,
        [this](const QString &errorMessage, Data::SyncthingErrorCategory, int, const QNetworkRequest &, const QByteArray &) {
            qDebug() << "connection error: " << errorMessage;
            if (errorMessage.contains(QStringLiteral("ignore pattern"))) {
                return; // ignore expected errors
            }
            m_loop.quit();
            QFAIL("Unexpected connection error occurred");
        });

    // request config and status and wait until available
    const auto applyConfig = [this] {
        if (!m_connection.rawConfig().isEmpty() && !m_connection.myId().isEmpty()) {
            m_connection.applyRawConfig();
            m_loop.quit();
        }
    };
    connect(&m_connection, &Data::SyncthingConnection::newConfig, this, applyConfig);
    connect(&m_connection, &Data::SyncthingConnection::myIdChanged, this, applyConfig);
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
    QCOMPARE(model.rowCount(folder1Idx), 11);
    QCOMPARE(model.index(0, 0, folder1Idx).data(), QStringLiteral("ID"));
    QCOMPARE(model.index(0, 1, folder1Idx).data(), QStringLiteral("GXWxf-3zgnU"));
    QCOMPARE(model.index(1, 0, folder1Idx).data(), QStringLiteral("Path"));
    QCOMPARE(model.index(1, 1, folder1Idx).data(), QStringLiteral("this/path/is/not/supposed/to/exist"));
    const auto folder2Idx = model.index(1, 0);
    QCOMPARE(model.rowCount(folder2Idx), 11);
    QCOMPARE(model.index(0, 1, folder2Idx).data(), QStringLiteral("zX8xfl3ygn-"));
    QCOMPARE(model.index(1, 1, folder2Idx).data(), QStringLiteral("..."));
    const auto folder3Idx = model.index(2, 0);
    QCOMPARE(model.rowCount(folder2Idx), 11);
    QCOMPARE(model.index(0, 1, folder3Idx).data(), QStringLiteral("forever-alone"));
    QCOMPARE(model.index(1, 1, folder3Idx).data(), QStringLiteral("..."));
}

void ModelTests::testDevicesModel()
{
    const auto model = Data::SyncthingDeviceModel(m_connection);
    QCOMPARE(model.rowCount(QModelIndex()), 2);
    QCOMPARE(model.index(0, 0).data(), QStringLiteral("Myself"));
    QCOMPARE(model.index(0, 1).data(), QStringLiteral("This Device"));
    QCOMPARE(model.index(1, 0).data(), QStringLiteral("Other instance"));
    QCOMPARE(model.index(1, 1).data(), QStringLiteral("Unknown"));
    const auto dev1Idx = model.index(0, 0);
    QCOMPARE(model.rowCount(dev1Idx), 7);
    QCOMPARE(model.index(0, 0, dev1Idx).data(), QStringLiteral("ID"));
    QCOMPARE(model.index(0, 1, dev1Idx).data(), QStringLiteral("P56IOI7-MZJNU2Y-IQGDREY-DM2MGTI-MGL3BXN-PQ6W5BM-TBBZ4TJ-XZWICQ2"));
    QCOMPARE(model.index(1, 0, dev1Idx).data(), QStringLiteral("Out of Sync items"));
    QCOMPARE(model.index(1, 1, dev1Idx).data(), QStringLiteral("none"));
    QCOMPARE(model.index(2, 0, dev1Idx).data(), QStringLiteral("Address"));
    QCOMPARE(model.index(2, 1, dev1Idx).data(), QStringLiteral("dynamic, tcp://192.168.1.2:22000"));
    QCOMPARE(model.index(5, 0, dev1Idx).data(), QStringLiteral("Introducer"));
    QCOMPARE(model.index(5, 1, dev1Idx).data(), QStringLiteral("no"));
    QCOMPARE(model.index(6, 0, dev1Idx).data(), QStringLiteral("Version"));
    QCOMPARE(model.index(6, 1, dev1Idx).data(), QStringLiteral("unknown"));
    const auto dev2Idx = model.index(1, 0);
    QCOMPARE(model.rowCount(dev2Idx), 6);
    QCOMPARE(model.index(0, 1, dev2Idx).data(), QStringLiteral("53STGR7-YBM6FCX-PAZ2RHM-YPY6OEJ-WYHVZO7-PCKQRCK-PZLTP7T"));
    QCOMPARE(model.index(2, 1, dev2Idx).data(), QStringLiteral("dynamic, tcp://192.168.1.3:22000"));
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
    model.setRecursiveSelectionEnabled(true);
    QCOMPARE(model.rowCount(QModelIndex()), 1);
    const auto rootIdx = QPersistentModelIndex(model.index(0, 0));
    QVERIFY(rootIdx.isValid());
    QCOMPARE(rootIdx.data(Data::SyncthingFileModel::NameRole).toString(), dirInfo->displayName());
    QVERIFY(!model.index(1, 0).isValid());
    QCOMPARE(model.rowCount(rootIdx), 1);
    QCOMPARE(model.index(0, 0, rootIdx).data(), QStringLiteral("Loading…"));
    QCOMPARE(model.index(1, 0, rootIdx).data(), QVariant());
    QVERIFY2(!model.canFetchMore(rootIdx), "cannot fetch more when already loading");

    // wait until the root has been updated
    connect(&model, &Data::SyncthingFileModel::fetchQueueEmpty, this, [this]() {
        m_timeout.stop();
        m_loop.quit();
    });
    m_timeout.start();
    m_loop.exec();

    QVERIFY(rootIdx.isValid());
    QCOMPARE(model.rowCount(rootIdx), 2);
    QCOMPARE(rootIdx.sibling(rootIdx.row(), 4).data(Qt::ToolTipRole), QStringLiteral("Exists locally and globally"));

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
    QVERIFY(model.index(0, 0, cameraIdx).data(Qt::DecorationRole).canConvert<QIcon>());
    QCOMPARE(model.index(0, 1, cameraIdx).data(), QStringLiteral("10.19 MiB"));
    QCOMPARE(model.index(0, 2, cameraIdx).data(), QStringLiteral("2020-12-16 22:31:34.500"));
    QCOMPARE(model.index(0, 3, cameraIdx).data(), QVariant());
    QVERIFY(model.index(0, 4, cameraIdx).data(Qt::DecorationRole).canConvert<QPixmap>());
    QCOMPARE(model.index(0, 4, cameraIdx).data(Qt::ToolTipRole), QStringLiteral("Exists only globally"));
    QCOMPARE(model.index(1, 0, cameraIdx).data(), QStringLiteral("IMG_20201213_122451.jpg"));
    QCOMPARE(model.index(2, 0, cameraIdx).data(), QStringLiteral("IMG_20201213_122504.jpg"));
    QCOMPARE(model.index(3, 0, cameraIdx).data(), QStringLiteral("IMG_20201213_122505.jpg"));
    QCOMPARE(model.index(4, 0, cameraIdx).data(), QStringLiteral("IMG_20201213_125329.jpg"));
    QCOMPARE(model.index(5, 0, cameraIdx).data(), QVariant());
    QCOMPARE(model.index(5, 0, cameraIdx).data(Qt::DecorationRole), QVariant());
    QCOMPARE(model.index(5, 1, cameraIdx).data(), QVariant());
    QCOMPARE(model.index(5, 2, cameraIdx).data(), QVariant());
    QCOMPARE(model.index(5, 3, cameraIdx).data(), QVariant());
    QCOMPARE(model.index(5, 3, cameraIdx).data(), QVariant());
    QCOMPARE(model.index(5, 4, cameraIdx).data(Qt::DecorationRole), QVariant());
    QCOMPARE(model.index(5, 4, cameraIdx).data(Qt::ToolTipRole), QVariant());

    // test conversion of indexes to/from paths
    const auto testPath = QStringLiteral("Camera/IMG_20201213_122504.jpg");
    const auto testPathIdx = model.index(2, 0, cameraIdx);
    QCOMPARE(model.path(testPathIdx), testPath);
    QCOMPARE(testPathIdx.data(Data::SyncthingFileModel::PathRole), testPath);
    QCOMPARE(model.index(testPath), testPathIdx);

    // re-load the data again and wait for the update
    QVERIFY2(!model.canFetchMore(rootIdx), "cannot fetch more when children already populated");
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
        QStringList({ QStringLiteral("refresh"), QStringLiteral("toggle-selection-recursively"), QStringLiteral("toggle-selection-single"),
            QStringLiteral("open"), QStringLiteral("copy-path") }));
    QCOMPARE(androidIdx2.data(Data::SyncthingFileModel::ActionNames).toStringList(),
        QStringList({ QStringLiteral("Refresh"), QStringLiteral("Select recursively"), QStringLiteral("Select single item"),
            QStringLiteral("Browse locally"), QStringLiteral("Copy local path") }));
    QCOMPARE(androidIdx2.data(Data::SyncthingFileModel::ActionIcons).toList().size(), 5);

    // selection actions when selection mode disabled
    auto actions = model.selectionActions();
    QVERIFY(!model.isSelectionModeEnabled());
    QCOMPARE(actions.at(0)->text(), QStringLiteral("Select items to sync/ignore"));
    QCOMPARE(actions.at(1)->text(), QStringLiteral("Ignore all items by default"));
    QCOMPARE(actions.size(), 2);
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
    QCOMPARE(actions.size(), 6);
    QCOMPARE(actions.at(0)->text(), QStringLiteral("Uncheck all and discard staged changes"));
    QCOMPARE(actions.at(1)->text(), QStringLiteral("Ignore checked items (and their children)"));
    QCOMPARE(actions.at(2)->text(), QStringLiteral("Ignore and locally delete checked items (and their children)"));
    QCOMPARE(actions.at(3)->text(), QStringLiteral("Include checked items (and their children)"));
    QCOMPARE(actions.at(4)->text(), QStringLiteral("Ignore all items by default"));
    QCOMPARE(actions.at(5)->text(), QStringLiteral("Remove ignore patterns matching checked items (may affect other items as well)"));
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

    // compute diff and new ignore patterns
    const auto testPatterns = QStringList{ QStringLiteral("foo"), QStringLiteral("bar"), QStringLiteral("baz") };
    const auto changedTestPatterns = QStringList{ QStringLiteral("// new comment at beginning"), testPatterns.front(), testPatterns.back(),
        QStringLiteral("biz"), QStringLiteral("buz") };
    model.m_presentIgnorePatterns.reserve(static_cast<std::size_t>(testPatterns.size()));
    for (const auto &pattern : testPatterns) {
        model.m_presentIgnorePatterns.emplace_back(QString(pattern));
    }
    QCOMPARE(model.computeIgnorePatternDiff(), QStringLiteral(" foo\n bar\n baz\n"));
    QCOMPARE(model.computeNewIgnorePatterns().ignore, testPatterns);
    model.m_stagedChanges[std::numeric_limits<std::size_t>::max()].prepend.append(changedTestPatterns.front());
    model.m_stagedChanges[1].replace = true; // removal
    auto &append = model.m_stagedChanges[2];
    append.append << changedTestPatterns.at(3) << changedTestPatterns.at(4);
    QCOMPARE(model.computeIgnorePatternDiff(), QStringLiteral("+// new comment at beginning\n foo\n-bar\n baz\n+biz\n+buz\n"));
    QCOMPARE(model.computeNewIgnorePatterns().ignore, changedTestPatterns);

    // clear all ignore pattern related state; diff and new ignore patterns should be computed to be empty
    model.m_isIgnoringAllByDefault = false;
    model.m_presentIgnorePatterns.clear();
    model.m_stagedChanges.clear();
    model.setCheckState(rootIdx, Qt::Unchecked, true);
    QCOMPARE(model.computeIgnorePatternDiff(), QString());
    QCOMPARE(model.computeNewIgnorePatterns().ignore, QStringList());

    // test ignoring items by default
    actions = model.selectionActions();
    QVERIFY(actions.size() > 4);
    QCOMPARE(actions.at(4)->text(), QStringLiteral("Ignore all items by default"));
    actions.at(4)->trigger();
    auto expectedDiff = QString(QChar('+') % model.m_ignoreAllByDefaultPattern % QChar('\n'));
    auto expectedPatterns = QStringList({ QStringLiteral("/**") });
    QCOMPARE(model.computeIgnorePatternDiff(), expectedDiff);
    QCOMPARE(model.computeNewIgnorePatterns().ignore, expectedPatterns);

    // test including items explicitly
    QCOMPARE(actions.at(3)->text(), QStringLiteral("Include checked items (and their children)"));
    model.setData(androidIdx2, Qt::Checked, Qt::CheckStateRole);
    expectedDiff.prepend(QStringLiteral("+!/100ANDRO\n"));
    expectedPatterns.prepend(QStringLiteral("!/100ANDRO"));
    for (auto i = 0; i != 2; ++i) { // perform action twice; this should not lead to a duplicate
        actions.at(3)->trigger();
        QCOMPARE(model.computeIgnorePatternDiff(), expectedDiff);
        QCOMPARE(model.computeNewIgnorePatterns().ignore, expectedPatterns);
    }
    model.setData(androidIdx2, Qt::Unchecked, Qt::CheckStateRole); // should the item be automatically unchecked?
    model.setData(cameraIdx2, Qt::Checked, Qt::CheckStateRole);
    expectedDiff.insert(QStringLiteral("+!/100ANDRO\n").size(), QStringLiteral("+!/Camera\n"));
    expectedPatterns.insert(1, QStringLiteral("!/Camera"));
    actions.at(3)->trigger();
    QCOMPARE(model.computeIgnorePatternDiff(), expectedDiff);
    QCOMPARE(model.computeNewIgnorePatterns().ignore, expectedPatterns);
    model.setData(cameraIdx2, Qt::Unchecked, Qt::CheckStateRole); // should the item be automatically unchecked?

    // test ignoring items explicitly
    model.setData(model.index(1, 0, cameraIdx2), Qt::Checked, Qt::CheckStateRole);
    model.setData(model.index(3, 0, cameraIdx2), Qt::Checked, Qt::CheckStateRole);
    QCOMPARE(actions.at(1)->text(), QStringLiteral("Ignore checked items (and their children)"));
    actions.at(1)->trigger();
    expectedDiff.insert(
        QStringLiteral("+!/100ANDRO\n").size(), QStringLiteral("+/Camera/IMG_20201213_122451.jpg\n+/Camera/IMG_20201213_122505.jpg\n"));
    expectedPatterns.insert(1, QStringLiteral("/Camera/IMG_20201213_122451.jpg"));
    expectedPatterns.insert(2, QStringLiteral("/Camera/IMG_20201213_122505.jpg"));
    QCOMPARE(model.computeIgnorePatternDiff(), expectedDiff);
    QCOMPARE(model.computeNewIgnorePatterns().ignore, expectedPatterns);

    // test changing default back to including all items by default
    // note: Doing this makes not much sense after including items explicitly like the previous tests just did but this should of course work in general.
    qDeleteAll(actions);
    actions = model.selectionActions();
    QVERIFY(actions.size() > 4);
    QCOMPARE(actions.at(4)->text(), QStringLiteral("Include all items by default"));
    actions.at(4)->trigger();
    expectedDiff.chop(model.m_ignoreAllByDefaultPattern.size() + 2);
    expectedPatterns.removeLast();
    QCOMPARE(model.computeIgnorePatternDiff(), expectedDiff);
    QCOMPARE(model.computeNewIgnorePatterns().ignore, expectedPatterns);

    // test reviewing/applying changes
    qDeleteAll(actions);
    actions = model.selectionActions();
    QVERIFY(actions.size() > 6);
    QCOMPARE(actions.at(6)->text(), QStringLiteral("Review and apply staged changes"));
    connect(
        &model, &Data::SyncthingFileModel::actionNeedsConfirmation, this,
        [&expectedDiff](QAction *action, const QString &message, const QString &diff, const QSet<QString> &localDeletions) {
            QCOMPARE(message, QStringLiteral("Do you want to apply the following changes?"));
            QCOMPARE(diff, expectedDiff);
            QCOMPARE(localDeletions, QSet<QString>());
            action->trigger();
        },
        Qt::QueuedConnection);
    connect(
        &model, &Data::SyncthingFileModel::notification, this,
        [this](const QString &type, const QString &message, const QString &details = QString()) {
            // log but otherwise ignore non-errors
            if (type != QLatin1String("error")) {
                qDebug() << "file model notification of type " << type << ": " << message;
                if (!details.isEmpty()) {
                    qDebug() << "details: " << details;
                }
                return;
            }
            m_timeout.stop();
            m_loop.quit();
            QCOMPARE(details, QString());
            qDebug() << "error message: " << message;
            QVERIFY(message.startsWith(QStringLiteral("Unable to change ignore patterns:")));
        },
        Qt::QueuedConnection);
    actions.at(6)->trigger();
    m_timeout.start();
    m_loop.exec();

    // test changing default back to including all items by default when "ignore all by default" pattern is present from the beginning
    model.m_presentIgnorePatterns.clear();
    model.m_stagedChanges.clear();
    for (const auto &pattern : expectedPatterns) {
        model.m_presentIgnorePatterns.emplace_back(QString(pattern));
    }
    model.m_presentIgnorePatterns.emplace_back(QString(model.m_ignoreAllByDefaultPattern));
    model.m_hasIgnorePatterns = true;
    model.m_isIgnoringAllByDefault = true;
    qDeleteAll(actions);
    actions = model.selectionActions();
    QVERIFY(actions.size() > 5);
    QCOMPARE(actions.at(4)->text(), QStringLiteral("Include all items by default"));
    actions.at(4)->trigger();
    expectedDiff = QStringLiteral(" !/100ANDRO\n /Camera/IMG_20201213_122451.jpg\n /Camera/IMG_20201213_122505.jpg\n !/Camera\n-")
        % model.m_ignoreAllByDefaultPattern % QChar('\n');
    QCOMPARE(model.computeIgnorePatternDiff(), expectedDiff);
    QCOMPARE(model.computeNewIgnorePatterns().ignore, expectedPatterns);

    // test removing matching ignore patterns
    model.resetMatchingIgnorePatterns();
    model.setCheckState(rootIdx, Qt::Unchecked, true);
    model.setCheckState(model.index(1, 0, cameraIdx2), Qt::Checked);
    QCOMPARE(actions.at(5)->text(), QStringLiteral("Remove ignore patterns matching checked items (may affect other items as well)"));
    actions.at(5)->trigger();
    const auto indexInDiff = expectedDiff.indexOf(QStringLiteral(" /Camera/IMG_20201213_122451.jpg"));
    expectedDiff[indexInDiff] = QChar('-');
    expectedPatterns.removeAt(1);
    QCOMPARE(model.computeIgnorePatternDiff(), expectedDiff);
    QCOMPARE(model.computeNewIgnorePatterns().ignore, expectedPatterns);
    // add the pattern back by ignoring the relevant item explicitly
    actions.at(1)->trigger();
    expectedDiff[indexInDiff] = QChar(' ');
    expectedPatterns.insert(1, QStringLiteral("/Camera/IMG_20201213_122451.jpg"));
    QCOMPARE(model.computeIgnorePatternDiff(), expectedDiff);
    QCOMPARE(model.computeNewIgnorePatterns().ignore, expectedPatterns);
    // remove the added-back pattern
    actions.at(5)->trigger();
    expectedDiff[indexInDiff] = QChar('-');
    expectedPatterns.removeAt(1);
    QCOMPARE(model.computeIgnorePatternDiff(), expectedDiff);
    QCOMPARE(model.computeNewIgnorePatterns().ignore, expectedPatterns);
}

QTEST_MAIN(ModelTests)
#include "models.moc"
