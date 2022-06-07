#include "./syncthingconnectionmockhelpers.h"

#include <c++utilities/conversion/stringbuilder.h>
#include <c++utilities/io/ansiescapecodes.h>
#include <c++utilities/io/misc.h>
#include <c++utilities/tests/testutils.h>

#include <QTimer>
#include <QUrlQuery>

#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>

using namespace std;
using namespace CppUtilities;
using namespace CppUtilities::EscapeCodes;

namespace Data {

/*!
 * \brief Contains test data for mocked SyncthingConnection.
 */
namespace TestData {
static bool initialized = false;
static string config, status, folderStats, deviceStats, errors, folderStatus, folderStatus2, folderStatus3, pullErrors, connections, version, empty;
static string events[7];
} // namespace TestData

/*!
 * \brief Returns the contents of the specified file and exits with an error message if an error occurs.
 */
static string readMockFile(const string &filePath)
{
    try {
        return readFile(filePath);
    } catch (const std::ios_base::failure &failure) {
        cerr << Phrases::Error << "An IO error occurred when reading mock config file \"" << filePath << "\": " << failure.what()
             << Phrases::EndFlush;
        exit(-2);
    }
}

/*!
 * \brief Loads test files for mocked configuration using TestApplication::testFilePath().
 * \remarks
 * - So TEST_FILE_PATH must be set to "$synthingtray_checkout/connector/testfiles" so this function can
 *   find the required files.
 * - In the error case, the application will be terminated.
 */
void setupTestData()
{
    // skip if already initialized
    using namespace TestData;
    if (initialized) {
        return;
    }

    // use a TestApplication to locate the test files
    const TestApplication testApp(0, nullptr);

    // read mock files for REST-API
    const char *const fileNames[] = { "config", "status", "folderstats", "devicestats", "errors", "folderstatus-01", "folderstatus-02",
        "folderstatus-03", "pullerrors-01", "connections", "version", "empty" };
    const char *const *fileName = fileNames;
    for (string *testDataVariable : { &config, &status, &folderStats, &deviceStats, &errors, &folderStatus, &folderStatus2, &folderStatus3,
             &pullErrors, &connections, &version, &empty }) {
        *testDataVariable = readMockFile(testApp.testFilePath(argsToString("mocks/", *fileName, ".json")));
        ++fileName;
    }

    // read mock files for Event-API
    unsigned int index = 1;
    for (string &event : events) {
        const char *const pad = index < 10 ? "0" : "";
        event = readMockFile(testApp.testFilePath(argsToString("mocks/events-", pad, index, ".json")));
        ++index;
    }

    initialized = true;
}

MockedReply::MockedReply(const string &buffer, int delay, QObject *parent)
    : QNetworkReply(parent)
    , m_buffer(buffer)
    , m_pos(buffer.data())
    , m_bytesLeft(static_cast<qint64>(m_buffer.size()))
{
    setOpenMode(QIODevice::ReadOnly);
    QTimer::singleShot(delay, this, &MockedReply::emitFinished);
}

MockedReply::~MockedReply()
{
}

void MockedReply::abort()
{
    close();
}

void MockedReply::close()
{
}

qint64 MockedReply::bytesAvailable() const
{
    return m_bytesLeft;
}

bool MockedReply::isSequential() const
{
    return true;
}

qint64 MockedReply::size() const
{
    return static_cast<qint64>(m_buffer.size());
}

qint64 MockedReply::readData(char *data, qint64 maxlen)
{
    if (!m_bytesLeft) {
        return -1;
    }
    const qint64 bytesToRead = static_cast<int>(min<qint64>(m_bytesLeft, maxlen));
    if (!bytesToRead) {
        return 0;
    }
    copy(m_pos, m_pos + bytesToRead, data);
    m_pos += bytesToRead;
    m_bytesLeft -= bytesToRead;
    return bytesToRead;
}

int MockedReply::s_eventIndex = 0;

MockedReply *MockedReply::forRequest(const QString &method, const QString &path, const QUrlQuery &query, bool rest)
{
    CPP_UTILITIES_UNUSED(query)
    CPP_UTILITIES_UNUSED(rest)

    // set "mock URL"
    QUrl url((rest ? QStringLiteral("mock://rest/") : QStringLiteral("mock://")) + path);
    url.setQuery(query);

    // find the correct buffer for the request
    static const string emptyBuffer;
    const string *buffer = &emptyBuffer;
    int delay = 5;
    {
        using namespace TestData;
        if (method == QLatin1String("GET")) {
            if (path == QLatin1String("system/config")) {
                buffer = &config;
            } else if (path == QLatin1String("system/status")) {
                buffer = &status;
            } else if (path == QLatin1String("stats/folder")) {
                buffer = &folderStats;
            } else if (path == QLatin1String("stats/device")) {
                buffer = &deviceStats;
            } else if (path == QLatin1String("system/error")) {
                buffer = &errors;
            } else if (path == QLatin1String("db/status")) {
                const QString folder(query.queryItemValue(QStringLiteral("folder")));
                if (folder == QLatin1String("GXWxf-3zgnU")) {
                    buffer = &folderStatus;
                } else if (folder == QLatin1String("zX8xfl3ygn-")) {
                    buffer = &folderStatus2;
                } else if (folder == QLatin1String("forever-alone")) {
                    buffer = &folderStatus3;
                }
            } else if (path == QLatin1String("folder/pullerrors")) {
                const QString folder(query.queryItemValue(QStringLiteral("folder")));
                if (folder == QLatin1String("GXWxf-3zgnU") && s_eventIndex >= 6) {
                    buffer = &pullErrors;
                }
            } else if (path == QLatin1String("system/connections")) {
                buffer = &connections;
            } else if (path == QLatin1String("system/version")) {
                buffer = &version;
            } else if (path == QLatin1String("events")) {
                buffer = &events[s_eventIndex];
                cerr << "mocking: at event index " << s_eventIndex << endl;
                // "emit" the first event almost immediately and further events each 2.5 seconds
                switch (s_eventIndex) {
                case 0:
                    delay = 200;
                    ++s_eventIndex;
                    break;
                case 6:
                    // continue emitting the last event every 10 seconds
                    delay = 10000;
                    break;
                default:
                    delay = 2000;
                    ++s_eventIndex;
                }
            } else if (path == QLatin1String("events/disk")) {
                buffer = &empty;
                delay = 5000;
            }
        }
    }

    // construct reply
    auto *const reply = new MockedReply(*buffer, delay);
    reply->setRequest(QNetworkRequest(url));
    return reply;
}

void MockedReply::emitFinished()
{
    if (m_buffer.empty()) {
        setError(QNetworkReply::InternalServerError, QStringLiteral("No mockup reply available for this request."));
        setAttribute(QNetworkRequest::HttpStatusCodeAttribute, 404);
        setAttribute(QNetworkRequest::HttpReasonPhraseAttribute, QLatin1String("Not found"));
    } else {
        setError(QNetworkReply::NoError, QString());
        setAttribute(QNetworkRequest::HttpStatusCodeAttribute, 200);
        setAttribute(QNetworkRequest::HttpReasonPhraseAttribute, QLatin1String("OK"));
    }
    setFinished(true);
    emit finished();
}
} // namespace Data
