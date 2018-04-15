#include "../interface.h"

#include <c++utilities/chrono/datetime.h>
#include <c++utilities/chrono/timespan.h>
#include <c++utilities/conversion/stringconversion.h>
#include <c++utilities/io/path.h>
#include <c++utilities/tests/testutils.h>

#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

using namespace std;
using namespace ChronoUtilities;
using namespace ConversionUtilities;
using namespace IoUtilities;
using namespace TestUtilities;
using namespace LibSyncthing;

using namespace CPPUNIT_NS;

/*!
 * \brief The InterfaceTests class tests the SyncthingConnector.
 */
class InterfaceTests : public TestFixture {
    CPPUNIT_TEST_SUITE(InterfaceTests);
    CPPUNIT_TEST(testRun);
    CPPUNIT_TEST(testVersion);
    CPPUNIT_TEST_SUITE_END();

public:
    InterfaceTests();

    void testRun();
    void testVersion();

    void setUp();
    void tearDown();

private:
};

CPPUNIT_TEST_SUITE_REGISTRATION(InterfaceTests);

InterfaceTests::InterfaceTests()
{
}

void InterfaceTests::setUp()
{
}

void InterfaceTests::tearDown()
{
}

/*!
 * \brief Tests running Syncthing.
 */
void InterfaceTests::testRun()
{
    CPPUNIT_ASSERT_MESSAGE("initially not running", !isSyncthingRunning());

    // stopping and restarting Syncthing when not running should not cause any trouble
    stopSyncthing();
    restartSyncthing();

    // setup Syncthing config (currently using same config as in connector test)
    const auto configFilePath(workingCopyPath("testconfig/config.xml"));
    if (configFilePath.empty()) {
        throw runtime_error("Unable to setup Syncthing config directory.");
    }

    // setup runtime options
    RuntimeOptions options;
    options.configDir = directory(configFilePath);

    // keep track of certain log messages
    const auto startTime(DateTime::gmtNow());
    bool myIdAnnounced = false, performanceAnnounced = false;
    bool testDir1Ready = false, testDir2Ready = false;
    bool testDev1Ready = false, testDev2Ready = false;
    bool shuttingDown = false, shutDownLogged = false;

    setLoggingCallback([&](LogLevel logLevel, const char *message, std::size_t messageSize) {
        // ignore debug/verbose messages
        if (logLevel < LogLevel::Info) {
            return;
        }

        CPPUNIT_ASSERT_MESSAGE("Syncthing should be running right now", isSyncthingRunning());

        // check whether the usual log messages appear
        const string msg(message, messageSize);
        if (startsWith(msg, "My ID: ")) {
            myIdAnnounced = true;
        } else if (startsWith(msg, "Single thread SHA256 performance is")) {
            performanceAnnounced = true;
        } else if (msg == "Ready to synchronize test1 (readwrite)") {
            testDir1Ready = true;
        } else if (msg == "Ready to synchronize test2 (readwrite)") {
            testDir2Ready = true;
        } else if (msg == "Device 6EIS2PN-J2IHWGS-AXS3YUL-HC5FT3K-77ZXTLL-AKQLJ4C-7SWVPUS-AZW4RQ4 is \"Test dev 1\" at [dynamic]") {
            testDev1Ready = true;
        } else if (msg == "Device MMGUI6U-WUEZQCP-XZZ6VYB-LCT4TVC-ER2HAVX-QYT6X7D-S6ZSG2B-323KLQ7 is \"Test dev 2\" at [tcp://192.168.2.2:22000]") {
            testDev2Ready = true;
        } else if (msg == "Shutting down") {
            shutDownLogged = true;
        }

        // print the message on cout (which results in duplicated messages, but allows to check whether we've got everything)
        cout << "logging callback (" << static_cast<std::underlying_type<LogLevel>::type>(logLevel) << ": ";
        cout.write(message, static_cast<std::streamsize>(messageSize));
        cout << endl;

        // stop Syncthing again if the found the messages we've been looking for or we've timed out
        const auto timeout((DateTime::gmtNow() - startTime) > TimeSpan::fromSeconds(30));
        if (!timeout && (!myIdAnnounced || !performanceAnnounced || !testDir1Ready || !testDev1Ready || !testDev2Ready)) {
            return;
        }
        if (!shuttingDown) {
            cerr << "stopping Syncthing again" << endl;
            shuttingDown = true;
            stopSyncthing();
        }
    });

    CPPUNIT_ASSERT_EQUAL_MESSAGE("Syncthing exited without error", 0ll, runSyncthing(options));

    // assert whether all expected log messages were present
    CPPUNIT_ASSERT(myIdAnnounced);
    CPPUNIT_ASSERT(performanceAnnounced);
    CPPUNIT_ASSERT(testDir1Ready);
    CPPUNIT_ASSERT(!testDir2Ready);
    CPPUNIT_ASSERT(testDev1Ready);
    CPPUNIT_ASSERT(testDev2Ready);
    CPPUNIT_ASSERT(shutDownLogged);
}

/*!
 * \brief Tests whether the version() functions at least return something.
 */
void InterfaceTests::testVersion()
{
    const auto version(syncthingVersion());
    const auto longVersion(longSyncthingVersion());
    cout << "\nversion: " << version;
    cout << "\nlong version: " << longVersion << endl;
    CPPUNIT_ASSERT(!version.empty());
    CPPUNIT_ASSERT(!longVersion.empty());
}
