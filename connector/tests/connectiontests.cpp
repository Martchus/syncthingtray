#include "../syncthingconnection.h"
#include "../syncthingconnectionsettings.h"

#include "../testhelper/helper.h"
#include "../testhelper/syncthingtestinstance.h"

#include <c++utilities/tests/testutils.h>

#include <cppunit/TestFixture.h>

#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QStringBuilder>

using namespace std;
using namespace Data;
using namespace ChronoUtilities;
using namespace TestUtilities;
using namespace TestUtilities::Literals;

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
    : function<void(void)>([this] {
        m_connectedAgain
            |= m_connection.statusText() == QStringLiteral("connected") || m_connection.statusText() == QStringLiteral("connected, scanning");
    })
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
    void waitForAllDirsAndDevsReady(bool initialConfig = false);
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

    void setUp();
    void tearDown();

private:
    template <typename Action, typename... SignalInfos> void waitForConnection(Action action, int timeout, const SignalInfos &... signalInfos);
    template <typename Signal, typename Handler = function<void(void)>>
    SignalInfo<Signal, Handler> connectionSignal(
        Signal signal, const Handler &handler = function<void(void)>(), bool *correctSignalEmitted = nullptr);
    static void (SyncthingConnection::*defaultConnect())(void);
    static void (SyncthingConnection::*defaultReconnect())(void);
    static void (SyncthingConnection::*defaultDisconnect())(void);

    template <typename Handler> TemporaryConnection handleNewDevices(Handler handler, bool *ok);
    template <typename Handler> TemporaryConnection handleNewDirs(Handler handler, bool *ok);
    WaitForConnected waitForConnected() const;

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
    SyncthingTestInstance::start();

    cerr << "\n - Preparing connection ..." << endl;
    m_connection.setSyncthingUrl(QStringLiteral("http://localhost:") + syncthingPort());

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
void ConnectionTests::waitForConnection(Action action, int timeout, const SignalInfos &... signalInfos)
{
    waitForSignals(bind(action, &m_connection), timeout, signalInfos...);
}

template <typename Signal, typename Handler>
SignalInfo<Signal, Handler> ConnectionTests::connectionSignal(Signal signal, const Handler &handler, bool *correctSignalEmitted)
{
    return SignalInfo<Signal, Handler>(&m_connection, signal, handler, correctSignalEmitted);
}

void (SyncthingConnection::*ConnectionTests::defaultConnect())(void)
{
    return static_cast<void (SyncthingConnection::*)(void)>(&SyncthingConnection::connect);
}

void (SyncthingConnection::*ConnectionTests::defaultReconnect())(void)
{
    return static_cast<void (SyncthingConnection::*)(void)>(&SyncthingConnection::reconnect);
}

void (SyncthingConnection::*ConnectionTests::defaultDisconnect())(void)
{
    return static_cast<void (SyncthingConnection::*)(void)>(&SyncthingConnection::disconnect);
}

WaitForConnected ConnectionTests::waitForConnected() const
{
    return WaitForConnected(m_connection);
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
}

void ConnectionTests::testErrorCases()
{
    cerr << "\n - Error handling in case of insufficient conficuration ..." << endl;
    waitForConnection(defaultConnect(), 1000, connectionSignal(&SyncthingConnection::error, [](const QString &errorMessage) {
        CPPUNIT_ASSERT_EQUAL(QStringLiteral("Connection configuration is insufficient."), errorMessage);
    }));

    cerr << "\n - Error handling in case of inavailability, wrong credentials and API key  ..." << endl;
    m_connection.setApiKey(QByteArray("wrong API key"));
    bool syncthingAvailable = false, authError = false, apiKeyError = false;
    const function<void(const QString &errorMessage)> errorHandler
        = [this, &syncthingAvailable, &authError, &apiKeyError](const QString &errorMessage) {
              if (errorMessage == QStringLiteral("Unable to request Syncthing config: Connection refused")
                  || errorMessage == QStringLiteral("Unable to request Syncthing status: Connection refused")) {
                  // Syncthing not ready yet, wait 100 ms till next connection attempt
                  wait(100);
                  return;
              }
              syncthingAvailable = true;
              if (errorMessage == QStringLiteral("Unable to request Syncthing status: Host requires authentication")
                  || errorMessage == QStringLiteral("Unable to request Syncthing config: Host requires authentication")) {
                  m_connection.setCredentials(QStringLiteral("nobody"), QStringLiteral("supersecret"));
                  authError = true;
                  return;
              }
              if ((errorMessage.startsWith(QStringLiteral("Unable to request Syncthing status: Error transferring "))
                      && errorMessage.endsWith(QStringLiteral("/rest/system/status - server replied: Forbidden")))
                  || (errorMessage.startsWith(QStringLiteral("Unable to request Syncthing config: Error transferring "))
                         && errorMessage.endsWith(QStringLiteral("/rest/system/config - server replied: Forbidden")))) {
                  m_connection.setApiKey(apiKey().toUtf8());
                  apiKeyError = true;
                  return;
              }
              CPPUNIT_FAIL(argsToString("wrong error message: ", errorMessage.toLocal8Bit().data()));
          };
    while (!syncthingAvailable || !authError || !apiKeyError) {
        waitForConnection(defaultConnect(), 5000, connectionSignal(&SyncthingConnection::error, errorHandler));
    }
}

