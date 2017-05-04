#include "../syncthingconnection.h"

#include "../testhelper/helper.h"
#include "../testhelper/syncthingtestinstance.h"

#include <c++utilities/tests/testutils.h>

#include <cppunit/TestFixture.h>

#include <QDir>
#include <QStringBuilder>

using namespace std;
using namespace Data;
using namespace TestUtilities;

using namespace CPPUNIT_NS;

/*!
 * \brief The ConnectionTests class tests the SyncthingConnector.
 */
class ConnectionTests : public TestFixture, private SyncthingTestInstance {
    CPPUNIT_TEST_SUITE(ConnectionTests);
    CPPUNIT_TEST(testConnection);
    CPPUNIT_TEST_SUITE_END();

public:
    ConnectionTests();

    void testConnection();

    void setUp();
    void tearDown();

private:
    template <typename Signal, typename Action, typename Handler = function<void(void)>>
    void waitForConnection(Signal signal, Action action, Handler handler = nullptr, bool *ok = nullptr, int timeout = 1000);
    template <typename Signal, typename Action, typename Handler = function<void(void)>>
    void waitForConnectionAnyAction(Signal signal, Action action, Handler handler = nullptr, bool *ok = nullptr, int timeout = 1000);
    template <typename Handler> QMetaObject::Connection handleNewDevices(Handler handler, bool *ok);
    template <typename Handler> QMetaObject::Connection handleNewDirs(Handler handler, bool *ok);

    SyncthingConnection m_connection;
};

CPPUNIT_TEST_SUITE_REGISTRATION(ConnectionTests);

ConnectionTests::ConnectionTests()
{
}

//
// test setup
//

/*!
 * \brief Starts Syncthing and prepares connecting.
 */
void ConnectionTests::setUp()
{
    SyncthingTestInstance::start();

    cerr << "\n - Preparing connection ..." << endl;
    m_connection.setSyncthingUrl(QStringLiteral("http://localhost:") + syncthingPort());

    // keep track of status changes
    QObject::connect(&m_connection, &SyncthingConnection::statusChanged,
        [this] { cerr << " - Connection status changed to: " << m_connection.statusText().toLocal8Bit().data() << endl; });
}

/*!
 * \brief Terminates Syncthing and prints stdout/stderr from Syncthing.
 */
void ConnectionTests::tearDown()
{
    SyncthingTestInstance::stop();
}

//
// test helper
//

/*!
 * \brief Variant of waitForSignal() where sender is the connection and the action is a method of the connection.
 */
template <typename Signal, typename Action, typename Handler>
void ConnectionTests::waitForConnection(Signal signal, Action action, Handler handler, bool *ok, int timeout)
{
    waitForSignal(&m_connection, signal, bind(action, &m_connection), timeout, handler, ok);
}

/*!
 * \brief Variant of waitForSignal() where sender is the connection.
 */
template <typename Signal, typename Action, typename Handler>
void ConnectionTests::waitForConnectionAnyAction(Signal signal, Action action, Handler handler, bool *ok, int timeout)
{
    waitForSignal(&m_connection, signal, action, timeout, handler, ok);
}

/*!
 * \brief Helps handling newDevices() signal when waiting for device change.
 */
template <typename Handler> QMetaObject::Connection ConnectionTests::handleNewDevices(Handler handler, bool *ok)
{
    return QObject::connect(&m_connection, &SyncthingConnection::newDevices, [ok, &handler](const std::vector<SyncthingDev> &devs) {
        for (const SyncthingDev &dev : devs) {
            handler(dev, 0);
        }
    });
}

/*!
 * \brief Helps handling newDirs() signal when waiting for directory change.
 */
template <typename Handler> QMetaObject::Connection ConnectionTests::handleNewDirs(Handler handler, bool *ok)
{
    return QObject::connect(&m_connection, &SyncthingConnection::newDirs, [ok, &handler](const std::vector<SyncthingDir> &dirs) {
        for (const SyncthingDir &dir : dirs) {
            handler(dir, 0);
        }
    });
}

//
// actual test
//

/*!
 * \brief Tests basic behaviour of the SyncthingConnection class.
 */
