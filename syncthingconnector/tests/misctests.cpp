#include "../syncthingconfig.h"
#include "../syncthingconnection.h"
#include "../syncthingconnectionsettings.h"
#include "../syncthingignorepattern.h"
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
    CPPUNIT_TEST(testIgnorePatternMatching);
    CPPUNIT_TEST_SUITE_END();

public:
    MiscTests();

    void testParsingConfig();
    void testSplittingArguments();
    void testUtils();
#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
    void testService();
#endif
    void testConnectionSettingsAndLoadingSelfSignedCert();
    void testSyncthingDir();
    void testIgnorePatternMatching();

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
    CPPUNIT_ASSERT_EQUAL_MESSAGE("status not updated", QStringLiteral("idle"), dir.statusString());
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
    CPPUNIT_ASSERT_EQUAL(QStringLiteral("idle"), dir.statusString());
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

    CPPUNIT_ASSERT(dir.assignStatus(QStringLiteral("wrong status"), updateEvent += 1, updateTime += TimeSpan::fromMinutes(1.5)));
    CPPUNIT_ASSERT_EQUAL_MESSAGE("wrong status treated as idle", QStringLiteral("idle"), dir.statusString());

    CPPUNIT_ASSERT_MESSAGE("older status discarded", !dir.assignStatus(QStringLiteral("scanning"), updateEvent - 1, updateTime));
    CPPUNIT_ASSERT_EQUAL(QStringLiteral("idle"), dir.statusString());

    dir.deviceIds.clear();
    CPPUNIT_ASSERT(!dir.assignStatus(QStringLiteral("idle"), updateEvent += 1, updateTime += TimeSpan::fromMinutes(1.5)));
    CPPUNIT_ASSERT_EQUAL_MESSAGE("dir considered unshared when no devs present", QStringLiteral("unshared"), dir.statusString());
    CPPUNIT_ASSERT(!dir.assignStatus(SyncthingDirStatus::Idle, updateEvent += 1, updateTime += TimeSpan::fromMinutes(1.5)));
    CPPUNIT_ASSERT_EQUAL_MESSAGE("dir considered unshared when no devs present", QStringLiteral("unshared"), dir.statusString());
    CPPUNIT_ASSERT_MESSAGE("same status again not considered an update",
        !dir.assignStatus(QStringLiteral("idle"), updateEvent += 1, updateTime += TimeSpan::fromMinutes(1.5)));
}