void ConnectionTests::testInitialConnection()
{
    cerr << "\n - Connecting initially ..." << endl;
    waitForAllDirsAndDevsReady(true);
    CPPUNIT_ASSERT_EQUAL_MESSAGE(
        "connected and paused (one dev is initially paused)", QStringLiteral("connected, paused"), m_connection.statusText());
    CPPUNIT_ASSERT_MESSAGE("no dirs out-of-sync", !m_connection.hasOutOfSyncDirs());
}

void ConnectionTests::testSendingError()
{
    bool newNotificationEmitted = false;
    const DateTime sentTime(DateTime::now());
    const QString sentMessage(QStringLiteral("test notification"));
    const function<void(ChronoUtilities::DateTime receivedTime, const QString &receivedMessage)> newNotificationHandler
        = [&](ChronoUtilities::DateTime receivedTime, const QString &receivedMessage) {
              newNotificationEmitted |= receivedTime == sentTime && receivedMessage == sentMessage;
          };
    waitForSignals([this, sentTime, &sentMessage] { m_connection.emitNotification(sentTime, sentMessage); }, 500,
        connectionSignal(&SyncthingConnection::newNotification, newNotificationHandler, &newNotificationEmitted));
}

/*!
 * \brief Ensures the connection is established and waits till all dirs and devs are ready.
 * \param initialConfig Whether to check for initial config (at least one dir and one dev is paused).
 */
void ConnectionTests::waitForAllDirsAndDevsReady(const bool initialConfig)
{
    bool allDirsReady, allDevsReady;
    bool oneDirPaused = false, oneDevPaused = false;
    bool isConnected = m_connection.isConnected();
    const function<void()> checkAllDirsReady([this, &allDirsReady, &initialConfig, &oneDirPaused] {
        for (const SyncthingDir &dir : m_connection.dirInfo()) {
            if (dir.status == SyncthingDirStatus::Unknown) {
                allDirsReady = false;
                return;
            }
            oneDirPaused |= dir.paused;
        }
        allDirsReady = !initialConfig || oneDirPaused;
    });
    const function<void()> checkAllDevsReady([this, &allDevsReady, &initialConfig, &oneDevPaused] {
        for (const SyncthingDev &dev : m_connection.devInfo()) {
            if (dev.status == SyncthingDevStatus::Unknown) {
                allDevsReady = false;
                return;
            }
            oneDevPaused |= dev.paused;
        }
        allDevsReady = !initialConfig || oneDevPaused;
    });
    const function<void(SyncthingStatus)> checkStatus([this, &isConnected](SyncthingStatus) { isConnected = m_connection.isConnected(); });
    checkAllDirsReady();
    checkAllDevsReady();
    if (allDirsReady && allDevsReady) {
        return;
    }
    waitForSignals(bind(defaultConnect(), &m_connection), 5000, connectionSignal(&SyncthingConnection::statusChanged, checkStatus, &isConnected),
        connectionSignal(&SyncthingConnection::dirStatusChanged, checkAllDirsReady, &allDirsReady),
        connectionSignal(&SyncthingConnection::newDirs, checkAllDirsReady, &allDirsReady),
        connectionSignal(&SyncthingConnection::devStatusChanged, checkAllDevsReady, &allDevsReady),
        connectionSignal(&SyncthingConnection::newDevices, checkAllDevsReady, &allDevsReady));
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
            CPPUNIT_ASSERT_EQUAL(1, dev.addresses.size());
            CPPUNIT_ASSERT_EQUAL(QStringLiteral("tcp://192.168.2.2:22000"), dev.addresses.front());
            dev2 = &dev;
            dev2Index = index;
        } else if (dev.id == QStringLiteral("6EIS2PN-J2IHWGS-AXS3YUL-HC5FT3K-77ZXTLL-AKQLJ4C-7SWVPUS-AZW4RQ4")) {
            CPPUNIT_ASSERT_EQUAL_MESSAGE("disconnected device", QStringLiteral("disconnected"), dev.statusString());
            CPPUNIT_ASSERT_EQUAL_MESSAGE("name", QStringLiteral("Test dev 1"), dev.name);
            CPPUNIT_ASSERT_MESSAGE("introducer", dev.introducer);
            CPPUNIT_ASSERT_EQUAL(1, dev.addresses.size());
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
    CPPUNIT_ASSERT(!dir1.readOnly);
    CPPUNIT_ASSERT(!dir1.paused);
    CPPUNIT_ASSERT_EQUAL(QSet<QString>({ QStringLiteral("MMGUI6U-WUEZQCP-XZZ6VYB-LCT4TVC-ER2HAVX-QYT6X7D-S6ZSG2B-323KLQ7"),
                             QStringLiteral("6EIS2PN-J2IHWGS-AXS3YUL-HC5FT3K-77ZXTLL-AKQLJ4C-7SWVPUS-AZW4RQ4") }),
        dir1.deviceIds.toSet());
    CPPUNIT_ASSERT_EQUAL(QSet<QString>({ QStringLiteral("Test dev 2"), QStringLiteral("Test dev 1") }), dir1.deviceNames.toSet());
    const SyncthingDir &dir2 = dirInfo.back();
    CPPUNIT_ASSERT_EQUAL(QStringLiteral("test2"), dir2.id);
    CPPUNIT_ASSERT_EQUAL(QStringLiteral("Test dir 2"), dir2.label);
    CPPUNIT_ASSERT_EQUAL(QStringLiteral("Test dir 2"), dir2.displayName());
    CPPUNIT_ASSERT_EQUAL(QStringLiteral("/tmp/some/path/2/"), dir2.path);
    CPPUNIT_ASSERT_EQUAL(QStringLiteral("/tmp/some/path/2"), dir2.pathWithoutTrailingSlash().toString());
    CPPUNIT_ASSERT_EQUAL(QStringLiteral("paused"), dir2.statusString());
    CPPUNIT_ASSERT(!dir2.readOnly);
    CPPUNIT_ASSERT(dir2.paused);
    CPPUNIT_ASSERT_EQUAL(
        QSet<QString>({ QStringLiteral("MMGUI6U-WUEZQCP-XZZ6VYB-LCT4TVC-ER2HAVX-QYT6X7D-S6ZSG2B-323KLQ7") }), dir2.deviceIds.toSet());
    CPPUNIT_ASSERT_EQUAL(QSet<QString>({ QStringLiteral("Test dev 2") }), dir2.deviceNames.toSet());
}

