#include "./syncthingconnectionmockhelpers.h"

#include <c++utilities/conversion/stringbuilder.h>
#include <c++utilities/io/ansiescapecodes.h>
#include <c++utilities/io/catchiofailure.h>
#include <c++utilities/io/misc.h>
#include <c++utilities/tests/testutils.h>

#include <QTimer>
#include <QUrlQuery>

#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>

using namespace std;
using namespace IoUtilities;
using namespace EscapeCodes;
using namespace ConversionUtilities;

namespace Data {

/*!
 * \brief Contains test data for mocked SyncthingConnection.
 */
namespace TestData {
static bool initialized = false;
static string config;
static string status;
static string folderStats;
static string deviceStats;
static string errors;
static string folderStatus;
static string folderStatus2;
static string connections;
static string events;
} // namespace TestData

/*!
 * \brief Loads test files for mocked configuration from directory specified via environment variable TESTFILE_PATH.
 *
 * So TESTFILE_PATH must be set to "$synthingtray_checkout/connector/testfiles" which contains the required files.
 *
 * \remarks In the error case, the application will be terminated.
 */
void setupTestData()
{
    using namespace TestData;

    if (initialized) {
        return;
    }

    const char *testfilePath = getenv("TESTFILE_PATH");
    if (!testfilePath || !*testfilePath) {
        cerr << Phrases::Error << "TESTFILE_PATH is not set; unable to initialize mock config." << Phrases::End << flush;
        exit(-1);
    }

    const char *const fileNames[]
        = { "config", "status", "folderstats", "devicestats", "errors", "folderstatus", "folderstatus2", "connections", "events" };
    const char *const *fileName = fileNames;
    for (string *testDataVariable : { &config, &status, &folderStats, &deviceStats, &errors, &folderStatus, &folderStatus2, &connections, &events }) {
        const string filePath(argsToString(testfilePath, "/mocks/", *fileName, ".json"));
        try {
            *testDataVariable = readFile(filePath);
            ++fileName;
        } catch (...) {
            const char *const what = catchIoFailure();
            cerr << "Error: An IO error occured when reading mock config file \"" << filePath << "\": " << what << endl;
            exit(-2);
        }
    }

    initialized = true;
}

MockedReply::MockedReply(const string &buffer, int delay, QObject *parent)
    : QNetworkReply(parent)
    , m_buffer(buffer)
    , m_pos(buffer.data())
    , m_bytesLeft(static_cast<qint64>(m_buffer.size()))
{
    delay = 5;
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

MockedReply *MockedReply::forRequest(const QString &method, const QString &path, const QUrlQuery &query, bool rest)
{
    VAR_UNUSED(query)
    VAR_UNUSED(rest)

    // set "mock URL"
    QUrl url((rest ? QStringLiteral("mock://rest/") : QStringLiteral("mock://")) + path);
    url.setQuery(query);

    // find the correct buffer for the request
    static const string emptyBuffer;
    const string *buffer = &emptyBuffer;
    int delay = 0;
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
                }
            } else if (path == QLatin1String("system/connections")) {
                buffer = &connections;
            } else if (path == QLatin1String("events")) {
                buffer = &events;
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
