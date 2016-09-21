#include "./syncthingconnection.h"

#include <c++utilities/tests/testutils.h>
#include <c++utilities/conversion/stringbuilder.h>

#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestFixture.h>

#include <QCoreApplication>
#include <QProcess>
#include <QFileInfo>
#include <QDir>
#include <QMetaMethod>

using namespace std;
using namespace Data;
using namespace TestUtilities;
using namespace ConversionUtilities;

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
    void waitForConnection(Signal signal, Action action, int timeout = 2500, Handler handler = nullptr);
    template<typename Signal, typename Handler = function<void(void)> >
    void waitForConnection(Signal signal, int timeout = 2500, Handler handler = nullptr);

    QString m_apiKey;
    QCoreApplication m_app;
    QProcess m_syncthingProcess;
    SyncthingConnection m_connection;
};

CPPUNIT_TEST_SUITE_REGISTRATION(ConnectionTests);

int dummy1 = 0;
char *dummy2;

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
 * \brief Prints a QString; required to use QString with CPPUNIT_ASSERT_EQUAL_MESSAGE.
 */
std::ostream &operator <<(std::ostream &o, const QString &qstring)
{
    o << qstring.toLocal8Bit().data();
    return o;
}

/*!
 * \brief Waits for the in ms specified \a duration keeping the event loop running.
 */
void wait(int duration)
{
    QEventLoop loop;
    QTimer::singleShot(duration, &loop, &QEventLoop::quit);
    loop.exec();
}

void noop()
{}

/*!
 * \brief Waits until the \a signal is emitted by \a sender when performing \a action and connects \a signal with \a handler if specified.
 * \throws Fails if \a signal is not emitted in at least \a timeout milliseconds or when at least one of the required
 *         connections can not be established.
 */
template<typename Signal, typename Action, typename Handler = std::function<void(void)> >
void waitForSignal(typename QtPrivate::FunctionPointer<Signal>::Object *sender, Signal signal, Action action, int timeout = 2500, Handler handler = nullptr)
{
    // determine name of the signal for error messages
    const QByteArray signalName(QMetaMethod::fromSignal(signal).name());

    // create loop and connect the signal to its quit slot
    QEventLoop loop;
    if(!QObject::connect(sender, signal, &loop, &QEventLoop::quit, Qt::DirectConnection)) {
        CPPUNIT_FAIL(argsToString("Unable to connect signal ", signalName.data(), " for waiting"));
    }

    // if specified, also connect handler to signal
    if(handler) {
        if(!QObject::connect(sender, signal, sender, handler, Qt::DirectConnection)) {
            CPPUNIT_FAIL(argsToString("Unable to connect signal ", signalName.data(), " to handler"));
        }
    }

    // handle case when signal is directly emitted
    bool signalDirectlyEmitted = false;
    if(!QObject::connect(sender, signal, sender, [&signalDirectlyEmitted] {
                         signalDirectlyEmitted = true;
        }, Qt::DirectConnection)) {
        CPPUNIT_FAIL(argsToString("Unable to connect signal ", signalName.data(), " to check for direct emmitation"));
    }

    // perform specified action
    action();

    // no reason to enter event loop when signal has been emitted directly
    if(signalDirectlyEmitted) {
        return;
    }

    // also connect and start a timer if a timeout has been specified
    QTimer timer;
    if(timeout) {
        QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit, Qt::DirectConnection);
        timer.setSingleShot(true);
        timer.setInterval(timeout);
        timer.start();
    }

    // enter event loop
    loop.exec();

    // check whether a timeout occured
    if(timeout && !timer.isActive()) {
        CPPUNIT_FAIL(argsToString("Signal ", signalName.data(), " has not emmitted within at least ", timeout, " ms."));
    }
}

/*!
 * \brief Variant of waitForSignal() where sender is the connection and the action is a method of the connection.
 */
template<typename Signal, typename Action, typename Handler>
void ConnectionTests::waitForConnection(Signal signal, Action action, int timeout, Handler handler)
{
    waitForSignal(&m_connection, signal, bind(action, &m_connection), timeout, handler);
}

/*!
 * \brief Variant of waitForSignal() where sender is the connection and the action is a method of the connection.
 */
template<typename Signal, typename Handler>
void ConnectionTests::waitForConnection(Signal signal, int timeout, Handler handler)
{
    waitForSignal(&m_connection, signal, noop, timeout, handler);
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

    // error in case of wrong API key
    m_connection.setAutoReconnectInterval(200);
    m_connection.setApiKey(QByteArray("wrong API key"));
    waitForConnection(&SyncthingConnection::error, static_cast<void(SyncthingConnection::*)(void)>(&SyncthingConnection::connect), 10000, [] (const QString &errorMessage) {
        if(errorMessage != QStringLiteral("Unable to request Syncthing config: Connection refused")
                && errorMessage != QStringLiteral("Unable to request Syncthing status: Connection refused")) {
            CPPUNIT_FAIL(argsToString("wrong error message in case of wrong API key: ", errorMessage.toLocal8Bit().data()));
        }
    });

    // initial connection
    m_connection.setApiKey(m_apiKey.toUtf8());
    waitForConnection(&SyncthingConnection::statusChanged, static_cast<void(SyncthingConnection::*)(void)>(&SyncthingConnection::connect), 10000);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("connected and paused (one dev is initially paused)", QStringLiteral("connected, paused"), m_connection.statusText());

    // dirs present
    const auto &dirInfo = m_connection.dirInfo();
    CPPUNIT_ASSERT_EQUAL_MESSAGE("2 dirs present", 2ul, dirInfo.size());
    CPPUNIT_ASSERT_EQUAL(QStringLiteral("test1"), dirInfo.front().id);
    CPPUNIT_ASSERT_EQUAL(QStringLiteral("test2"), dirInfo.back().id);

    // devs present
    const auto &devInfo = m_connection.devInfo();
    CPPUNIT_ASSERT_EQUAL_MESSAGE("3 devs present", 3ul, devInfo.size());

    // reconnecting
    cerr << "\n - Reconnecting ..." << endl;
    waitForConnection(&SyncthingConnection::statusChanged, static_cast<void(SyncthingConnection::*)(void)>(&SyncthingConnection::reconnect), 1000);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("reconnecting", QStringLiteral("reconnecting"), m_connection.statusText());
    waitForConnection(&SyncthingConnection::statusChanged, 1000);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("connected again", QStringLiteral("connected, paused"), m_connection.statusText());

    // disconnecting
    cerr << "\n - Disconnecting ..." << endl;
    waitForConnection(&SyncthingConnection::statusChanged, static_cast<void(SyncthingConnection::*)(void)>(&SyncthingConnection::disconnect), 5000);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("disconnected", QStringLiteral("disconnected"), m_connection.statusText());
}

