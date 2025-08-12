#include "../interface.h"

#include <c++utilities/chrono/datetime.h>
#include <c++utilities/chrono/timespan.h>
#include <c++utilities/conversion/stringbuilder.h>
#include <c++utilities/conversion/stringconversion.h>
#include <c++utilities/io/path.h>
#include <c++utilities/tests/testutils.h>

#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <functional>
#include <thread>

#ifdef PLATFORM_WINDOWS
#include <windows.h>
#endif

using namespace std;
using namespace CppUtilities;
using namespace LibSyncthing;

using namespace CPPUNIT_NS;

/*!
 * \brief The InterfaceTests class tests the C++ interface of "libsyncthing".
 */
class InterfaceTests : public TestFixture {
    CPPUNIT_TEST_SUITE(InterfaceTests);
    CPPUNIT_TEST(testInitialState);
    CPPUNIT_TEST(testVersion);
    CPPUNIT_TEST(testRunWithoutConfig);
    CPPUNIT_TEST(testRunWithConfig);
    CPPUNIT_TEST(testRunCli);
    CPPUNIT_TEST(testRunCommand);
    CPPUNIT_TEST(testVerify);
    CPPUNIT_TEST_SUITE_END();

public:
    InterfaceTests();

    void testInitialState();
    void testVersion();
    void testRunWithoutConfig();
    void testRunWithConfig();
    void testRunCli();
    void testRunCommand();
    void testVerify();

    void setUp() override;
    void tearDown() override;

private:
    std::string setupTestConfigDir();
    void testRun(const std::function<long long(void)> &runFunction, bool assertTestConfig);
};

CPPUNIT_TEST_SUITE_REGISTRATION(InterfaceTests);

InterfaceTests::InterfaceTests()
{
#ifdef PLATFORM_WINDOWS
    SetEnvironmentVariableW(L"STNOUPGRADE", L"1");
#else
    setenv("STNOUPGRADE", "1", 1);
#endif
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
string InterfaceTests::setupTestConfigDir()
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
 * \brief Test helper for running Syncthing and checking log (according the the test configuration).
 */
void InterfaceTests::testRun(const std::function<long long()> &runFunction, bool assertTestConfig)
{
    // keep track of certain log messages
    const auto startTime(DateTime::gmtNow());
    bool myIdAnnounced = false, performanceAnnounced = false, guiAnnounced = false;
    bool testDir1Ready = false, testDir2Ready = false; // don't wait on dir2 as it is paused
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
        if (startsWith(msg, "My ID: ") || startsWith(msg, "Calculated our device ID")) {
            myIdAnnounced = true;
        } else if (startsWith(msg, "Single thread SHA256 performance is") || startsWith(msg, "Hashing performance is")
            || startsWith(msg, "Measured hashing performance")) {
            performanceAnnounced = true;
        } else if (startsWith(msg, "GUI and API listening")) {
            guiAnnounced = true;
        } else if (msg == "Ready to synchronize test1 (sendreceive)"
            || startsWith(msg, "Ready to synchronize (folder.id=test1 folder.type=sendreceive")) {
            testDir1Ready = true;
        } else if (msg == "Ready to synchronize test2 (sendreceive)" || msg == "Ready to synchronize \"Test dir 2\" (test2) (sendreceive)"
            || startsWith(msg, "Ready to synchronize (folder.id=test2 folder.type=sendreceive")) {
            testDir2Ready = true;
        } else if (msg == "Device 6EIS2PN-J2IHWGS-AXS3YUL-HC5FT3K-77ZXTLL-AKQLJ4C-7SWVPUS-AZW4RQ4 is \"Test dev 1\" at [dynamic]"
            || startsWith(msg, "Loaded peer device configuration (device=6EIS2PN name=\"Test dev 1\" address=\"[dynamic]\"")) {
            testDev1Ready = true;
        } else if (startsWith(msg, "Device MMGUI6U-WUEZQCP-XZZ6VYB-LCT4TVC-ER2HAVX-QYT6X7D-S6ZSG2B-323KLQ7 is \"Test dev 2\" at [tcp://192.168.2.2")
            || startsWith(msg, "Loaded peer device configuration (device=MMGUI6U name=\"Test dev 2\" address=\"[tcp://192.168.2.2")) {
            testDev2Ready = true;
        } else if (startsWith(msg, "Exiting")) {
            shutDownLogged = true;
        }

        // print the message on cout (which results in duplicated messages, but allows to check whether we've got everything)
        cout << "logging callback (" << static_cast<std::underlying_type<LogLevel>::type>(logLevel) << "): ";
        cout.write(message, static_cast<std::streamsize>(messageSize));
        cout << endl;

        // stop Syncthing again if the messages we've been looking for were logged or we've timed out
        const auto timeout((DateTime::gmtNow() - startTime) > TimeSpan::fromSeconds(30));
        if (!timeout
            && (!myIdAnnounced || !performanceAnnounced || !guiAnnounced
                || (assertTestConfig && (!testDir1Ready || !testDev1Ready || !testDev2Ready)))) {
            // log status
            cout << "still waiting for:";
            if (!myIdAnnounced) {
                cout << " myIdAnnounced";
            }
            if (!performanceAnnounced) {
                cout << " performanceAnnounced";
            }
            if (!guiAnnounced) {
                cout << " guiAnnounced";
            }
            if (assertTestConfig) {
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
            }
            cout << endl;
            return;
        }
        if (!shuttingDown) {
            cerr << "stopping Syncthing again" << endl;
            shuttingDown = true;
            std::thread(stopSyncthing).detach();
        }
    });

    CPPUNIT_ASSERT_EQUAL_MESSAGE("Syncthing exited without error", 0ll, runFunction());

    // assert whether all expected log messages were present
    CPPUNIT_ASSERT(myIdAnnounced);
    CPPUNIT_ASSERT(performanceAnnounced);
    if (assertTestConfig) {
        CPPUNIT_ASSERT(testDir1Ready);
        CPPUNIT_ASSERT(!testDir2Ready);
        CPPUNIT_ASSERT(testDev1Ready);
        CPPUNIT_ASSERT(testDev2Ready);
    }
    CPPUNIT_ASSERT(shutDownLogged);

    // check for random crashes afterwards
    if (assertTestConfig) {
        cerr << "\nkeep running a bit longer to check whether the application would not crash in the next few seconds"
                "\n(could happen if Syncthing's extra threads haven't been stopped correctly)";
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
}

