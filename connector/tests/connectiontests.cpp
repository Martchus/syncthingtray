#include "../syncthingconnection.h"

#include "../testhelper/helper.h"
#include "../testhelper/syncthingtestinstance.h"

#include <c++utilities/tests/testutils.h>

#include <cppunit/TestFixture.h>

#include <QDir>
#include <QJsonArray>
#include <QStringBuilder>
#include <QJsonDocument>

using namespace std;
using namespace Data;
using namespace TestUtilities;
using namespace TestUtilities::Literals;

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

    void testErrorCases();
    void testInitialConnection();
    void checkDevices();
    void checkDirectories() const;
    void testReconnecting();
    void testResumingAllDevices();
    void testResumingDirectory();
    void testPausingDirectory();
    void testDisconnecting();

    void setUp();
    void tearDown();

private:
    template <typename Action, typename... SignalInfos>
    void waitForConnection(Action action, int timeout, const SignalInfos &... signalInfos);
    template<typename Signal, typename Handler = function<void(void)>>
    SignalInfo<Signal, Handler> connectionSignal(Signal signal, const Handler &handler = function<void(void)>(), bool *correctSignalEmitted = nullptr);
    static void (SyncthingConnection::*defaultConnect())(void);
    static void (SyncthingConnection::*defaultReconnect())(void);
    static void (SyncthingConnection::*defaultDisconnect())(void);

    template <typename Handler> TemporaryConnection handleNewDevices(Handler handler, bool *ok);
    template <typename Handler> TemporaryConnection handleNewDirs(Handler handler, bool *ok);

    SyncthingConnection m_connection;
    QString m_ownDevId;
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

    // log configuration change
    QObject::connect(&m_connection, &SyncthingConnection::newConfig,
        [] (const QJsonObject &config) { cerr << " - New config: " << QJsonDocument(config).toJson(QJsonDocument::Compact).data() << endl; });
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
template <typename Action, typename... SignalInfos>
void ConnectionTests::waitForConnection(Action action, int timeout, const SignalInfos &...signalInfos)
{
    waitForSignals(bind(action, &m_connection), timeout, signalInfos...);
}

template<typename Signal, typename Handler>
SignalInfo<Signal, Handler> ConnectionTests::connectionSignal(Signal signal, const Handler &handler, bool *correctSignalEmitted)
{
    return SignalInfo<Signal, Handler>(&m_connection, signal, handler, correctSignalEmitted);
}

void (SyncthingConnection::*ConnectionTests::defaultConnect())(void)
{
    return static_cast<void(SyncthingConnection::*)(void)>(&SyncthingConnection::connect);
}

void (SyncthingConnection::*ConnectionTests::defaultReconnect())(void)
{
    return static_cast<void(SyncthingConnection::*)(void)>(&SyncthingConnection::reconnect);
}

void (SyncthingConnection::*ConnectionTests::defaultDisconnect())(void)
{
    return static_cast<void(SyncthingConnection::*)(void)>(&SyncthingConnection::disconnect);
}

/*!
 * \brief Helps handling newDevices() signal when waiting for device change.
 */
template <typename Handler> TemporaryConnection ConnectionTests::handleNewDevices(Handler handler, bool *ok)
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
template <typename Handler> TemporaryConnection ConnectionTests::handleNewDirs(Handler handler, bool *ok)
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
    testErrorCases();
    testInitialConnection();
    checkDevices();
    checkDirectories();
    testReconnecting();
    testResumingAllDevices();
    testResumingDirectory();
    testPausingDirectory();
    testDisconnecting();
}

void ConnectionTests::testErrorCases()
{
    cerr << "\n - Error handling in case of insufficient conficuration ..." << endl;
    waitForConnection(defaultConnect(), 1000,
                      connectionSignal(&SyncthingConnection::error, [](const QString &errorMessage) {
                        CPPUNIT_ASSERT_EQUAL(QStringLiteral("Connection configuration is insufficient."), errorMessage);
    }));

    cerr << "\n - Error handling in case of inavailability and wrong API key  ..." << endl;
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
        waitForConnection(defaultConnect(), 5000, connectionSignal(&SyncthingConnection::error, errorHandler));
    }
}