void ConnectionTests::testReconnecting()
{
    cerr << "\n - Reconnecting ..." << endl;
    waitForConnection(defaultReconnect(), 1000, connectionSignal(&SyncthingConnection::statusChanged));
    waitForAllDirsAndDevsReady(true);
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
    const function<void(const QStringList &)> devResumedTriggeredHandler
        = [this](const QStringList &devIds) { CPPUNIT_ASSERT_EQUAL(m_connection.deviceIds(), devIds); };
    const auto newDevsConnection = handleNewDevices(devResumedHandler, &devResumed);
    waitForConnection(&SyncthingConnection::resumeAllDevs, 7500, waitForConnected(),
        connectionSignal(&SyncthingConnection::devStatusChanged, devResumedHandler, &devResumed),
        connectionSignal(&SyncthingConnection::deviceResumeTriggered, devResumedTriggeredHandler));
    CPPUNIT_ASSERT(devResumed);
    for (const QJsonValue &devValue : m_connection.m_rawConfig.value(QStringLiteral("devices")).toArray()) {
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
    const function<void(const SyncthingDir &, int)> dirResumedHandler = [&dirResumed](const SyncthingDir &dir, int) {
        if (dir.id == QStringLiteral("test2") && !dir.paused) {
            dirResumed = true;
        }
    };
    const function<void(const QStringList &)> dirResumedTriggeredHandler
        = [this](const QStringList &devIds) { CPPUNIT_ASSERT_EQUAL(m_connection.directoryIds(), devIds); };
    const auto newDirsConnection = handleNewDirs(dirResumedHandler, &dirResumed);
    waitForConnection(&SyncthingConnection::resumeAllDirs, 7500, waitForConnected(),
        connectionSignal(&SyncthingConnection::dirStatusChanged, dirResumedHandler, &dirResumed),
        connectionSignal(&SyncthingConnection::directoryResumeTriggered, dirResumedTriggeredHandler));
    CPPUNIT_ASSERT(dirResumed);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("still 2 dirs present", 2_st, m_connection.dirInfo().size());
    CPPUNIT_ASSERT_MESSAGE("resuming all dirs should not cause another request again", !m_connection.resumeAllDirs());
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
    const QStringList ids({ QStringLiteral("test1") });
    const function<void(const QStringList &)> dirPausedTriggeredHandler
        = [this, &ids](const QStringList &devIds) { CPPUNIT_ASSERT_EQUAL(ids, devIds); };
    const auto newDirsConnection = handleNewDirs(dirPausedHandler, &dirPaused);
    waitForSignals(bind(&SyncthingConnection::pauseDirectories, &m_connection, ids), 7500, waitForConnected(),
        connectionSignal(&SyncthingConnection::dirStatusChanged, dirPausedHandler, &dirPaused),
        connectionSignal(&SyncthingConnection::directoryPauseTriggered, dirPausedTriggeredHandler));
    CPPUNIT_ASSERT(dirPaused);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("still 2 dirs present", 2_st, m_connection.dirInfo().size());
    CPPUNIT_ASSERT_MESSAGE("pausing should not cause another request again", !m_connection.pauseDirectories(ids));
}

void ConnectionTests::testRequestingLog()
{
    cerr << "\n - Requesting log ..." << endl;

    // timeout after 1 second
    QTimer timeout;
    timeout.setSingleShot(true);
    timeout.setInterval(1000);
    QEventLoop loop;
    QObject::connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);

    bool callbackOk = false;
    const auto request = m_connection.requestLog([&callbackOk, &loop](const std::vector<SyncthingLogEntry> &logEntries) {
        callbackOk = true;
        CPPUNIT_ASSERT(!logEntries.empty());
        CPPUNIT_ASSERT(!logEntries[0].when.isEmpty());
        CPPUNIT_ASSERT(!logEntries[0].message.isEmpty());
        loop.quit();
    });

    timeout.start();
    loop.exec();
    QObject::disconnect(request); // ensure callback is not called after return (in error case)
    CPPUNIT_ASSERT_MESSAGE("log entries returned before timeout", callbackOk);
}

