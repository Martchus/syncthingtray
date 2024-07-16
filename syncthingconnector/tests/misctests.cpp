#include "../syncthingconfig.h"
#include "../syncthingconnection.h"
#include "../syncthingconnectionsettings.h"
#include "../syncthingprocess.h"
#include "../syncthingservice.h"
#include "../utils.h"

#include <c++utilities/chrono/datetime.h>
#include <c++utilities/chrono/format.h>
#include <c++utilities/chrono/timespan.h>
#include <c++utilities/tests/testutils.h>

#include "../../testhelper/helper.h"

#include <cppunit/TestFixture.h>

#include <QFile>
#include <QUrl>

#include <iostream>

using namespace std;
using namespace Data;
using namespace CppUtilities;
using namespace CppUtilities::Literals;

using namespace CPPUNIT_NS;

/*!
 * \brief The MiscTests class tests various features of the connector library.
 */
class MiscTests : public TestFixture {
    CPPUNIT_TEST_SUITE(MiscTests);
    CPPUNIT_TEST(testParsingConfig);
    CPPUNIT_TEST(testSplittingArguments);
    CPPUNIT_TEST(testUtils);
#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
    CPPUNIT_TEST(testService);
#endif
    CPPUNIT_TEST(testConnectionSettingsAndLoadingSelfSignedCert);
    CPPUNIT_TEST(testSyncthingDir);
    CPPUNIT_TEST_SUITE_END();

public:
    MiscTests();

    void testParsingConfig();
    void testSplittingArguments();
    void testUtils();
#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
    void testService();
#endif
#ifndef QT_NO_SSL
    void testConnectionSettingsAndLoadingSelfSignedCert();
#endif
    void testSyncthingDir();

    void setUp() override;
    void tearDown() override;

private:
};

CPPUNIT_TEST_SUITE_REGISTRATION(MiscTests);

MiscTests::MiscTests()
{
}

//
// test setup
//

void MiscTests::setUp()
{
}

void MiscTests::tearDown()
{
}

//
// actual test
//

/*!
 * \brief Tests basic behaviour of the SyncthingConnection class.
 */
void MiscTests::testParsingConfig()
{
    SyncthingConfig config;
    CPPUNIT_ASSERT(!config.restore(QStringLiteral("non-existant-file")));
    CPPUNIT_ASSERT(config.restore(QString::fromLocal8Bit(testFilePath("testconfig/config.xml").data())));
    CPPUNIT_ASSERT_EQUAL_MESSAGE("address", QStringLiteral("127.0.0.1:4001"), config.guiAddress);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("API key", QStringLiteral("syncthingconnectortest"), config.guiApiKey);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("user", QStringLiteral("nobody"), config.guiUser);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("password", QStringLiteral("$2a$12$35MnbsQgQNn1hzPYK/lWXOaP.U5D2TO0nuuQy2M4gsqJB4ff4q2RK"), config.guiPasswordHash);
    CPPUNIT_ASSERT_MESSAGE("TLS", !config.guiEnforcesSecureConnection);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("url", QStringLiteral("http://127.0.0.1:4001"), config.syncthingUrl());
    config.guiEnforcesSecureConnection = true;
    CPPUNIT_ASSERT_EQUAL_MESSAGE("url", QStringLiteral("https://127.0.0.1:4001"), config.syncthingUrl());
    const QString configFile(SyncthingConfig::locateConfigFile());
    CPPUNIT_ASSERT(configFile.isEmpty() || QFile::exists(configFile));
    const QString httpsCert(SyncthingConfig::locateHttpsCertificate());
    CPPUNIT_ASSERT(httpsCert.isEmpty() || QFile::exists(httpsCert));
}

/*!
 * \brief Test splitting arguments via SyncthingProcess::splitArguments().
 */
