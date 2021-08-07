#include "../syncthingconnection.h"
#include "../syncthingconnectionsettings.h"

#include "../../testhelper/helper.h"
#include "../../testhelper/syncthingtestinstance.h"

#include <c++utilities/tests/testutils.h>

#include <cppunit/TestFixture.h>

#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QStringBuilder>

using namespace std;
using namespace Data;
using namespace CppUtilities;
using namespace CppUtilities::Literals;

using namespace CPPUNIT_NS;

class WaitForConnected : private function<void(void)>, public SignalInfo<decltype(&SyncthingConnection::statusChanged), function<void(void)>> {
public:
    WaitForConnected(const SyncthingConnection &connection);
    operator bool() const;

private:
    const SyncthingConnection &m_connection;
    bool m_connectedAgain;
};

WaitForConnected::WaitForConnected(const SyncthingConnection &connection)
    : function<void(void)>([this] { m_connectedAgain = m_connectedAgain || m_connection.isConnected(); })
    , SignalInfo<decltype(&SyncthingConnection::statusChanged), function<void(void)>>(
          &connection, &SyncthingConnection::statusChanged, (*static_cast<const function<void(void)> *>(this)), &m_connectedAgain)
    , m_connection(connection)
    , m_connectedAgain(false)
{
}

WaitForConnected::operator bool() const
{
    (*static_cast<const function<void(void)> *>(this))(); // if the connection has already been connected it is ok, too
    return m_connectedAgain;
}

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
    void testSendingError();
    void checkDevices();
    void checkDirectories() const;
    void testReconnecting();
    void testResumingAllDevices();
    void testResumingDirectory();
    void testPausingDirectory();
    void testRequestingLog();
    void testRequestingQrCode();
    void testDisconnecting();
    void testConnectingWithSettings();
    void testRequestingRescan();
    void testDealingWithArbitraryConfig();

    void setUp() override;
    void tearDown() override;

private:
    template <typename Action, typename... Signalinfo> void waitForConnection(Action action, int timeout, const Signalinfo &...signalInfo);
    template <typename Action, typename FailureSignalInfo, typename... Signalinfo>
    void waitForConnectionOrFail(Action action, int timeout, const FailureSignalInfo &failureSignalInfo, const Signalinfo &...signalInfo);
    template <typename Signal, typename Handler = function<void(void)>>
    SignalInfo<Signal, Handler> connectionSignal(
        Signal signal, const Handler &handler = function<void(void)>(), bool *correctSignalEmitted = nullptr);
    static void (SyncthingConnection::*defaultConnect())(void);
    static void (SyncthingConnection::*defaultReconnect())(void);
    static void (SyncthingConnection::*defaultDisconnect())(void);

    template <typename Handler> TemporaryConnection handleNewDevices(const Handler &handler);
    template <typename Handler> TemporaryConnection handleNewDirs(const Handler &handler);
    WaitForConnected connectedSignal() const;
    void waitForConnected(int timeout = 5000);
    void waitForAllDirsAndDevsReady(bool initialConfig = false);

    SyncthingConnection m_connection;
    QString m_ownDevId;
    QString m_ownDevName;
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
    setInterleavedOutputEnabledFromEnv();
    SyncthingTestInstance::start();

    cerr << "\n - Preparing connection ..." << endl;
    m_connection.setSyncthingUrl(QStringLiteral("http://127.0.0.1:") + syncthingPort());

    // keep track of status changes
    QObject::connect(&m_connection, &SyncthingConnection::statusChanged,
        [this] { cerr << " - Connection status changed to: " << m_connection.statusText().toLocal8Bit().data() << endl; });

    // log configuration change
    if (qEnvironmentVariableIsSet("SYNCTHING_TEST_DUMP_CONFIG_UPDATES")) {
        QObject::connect(&m_connection, &SyncthingConnection::newConfig,
            [](const QJsonObject &config) { cerr << " - New config: " << QJsonDocument(config).toJson(QJsonDocument::Indented).data() << endl; });
    }

    // log errors
    QObject::connect(&m_connection, &SyncthingConnection::error,
        [](const QString &message) { cerr << " - Connection error: " << message.toLocal8Bit().data() << endl; });

    // reduce traffic poll interval to 10 seconds
    m_connection.setTrafficPollInterval(10000);
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
 * \brief Variant of waitForSignal() where the sender is the connection and the action is a method of the connection.
 */