void ConnectionTests::testRequestingQrCode()
{
    cerr << "\n - Requesting QR-Code for own device ID ..." << endl;

    // timeout after 2 seconds
    QTimer timeout;
    timeout.setSingleShot(true);
    timeout.setInterval(2000);
    QEventLoop loop;
    QObject::connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);

    bool callbackOk = false;
    const auto request = m_connection.requestQrCode(m_ownDevId, [this, &callbackOk, &loop](const QByteArray &data) {
        callbackOk = true;
        CPPUNIT_ASSERT(!data.isEmpty());
        loop.quit();
    });

    timeout.start();
    loop.exec();
    QObject::disconnect(request); // ensure callback is not called after return (in error case)
    CPPUNIT_ASSERT_MESSAGE("QR code returned before timeout", callbackOk);
}

void ConnectionTests::testDisconnecting()
{
    cerr << "\n - Disconnecting ..." << endl;
    waitForConnection(defaultDisconnect(), 1000, connectionSignal(&SyncthingConnection::statusChanged));
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
    const function<void(SyncthingStatus)> checkStatus([this, &isConnected](SyncthingStatus) { isConnected = m_connection.isConnected(); });
    waitForSignals(
        bind(static_cast<void (SyncthingConnection::*)(SyncthingConnectionSettings &)>(&SyncthingConnection::connect), &m_connection, ref(settings)),
        5000, connectionSignal(&SyncthingConnection::statusChanged, checkStatus, &isConnected));
}

void ConnectionTests::testRequestingRescan()
{
    cerr << "\n - Requesting rescan ..." << endl;

    bool rescanTriggered = false;
    function<void(const QString &)> rescanTriggeredHandler = [&rescanTriggered](const QString &dir) {
        CPPUNIT_ASSERT_EQUAL(QStringLiteral("test2"), dir);
        rescanTriggered = true;
    };
    waitForSignals(bind(&SyncthingConnection::rescanAllDirs, &m_connection), 5000,
        connectionSignal(&SyncthingConnection::rescanTriggered, rescanTriggeredHandler, &rescanTriggered));

    bool errorOccured = false;
    function<void(const QString &)> errorHandler = [&errorOccured](const QString &message) {
        errorOccured |= message.startsWith(QStringLiteral("Unable to request rescan: Error transferring"))
            && message.endsWith(QStringLiteral("/rest/db/scan?folder=non-existing-dir&sub=sub/path - server replied: Internal Server Error"));
    };
    waitForSignals(bind(&SyncthingConnection::rescan, &m_connection, QStringLiteral("non-existing-dir"), QStringLiteral("sub/path")), 5000,
        connectionSignal(&SyncthingConnection::error, errorHandler, &errorOccured));
}