void ConnectionTests::testConnection()
{
    cerr << "\n - Connecting initially ..." << endl;

    // error in case of inavailability and wrong API key
    m_connection.setApiKey(QByteArray("wrong API key"));
    bool syncthingAvailable = false;
    const function<void(const QString &errorMessage)> errorHandler = [this, &syncthingAvailable](const QString &errorMessage) {
        if (errorMessage == QStringLiteral("Unable to request Syncthing config: Connection refused")
            || errorMessage == QStringLiteral("Unable to request Syncthing status: Connection refused")) {
            return; // Syncthing not ready yet
        }
        syncthingAvailable = true;
        if (errorMessage
                != QStringLiteral("Unable to request Syncthing config: Error transferring ") % m_connection.syncthingUrl()
                    % QStringLiteral("/rest/system/config - server replied: Forbidden")
            && errorMessage
                != QStringLiteral("Unable to request Syncthing status: Error transferring ") % m_connection.syncthingUrl()
                    % QStringLiteral("/rest/system/status - server replied: Forbidden")) {
            CPPUNIT_FAIL(argsToString("wrong error message in case of wrong API key: ", errorMessage.toLocal8Bit().data()));
        }
    };
    while (!syncthingAvailable) {
        waitForConnection(&SyncthingConnection::error, static_cast<void (SyncthingConnection::*)(void)>(&SyncthingConnection::connect), errorHandler);
    }

    // initial connection
    m_connection.setApiKey(apiKey().toUtf8());
    waitForConnection(&SyncthingConnection::statusChanged, static_cast<void (SyncthingConnection::*)(void)>(&SyncthingConnection::connect));
    CPPUNIT_ASSERT_EQUAL_MESSAGE(
        "connected and paused (one dev is initially paused)", QStringLiteral("connected, paused"), m_connection.statusText());

    // devs present
    const auto &devInfo = m_connection.devInfo();
    CPPUNIT_ASSERT_EQUAL_MESSAGE("3 devs present", 3ul, devInfo.size());
    QString ownDevId;
    for (const SyncthingDev &dev : devInfo) {
        if (dev.id != QStringLiteral("MMGUI6U-WUEZQCP-XZZ6VYB-LCT4TVC-ER2HAVX-QYT6X7D-S6ZSG2B-323KLQ7")
            && dev.id != QStringLiteral("6EIS2PN-J2IHWGS-AXS3YUL-HC5FT3K-77ZXTLL-AKQLJ4C-7SWVPUS-AZW4RQ4")) {
            CPPUNIT_ASSERT_EQUAL_MESSAGE("own device", QStringLiteral("own device"), dev.statusString());
            ownDevId = dev.id;
        }
    }
    for (const SyncthingDev &dev : devInfo) {
        if (dev.id == QStringLiteral("MMGUI6U-WUEZQCP-XZZ6VYB-LCT4TVC-ER2HAVX-QYT6X7D-S6ZSG2B-323KLQ7")) {
            CPPUNIT_ASSERT_EQUAL_MESSAGE("paused device", QStringLiteral("paused"), dev.statusString());
            CPPUNIT_ASSERT_EQUAL_MESSAGE("name", QStringLiteral("Test dev 2"), dev.name);
            CPPUNIT_ASSERT_MESSAGE("no introducer", !dev.introducer);
            CPPUNIT_ASSERT_EQUAL(1, dev.addresses.size());
            CPPUNIT_ASSERT_EQUAL(QStringLiteral("tcp://192.168.2.2:22000"), dev.addresses.front());
        } else if (dev.id == QStringLiteral("6EIS2PN-J2IHWGS-AXS3YUL-HC5FT3K-77ZXTLL-AKQLJ4C-7SWVPUS-AZW4RQ4")) {
            CPPUNIT_ASSERT_EQUAL_MESSAGE("disconnected device", QStringLiteral("disconnected"), dev.statusString());
            CPPUNIT_ASSERT_EQUAL_MESSAGE("name", QStringLiteral("Test dev 1"), dev.name);
            CPPUNIT_ASSERT_MESSAGE("introducer", dev.introducer);
            CPPUNIT_ASSERT_EQUAL(1, dev.addresses.size());
            CPPUNIT_ASSERT_EQUAL(QStringLiteral("dynamic"), dev.addresses.front());
        }
    }

    // dirs present
    const auto &dirInfo = m_connection.dirInfo();
    CPPUNIT_ASSERT_EQUAL_MESSAGE("2 dirs present", 2ul, dirInfo.size());
    const SyncthingDir &dir1 = dirInfo.front();
    CPPUNIT_ASSERT_EQUAL(QStringLiteral("test1"), dir1.id);
    CPPUNIT_ASSERT_EQUAL(QStringLiteral(""), dir1.label);
    CPPUNIT_ASSERT_EQUAL(QStringLiteral("test1"), dir1.displayName());
    CPPUNIT_ASSERT_EQUAL(QStringLiteral("/tmp/some/path/1/"), dir1.path);
    CPPUNIT_ASSERT(!dir1.readOnly);
    CPPUNIT_ASSERT(!dir1.paused);
    CPPUNIT_ASSERT_EQUAL(dir1.devices.toSet(), QSet<QString>({ QStringLiteral("MMGUI6U-WUEZQCP-XZZ6VYB-LCT4TVC-ER2HAVX-QYT6X7D-S6ZSG2B-323KLQ7"),
                                                   QStringLiteral("6EIS2PN-J2IHWGS-AXS3YUL-HC5FT3K-77ZXTLL-AKQLJ4C-7SWVPUS-AZW4RQ4"), ownDevId }));
    const SyncthingDir &dir2 = dirInfo.back();
    CPPUNIT_ASSERT_EQUAL(QStringLiteral("test2"), dir2.id);
    CPPUNIT_ASSERT_EQUAL(QStringLiteral("Test dir 2"), dir2.label);
    CPPUNIT_ASSERT_EQUAL(QStringLiteral("Test dir 2"), dir2.displayName());
    CPPUNIT_ASSERT_EQUAL(QStringLiteral("/tmp/some/path/2/"), dir2.path);
    CPPUNIT_ASSERT(!dir2.readOnly);
    CPPUNIT_ASSERT(dir2.paused);
    CPPUNIT_ASSERT_EQUAL(
        dir2.devices.toSet(), QSet<QString>({ QStringLiteral("MMGUI6U-WUEZQCP-XZZ6VYB-LCT4TVC-ER2HAVX-QYT6X7D-S6ZSG2B-323KLQ7"), ownDevId }));
    // reconnecting
    cerr << "\n - Reconnecting ..." << endl;
    waitForConnection(&SyncthingConnection::statusChanged, static_cast<void (SyncthingConnection::*)(void)>(&SyncthingConnection::reconnect));
    CPPUNIT_ASSERT_EQUAL_MESSAGE("reconnecting", QStringLiteral("reconnecting"), m_connection.statusText());
    waitForConnectionAnyAction(&SyncthingConnection::statusChanged, noop);
    if (m_connection.isConnected() && m_connection.status() != SyncthingStatus::Paused) {
        // FIXME: Maybe it takes one further update to recon paused dev?
        waitForConnectionAnyAction(&SyncthingConnection::statusChanged, noop);
    }
    CPPUNIT_ASSERT_EQUAL_MESSAGE("connected again", QStringLiteral("connected, paused"), m_connection.statusText());

    cerr << "\n - Pausing/resuming devs/dirs ..." << endl;
    // resume all devs
    bool devResumed = false;
    const function<void(const SyncthingDev &, int)> devResumedHandler = [&devResumed](const SyncthingDev &dev, int) {
        if (dev.name == QStringLiteral("Test dev 2") && !dev.paused) {
            devResumed = true;
        }
    };
    const auto newDevsConnection1 = handleNewDevices(devResumedHandler, &devResumed);
    waitForConnection(&SyncthingConnection::devStatusChanged, &SyncthingConnection::resumeAllDevs, devResumedHandler, &devResumed);
    QObject::disconnect(newDevsConnection1);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("not paused anymore", QStringLiteral("connected"), m_connection.statusText());

    // resume all dirs
    bool dirResumed = false;
    const function<void(const SyncthingDir &, int)> dirResumedHandler = [&dirResumed](const SyncthingDir &dir, int) {
        if (dir.id == QStringLiteral("test2") && !dir.paused) {
            dirResumed = true;
        }
    };
    const auto newDirsConnection1 = handleNewDirs(dirResumedHandler, &dirResumed);
    waitForConnection(&SyncthingConnection::dirStatusChanged, &SyncthingConnection::resumeAllDirs, dirResumedHandler, &dirResumed);
    QObject::disconnect(newDirsConnection1);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("still 2 dirs present", 2ul, m_connection.dirInfo().size());

    // pause dir 1
    bool dirPaused = false;
    const function<void(const SyncthingDir &, int)> dirPausedHandler = [&dirPaused](const SyncthingDir &dir, int) {
        if (dir.id == QStringLiteral("test1") && dir.paused) {
            dirPaused = true;
        }
    };
    const auto newDirsConnection2 = handleNewDirs(dirPausedHandler, &dirPaused);
    waitForConnectionAnyAction(&SyncthingConnection::dirStatusChanged,
        bind(&SyncthingConnection::pauseDirectories, &m_connection, QStringList({ QStringLiteral("test1") })), dirPausedHandler, &dirPaused);
    QObject::disconnect(newDirsConnection2);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("still 2 dirs present", 2ul, m_connection.dirInfo().size());

    // disconnecting
    cerr << "\n - Disconnecting ..." << endl;
    waitForConnection(&SyncthingConnection::statusChanged, static_cast<void (SyncthingConnection::*)(void)>(&SyncthingConnection::disconnect));
    CPPUNIT_ASSERT_EQUAL_MESSAGE("disconnected", QStringLiteral("disconnected"), m_connection.statusText());
}