void MiscTests::testIgnorePatternMatching()
{
    auto p1 = SyncthingIgnorePattern(QStringLiteral("foo"));
    CPPUNIT_ASSERT(!p1.comment);
    CPPUNIT_ASSERT(!p1.caseInsensitive);
    CPPUNIT_ASSERT(p1.ignore);
    CPPUNIT_ASSERT(!p1.allowRemovalOnParentDirRemoval);
    CPPUNIT_ASSERT_EQUAL(QStringLiteral("foo"), p1.glob);
    CPPUNIT_ASSERT(p1.matches(QStringLiteral("foo")));
    CPPUNIT_ASSERT(!p1.matches(QStringLiteral("foofoo")));
    CPPUNIT_ASSERT(p1.matches(QStringLiteral("foo/foo")));

    auto p2 = SyncthingIgnorePattern(QStringLiteral("(?d)(?i)foo*"));
    CPPUNIT_ASSERT(p2.allowRemovalOnParentDirRemoval);
    CPPUNIT_ASSERT(p2.caseInsensitive);
    CPPUNIT_ASSERT(p2.matches(QStringLiteral("foo")));
    CPPUNIT_ASSERT(p2.matches(QStringLiteral("FooBar")));
    CPPUNIT_ASSERT(!p2.matches(QStringLiteral("barfoo")));
    CPPUNIT_ASSERT(p2.matches(QStringLiteral("bar/foo")));
    CPPUNIT_ASSERT(p2.matches(QStringLiteral("bar/foobaz")));
    CPPUNIT_ASSERT(!p2.matches(QStringLiteral("bar/foo/baz")));

    auto p2a = SyncthingIgnorePattern(QStringLiteral("(?d)foo**"));
    CPPUNIT_ASSERT(p2a.allowRemovalOnParentDirRemoval);
    CPPUNIT_ASSERT(p2a.matches(QStringLiteral("foo")));
    CPPUNIT_ASSERT(p2a.matches(QStringLiteral("foobar")));
    CPPUNIT_ASSERT(!p2a.matches(QStringLiteral("barfoo")));
    CPPUNIT_ASSERT(p2a.matches(QStringLiteral("bar/foo")));
    CPPUNIT_ASSERT(p2a.matches(QStringLiteral("bar/foobaz")));
    CPPUNIT_ASSERT(p2a.matches(QStringLiteral("bar/foo/baz")));

    auto p3 = SyncthingIgnorePattern(QStringLiteral("fo*ar"));
    CPPUNIT_ASSERT(!p3.matches(QStringLiteral("foo")));
    CPPUNIT_ASSERT(p3.matches(QStringLiteral("foar")));
    CPPUNIT_ASSERT(p3.matches(QStringLiteral("foobar")));
    CPPUNIT_ASSERT(!p3.matches(QStringLiteral("foobaR")));
    CPPUNIT_ASSERT(p3.matches(QStringLiteral("foobaRar")));
    CPPUNIT_ASSERT(p3.matches(QStringLiteral("foo*bar")));

    auto p4 = SyncthingIgnorePattern(QStringLiteral("fo\\*ar"));
    CPPUNIT_ASSERT(!p4.matches(QStringLiteral("foar")));
    CPPUNIT_ASSERT(!p4.matches(QStringLiteral("foobar")));
    CPPUNIT_ASSERT(!p4.matches(QStringLiteral("foo*bar")));
    CPPUNIT_ASSERT(p4.matches(QStringLiteral("fo*ar")));

    auto p5 = SyncthingIgnorePattern(QStringLiteral("te*ne"));
    CPPUNIT_ASSERT(p5.matches(QStringLiteral("telephone")));
    CPPUNIT_ASSERT(p5.matches(QStringLiteral("subdir/telephone")));
    CPPUNIT_ASSERT(!p5.matches(QStringLiteral("tele/phone")));

    auto p6 = SyncthingIgnorePattern(QStringLiteral("te**ne"));
    CPPUNIT_ASSERT(p6.matches(QStringLiteral("telephone")));
    CPPUNIT_ASSERT(p6.matches(QStringLiteral("subdir/telephone")));
    CPPUNIT_ASSERT(p6.matches(QStringLiteral("tele/phone")));

    auto p7 = SyncthingIgnorePattern(QStringLiteral("te??st"));
    CPPUNIT_ASSERT(p7.matches(QStringLiteral("tebest")));
    CPPUNIT_ASSERT(!p7.matches(QStringLiteral("teb/st")));
    CPPUNIT_ASSERT(!p7.matches(QStringLiteral("test")));

    auto p8 = SyncthingIgnorePattern(QStringLiteral("fo*ar*y"));
    CPPUNIT_ASSERT(!p8.matches(QStringLiteral("fooy")));
    CPPUNIT_ASSERT(p8.matches(QStringLiteral("foary")));
    CPPUNIT_ASSERT(p8.matches(QStringLiteral("foobary")));
    CPPUNIT_ASSERT(!p8.matches(QStringLiteral("foobaRy")));
    CPPUNIT_ASSERT(p8.matches(QStringLiteral("foobaRary")));
    CPPUNIT_ASSERT(!p8.matches(QStringLiteral("foobaRaRy")));
    CPPUNIT_ASSERT(p8.matches(QStringLiteral("foobaRaRarxy")));
    CPPUNIT_ASSERT(p8.matches(QStringLiteral("bar/foobaRaRarxy")));

    auto p9 = SyncthingIgnorePattern(QStringLiteral("/foo*"));
    CPPUNIT_ASSERT(p9.matches(QStringLiteral("foo")));
    CPPUNIT_ASSERT(p9.matches(QStringLiteral("foobar")));
    CPPUNIT_ASSERT(!p9.matches(QStringLiteral("barfoo")));
    CPPUNIT_ASSERT(!p9.matches(QStringLiteral("bar/foo")));

    auto p10 = SyncthingIgnorePattern(QStringLiteral("/fo?"));
    CPPUNIT_ASSERT(p10.matches(QStringLiteral("foO")));
    CPPUNIT_ASSERT(!p10.matches(QStringLiteral("foO/")));
    CPPUNIT_ASSERT(!p10.matches(QStringLiteral("fo/")));
    CPPUNIT_ASSERT(!p10.matches(QStringLiteral("bar/foO")));

    auto p11 = SyncthingIgnorePattern(QStringLiteral("/fo[o0.]/bar"));
    CPPUNIT_ASSERT(p11.matches(QStringLiteral("foo/bar")));
    CPPUNIT_ASSERT(p11.matches(QStringLiteral("fo0/bar")));
    CPPUNIT_ASSERT(p11.matches(QStringLiteral("fo./bar")));
    CPPUNIT_ASSERT(!p11.matches(QStringLiteral("foO/bar")));
    CPPUNIT_ASSERT(!p11.matches(QStringLiteral("fo?/bar")));

    auto p11a = SyncthingIgnorePattern(QStringLiteral("(?i)/fo[o0.]/bar"));
    CPPUNIT_ASSERT(p11a.caseInsensitive);
    CPPUNIT_ASSERT(p11a.matches(QStringLiteral("foO/bar")));

    auto p12 = SyncthingIgnorePattern(QStringLiteral("/f[oA-C0-3.]o/bar"));
    CPPUNIT_ASSERT(p12.matches(QStringLiteral("f0o/bar")));
    CPPUNIT_ASSERT(p12.matches(QStringLiteral("f1o/bar")));
    CPPUNIT_ASSERT(p12.matches(QStringLiteral("f2o/bar")));
    CPPUNIT_ASSERT(p12.matches(QStringLiteral("f3o/bar")));
    CPPUNIT_ASSERT(!p12.matches(QStringLiteral("f4o/bar")));
    CPPUNIT_ASSERT(p12.matches(QStringLiteral("foo/bar")));
    CPPUNIT_ASSERT(!p12.matches(QStringLiteral("foO/bar")));
    CPPUNIT_ASSERT(p12.matches(QStringLiteral("fAo/bar")));
    CPPUNIT_ASSERT(p12.matches(QStringLiteral("fBo/bar")));
    CPPUNIT_ASSERT(p12.matches(QStringLiteral("fCo/bar")));
    CPPUNIT_ASSERT(!p12.matches(QStringLiteral("fDo/bar")));

    auto p13 = SyncthingIgnorePattern(QStringLiteral("/f{o,0o0,l}o/{bar,biz}"));
    CPPUNIT_ASSERT(p13.matches(QStringLiteral("foo/bar")));
    CPPUNIT_ASSERT(p13.matches(QStringLiteral("foo/biz")));
    CPPUNIT_ASSERT(!p13.matches(QStringLiteral("f/biz")));
    CPPUNIT_ASSERT(!p13.matches(QStringLiteral("f0o0/bar")));
    CPPUNIT_ASSERT(p13.matches(QStringLiteral("f0o0o/bar")));
    CPPUNIT_ASSERT(!p13.matches(QStringLiteral("f{o,0o0,}o/{bar,biz}")));

    auto p14 = SyncthingIgnorePattern(QStringLiteral("/rust/**/target"));
    CPPUNIT_ASSERT(p14.matches(QStringLiteral("rust/formatter/target")));
    CPPUNIT_ASSERT(!p14.matches(QStringLiteral("rust/formatter/target/CACHEDIR.TAG")));

    auto p14a = SyncthingIgnorePattern(QStringLiteral("/rust/**/target/"));
    CPPUNIT_ASSERT(!p14a.matches(QStringLiteral("rust/formatter/target")));
    CPPUNIT_ASSERT(!p14a.matches(QStringLiteral("rust/formatter/target/CACHEDIR.TAG")));

    auto p15 = SyncthingIgnorePattern(QStringLiteral("/fo[\\]o]/bar"));
    CPPUNIT_ASSERT(p15.matches(QStringLiteral("foo/bar")));
    CPPUNIT_ASSERT(p15.matches(QStringLiteral("fo]/bar")));
    CPPUNIT_ASSERT(!p15.matches(QStringLiteral("fo\\/bar")));

    auto p16 = SyncthingIgnorePattern(QStringLiteral("/fo{o\\},o}/bar"));
    CPPUNIT_ASSERT(p16.matches(QStringLiteral("foo/bar")));
    CPPUNIT_ASSERT(p16.matches(QStringLiteral("foo}/bar")));
    CPPUNIT_ASSERT(!p16.matches(QStringLiteral("/foo\\,o}/bar")));

    auto p17 = SyncthingIgnorePattern(QStringLiteral("///fo{o\\},o}/bar"));
    CPPUNIT_ASSERT(p17.comment);
    CPPUNIT_ASSERT(!p17.matches(QStringLiteral("foo/bar")));

    auto p18 = SyncthingIgnorePattern(QStringLiteral("!Saved\\Logs"));
    CPPUNIT_ASSERT(!p18.matches(QStringLiteral("Documents\\Saved\\Logs")));
    CPPUNIT_ASSERT(p18.matches(QStringLiteral("Documents\\Saved\\Logs"), QChar('\\')));
    CPPUNIT_ASSERT(!p18.matches(QStringLiteral("Documents/Saved/Logs"), QChar('/'))); // see remarks in doc
    auto p18a = SyncthingIgnorePattern(QStringLiteral("!Saved/Logs"));
    CPPUNIT_ASSERT(p18a.matches(QStringLiteral("Documents\\Saved\\Logs"), QChar('\\')));
    CPPUNIT_ASSERT(p18a.matches(QStringLiteral("Documents/Saved/Logs"), QChar('\\')));
    auto p18b = SyncthingIgnorePattern(QStringLiteral("!/Documents/Saved/Logs"));
    CPPUNIT_ASSERT(p18b.matches(QStringLiteral("Documents\\Saved\\Logs"), QChar('\\')));
    CPPUNIT_ASSERT(p18b.matches(QStringLiteral("Documents/Saved/Logs"), QChar('\\')));
    CPPUNIT_ASSERT(!p18b.matches(QStringLiteral("Saved\\Logs"), QChar('\\')));
    CPPUNIT_ASSERT(!p18b.matches(QStringLiteral("Saved/Logs"), QChar('\\')));
}