/*!
 * \brief Tests whether Syncthing can be started (and stopped again) when the config directory has not been created yet.
 * \remarks It is expected that Syncthing creates the config directory automatically with a default config and new certs.
 */
void InterfaceTests::testRunWithoutConfig()
{
    RuntimeOptions options;
    options.configDir = TestApplication::instance()->workingDirectory() + "/does/not/exist";
    options.dataDir = TestApplication::instance()->workingDirectory() + "/does/also/not/exist";
    filesystem::remove_all(TestApplication::instance()->workingDirectory() + "/does");
    testRun(bind(static_cast<std::int64_t (*)(const RuntimeOptions &)>(&runSyncthing), cref(options)), false);
}

/*!
 * \brief Tests whether Syncthing can be started (and stopped again).
 * \remarks This test uses the usual test config (same as for connector and CLI) and runs some checks against it.
 */
void InterfaceTests::testRunWithConfig()
{
    RuntimeOptions options;
    options.configDir = options.dataDir = setupTestConfigDir();
    testRun(bind(static_cast<std::int64_t (*)(const RuntimeOptions &)>(&runSyncthing), cref(options)), true);
}

/*!
 * \brief Tests running Syncthing's CLI.
 */
void InterfaceTests::testRunCli()
{
    CPPUNIT_ASSERT_EQUAL_MESSAGE("run arbitrary CLI command", 0ll, runCli({ "config", "version", "--help" }));
}

/*!
 * \brief Tests running Syncthing command.
 */
void InterfaceTests::testRunCommand()
{
    CPPUNIT_ASSERT_EQUAL_MESSAGE("run arbitrary CLI command", 0ll, runCommand({ "--help" }));
}

/*!
 * \brief Tests the signature verification.
 */
void InterfaceTests::testVerify()
{
    const auto key = std::string_view(
        R"(-----BEGIN PUBLIC KEY-----
MIGbMBAGByqGSM49AgEGBSuBBAAjA4GGAAQBzGxkQSS43eE4r+A7HjlcEch5apsn
fKOgJWaRE2TOD9dNoBO2RSaJEAzzOXg2BPMsiPdr+Ty99FZtX8fmIcgJHGoB3sE1
PmSOaw3YWAXrHUYslrVRJI4iYCLuT4qjFMHgmqvphEE/zGDZ5Tyu6FwVlSjCO4Yy
FdsjpzKV6nrX6EsK++o=
-----END PUBLIC KEY-----)");
    const auto signature = std::string_view(
        R"(-----BEGIN SIGNATURE-----
MIGIAkIAzBD4hoa3O2V0PAwetDyU0/XyT3877ENN8VnOcpfIJDjNdHg5VErCZH1a
o+TV7g5vQt1o6UYBlw4h/cV7DS3y4I0CQgG5B5EfO/8lAA5+0KspBpZEEHDg99Jk
gACrHJpVPGnex+8fyo/y94wOi9y40tkItuNou136Z9DdfypZI4R/vNf0tA==
-----END SIGNATURE-----)");

    auto message = std::string("test message");
    CPPUNIT_ASSERT_EQUAL_MESSAGE("valid message", std::string(), LibSyncthing::verify(key, signature, message));

    message[5] = '?';
    CPPUNIT_ASSERT_EQUAL_MESSAGE("manipulated message", "incorrect signature"s, LibSyncthing::verify(key, signature, message));
}