void MiscTests::testSplittingArguments()
{
    CPPUNIT_ASSERT_EQUAL_MESSAGE("empty arguments", QStringList(), SyncthingProcess::splitArguments(QString()));
    CPPUNIT_ASSERT_EQUAL_MESSAGE("one argument without special characters", QStringList({ QStringLiteral("-simple") }),
        SyncthingProcess::splitArguments(QStringLiteral("-simple")));
    CPPUNIT_ASSERT_EQUAL_MESSAGE("multiple arguments without special characters",
        QStringList({ QStringLiteral("-home"), QStringLiteral("some dir"), QStringLiteral("-no-restart") }),
        SyncthingProcess::splitArguments(QStringLiteral("-home \"some dir\" -no-restart")));
    CPPUNIT_ASSERT_EQUAL_MESSAGE("quotation", QStringList({ QStringLiteral("-home"), QStringLiteral("some  dir"), QStringLiteral("-no-restart") }),
        SyncthingProcess::splitArguments(QStringLiteral(" -home \"some  dir\"   -no-restart ")));
    CPPUNIT_ASSERT_EQUAL_MESSAGE("escaped quotation",
        QStringList({ QStringLiteral("-home"), QStringLiteral("\"some"), QStringLiteral("dir\""), QStringLiteral("-no-restart") }),
        SyncthingProcess::splitArguments(QStringLiteral("-home \\\"some dir\\\" -no-restart")));
    CPPUNIT_ASSERT_EQUAL_MESSAGE("escaped spaces",
        QStringList({ QStringLiteral("-home"), QStringLiteral("some dir"), QStringLiteral("-no-restart") }),
        SyncthingProcess::splitArguments(QStringLiteral("-home \\ some\\ dir  -no-restart")));
    CPPUNIT_ASSERT_EQUAL_MESSAGE("spaces at the beginning through quotes", QStringList({ QStringLiteral("foo"), QStringLiteral(" bar") }),
        SyncthingProcess::splitArguments(QStringLiteral("foo \" bar\"")));
    CPPUNIT_ASSERT_EQUAL_MESSAGE("spaces at the end through quotes", QStringList({ QStringLiteral("-home"), QStringLiteral("-no-restart ") }),
        SyncthingProcess::splitArguments(QStringLiteral("-home \"-no-restart \"")));
    CPPUNIT_ASSERT_EQUAL_MESSAGE("don't care about missing quote at the end", QStringList({ QStringLiteral("foo"), QStringLiteral(" bar") }),
        SyncthingProcess::splitArguments(QStringLiteral("foo \" bar")));
}

/*!
 * \brief Tests utils.
 */
void MiscTests::testUtils()
{
    CPPUNIT_ASSERT_EQUAL(QStringLiteral("right now"), agoString(DateTime::now()));
    CPPUNIT_ASSERT_EQUAL(QStringLiteral("5 h ago"), agoString(DateTime::now() - TimeSpan::fromHours(5.0)));
    CPPUNIT_ASSERT(isLocal(QUrl(QStringLiteral("http://127.0.0.1"))));
    CPPUNIT_ASSERT(isLocal(QUrl(QStringLiteral("http://[::1]"))));
    CPPUNIT_ASSERT(isLocal(QUrl(QStringLiteral("http://localhost/"))));
    CPPUNIT_ASSERT(!isLocal(QUrl(QStringLiteral("http://157.3.52.34"))));
    CPPUNIT_ASSERT_EQUAL(
        QStringLiteral("/some/path"), substituteTilde(QStringLiteral("/some/path"), QStringLiteral("/home/foo"), QStringLiteral("/")));
    CPPUNIT_ASSERT_EQUAL(
        QStringLiteral("/home/foo/some/path"), substituteTilde(QStringLiteral("~/some/path"), QStringLiteral("/home/foo"), QStringLiteral("/")));
    CPPUNIT_ASSERT_EQUAL(
        QStringLiteral("~bar/some/path"), substituteTilde(QStringLiteral("~bar/some/path"), QStringLiteral("/home/foo"), QStringLiteral("/")));
    CPPUNIT_ASSERT_EQUAL(QStringLiteral("/home/foobar/some/path"),
        substituteTilde(QStringLiteral("~bar/some/path"), QStringLiteral("/home/foo"), QStringLiteral("bar/")));
    CPPUNIT_ASSERT_EQUAL(QStringLiteral("/home/foo"), substituteTilde(QStringLiteral("~"), QStringLiteral("/home/foo"), QStringLiteral("\\")));
}

#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
/*!
 * \brief Tests SyncthingService class, but only error cases with a non-existent service so far.
 */
void MiscTests::testService()
{
    SyncthingService service;
    service.isSystemdAvailable();
    service.setUnitName(QStringLiteral("non-existent.service"));
    CPPUNIT_ASSERT(!service.isUnitAvailable());
    CPPUNIT_ASSERT_EQUAL(QString(), service.description());
    CPPUNIT_ASSERT(!service.isRunning());
    CPPUNIT_ASSERT(!service.isEnabled());
    service.toggleRunning();
    service.setEnabled(true);
}
#endif

