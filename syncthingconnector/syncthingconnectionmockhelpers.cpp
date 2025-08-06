#include "./syncthingconnectionmockhelpers.h"

#include <c++utilities/conversion/stringbuilder.h>
#include <c++utilities/io/ansiescapecodes.h>
#include <c++utilities/io/misc.h>
#include <c++utilities/tests/testutils.h>

#include <QDebug>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QTimer>
#include <QUrlQuery>

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>

using namespace CppUtilities;
using namespace CppUtilities::EscapeCodes;

namespace Data {

/*!
 * \brief Contains test data for mocked SyncthingConnection.
 */
namespace TestData {
LIB_SYNCTHING_CONNECTOR_EXPORT extern bool initialized;
LIB_SYNCTHING_CONNECTOR_EXPORT extern std::string config, status, folderStats, deviceStats, errors, folderStatus, folderStatus2, folderStatus3, pullErrors, connections, version, empty, browse, needed, pendingFolders, pendingDevices;
LIB_SYNCTHING_CONNECTOR_EXPORT extern std::string events[7];

#if (defined(LIB_SYNCTHING_CONNECTOR_CONNECTION_MOCKED) && !defined(LIB_SYNCTHING_CONNECTOR_MOCKED)) || (!defined(LIB_SYNCTHING_CONNECTOR_CONNECTION_MOCKED) && defined(LIB_SYNCTHING_CONNECTOR_MOCKED))
#define LIB_SYNCTHING_CONNECTOR_CONNECTION_MOCKED_IMPLEMENTATION
bool initialized = false;
std::string config, status, folderStats, deviceStats, errors, folderStatus, folderStatus2, folderStatus3, pullErrors, connections, version, empty, browse, needed, pendingFolders, pendingDevices;
std::string events[7];
#endif
} // namespace TestData

#ifdef LIB_SYNCTHING_CONNECTOR_CONNECTION_MOCKED_IMPLEMENTATION

/*!
 * \brief Returns the contents of the specified file and exits with an error message if an error occurs.
 */
static std::string readMockFile(const std::string &filePath)
{
    try {
        return readFile(filePath);
    } catch (const std::ios_base::failure &failure) {
        std::cerr << Phrases::Error << "An IO error occurred when reading mock config file \"" << filePath << "\": " << failure.what()
             << Phrases::EndFlush;
        std::exit(-2);
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
        "folderstatus-03", "pullerrors-01", "connections", "version", "empty", "browse", "needed", "pending-folders", "pending-devices" };
    const char *const *fileName = fileNames;
    for (auto *const testDataVariable : { &config, &status, &folderStats, &deviceStats, &errors, &folderStatus, &folderStatus2, &folderStatus3,
             &pullErrors, &connections, &version, &empty, &browse, &needed, &pendingFolders, &pendingDevices }) {
        *testDataVariable = readMockFile(testApp.testFilePath(argsToString("mocks/", *fileName, ".json")));
        qDebug() << "Adding mock file: " << *fileName;
        ++fileName;
    }

    // read mock files for Event-API
    unsigned int index = 1;
    for (auto &event : events) {
        const char *const pad = index < 10 ? "0" : "";
        event = readMockFile(testApp.testFilePath(argsToString("mocks/events-", pad, index, ".json")));
        ++index;
    }

    initialized = true;
}

#endif

MockedReply::MockedReply(QByteArray &&buffer, int delay, QObject *parent)
    : QNetworkReply(parent)
    , m_buffer(std::move(buffer))
    , m_view(std::string_view(m_buffer.data(), static_cast<std::size_t>(m_buffer.size())))
    , m_pos(m_view.data())
    , m_bytesLeft(static_cast<qint64>(m_view.size()))
{
    init(delay);
}

MockedReply::MockedReply(std::string_view view, int delay, QObject *parent)
    : QNetworkReply(parent)
    , m_view(view)
    , m_pos(view.data())
    , m_bytesLeft(static_cast<qint64>(view.size()))
{
    init(delay);
}

void Data::MockedReply::init(int delay)
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
    return static_cast<qint64>(m_view.size());
}