template <typename Action, typename... SignalInfo>
void ConnectionTests::waitForConnection(Action action, int timeout, const SignalInfo &...signalInfo)
{
    waitForSignals(bind(action, &m_connection), timeout, signalInfo...);
}

/*!
 * \brief Variant of waitForSignalOrFail() where the sender is the connection and the action is a method of the connection.
 */
template <typename Action, typename FailureSignalInfo, typename... SignalInfo>
void ConnectionTests::waitForConnectionOrFail(Action action, int timeout, const FailureSignalInfo &failureSignalInfo, const SignalInfo &...signalInfo)
{
    waitForSignalsOrFail(bind(action, &m_connection), timeout, failureSignalInfo, signalInfo...);
}

/*!
 * \brief Returns a SignalInfo for the test's connection.
 */
template <typename Signal, typename Handler>
SignalInfo<Signal, Handler> ConnectionTests::connectionSignal(Signal signal, const Handler &handler, bool *correctSignalEmitted)
{
    return SignalInfo<Signal, Handler>(&m_connection, signal, handler, correctSignalEmitted);
}

/*!
 * \brief Returns the default connect() signal (no args) for the test's connection.
 */
void (SyncthingConnection::*ConnectionTests::defaultConnect())(void)
{
    return static_cast<void (SyncthingConnection::*)(void)>(&SyncthingConnection::connect);
}

/*!
 * \brief Returns the default reconnect() signal (no args) for the test's connection.
 */
void (SyncthingConnection::*ConnectionTests::defaultReconnect())(void)
{
    return static_cast<void (SyncthingConnection::*)(void)>(&SyncthingConnection::reconnect);
}

/*!
 * \brief Returns the default disconnect() signal (no args) for the test's connection.
 */
void (SyncthingConnection::*ConnectionTests::defaultDisconnect())(void)
{
    return static_cast<void (SyncthingConnection::*)(void)>(&SyncthingConnection::disconnect);
}

/*!
 * \brief Returns a SignalInfo to wait until the connected (again).
 */
WaitForConnected ConnectionTests::connectedSignal() const
{
    return WaitForConnected(m_connection);
}

/*!
 * \brief Waits until connected (again).
 * \remarks
 * - Does nothing if already connected.
 * - Used to keep tests passing even though Syncthing dies and restarts during the testrun.
 */
void ConnectionTests::waitForConnected(int timeout)
{
    waitForConnection(defaultConnect(), timeout, connectedSignal());
}

/*!
 * \brief Ensures the connection is established and waits till all dirs and devs are ready.
 * \param initialConfig Whether to check for initial config (at least one dir and one dev is paused).
 */
void ConnectionTests::waitForAllDirsAndDevsReady(const bool initialConfig)
{
    bool allDirsReady, allDevsReady;
    bool isConnected = m_connection.isConnected();
    const auto checkAllDirsReady([this, &allDirsReady, &initialConfig] {
        bool oneDirPaused = false;
        for (const SyncthingDir &dir : m_connection.dirInfo()) {
            if (dir.status == SyncthingDirStatus::Unknown && !dir.paused) {
                allDirsReady = false;
                return;
            }
            oneDirPaused = oneDirPaused || dir.paused;
        }
        allDirsReady = !initialConfig || oneDirPaused;
    });
    const auto checkAllDevsReady([this, &allDevsReady, &initialConfig] {
        bool oneDevPaused = false;
        for (const SyncthingDev &dev : m_connection.devInfo()) {
            if (dev.status == SyncthingDevStatus::Unknown && !dev.paused) {
                allDevsReady = false;
                return;
            }
            oneDevPaused = oneDevPaused || dev.paused;
        }
        allDevsReady = !initialConfig || oneDevPaused;
    });
    auto checkStatus([this, &isConnected](SyncthingStatus) { isConnected = m_connection.isConnected(); });
    checkAllDirsReady();
    checkAllDevsReady();
    if (allDirsReady && allDevsReady) {
        return;
    }

    waitForSignalsOrFail(bind(defaultConnect(), &m_connection), 10000, connectionSignal(&SyncthingConnection::error),
        connectionSignal(&SyncthingConnection::statusChanged, checkStatus, &isConnected),
        connectionSignal(&SyncthingConnection::dirStatusChanged, checkAllDirsReady, &allDirsReady),
        connectionSignal(&SyncthingConnection::newDirs, checkAllDirsReady, &allDirsReady),
        connectionSignal(&SyncthingConnection::devStatusChanged, checkAllDevsReady, &allDevsReady),
        connectionSignal(&SyncthingConnection::newDevices, checkAllDevsReady, &allDevsReady));
}