#ifndef QT_NO_SSL
void MiscTests::testConnectionSettingsAndLoadingSelfSignedCert()
{
    SyncthingConnectionSettings settings;
    settings.syncthingUrl = QStringLiteral("http://localhost:8080");
    settings.apiKey = QByteArrayLiteral("foo");
    settings.httpsCertPath = SyncthingConfig::locateHttpsCertificate();
    if (!settings.httpsCertPath.isEmpty() && settings.loadHttpsCert()) {
        CPPUNIT_ASSERT_GREATER(static_cast<decltype(settings.expectedSslErrors.size())>(0), settings.expectedSslErrors.size());
    } else {
        CPPUNIT_ASSERT_EQUAL(static_cast<decltype(settings.expectedSslErrors.size())>(0), settings.expectedSslErrors.size());
    }
    SyncthingConnection connection;
    CPPUNIT_ASSERT(connection.applySettings(settings));
    CPPUNIT_ASSERT(!connection.loadSelfSignedCertificate());
    settings.syncthingUrl = QStringLiteral("https://localhost:8080");
    CPPUNIT_ASSERT(connection.applySettings(settings));
    connection.m_configDir = QStringLiteral("some-non/existent-dir");
    connection.clearSelfSignedCertificate();
    const auto expectedErrorMessage = QStringLiteral("Unable to load certificate used by Syncthing.");
    auto expectedErrorOccured = false;
    const function<void(const QString &)> errorHandler = [&expectedErrorOccured, &expectedErrorMessage](const QString &message) {
        std::cout << "\n - error: " << message.toLocal8Bit().data() << '\n';
        expectedErrorOccured |= message == expectedErrorMessage;
    };
    waitForSignals(bind(&SyncthingConnection::loadSelfSignedCertificate, &connection, QUrl()), 1000,
        signalInfo(&connection, &SyncthingConnection::error, errorHandler, &expectedErrorOccured));
    settings.expectedSslErrors.clear();
    settings.httpsCertPath.clear();
    CPPUNIT_ASSERT(!connection.applySettings(settings));
}
#endif

