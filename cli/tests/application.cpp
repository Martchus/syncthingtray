#include "../../testhelper/syncthingtestinstance.h"

#include <c++utilities/io/misc.h>
#include <c++utilities/tests/testutils.h>

#include <cppunit/TestFixture.h>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

#include <regex>

#include <sys/stat.h>
#include <unistd.h>

using namespace std;
using namespace Data;
using namespace ChronoUtilities;
using namespace IoUtilities;
using namespace TestUtilities;
using namespace TestUtilities::Literals;

using namespace CPPUNIT_NS;

/*!
 * \brief The ApplicationTests class tests the overall CLI application.
 */
class ApplicationTests : public TestFixture, private SyncthingTestInstance {
    CPPUNIT_TEST_SUITE(ApplicationTests);
#ifdef PLATFORM_UNIX
    CPPUNIT_TEST(test);
#endif
    CPPUNIT_TEST_SUITE_END();

public:
    ApplicationTests();

    void test();

    void setUp();
    void tearDown();

private:
    DateTime m_startTime;
};

CPPUNIT_TEST_SUITE_REGISTRATION(ApplicationTests);

ApplicationTests::ApplicationTests()
{
}

/*!
 * \brief Ensures Syncthing dirs are empty and starts Syncthing.
 */
void ApplicationTests::setUp()
{
    remove("/tmp/some/path/1/new-file.txt");
    remove("/tmp/some/path/1/newdir/yet-another-file.txt");
    remove("/tmp/some/path/1/newdir/default.profraw");
    rmdir("/tmp/some/path/1/newdir");

    SyncthingTestInstance::start();
    m_startTime = DateTime::gmtNow();
}

/*!
 * \brief Terminates Syncthing and prints stdout/stderr from Syncthing.
 */
void ApplicationTests::tearDown()
{
    SyncthingTestInstance::stop();
}

#ifdef PLATFORM_UNIX
/*!
 * \brief Tests the overall CLI application.
 */