qint64 MockedReply::readData(char *data, qint64 maxlen)
{
    if (!m_bytesLeft) {
        return -1;
    }
    const auto bytesToRead = std::min<qint64>(m_bytesLeft, maxlen);
    if (!bytesToRead) {
        return 0;
    }
    std::copy(m_pos, m_pos + bytesToRead, data);
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

    // find the correct response for the request
    auto buffer = QByteArray();
    auto response = std::string_view();
    int delay = 5;
    {
        using namespace TestData;
        if (method == QLatin1String("GET")) {
            if (path == QLatin1String("system/config")) {
                response = config;
            } else if (path == QLatin1String("system/status")) {
                response = status;
            } else if (path == QLatin1String("stats/folder")) {
                response = folderStats;
            } else if (path == QLatin1String("stats/device")) {
                response = deviceStats;
            } else if (path == QLatin1String("system/error")) {
                response = errors;
            } else if (path == QLatin1String("db/status")) {
                const QString folder(query.queryItemValue(QStringLiteral("folder")));
                if (folder == QLatin1String("GXWxf-3zgnU")) {
                    response = folderStatus;
                } else if (folder == QLatin1String("zX8xfl3ygn-")) {
                    response = folderStatus2;
                } else if (folder == QLatin1String("forever-alone")) {
                    response = folderStatus3;
                }
            } else if (path == QLatin1String("db/browse") && !query.hasQueryItem(QStringLiteral("prefix"))) {
                const auto folder = query.queryItemValue(QStringLiteral("folder"));
                if (folder == QLatin1String("GXWxf-3zgnU")) {
                    response = browse;
                }
            } else if (path == QLatin1String("folder/pullerrors")) {
                const QString folder(query.queryItemValue(QStringLiteral("folder")));
                if (folder == QLatin1String("GXWxf-3zgnU") && s_eventIndex >= 6) {
                    response = pullErrors;
                }
            } else if (path == QLatin1String("db/need")) {
                const auto folder = query.queryItemValue(QStringLiteral("folder"));
                if (folder == QLatin1String("GXWxf-3zgnU")) {
                    const auto page = query.queryItemValue(QStringLiteral("page")).toInt();
                    const auto perPage = query.queryItemValue(QStringLiteral("perpage")).toInt();
                    if (page > 0 || perPage > 0) {
                        const auto minIndex = (page - 1) * perPage;
                        const auto maxIndex = minIndex + perPage - 1;
                        auto currentIndex = 0;
                        auto obj = QJsonDocument::fromJson(QByteArray(needed.data(), static_cast<QByteArray::size_type>(needed.size()))).object();
                        auto keys = obj.keys();
                        auto filteredObj = QJsonObject();
                        for (const auto &key : keys) {
                            auto value = obj.value(key);
                            if (key == QStringLiteral("page") || key == QStringLiteral("perpage")) {
                                filteredObj.insert(key, value);
                                continue;
                            }
                            auto array = value.toArray();
                            auto newArray = QJsonArray();
                            for (const auto &item : array) {
                                if (currentIndex >= minIndex && currentIndex <= maxIndex) {
                                    newArray.append(item);
                                }
                                ++currentIndex;
                            }
                            filteredObj.insert(key, newArray);
                        }
                        buffer = QJsonDocument(filteredObj).toJson();
                    } else {
                        response = needed;
                    }
                }
            } else if (path == QLatin1String("system/connections")) {
                response = connections;
            } else if (path == QLatin1String("system/version")) {
                response = version;
            } else if (path == QLatin1String("cluster/pending/folders")) {
                response = pendingFolders;
            } else if (path == QLatin1String("cluster/pending/devices")) {
                response = pendingDevices;
            } else if (path == QLatin1String("events")) {
                response = events[s_eventIndex];
                std::cerr << "mocking: at event index " << s_eventIndex << std::endl;
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
                response = empty;
                delay = 5000;
            }
        }
    }

    // construct reply
    auto *const reply = buffer.isEmpty() ? new MockedReply(response, delay) : new MockedReply(std::move(buffer), delay);
    reply->setRequest(QNetworkRequest(url));
    return reply;
}

void MockedReply::emitFinished()
{
    if (m_view.empty()) {
        setError(QNetworkReply::InternalServerError, QStringLiteral("No mockup reply available for request: ") + request().url().toString());
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