void MiscTests::testSyncthingDir()
{
    SyncthingDir dir;
    dir.status = SyncthingDirStatus::Unknown;

    auto updateEvent = static_cast<SyncthingEventId>(42);
    auto updateTime = DateTime(DateTime::fromDate(2005, 2, 3));
    CPPUNIT_ASSERT(dir.assignStatus(SyncthingDirStatus::Idle, updateEvent, updateTime));
    CPPUNIT_ASSERT_EQUAL_MESSAGE("status updated", QStringLiteral("unshared"), dir.statusString());
    CPPUNIT_ASSERT_EQUAL_MESSAGE("event updated", updateEvent, dir.lastStatusUpdateEvent);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("time updated", updateTime, dir.lastStatusUpdateTime);

    dir.deviceIds << QStringLiteral("dev1") << QStringLiteral("dev2");

    CPPUNIT_ASSERT(!dir.assignStatus(SyncthingDirStatus::Scanning, updateEvent - 1, updateTime + TimeSpan::fromDays(1.0)));
    CPPUNIT_ASSERT_EQUAL_MESSAGE("status not updated", QStringLiteral("up to date"), dir.statusString());
    CPPUNIT_ASSERT_EQUAL_MESSAGE("event not updated", updateEvent, dir.lastStatusUpdateEvent);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("time not updated", updateTime, dir.lastStatusUpdateTime);

    const auto lastScanTime = DateTime(DateTime::now());
    CPPUNIT_ASSERT(dir.assignStatus(SyncthingDirStatus::WaitingToScan, updateEvent += 1, updateTime += TimeSpan::fromSeconds(5)));
    CPPUNIT_ASSERT(dir.lastScanTime.isNull());
    CPPUNIT_ASSERT_EQUAL(QStringLiteral("waiting to scan"), dir.statusString());

    CPPUNIT_ASSERT(dir.assignStatus(SyncthingDirStatus::Scanning, updateEvent += 1, updateTime += TimeSpan::fromSeconds(5)));
    CPPUNIT_ASSERT(dir.lastScanTime.isNull());
    CPPUNIT_ASSERT_EQUAL(QStringLiteral("scanning"), dir.statusString());

    CPPUNIT_ASSERT(dir.assignStatus(SyncthingDirStatus::Idle, updateEvent += 1, updateTime += TimeSpan::fromSeconds(2)));
    CPPUNIT_ASSERT_EQUAL_MESSAGE("event updated", updateEvent, dir.lastStatusUpdateEvent);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("time updated", updateTime, dir.lastStatusUpdateTime);
    CPPUNIT_ASSERT(dir.lastScanTime >= lastScanTime);

    dir.status = SyncthingDirStatus::Unknown;
    dir.lastSyncStartedTime = DateTime(1);
    dir.itemErrors.emplace_back(QStringLiteral("message"), QStringLiteral("path"));
    CPPUNIT_ASSERT(dir.assignStatus(SyncthingDirStatus::Idle, updateEvent += 1, updateTime += TimeSpan::fromMinutes(1.5)));
    CPPUNIT_ASSERT_EQUAL(QStringLiteral("up to date"), dir.statusString());
    CPPUNIT_ASSERT_EQUAL(1_st, dir.itemErrors.size());
    dir.lastSyncStartedTime = DateTime();
    CPPUNIT_ASSERT(!dir.assignStatus(SyncthingDirStatus::Idle, updateEvent += 1, updateTime += TimeSpan::fromMinutes(1.5)));
    CPPUNIT_ASSERT_EQUAL(updateTime, dir.lastSyncStartedTime);
    const auto lastSyncTime = updateTime += TimeSpan::fromMinutes(1.5);
    dir.itemErrors.emplace_back();
    CPPUNIT_ASSERT(dir.assignStatus(SyncthingDirStatus::Synchronizing, updateEvent += 1, lastSyncTime));
    CPPUNIT_ASSERT_EQUAL(QStringLiteral("synchronizing"), dir.statusString());
    CPPUNIT_ASSERT_EQUAL(0_st, dir.itemErrors.size());
    CPPUNIT_ASSERT_EQUAL(lastSyncTime, dir.lastSyncStartedTime);
    const auto lastSyncTime2 = updateTime += TimeSpan::fromMinutes(2.0);
    dir.itemErrors.emplace_back();
    CPPUNIT_ASSERT(dir.assignStatus(SyncthingDirStatus::PreparingToSync, updateEvent += 1, lastSyncTime2));
    CPPUNIT_ASSERT_EQUAL(QStringLiteral("preparing to sync"), dir.statusString());
    CPPUNIT_ASSERT_EQUAL(0_st, dir.itemErrors.size());
    CPPUNIT_ASSERT_EQUAL(lastSyncTime2, dir.lastSyncStartedTime);

    CPPUNIT_ASSERT(dir.assignStatus(SyncthingDirStatus::Idle, updateEvent += 1, updateTime += TimeSpan::fromMinutes(1.5)));
    CPPUNIT_ASSERT_EQUAL(lastSyncTime2, dir.lastSyncStartedTime);
    CPPUNIT_ASSERT(dir.assignStatus(QStringLiteral("syncing"), updateEvent += 1, updateTime += TimeSpan::fromMinutes(1.5)));
    CPPUNIT_ASSERT_EQUAL(updateTime, dir.lastSyncStartedTime);

    dir.itemErrors.clear();
    CPPUNIT_ASSERT(dir.assignStatus(QStringLiteral("error"), updateEvent += 1, updateTime += TimeSpan::fromMinutes(1.5)));
    CPPUNIT_ASSERT_EQUAL(QStringLiteral("out of sync"), dir.statusString());

    CPPUNIT_ASSERT_MESSAGE("older status discarded", !dir.assignStatus(QStringLiteral("scanning"), updateEvent - 1, updateTime));
    CPPUNIT_ASSERT_EQUAL(QStringLiteral("out of sync"), dir.statusString());

    dir.deviceIds.clear();
    CPPUNIT_ASSERT(dir.assignStatus(QStringLiteral("idle"), updateEvent += 1, updateTime += TimeSpan::fromMinutes(1.5)));
    CPPUNIT_ASSERT_EQUAL_MESSAGE("dir considered unshared when no devs present", QStringLiteral("unshared"), dir.statusString());
    CPPUNIT_ASSERT(!dir.assignStatus(SyncthingDirStatus::Idle, updateEvent += 1, updateTime += TimeSpan::fromMinutes(1.5)));
    CPPUNIT_ASSERT_EQUAL_MESSAGE("dir considered unshared when no devs present", QStringLiteral("unshared"), dir.statusString());
    CPPUNIT_ASSERT_MESSAGE("same status again not considered an update",
        !dir.assignStatus(QStringLiteral("idle"), updateEvent += 1, updateTime += TimeSpan::fromMinutes(1.5)));
}