void ApplicationTests::test()
{
    // prepare executing syncthingctl
    string stdout, stderr;
    const auto apiKey(this->apiKey().toLocal8Bit());
    const auto url(argsToString("http://localhost:", syncthingPort().toInt()));

    // disable colorful output
    setenv("ENABLE_ESCAPE_CODES", "0", true);

    // load expected status
    const auto expectedStatusData(readFile(testFilePath("expected-status.txt"), 2048));
    const regex expectedStatus(expectedStatusData);

    // save cwd (to restore later)
    char cwd[1024];
    const bool hasCwd(getcwd(cwd, sizeof(cwd)));

    // wait till Syncthing GUI becomes available
    cerr << "\nWaiting till Syncthing GUI becomes available ...";
    QByteArray syncthingOutput;
    do {
        // wait for output
        const auto timeout = static_cast<int>(15000 * TestUtilities::timeoutFactor);
        if (!syncthingProcess().bytesAvailable()) {
            // fail when already waiting for over 15 seconds
            const auto waitingTime(DateTime::gmtNow() - m_startTime);
            if (waitingTime.milliseconds() > timeout) {
                CPPUNIT_FAIL(argsToString("Syncthing needs longer than ", (timeout / 1000), " seconds to become available."));
            }
            syncthingProcess().waitForReadyRead(timeout - waitingTime.milliseconds());
        }
        syncthingOutput.append(syncthingProcess().readAll());
    } while (!syncthingOutput.contains("Access the GUI via the following URL"));

    // test status for all dirs and devs
    const char *const statusArgs[] = { "syncthingctl", "status", "--api-key", apiKey.data(), "--url", url.data(), nullptr };
    TESTUTILS_ASSERT_EXEC(statusArgs);
    cout << stdout;
    if (!regex_search(stdout, expectedStatus)) {
        cout << "expected status: " << expectedStatusData << '\n';
        CPPUNIT_FAIL("Actual status does not match expected status.");
    }

    // test log
    const char *const logArgs[] = { "syncthingctl", "log", "--api-key", apiKey.data(), "--url", url.data(), nullptr };
    TESTUTILS_ASSERT_EXEC(logArgs);
    cout << stdout;
    CPPUNIT_ASSERT(stdout.find("syncthing v") != string::npos);
    CPPUNIT_ASSERT(stdout.find("My ID") != string::npos);
    CPPUNIT_ASSERT(stdout.find("Startup complete") != string::npos);
    CPPUNIT_ASSERT(stdout.find("Access the GUI via the following URL") != string::npos);

    // use environment variables to specify API-key and URL
    setenv("SYNCTHING_CTL_API_KEY", apiKey.data(), true);
    setenv("SYNCTHING_CTL_URL", url.data(), true);

    // test resume, verify via status for dirs only
    const char *const resumeArgs[] = { "syncthingctl", "resume", "--dir", "test2", nullptr };
    const char *const statusDirsOnlyArgs[] = { "syncthingctl", "status", "--all-dirs", nullptr };
    TESTUTILS_ASSERT_EXEC(resumeArgs);
    TESTUTILS_ASSERT_EXEC(statusDirsOnlyArgs);
    CPPUNIT_ASSERT(stdout.find("test2") != string::npos);
    CPPUNIT_ASSERT(stdout.find("paused") == string::npos);

    // test pause, verify via status on specific dir
    const char *const pauseArgs[] = { "syncthingctl", "pause", "--dir", "test2", nullptr };
    const char *const statusTest2Args[] = { "syncthingctl", "status", "--dir", "test2", nullptr };
    TESTUTILS_ASSERT_EXEC(pauseArgs);
    TESTUTILS_ASSERT_EXEC(statusTest2Args);
    CPPUNIT_ASSERT(stdout.find("test1") == string::npos);
    CPPUNIT_ASSERT(stdout.find("test2") != string::npos);
    CPPUNIT_ASSERT(stdout.find("paused") != string::npos);

    // test cat
    const char *const catArgs[] = { "syncthingctl", "cat", nullptr };
    TESTUTILS_ASSERT_EXEC(catArgs);
    cout << stdout;
    QJsonParseError error;
    const auto doc(QJsonDocument::fromJson(QByteArray(stdout.data(), stdout.size()), &error));
    CPPUNIT_ASSERT_EQUAL(QJsonParseError::NoError, error.error);
    const auto object(doc.object());
    CPPUNIT_ASSERT(object.value(QLatin1String("options")).isObject());
    CPPUNIT_ASSERT(object.value(QLatin1String("devices")).isArray());
    CPPUNIT_ASSERT(object.value(QLatin1String("folders")).isArray());

    // test edit
#if defined(SYNCTHINGCTL_USE_JSENGINE) || defined(SYNCTHINGCTL_USE_SCRIPT)
    const char *const editArgs[] = { "syncthingctl", "edit", "--js-lines", "assignIfPresent(findFolder('test1'), 'rescanIntervalS', 0);", nullptr };
    const char *const statusTest1Args[] = { "syncthingctl", "status", "--dir", "test1", nullptr };
    TESTUTILS_ASSERT_EXEC(editArgs);
    cout << stdout;
    TESTUTILS_ASSERT_EXEC(statusTest1Args);
    cout << stdout;
    CPPUNIT_ASSERT(stdout.find("test1") != string::npos);
    CPPUNIT_ASSERT(stdout.find("test2") == string::npos);
    CPPUNIT_ASSERT(stdout.find("Rescan interval               file system watcher and periodic rescan disabled") != string::npos);
#endif

    // test rescan: create new file, trigger rescan, check status
    CPPUNIT_ASSERT(ofstream("/tmp/some/path/1/new-file.txt") << "foo");
    const char *const rescanArgs[] = { "syncthingctl", "rescan", "test1", nullptr };
    TESTUTILS_ASSERT_EXEC(rescanArgs);
    cout << stdout;
    TESTUTILS_ASSERT_EXEC(statusTest1Args);
    cout << stdout;
    CPPUNIT_ASSERT(stdout.find("test1") != string::npos);
    CPPUNIT_ASSERT(stdout.find("Local                         1 file(s), 0 dir(s), 3 bytes") != string::npos);

    // test pwd
    // -> create and enter new dir, also create a 2nd file in it
    chdir("/tmp/some/path/1");
    mkdir("newdir", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    chdir("newdir");
    CPPUNIT_ASSERT(ofstream("yet-another-file.txt") << "bar");
    // -> change LLVM_PROFILE_FILE to prevent default.profraw file being created in the new directory
    const char *const llvmProfileFile(getenv("LLVM_PROFILE_FILE"));
    setenv("LLVM_PROFILE_FILE", "/tmp/syncthingctl-%p.profraw", true);
    // -> do actual test
    const char *const pwdRescanArgs[] = { "syncthingctl", "pwd", "rescan", nullptr };
    TESTUTILS_ASSERT_EXEC(pwdRescanArgs);
    cout << stdout;
    // -> restore LLVM_PROFILE_FILE
    if (llvmProfileFile) {
        setenv("LLVM_PROFILE_FILE", llvmProfileFile, true);
    } else {
        unsetenv("LLVM_PROFILE_FILE");
    }
    // -> verify result
    const char *const pwdStatusArgs[] = { "syncthingctl", "pwd", "status", nullptr };
    TESTUTILS_ASSERT_EXEC(pwdStatusArgs);
    cout << stdout;
    CPPUNIT_ASSERT(stdout.find("test1") != string::npos);
    CPPUNIT_ASSERT(stdout.find("Local                         2 file(s), 1 dir(s)") != string::npos);

    // switch back to initial working dir
    if (hasCwd) {
        chdir(cwd);
    }
}
#endif
