#include "./helper.h"
#include "../syncthingconnection.h"

#include <c++utilities/tests/testutils.h>

#include <cppunit/TestFixture.h>

#include <QCoreApplication>
#include <QProcess>
#include <QFileInfo>
#include <QDir>
#include <QStringBuilder>

using namespace std;
using namespace Data;
using namespace TestUtilities;

using namespace CPPUNIT_NS;

/*!
 * \brief The ConnectionTests class tests the SyncthingConnector.
 */
class ConnectionTests : public TestFixture
{
    CPPUNIT_TEST_SUITE(ConnectionTests);
    CPPUNIT_TEST(testConnection);
    CPPUNIT_TEST_SUITE_END();

public:
    ConnectionTests();

    void testConnection();

    void setUp();
    void tearDown();

private:
    template<typename Signal, typename Action, typename Handler = function<void(void)> >
    void waitForConnection(Signal signal, Action action, Handler handler = nullptr, bool *ok = nullptr, int timeout = 1000);
    template<typename Signal, typename Action, typename Handler = function<void(void)> >
    void waitForConnectionAnyAction(Signal signal, Action action, Handler handler = nullptr, bool *ok = nullptr, int timeout = 1000);

    QString m_apiKey;
    QCoreApplication m_app;
    QProcess m_syncthingProcess;
    SyncthingConnection m_connection;
};

CPPUNIT_TEST_SUITE_REGISTRATION(ConnectionTests);

static int dummy1 = 0;
static char *dummy2;

ConnectionTests::ConnectionTests() :
    m_apiKey(QStringLiteral("syncthingconnectortest")),
    m_app(dummy1, &dummy2)
{}

//
// test setup
//

/*!
 * \brief Starts Syncthing and prepares connecting.
 */
void ConnectionTests::setUp()
{
    cerr << "\n - Launching Syncthing and setup connection ..." << endl;

    // setup st config
    const string configFilePath = workingCopyPath("testconfig/config.xml");
    if(configFilePath.empty()) {
        throw runtime_error("Unable to setup Syncthing config directory.");
    }
    const QFileInfo configFile(QString::fromLocal8Bit(configFilePath.data()));
    // clean config dir
    const QDir configDir(configFile.dir());
    for(QFileInfo &configEntry : configDir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot)) {
        if(configEntry.isDir()) {
            QDir(configEntry.absoluteFilePath()).removeRecursively();
        } else if(configEntry.fileName() != QStringLiteral("config.xml")) {
            QFile::remove(configEntry.absoluteFilePath());
        }
    }

    // determine st path
    const QByteArray syncthingPathFromEnv(qgetenv("SYNCTHING_PATH"));
    const QString syncthingPath(syncthingPathFromEnv.isEmpty() ? QStringLiteral("syncthing") : QString::fromLocal8Bit(syncthingPathFromEnv));

    // determine st port
    const int syncthingPortFromEnv(qEnvironmentVariableIntValue("SYNCTHING_PORT"));
    const QString syncthingPort(!syncthingPortFromEnv ? QStringLiteral("4001") : QString::number(syncthingPortFromEnv));

    // start st
    QStringList args;
    args.reserve(2);
    args << QStringLiteral("-gui-address=http://localhost:") + syncthingPort;
    args << QStringLiteral("-gui-apikey=") + m_apiKey;
    args << QStringLiteral("-home=") + configFile.absolutePath();
    args << QStringLiteral("-no-browser");
    args << QStringLiteral("-verbose");
    m_syncthingProcess.start(syncthingPath, args);

    // setup connection
    m_connection.setSyncthingUrl(QStringLiteral("http://localhost:") + syncthingPort);

    // keep track of status changes
    QObject::connect(&m_connection, &SyncthingConnection::statusChanged, [this] {
        cerr << " - Connection status changed to: " << m_connection.statusText().toLocal8Bit().data() << endl;
    });
}

/*!
 * \brief Terminates Syncthing and prints stdout/stderr from Syncthing.
 */