/*!
 * \brief Helps handling newDevices() signal when waiting for device change.
 */
template <typename Handler> TemporaryConnection ConnectionTests::handleNewDevices(const Handler &handler)
{
    return QObject::connect(&m_connection, &SyncthingConnection::newDevices, [&handler](const std::vector<SyncthingDev> &devs) {
        for (const SyncthingDev &dev : devs) {
            handler(dev, 0);
        }
    });
}

/*!
 * \brief Helps handling newDirs() signal when waiting for directory change.
 */
template <typename Handler> TemporaryConnection ConnectionTests::handleNewDirs(const Handler &handler)
{
    return QObject::connect(&m_connection, &SyncthingConnection::newDirs, [&handler](const std::vector<SyncthingDir> &dirs) {
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
 * \remarks Some tests are currently disabled for release mode because they sometimes fail.
 * \todo Find out why some tests are flaky.
 */
void ConnectionTests::testConnection()
{
    testErrorCases();
    testInitialConnection();
    checkDevices();
    checkDirectories();
    testSendingError();
    testReconnecting();
    testResumingAllDevices();
    testResumingDirectory();
    testPausingDirectory();
    testRequestingLog();
    testRequestingQrCode();
    testDisconnecting();
    testConnectingWithSettings();
    testRequestingRescan();
    testDealingWithArbitraryConfig();
}

void ConnectionTests::testErrorCases()
{
    cerr << "\n - Error handling in case of insufficient configuration ..." << endl;
    waitForConnection(defaultConnect(), 1000, connectionSignal(&SyncthingConnection::error, [](const QString &errorMessage) {
        CPPUNIT_ASSERT_EQUAL(QStringLiteral("Connection configuration is insufficient."), errorMessage);
    }));

    // setup/define test for error handling
    m_connection.setApiKey(QByteArray("wrong API key"));
    bool syncthingAvailable = false;
    constexpr auto syncthingCheckInterval = TimeSpan::fromMilliseconds(200.0);
    const auto maxSyncthingStartupTime = TimeSpan::fromSeconds(15.0 * max(timeoutFactor, 5.0));
    auto remainingTimeForSyncthingToComeUp = maxSyncthingStartupTime;
    bool authErrorStatus = false, authErrorConfig = false;
    bool apiKeyErrorStatus = false, apiKeyErrorConfig = false;
    bool allErrorsEmitted = false;
    const auto errorHandler = [&](const QString &errorMessage) {
        // check whether Syncthing is available
        if ((errorMessage == QStringLiteral("Unable to request Syncthing status: Connection refused"))
            || (errorMessage == QStringLiteral("Unable to request Syncthing config: Connection refused"))) {
            // consider test failed if we receive "Connection refused" when another error has already occurred
            if (syncthingAvailable) {
                CPPUNIT_FAIL("Syncthing became unavailable after another error had already occurred");
            }

            // consider test failed if Syncthing takes too long to come up (or we fail to connect)
            if ((remainingTimeForSyncthingToComeUp -= syncthingCheckInterval).isNegative()) {
                CPPUNIT_FAIL(
                    argsToString("unable to connect to Syncthing within ", maxSyncthingStartupTime.toString(TimeSpanOutputFormat::WithMeasures)));
            }

            // give Syncthing a bit more time and check again
            wait(static_cast<int>(syncthingCheckInterval.totalMilliseconds()));
            return;
        }
        syncthingAvailable = true;

        // check for HTTP authentication error
        if (errorMessage == QStringLiteral("Unable to request Syncthing status: Host requires authentication")) {
            authErrorStatus = true;
            return;
        }
        if (errorMessage == QStringLiteral("Unable to request Syncthing config: Host requires authentication")) {
            authErrorConfig = true;
            return;
        }

        // check API key error
        if ((errorMessage.startsWith(QStringLiteral("Unable to request Syncthing status: Error transferring "))
                && errorMessage.endsWith(QStringLiteral("/rest/system/status - server replied: Forbidden")))) {
            m_connection.setApiKey(apiKey().toUtf8());
            apiKeyErrorStatus = true;
            return;
        }
        if ((errorMessage.startsWith(QStringLiteral("Unable to request Syncthing config: Error transferring "))
                && errorMessage.endsWith(QStringLiteral("/rest/system/config - server replied: Forbidden")))) {
            apiKeyErrorConfig = true;
            return;
        }

        // fail on unexpected error messages
        allErrorsEmitted = authErrorStatus && authErrorConfig && apiKeyErrorStatus && apiKeyErrorConfig;
        CPPUNIT_FAIL(argsToString("wrong error message: ", errorMessage.toLocal8Bit().data()));
    };

    cerr << "\n - Error handling in case of inavailability ..." << endl;
    while (!syncthingAvailable) {
        waitForConnection(defaultConnect(), 5000, connectionSignal(&SyncthingConnection::error, errorHandler));
    }

    cerr << "\n - Error handling in case of wrong credentials ..." << endl;
    waitForConnection(defaultConnect(), 5000, connectionSignal(&SyncthingConnection::error, errorHandler));
    while (!authErrorStatus && !authErrorConfig) {
        waitForSignals(noop, 5000, connectionSignal(&SyncthingConnection::error, errorHandler));
    }

    cerr << "\n - Error handling in case of wrong API key  ..." << endl;
    m_connection.setCredentials(QStringLiteral("nobody"), QStringLiteral("supersecret"));
    waitForConnection(defaultConnect(), 5000, connectionSignal(&SyncthingConnection::error, errorHandler));
    while (!apiKeyErrorStatus && !apiKeyErrorConfig) {
        waitForSignals(noop, 5000, connectionSignal(&SyncthingConnection::error, errorHandler));
    }
}

void ConnectionTests::testInitialConnection()
{
    cerr << "\n - Connecting initially ..." << endl;
    m_connection.setApiKey(apiKey().toUtf8());
    waitForAllDirsAndDevsReady(true);
    CPPUNIT_ASSERT_EQUAL_MESSAGE(
        "connected and paused (one dev is initially paused)", QStringLiteral("connected, paused"), m_connection.statusText());
    CPPUNIT_ASSERT_MESSAGE("no dirs out-of-sync", !m_connection.hasOutOfSyncDirs());
}

void ConnectionTests::testSendingError()
{
    auto newNotificationEmitted = false;
    const auto sentTime(DateTime::now());
    const auto sentMessage(QStringLiteral("test notification"));
    const auto newNotificationHandler = [&](DateTime receivedTime, const QString &receivedMessage) {
        newNotificationEmitted |= receivedTime == sentTime && receivedMessage == sentMessage;
    };
    waitForSignals([this, sentTime, &sentMessage] { m_connection.emitNotification(sentTime, sentMessage); }, 500,
        connectionSignal(&SyncthingConnection::newNotification, newNotificationHandler, &newNotificationEmitted));
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
            m_ownDevName = dev.name;
        }
    }
    const SyncthingDev *dev1 = nullptr, *dev2 = nullptr;
    int index = 0, dev1Index, dev2Index;
    for (const SyncthingDev &dev : devInfo) {
        CPPUNIT_ASSERT(!dev.isConnected());
        if (dev.id == QStringLiteral("MMGUI6U-WUEZQCP-XZZ6VYB-LCT4TVC-ER2HAVX-QYT6X7D-S6ZSG2B-323KLQ7")) {
            CPPUNIT_ASSERT_EQUAL_MESSAGE("paused device", QStringLiteral("paused"), dev.statusString());
            CPPUNIT_ASSERT_EQUAL_MESSAGE("name", QStringLiteral("Test dev 2"), dev.name);
            CPPUNIT_ASSERT_MESSAGE("no introducer", !dev.introducer);
            CPPUNIT_ASSERT_EQUAL(static_cast<decltype(dev.addresses.size())>(1), dev.addresses.size());
            CPPUNIT_ASSERT_EQUAL(QStringLiteral("tcp://192.168.2.2:22001"), dev.addresses.front());
            dev2 = &dev;
            dev2Index = index;
        } else if (dev.id == QStringLiteral("6EIS2PN-J2IHWGS-AXS3YUL-HC5FT3K-77ZXTLL-AKQLJ4C-7SWVPUS-AZW4RQ4")) {
            CPPUNIT_ASSERT_EQUAL_MESSAGE("disconnected device", QStringLiteral("disconnected"), dev.statusString());
            CPPUNIT_ASSERT_EQUAL_MESSAGE("name", QStringLiteral("Test dev 1"), dev.name);
            CPPUNIT_ASSERT_MESSAGE("introducer", dev.introducer);
            CPPUNIT_ASSERT_EQUAL(static_cast<decltype(dev.addresses.size())>(1), dev.addresses.size());
            CPPUNIT_ASSERT_EQUAL(QStringLiteral("dynamic"), dev.addresses.front());
            dev1 = &dev;
            dev1Index = index;
        }
        ++index;
    }

    CPPUNIT_ASSERT(dev1 && dev2);
    CPPUNIT_ASSERT(dev1 == m_connection.findDevInfo(QStringLiteral("6EIS2PN-J2IHWGS-AXS3YUL-HC5FT3K-77ZXTLL-AKQLJ4C-7SWVPUS-AZW4RQ4"), index));
    CPPUNIT_ASSERT_EQUAL(dev1Index, index);
    CPPUNIT_ASSERT(dev2 == m_connection.findDevInfoByName(QStringLiteral("Test dev 2"), index));
    CPPUNIT_ASSERT_EQUAL(dev2Index, index);
    CPPUNIT_ASSERT(!m_connection.findDevInfoByName(QStringLiteral("does not exist"), index));
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
    CPPUNIT_ASSERT_EQUAL(QStringLiteral("idle"), dir1.statusString());
    CPPUNIT_ASSERT_EQUAL(SyncthingDirType::SendReceive, dir1.dirType);
    CPPUNIT_ASSERT(!dir1.paused);
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
    const auto devIds = dir1.deviceIds.toSet();
    const auto devNames = dir1.deviceNames.toSet();
#else
    const auto devIds = QSet(dir1.deviceIds.begin(), dir1.deviceIds.end());
    const auto devNames = QSet(dir1.deviceNames.begin(), dir1.deviceNames.end());
#endif
    CPPUNIT_ASSERT_EQUAL(QSet<QString>({ QStringLiteral("MMGUI6U-WUEZQCP-XZZ6VYB-LCT4TVC-ER2HAVX-QYT6X7D-S6ZSG2B-323KLQ7"),
                             QStringLiteral("6EIS2PN-J2IHWGS-AXS3YUL-HC5FT3K-77ZXTLL-AKQLJ4C-7SWVPUS-AZW4RQ4") }),
        devIds);
    CPPUNIT_ASSERT_EQUAL(QSet<QString>({ QStringLiteral("Test dev 2"), QStringLiteral("Test dev 1") }), devNames);
    const SyncthingDir &dir2 = dirInfo.back();
    CPPUNIT_ASSERT_EQUAL(QStringLiteral("test2"), dir2.id);
    CPPUNIT_ASSERT_EQUAL(QStringLiteral("Test dir 2"), dir2.label);
    CPPUNIT_ASSERT_EQUAL(QStringLiteral("Test dir 2"), dir2.displayName());
    CPPUNIT_ASSERT_EQUAL(QStringLiteral("/tmp/some/path/2/"), dir2.path);
    CPPUNIT_ASSERT_EQUAL(QStringLiteral("/tmp/some/path/2"), dir2.pathWithoutTrailingSlash().toString());
    CPPUNIT_ASSERT_EQUAL(QStringLiteral("paused"), dir2.statusString());
    CPPUNIT_ASSERT_EQUAL(SyncthingDirType::SendReceive, dir2.dirType);
    CPPUNIT_ASSERT(dir2.paused);
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
    const auto devIds2 = dir2.deviceIds.toSet();
    const auto devNames2 = dir2.deviceNames.toSet();
#else
    const auto devIds2 = QSet(dir2.deviceIds.begin(), dir2.deviceIds.end());
    const auto devNames2 = QSet(dir2.deviceNames.begin(), dir2.deviceNames.end());
#endif
    CPPUNIT_ASSERT_EQUAL(QSet<QString>({ QStringLiteral("MMGUI6U-WUEZQCP-XZZ6VYB-LCT4TVC-ER2HAVX-QYT6X7D-S6ZSG2B-323KLQ7") }), devIds2);
    CPPUNIT_ASSERT_EQUAL(QSet<QString>({ QStringLiteral("Test dev 2") }), devNames2);
}

void ConnectionTests::testReconnecting()
{
    cerr << "\n - Reconnecting ...\n";
    waitForConnection(defaultReconnect(), 1000, connectionSignal(&SyncthingConnection::statusChanged));
    cerr << "\n - Waiting for dirs/devs after reconnect ...\n";
    waitForAllDirsAndDevsReady(true);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("connected again", QStringLiteral("connected, paused"), m_connection.statusText());
}

void ConnectionTests::testResumingAllDevices()
{
    cerr << "\n - Resuming all devices ..." << endl;
    bool devResumed = false;
    const auto devResumedHandler = [&devResumed](const SyncthingDev &dev, int) {
        if (dev.name == QStringLiteral("Test dev 2") && !dev.paused) {
            devResumed = true;
        }
    };
    const auto newDevsHandler = [&devResumedHandler](const std::vector<SyncthingDev> &devs) {
        for (const auto &dev : devs) {
            devResumedHandler(dev, 0);
        }
    };
    const auto devResumedTriggeredHandler = [this](const QStringList &devIds) { CPPUNIT_ASSERT_EQUAL(m_connection.deviceIds(), devIds); };
    const auto newDevsConnection = handleNewDevices(devResumedHandler);
    waitForConnected();
    waitForConnection(&SyncthingConnection::resumeAllDevs, 7500, connectedSignal(),
        connectionSignal(&SyncthingConnection::devStatusChanged, devResumedHandler, &devResumed),
        connectionSignal(&SyncthingConnection::newDevices, newDevsHandler, &devResumed),
        connectionSignal(&SyncthingConnection::deviceResumeTriggered, devResumedTriggeredHandler));
    CPPUNIT_ASSERT(devResumed);
    for (const QJsonValueRef devValue : m_connection.m_rawConfig.value(QStringLiteral("devices")).toArray()) {
        const QJsonObject &devObj(devValue.toObject());
        CPPUNIT_ASSERT(!devObj.isEmpty());
        CPPUNIT_ASSERT_MESSAGE("raw config updated accordingly", !devObj.value(QStringLiteral("paused")).toBool(true));
    }
    CPPUNIT_ASSERT_MESSAGE("resuming all devs should not cause another request again", !m_connection.resumeAllDevs());
}

void ConnectionTests::testResumingDirectory()
{
    cerr << "\n - Resuming all dirs ..." << endl;
    bool dirResumed = false;
    const auto dirResumedHandler = [&dirResumed](const SyncthingDir &dir, int) {
        if (dir.id == QStringLiteral("test2") && !dir.paused) {
            dirResumed = true;
        }
    };
    const auto newDirsHandler = [&dirResumedHandler](const std::vector<SyncthingDir> &dirs) {
        for (const auto &dir : dirs) {
            dirResumedHandler(dir, 0);
        }
    };
    const auto dirResumedTriggeredHandler = [this](const QStringList &devIds) { CPPUNIT_ASSERT_EQUAL(m_connection.directoryIds(), devIds); };
    const auto newDirsConnection = handleNewDirs(dirResumedHandler);
    waitForConnected();
    waitForConnection(&SyncthingConnection::resumeAllDirs, 7500, connectedSignal(),
        connectionSignal(&SyncthingConnection::dirStatusChanged, dirResumedHandler, &dirResumed),
        connectionSignal(&SyncthingConnection::newDirs, newDirsHandler, &dirResumed),
        connectionSignal(&SyncthingConnection::directoryResumeTriggered, dirResumedTriggeredHandler));
    CPPUNIT_ASSERT(dirResumed);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("still 2 dirs present", 2_st, m_connection.dirInfo().size());
    CPPUNIT_ASSERT_MESSAGE("resuming all dirs should not cause another request again", !m_connection.resumeAllDirs());
}

void ConnectionTests::testPausingDirectory()
{
    cerr << "\n - Pause dir 1 ..." << endl;
    bool dirPaused = false;
    const auto dirPausedHandler = [&dirPaused](const SyncthingDir &dir, int) {
        if (dir.id == QStringLiteral("test1") && dir.paused) {
            dirPaused = true;
        }
    };
    const QStringList ids({ QStringLiteral("test1") });
    const auto dirPausedTriggeredHandler = [&ids](const QStringList &devIds) { CPPUNIT_ASSERT_EQUAL(ids, devIds); };
    const auto newDirsConnection = handleNewDirs(dirPausedHandler);
    waitForConnected();
    waitForSignals(bind(&SyncthingConnection::pauseDirectories, &m_connection, ids), 7500, connectedSignal(),
        connectionSignal(&SyncthingConnection::dirStatusChanged, dirPausedHandler, &dirPaused),
        connectionSignal(&SyncthingConnection::directoryPauseTriggered, dirPausedTriggeredHandler));
    CPPUNIT_ASSERT(dirPaused);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("still 2 dirs present", 2_st, m_connection.dirInfo().size());
    CPPUNIT_ASSERT_MESSAGE("pausing should not cause another request again", !m_connection.pauseDirectories(ids));
}

void ConnectionTests::testRequestingLog()
{
    cerr << "\n - Requesting log ..." << endl;
    waitForConnected();

    const auto handleLogAvailable = [](const vector<SyncthingLogEntry> &logEntries) {
        CPPUNIT_ASSERT(!logEntries.empty());
        CPPUNIT_ASSERT(!logEntries[0].when.isEmpty());
        CPPUNIT_ASSERT(!logEntries[0].message.isEmpty());
    };
    waitForConnectionOrFail(&SyncthingConnection::requestLog, 5000, connectionSignal(&SyncthingConnection::error),
        connectionSignal(&SyncthingConnection::logAvailable, handleLogAvailable));
}

void ConnectionTests::testRequestingQrCode()
{
    cerr << "\n - Requesting QR-Code for own device ID ..." << endl;
    waitForConnected();

    const auto handleQrCodeAvailable = [](const QString &qrText, const QByteArray &data) {
        CPPUNIT_ASSERT_EQUAL(QStringLiteral("some text"), qrText);
        CPPUNIT_ASSERT(!data.isEmpty());
    };
    waitForSignalsOrFail(bind(&SyncthingConnection::requestQrCode, &m_connection, QStringLiteral("some text")), 5000,
        connectionSignal(&SyncthingConnection::error), connectionSignal(&SyncthingConnection::qrCodeAvailable, handleQrCodeAvailable));
}

void ConnectionTests::testDisconnecting()
{
    cerr << "\n - Disconnecting while there are outstanding requests ..." << endl;
    waitForConnected();
    m_connection.requestVersion();
    m_connection.requestDirStatistics();
    waitForSignals(
        [this] {
            m_connection.requestDeviceStatistics();
            m_connection.requestCompletion("MMGUI6U-WUEZQCP-XZZ6VYB-LCT4TVC-ER2HAVX-QYT6X7D-S6ZSG2B-323KLQ7", "test1");
            QTimer::singleShot(0, &m_connection, defaultDisconnect());
        },
        5000, connectionSignal(&SyncthingConnection::statusChanged));
    CPPUNIT_ASSERT_EQUAL_MESSAGE("disconnected", QStringLiteral("disconnected"), m_connection.statusText());
}

void ConnectionTests::testConnectingWithSettings()
{
    cerr << "\n - Connecting with settings ..." << endl;
    SyncthingConnectionSettings settings;
    settings.syncthingUrl = m_connection.syncthingUrl();
    settings.apiKey = m_connection.apiKey();
    settings.userName = m_connection.user();
    settings.password = m_connection.password();

    bool isConnected;
    const auto checkStatus([this, &isConnected](SyncthingStatus) { isConnected = m_connection.isConnected(); });
    waitForSignals(
        bind(static_cast<void (SyncthingConnection::*)(SyncthingConnectionSettings &)>(&SyncthingConnection::connect), &m_connection, ref(settings)),
        5000, connectionSignal(&SyncthingConnection::statusChanged, checkStatus, &isConnected));
}

void ConnectionTests::testRequestingRescan()
{
    cerr << "\n - Requesting rescan ..." << endl;
    waitForConnected();

    bool rescanTriggered = false;
    const auto rescanTriggeredHandler = [&rescanTriggered](const QString &dir) {
        CPPUNIT_ASSERT_EQUAL(QStringLiteral("test2"), dir);
        rescanTriggered = true;
    };
    waitForSignalsOrFail(bind(&SyncthingConnection::rescanAllDirs, &m_connection), 5000, connectionSignal(&SyncthingConnection::error),
        connectionSignal(&SyncthingConnection::rescanTriggered, rescanTriggeredHandler, &rescanTriggered));

    bool errorOccured = false;
    const auto errorHandler = [&errorOccured](const QString &message) {
        errorOccured |= message.startsWith(QStringLiteral("Unable to request rescan: Error transferring"))
            && message.endsWith(QStringLiteral("/rest/db/scan?folder=non-existing-dir&sub=sub/path - server replied: Internal Server Error"));
    };
    waitForSignals(bind(&SyncthingConnection::rescan, &m_connection, QStringLiteral("non-existing-dir"), QStringLiteral("sub/path")), 5000,
        connectionSignal(&SyncthingConnection::error, errorHandler, &errorOccured));
}

void ConnectionTests::testDealingWithArbitraryConfig()
{
    cerr << "\n - Changing arbitrary config ..." << endl;
    waitForConnected();

    // read some value, eg. options.relayReconnectIntervalM
    auto rawConfig(m_connection.rawConfig());
    auto optionsIterator(rawConfig.find(QLatin1String("options")));
    CPPUNIT_ASSERT(optionsIterator != rawConfig.end());
    auto optionsRef(optionsIterator.value());
    CPPUNIT_ASSERT_EQUAL(QJsonValue::Object, optionsRef.type());
    auto options(optionsRef.toObject());
    CPPUNIT_ASSERT_EQUAL(10, options.value(QLatin1String("relayReconnectIntervalM")).toInt());

    // change a value
    options.insert(QLatin1String("relayReconnectIntervalM"), 75);
    optionsRef = options;

    // expect the change via newConfig() signal
    bool hasNewConfig = false;
    const auto handleNewConfig([&hasNewConfig](const QJsonObject &newConfig) {
        const auto newIntervall(newConfig.value(QLatin1String("options")).toObject().value(QLatin1String("relayReconnectIntervalM")).toInt());
        if (newIntervall == 75) {
            hasNewConfig = true;
        }
    });

    // post new config
    waitForConnected();
    waitForSignalsOrFail(bind(&SyncthingConnection::postConfigFromJsonObject, &m_connection, ref(rawConfig)), 10000,
        connectionSignal(&SyncthingConnection::error), connectionSignal(&SyncthingConnection::newConfigTriggered),
        connectionSignal(&SyncthingConnection::newConfig, handleNewConfig, &hasNewConfig));
}
