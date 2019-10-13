#include "../interface.h"

#include <c++utilities/chrono/datetime.h>
#include <c++utilities/chrono/timespan.h>
#include <c++utilities/conversion/stringbuilder.h>
#include <c++utilities/conversion/stringconversion.h>
#include <c++utilities/io/path.h>
#include <c++utilities/tests/testutils.h>

#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#include <cstdlib>
#include <filesystem>
#include <functional>
#include <thread>

#include <unistd.h>

using namespace std;
using namespace CppUtilities;
using namespace LibSyncthing;

using namespace CPPUNIT_NS;

/*!
 * \brief The InterfaceTests class tests the SyncthingConnector.
 */
class InterfaceTests : public TestFixture {
    CPPUNIT_TEST_SUITE(InterfaceTests);
    CPPUNIT_TEST(testInitialState);
    CPPUNIT_TEST(testVersion);
    CPPUNIT_TEST(testRunWidthConfig);
    CPPUNIT_TEST_SUITE_END();

public:
    InterfaceTests();

    void testInitialState();
    void testVersion();
    void testRunWidthConfig();

    void setUp() override;
    void tearDown() override;

private:
    std::string setupConfigDir();
    void testRun(const std::function<long long(void)> &runFunction);
};

CPPUNIT_TEST_SUITE_REGISTRATION(InterfaceTests);

InterfaceTests::InterfaceTests()
{
    setenv("STNOUPGRADE", "1", 1);
}

void InterfaceTests::setUp()
{
}

void InterfaceTests::tearDown()
{
}

/*!
 * \brief Initializes the Syncthing config for this fixture (currently using same config as in connector test).
 * \returns Returns the config directory.
 */
string InterfaceTests::setupConfigDir()
{
    // setup Syncthing config (currently using same config as in connector test)
    const auto configFilePath(workingCopyPath("testconfig/config.xml"));
    if (configFilePath.empty()) {
        throw runtime_error("Unable to setup Syncthing config directory.");
    }

    // clean database
    const auto configDir(directory(configFilePath));
    try {
        const auto dirIterator = filesystem::directory_iterator(configDir);
        for (const auto &dir : dirIterator) {
            const auto dirPath = dir.path();
            if (!dir.is_directory() || dirPath == "." || dirPath == "..") {
                continue;
            }
            const auto subdirIterator = filesystem::directory_iterator(dirPath);
            for (const auto &file : subdirIterator) {
                if (file.is_directory()) {
                    continue;
                }
                const auto toRemove = file.path().string();
                CPPUNIT_ASSERT_EQUAL_MESSAGE("removing " + toRemove, 0, remove(toRemove.data()));
            }
        }

    } catch (const filesystem::filesystem_error &error) {
        CPPUNIT_FAIL(argsToString("Unable to clean config dir ", configDir, ": ", error.what()));
    }
    return configDir;
}

/*!
 * \brief Tests behavior in initial state, when Syncthing isn't supposed to be running.
 */
void InterfaceTests::testInitialState()
{
    CPPUNIT_ASSERT_MESSAGE("initially not running", !isSyncthingRunning());

    // stopping Syncthing when not running should not cause any trouble
    stopSyncthing();
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

/*!
 * \brief Test helper for running Syncthing and checking log according to test configuration.
 */
void InterfaceTests::testRun(const std::function<long long()> &runFunction)
{
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
        } else if (msg == "Ready to synchronize test1 (sendreceive)") {
            testDir1Ready = true;
        } else if (msg == "Ready to synchronize test2 (sendreceive)") {
            testDir2Ready = true;
        } else if (msg == "Device 6EIS2PN-J2IHWGS-AXS3YUL-HC5FT3K-77ZXTLL-AKQLJ4C-7SWVPUS-AZW4RQ4 is \"Test dev 1\" at [dynamic]") {
            testDev1Ready = true;
        } else if (msg == "Device MMGUI6U-WUEZQCP-XZZ6VYB-LCT4TVC-ER2HAVX-QYT6X7D-S6ZSG2B-323KLQ7 is \"Test dev 2\" at [tcp://192.168.2.2:22001]") {
            testDev2Ready = true;
        } else if (msg == "Exiting") {
            shutDownLogged = true;
        }

        // print the message on cout (which results in duplicated messages, but allows to check whether we've got everything)
        cout << "logging callback (" << static_cast<std::underlying_type<LogLevel>::type>(logLevel) << "): ";
        cout.write(message, static_cast<std::streamsize>(messageSize));
        cout << endl;

        // stop Syncthing again if the found the messages we've been looking for or we've timed out
        const auto timeout((DateTime::gmtNow() - startTime) > TimeSpan::fromSeconds(30));
        if (!timeout && (!myIdAnnounced || !performanceAnnounced || !testDir1Ready || !testDev1Ready || !testDev2Ready)) {
            // log status
            cout << "still wating for:";
            if (!myIdAnnounced) {
                cout << " myIdAnnounced";
            }
            if (!performanceAnnounced) {
                cout << " performanceAnnounced";
            }
            if (!testDir1Ready) {
                cout << " testDir1Ready";
            }
            if (!testDir2Ready) {
                cout << " testDir2Ready";
            }
            if (!testDev1Ready) {
                cout << " testDev1Ready";
            }
            if (!testDev2Ready) {
                cout << " testDev2Ready";
            }
            cout << endl;
            return;
        }
        if (!shuttingDown) {
            cerr << "stopping Syncthing again" << endl;
            shuttingDown = true;
            std::thread stopThread(stopSyncthing);
            stopThread.detach();
        }
    });

    CPPUNIT_ASSERT_EQUAL_MESSAGE("Syncthing exited without error", 0ll, runFunction());

    // assert whether all expected log messages were present
    CPPUNIT_ASSERT(myIdAnnounced);
    CPPUNIT_ASSERT(performanceAnnounced);
    CPPUNIT_ASSERT(testDir1Ready);
    CPPUNIT_ASSERT(!testDir2Ready);
    CPPUNIT_ASSERT(testDev1Ready);
    CPPUNIT_ASSERT(testDev2Ready);
    CPPUNIT_ASSERT(shutDownLogged);

    // keep running a bit longer to check whether would not crash in the next few seconds
    // (could happen if Syncthing's extra threads haven't been stopped correctly)
    sleep(5);
}

/*!
 * \brief Tests whether Syncthing can be started (and stopped again) using the specified test config.
 */
void InterfaceTests::testRunWidthConfig()
{
    RuntimeOptions options;
    options.configDir = setupConfigDir();
    testRun(bind(static_cast<std::int64_t (*)(const RuntimeOptions &)>(&runSyncthing), cref(options)));
}