void ConnectionTests::tearDown()
{
    if(m_syncthingProcess.state() == QProcess::Running) {
        cerr << "\n - Waiting for Syncthing to terminate ..." << endl;
        m_syncthingProcess.terminate();
        m_syncthingProcess.waitForFinished();
    }
    if(m_syncthingProcess.isOpen()) {
        cerr << "\n - Syncthing terminated with exit code " << m_syncthingProcess.exitCode() << ".\n";
        cerr << "\n - Syncthing stdout during the testrun:\n" << m_syncthingProcess.readAllStandardOutput().data();
        cerr << "\n - Syncthing stderr during the testrun:\n" << m_syncthingProcess.readAllStandardError().data();
    }
}

//
// test helper
//

/*!
 * \brief Variant of waitForSignal() where sender is the connection and the action is a method of the connection.
 */
template<typename Signal, typename Action, typename Handler>
void ConnectionTests::waitForConnection(Signal signal, Action action, Handler handler, bool *ok, int timeout)
{
    waitForSignal(&m_connection, signal, bind(action, &m_connection), timeout, handler);
}

/*!
 * \brief Variant of waitForSignal() where sender is the connection.
 */
template<typename Signal, typename Action, typename Handler>
void ConnectionTests::waitForConnectionAnyAction(Signal signal, Action action, Handler handler, bool *ok, int timeout)
{
    waitForSignal(&m_connection, signal, action, timeout, handler);
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
    const function<void(const QString &errorMessage)> errorHandler = [this, &syncthingAvailable] (const QString &errorMessage) {
                if(errorMessage == QStringLiteral("Unable to request Syncthing config: Connection refused")
                        || errorMessage == QStringLiteral("Unable to request Syncthing status: Connection refused")) {
                    return; // Syncthing not ready yet
                }
                syncthingAvailable = true;
                if(errorMessage != QStringLiteral("Unable to request Syncthing config: Error transferring ") % m_connection.syncthingUrl() % QStringLiteral("/rest/system/config - server replied: Forbidden")
                        && errorMessage != QStringLiteral("Unable to request Syncthing status: Error transferring ") % m_connection.syncthingUrl() % QStringLiteral("/rest/system/status - server replied: Forbidden")) {
                    CPPUNIT_FAIL(argsToString("wrong error message in case of wrong API key: ", errorMessage.toLocal8Bit().data()));
                }
            };
    while(!syncthingAvailable) {
        waitForConnection(&SyncthingConnection::error, static_cast<void(SyncthingConnection::*)(void)>(&SyncthingConnection::connect), errorHandler);
    }

    // initial connection
    m_connection.setApiKey(m_apiKey.toUtf8());
    waitForConnection(&SyncthingConnection::statusChanged, static_cast<void(SyncthingConnection::*)(void)>(&SyncthingConnection::connect));
    CPPUNIT_ASSERT_EQUAL_MESSAGE("connected and paused (one dev is initially paused)", QStringLiteral("connected, paused"), m_connection.statusText());

    // devs present
    const auto &devInfo = m_connection.devInfo();
    CPPUNIT_ASSERT_EQUAL_MESSAGE("3 devs present", 3ul, devInfo.size());
    QString ownDevId;
    for(const SyncthingDev &dev : devInfo) {
        if(dev.id != QStringLiteral("MMGUI6U-WUEZQCP-XZZ6VYB-LCT4TVC-ER2HAVX-QYT6X7D-S6ZSG2B-323KLQ7") && dev.id != QStringLiteral("6EIS2PN-J2IHWGS-AXS3YUL-HC5FT3K-77ZXTLL-AKQLJ4C-7SWVPUS-AZW4RQ4")) {
            CPPUNIT_ASSERT_EQUAL_MESSAGE("own device", QStringLiteral("own device"), dev.statusString());
            ownDevId = dev.id;
        }
    }
    for(const SyncthingDev &dev : devInfo) {
        if(dev.id == QStringLiteral("MMGUI6U-WUEZQCP-XZZ6VYB-LCT4TVC-ER2HAVX-QYT6X7D-S6ZSG2B-323KLQ7")) {
            CPPUNIT_ASSERT_EQUAL_MESSAGE("paused device", QStringLiteral("paused"), dev.statusString());
            CPPUNIT_ASSERT_EQUAL_MESSAGE("name", QStringLiteral("Test dev 2"), dev.name);
            CPPUNIT_ASSERT_MESSAGE("no introducer", !dev.introducer);
            CPPUNIT_ASSERT_EQUAL(1, dev.addresses.size());
            CPPUNIT_ASSERT_EQUAL(QStringLiteral("tcp://192.168.2.2:22000"), dev.addresses.front());
        } else if(dev.id == QStringLiteral("6EIS2PN-J2IHWGS-AXS3YUL-HC5FT3K-77ZXTLL-AKQLJ4C-7SWVPUS-AZW4RQ4")) {
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
    CPPUNIT_ASSERT_EQUAL(dir1.devices, QStringList({QStringLiteral("MMGUI6U-WUEZQCP-XZZ6VYB-LCT4TVC-ER2HAVX-QYT6X7D-S6ZSG2B-323KLQ7"),
                                                    QStringLiteral("6EIS2PN-J2IHWGS-AXS3YUL-HC5FT3K-77ZXTLL-AKQLJ4C-7SWVPUS-AZW4RQ4"),
                                                   ownDevId}));
    const SyncthingDir &dir2 = dirInfo.back();
    CPPUNIT_ASSERT_EQUAL(QStringLiteral("test2"), dir2.id);
    CPPUNIT_ASSERT_EQUAL(QStringLiteral("Test dir 2"), dir2.label);
    CPPUNIT_ASSERT_EQUAL(QStringLiteral("Test dir 2"), dir2.displayName());
    CPPUNIT_ASSERT_EQUAL(QStringLiteral("/tmp/some/path/2/"), dir2.path);
    CPPUNIT_ASSERT(!dir2.readOnly);
    CPPUNIT_ASSERT(dir2.paused);
    CPPUNIT_ASSERT_EQUAL(dir2.devices, QStringList({QStringLiteral("MMGUI6U-WUEZQCP-XZZ6VYB-LCT4TVC-ER2HAVX-QYT6X7D-S6ZSG2B-323KLQ7"),
                                                   ownDevId}));

    // reconnecting
    cerr << "\n - Reconnecting ..." << endl;
    waitForConnection(&SyncthingConnection::statusChanged, static_cast<void(SyncthingConnection::*)(void)>(&SyncthingConnection::reconnect));
    CPPUNIT_ASSERT_EQUAL_MESSAGE("reconnecting", QStringLiteral("reconnecting"), m_connection.statusText());
    waitForConnectionAnyAction(&SyncthingConnection::statusChanged, noop);
    if(m_connection.isConnected() && m_connection.status() != SyncthingStatus::Paused) {
        // FIXME: Maybe it takes one further update to recon paused dev?
        waitForConnectionAnyAction(&SyncthingConnection::statusChanged, noop);
    }
    CPPUNIT_ASSERT_EQUAL_MESSAGE("connected again", QStringLiteral("connected, paused"), m_connection.statusText());

    // resume all devs
    bool devPaused = false;
    waitForConnection(&SyncthingConnection::devStatusChanged, &SyncthingConnection::resumeAllDevs, static_cast<function<void(const SyncthingDev &, int)> >([&devPaused] (const SyncthingDev &dev, int) {
        if(dev.name == QStringLiteral("Test dev 2") && !dev.paused) {
            devPaused = true;
        }
    }), &devPaused);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("not paused anymore", QStringLiteral("connected"), m_connection.statusText());

    // resume all dirs
    bool dirPaused = false;
    waitForConnection(&SyncthingConnection::dirStatusChanged, &SyncthingConnection::resumeAllDirs, static_cast<function<void(const SyncthingDir &, int)> >([&dirPaused] (const SyncthingDir &dir, int) {
        if(dir.id == QStringLiteral("test2") && dir.paused) {
            dirPaused = true;
        }
    }), &dirPaused);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("still 2 dirs present", 2ul, m_connection.dirInfo().size());

    // disconnecting
    cerr << "\n - Disconnecting ..." << endl;
    waitForConnection(&SyncthingConnection::statusChanged, static_cast<void(SyncthingConnection::*)(void)>(&SyncthingConnection::disconnect));
    CPPUNIT_ASSERT_EQUAL_MESSAGE("disconnected", QStringLiteral("disconnected"), m_connection.statusText());
}