void ConnectionTests::testInitialConnection()
{
    cerr << "\n - Connecting initially ..." << endl;
    m_connection.setApiKey(apiKey().toUtf8());
    waitForConnection(defaultConnect(), 5000, connectionSignal(&SyncthingConnection::statusChanged));
    CPPUNIT_ASSERT_EQUAL_MESSAGE(
        "connected and paused (one dev is initially paused)", QStringLiteral("connected, paused"), m_connection.statusText());
    CPPUNIT_ASSERT_MESSAGE("no dirs out-of-sync", !m_connection.hasOutOfSyncDirs());
}

void ConnectionTests::checkDevices()
{
    const auto &devInfo = m_connection.devInfo();
    CPPUNIT_ASSERT_EQUAL_MESSAGE("3 devs present", 3_st, devInfo.size());
    for (const SyncthingDev &dev : devInfo) {
        if (dev.id != QStringLiteral("MMGUI6U-WUEZQCP-XZZ6VYB-LCT4TVC-ER2HAVX-QYT6X7D-S6ZSG2B-323KLQ7")
            && dev.id != QStringLiteral("6EIS2PN-J2IHWGS-AXS3YUL-HC5FT3K-77ZXTLL-AKQLJ4C-7SWVPUS-AZW4RQ4")) {
            CPPUNIT_ASSERT_EQUAL_MESSAGE("own device", QStringLiteral("own device"), dev.statusString());
            m_ownDevId = dev.id;
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
}

void ConnectionTests::checkDirectories() const
{
    const auto &dirInfo = m_connection.dirInfo();
    CPPUNIT_ASSERT_EQUAL_MESSAGE("2 dirs present", 2_st, dirInfo.size());
    const SyncthingDir &dir1 = dirInfo.front();
    CPPUNIT_ASSERT_EQUAL(QStringLiteral("test1"), dir1.id);
    CPPUNIT_ASSERT_EQUAL(QStringLiteral(""), dir1.label);
    CPPUNIT_ASSERT_EQUAL(QStringLiteral("test1"), dir1.displayName());
    CPPUNIT_ASSERT_EQUAL(QStringLiteral("/tmp/some/path/1/"), dir1.path);
    CPPUNIT_ASSERT(!dir1.readOnly);
    CPPUNIT_ASSERT(!dir1.paused);
    CPPUNIT_ASSERT_EQUAL(dir1.devices.toSet(), QSet<QString>({ QStringLiteral("MMGUI6U-WUEZQCP-XZZ6VYB-LCT4TVC-ER2HAVX-QYT6X7D-S6ZSG2B-323KLQ7"),
                                                   QStringLiteral("6EIS2PN-J2IHWGS-AXS3YUL-HC5FT3K-77ZXTLL-AKQLJ4C-7SWVPUS-AZW4RQ4"), m_ownDevId }));
    const SyncthingDir &dir2 = dirInfo.back();
    CPPUNIT_ASSERT_EQUAL(QStringLiteral("test2"), dir2.id);
    CPPUNIT_ASSERT_EQUAL(QStringLiteral("Test dir 2"), dir2.label);
    CPPUNIT_ASSERT_EQUAL(QStringLiteral("Test dir 2"), dir2.displayName());
    CPPUNIT_ASSERT_EQUAL(QStringLiteral("/tmp/some/path/2/"), dir2.path);
    CPPUNIT_ASSERT(!dir2.readOnly);
    CPPUNIT_ASSERT(dir2.paused);
    CPPUNIT_ASSERT_EQUAL(
        dir2.devices.toSet(), QSet<QString>({ QStringLiteral("MMGUI6U-WUEZQCP-XZZ6VYB-LCT4TVC-ER2HAVX-QYT6X7D-S6ZSG2B-323KLQ7"), m_ownDevId }));
}

void ConnectionTests::testReconnecting()
{
    cerr << "\n - Reconnecting ..." << endl;
    waitForConnection(defaultReconnect(), 1000, connectionSignal(&SyncthingConnection::statusChanged));
    CPPUNIT_ASSERT_EQUAL_MESSAGE("reconnecting", QStringLiteral("reconnecting"), m_connection.statusText());
    waitForSignals(noop, 1000, connectionSignal(&SyncthingConnection::statusChanged));
    if (m_connection.isConnected() && m_connection.status() != SyncthingStatus::Paused) {
        // FIXME: Maybe it takes one further update to recognize paused dev?
        waitForSignals(noop, 1000, connectionSignal(&SyncthingConnection::statusChanged));
    }
    CPPUNIT_ASSERT_EQUAL_MESSAGE("connected again", QStringLiteral("connected, paused"), m_connection.statusText());
}

void ConnectionTests::testResumingAllDevices()
{
    cerr << "\n - Resuming all devices ..." << endl;
    bool devResumed = false;
    const function<void(const SyncthingDev &, int)> devResumedHandler = [&devResumed](const SyncthingDev &dev, int) {
        if (dev.name == QStringLiteral("Test dev 2") && !dev.paused) {
            devResumed = true;
        }
    };
    const function<void(const QStringList &)> devResumedTriggeredHandler = [this](const QStringList &devIds) {
        CPPUNIT_ASSERT_EQUAL(m_connection.deviceIds(), devIds);
    };
    const auto newDevsConnection = handleNewDevices(devResumedHandler, &devResumed);
    waitForConnection(&SyncthingConnection::resumeAllDevs, 1000,
                      connectionSignal(&SyncthingConnection::devStatusChanged, devResumedHandler, &devResumed),
                      connectionSignal(&SyncthingConnection::deviceResumeTriggered, devResumedTriggeredHandler));
    CPPUNIT_ASSERT(devResumed);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("not paused anymore", QStringLiteral("connected"), m_connection.statusText());
    for (const QJsonValue &devValue : m_connection.m_rawConfig.value(QStringLiteral("devices")).toArray()) {
        const QJsonObject &devObj(devValue.toObject());
        CPPUNIT_ASSERT(!devObj.isEmpty());
        CPPUNIT_ASSERT_MESSAGE("raw config updated accordingly", !devObj.value(QStringLiteral("paused")).toBool(true));
    }
}

void ConnectionTests::testResumingDirectory()
{
    cerr << "\n - Resuming all dirs ..." << endl;
    bool dirResumed = false;
    const function<void(const SyncthingDir &, int)> dirResumedHandler = [&dirResumed](const SyncthingDir &dir, int) {
        if (dir.id == QStringLiteral("test2") && !dir.paused) {
            dirResumed = true;
        }
    };
    const auto newDirsConnection = handleNewDirs(dirResumedHandler, &dirResumed);
    waitForConnection(&SyncthingConnection::resumeAllDirs, 1000, connectionSignal(&SyncthingConnection::dirStatusChanged, dirResumedHandler, &dirResumed));
    CPPUNIT_ASSERT(dirResumed);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("still 2 dirs present", 2_st, m_connection.dirInfo().size());
    CPPUNIT_ASSERT_EQUAL_MESSAGE("still not paused anymore", QStringLiteral("connected"), m_connection.statusText());
}

void ConnectionTests::testPausingDirectory()
{
    cerr << "\n - Pause dir 1 ..." << endl;
    bool dirPaused = false;
    const function<void(const SyncthingDir &, int)> dirPausedHandler = [&dirPaused](const SyncthingDir &dir, int) {
        if (dir.id == QStringLiteral("test1") && dir.paused) {
            dirPaused = true;
        }
    };
    const auto newDirsConnection = handleNewDirs(dirPausedHandler, &dirPaused);
    waitForSignals(bind(&SyncthingConnection::pauseDirectories, &m_connection, QStringList({ QStringLiteral("test1") })), 1000,
                   connectionSignal(&SyncthingConnection::dirStatusChanged, dirPausedHandler, &dirPaused));
    CPPUNIT_ASSERT(dirPaused);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("still 2 dirs present", 2_st, m_connection.dirInfo().size());
    CPPUNIT_ASSERT_EQUAL_MESSAGE("still not paused anymore", QStringLiteral("connected"), m_connection.statusText());
}

void ConnectionTests::testDisconnecting()
{
    cerr << "\n - Disconnecting ..." << endl;
    waitForConnection(defaultDisconnect(), 1000, connectionSignal(&SyncthingConnection::statusChanged));
    CPPUNIT_ASSERT_EQUAL_MESSAGE("disconnected", QStringLiteral("disconnected"), m_connection.statusText());
}
