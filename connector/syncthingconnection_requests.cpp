#include "./syncthingconnection.h"
#include "./utils.h"

#ifdef LIB_SYNCTHING_CONNECTOR_CONNECTION_MOCKED
#include "./syncthingconnectionmockhelpers.h"
#endif

#include <c++utilities/conversion/conversionexception.h>
#include <c++utilities/conversion/stringconversion.h>
#include <c++utilities/io/ansiescapecodes.h>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonValue>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QStringBuilder>
#include <QTimer>
#include <QUrlQuery>

#include <iostream>
#include <utility>

using namespace std;
using namespace CppUtilities;
using namespace CppUtilities::EscapeCodes;

namespace Data {

// helper to create QNetworkRequest

/*!
 * \brief Prepares a request for the specified \a path and \a query.
 */
QNetworkRequest SyncthingConnection::prepareRequest(const QString &path, const QUrlQuery &query, bool rest)
{
    QUrl url(m_syncthingUrl);
    url.setPath(rest ? (url.path() % QStringLiteral("/rest/") % path) : (url.path() + path));
    url.setUserName(user());
    url.setPassword(password());
    url.setQuery(query);
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, QByteArray("application/x-www-form-urlencoded"));
    request.setRawHeader("X-API-Key", m_apiKey);
    return request;
}

/*!
 * \brief Requests asynchronously data using the rest API.
 */
QNetworkReply *SyncthingConnection::requestData(const QString &path, const QUrlQuery &query, bool rest)
{
#ifndef LIB_SYNCTHING_CONNECTOR_CONNECTION_MOCKED
    auto *const reply = networkAccessManager().get(prepareRequest(path, query, rest));
    QObject::connect(reply, &QNetworkReply::sslErrors, this, &SyncthingConnection::handleSslErrors);
    if (loggingFlags() & SyncthingConnectionLoggingFlags::ApiCalls) {
        cerr << Phrases::Info << "Querying API: GET " << reply->url().toString().toStdString() << Phrases::EndFlush;
    }
    return reply;
#else
    return MockedReply::forRequest(QStringLiteral("GET"), path, query, rest);
#endif
}

/*!
 * \brief Posts asynchronously data using the rest API.
 */
QNetworkReply *SyncthingConnection::postData(const QString &path, const QUrlQuery &query, const QByteArray &data)
{
    auto *const reply = networkAccessManager().post(prepareRequest(path, query), data);
    QObject::connect(reply, &QNetworkReply::sslErrors, this, &SyncthingConnection::handleSslErrors);
    if (loggingFlags() & SyncthingConnectionLoggingFlags::ApiCalls) {
        cerr << Phrases::Info << "Querying API: POST " << reply->url().toString().toStdString() << Phrases::EndFlush;
    }
    return reply;
}

/*!
 * \brief Prepares the current reply.
 */
SyncthingConnection::Reply SyncthingConnection::prepareReply(bool readData, bool handleAborting)
{
    return handleReply(static_cast<QNetworkReply *>(sender()), readData, handleAborting);
}

/*!
 * \brief Prepares the current reply.
 */
SyncthingConnection::Reply SyncthingConnection::prepareReply(QNetworkReply *&expectedReply, bool readData, bool handleAborting)
{
    auto *const reply = static_cast<QNetworkReply *>(sender());
    if (reply == expectedReply) {
        expectedReply = nullptr; // unset the expected reply so it is no longer considered pending
    }
    return handleReply(reply, readData, handleAborting);
}

/*!
 * \brief Prepares the current reply.
 */
SyncthingConnection::Reply SyncthingConnection::prepareReply(QList<QNetworkReply *> &expectedReplies, bool readData, bool handleAborting)
{
    auto *const reply = static_cast<QNetworkReply *>(sender());
    expectedReplies.removeAll(reply); // unset the expected reply so it is no longer considered pending
    return handleReply(reply, readData, handleAborting);
}

/// \cond
static QString certText(const QSslCertificate &cert)
{
    auto text = cert.toText();
    if (text.startsWith(QStringLiteral("Certificate: "))) {
        return text;
    }
    if (text.isEmpty()) {
        // .toText() is not implemented for all backends so use .toPem() as fallback
        text = QChar('\n') + QString::fromUtf8(cert.toPem());
    }
    return text.isEmpty() ? QStringLiteral("Certificate: [no information available]") : QStringLiteral("Certificate: ") + text;
}
/// \endcond

/*!
 * \brief Handles SSL errors of replies; just for logging purposes at this point.
 */
void SyncthingConnection::handleSslErrors(const QList<QSslError> &errors)
{
    // check SSL errors for replies
    auto *const reply = static_cast<QNetworkReply *>(sender());
    auto hasUnexpectedErrors = false;

    for (const auto &error : errors) {
        // skip expected errors
        // note: This would be required even when calling reply->ignoreSslErrors(m_expectedSslErrors) before so we
        //       are omitting that call and just check it here.
        if (m_expectedSslErrors.contains(error)) {
            continue;
        }

        // handle the error by emitting the error signal with all the details including the certificate
        // note: Of course the failing request would cause a QNetworkReply::SslHandshakeFailedError anyways. However,
        //       at this point the concrete SSL error with the certificate is not accessible anymore.
        auto errorMessage
            = QString(QStringLiteral("TLS error: ") % error.errorString() % QChar(' ') % QChar('(') % QString::number(error.error()) % QChar(')'));
        if (const auto cert = error.certificate(); !cert.isNull()) {
            errorMessage += QChar('\n');
            if (cert == m_certFromLastSslError) {
                errorMessage += QStringLiteral("Certificate: same as last");
            } else {
                errorMessage += certText(cert);
                if (!m_expectedSslErrors.isEmpty()) {
                    errorMessage += QStringLiteral("\nExpected ") + certText(m_expectedSslErrors.front().certificate());
                }
                m_certFromLastSslError = cert;
            }
        }
        emit this->error(errorMessage, SyncthingErrorCategory::TLS, QNetworkReply::NoError, reply->request());
        hasUnexpectedErrors = true;
    }

    // proceed if all errors are expected
    if (!hasUnexpectedErrors) {
        reply->ignoreSslErrors();
    }
}

/*!
 * \brief Handles the specified \a reply; invoked by the prepareReply() functions.
 */
SyncthingConnection::Reply SyncthingConnection::handleReply(QNetworkReply *reply, bool readData, bool handleAborting)
{
    const auto log = m_loggingFlags & SyncthingConnectionLoggingFlags::ApiReplies;
    readData = (readData || log) && reply->isOpen();
    handleAborting = handleAborting && m_abortingAllRequests;
    const auto data = Reply{
        .reply = handleAborting ? nullptr : reply, // skip further processing if aborting to reconnect
        .response = readData ? reply->readAll() : QByteArray(),
    };
    reply->deleteLater();

    if (log) {
        const auto url = reply->url();
        const auto path = url.path().toUtf8();
        const auto urlStr = url.toString().toUtf8();
        cerr << Phrases::Info << "Received reply for: " << std::string_view(urlStr.data(), static_cast<std::string_view::size_type>(urlStr.size()))
             << Phrases::EndFlush;
        if (!data.response.isEmpty() && path != "/rest/events"
            && path != "/rest/events/disk") { // events are logged separately because they are not always useful but make the log very verbose
            cerr << std::string_view(data.response.data(), static_cast<std::string_view::size_type>(data.response.size()));
        }
    }
    if (handleAborting) {
        handleAdditionalRequestCanceled();
    }
    return data;
}

// pause/resume devices

/*!
 * \brief Requests pausing the devices with the specified IDs.
 *
 * The signal error() is emitted when the request was not successful.
 */
bool SyncthingConnection::pauseDevice(const QStringList &devIds)
{
    return pauseResumeDevice(devIds, true);
}

/*!
 * \brief Requests pausing all devices.
 *
 * The signal error() is emitted when the request was not successful.
 */
bool SyncthingConnection::pauseAllDevs()
{
    return pauseResumeDevice(deviceIds(), true);
}

/*!
 * \brief Requests resuming the devices with the specified IDs.
 *
 * The signal error() is emitted when the request was not successful.
 */
bool SyncthingConnection::resumeDevice(const QStringList &devIds)
{
    return pauseResumeDevice(devIds, false);
}

/*!
 * \brief Requests resuming all devices.
 *
 * The signal error() is emitted when the request was not successful.
 */
bool SyncthingConnection::resumeAllDevs()
{
    return pauseResumeDevice(deviceIds(), false);
}

/*!
 * \brief Internally used to pause/resume directories.
 * \returns Returns whether a request has been made.
 * \remarks This might currently result in errors caused by Syncthing not
 *          handling E notation correctly when using Qt < 5.9:
 *          https://github.com/syncthing/syncthing/issues/4001
 */
bool SyncthingConnection::pauseResumeDevice(const QStringList &devIds, bool paused)
{
    if (devIds.isEmpty()) {
        return false;
    }
    if (!isConnected()) {
        emit error(tr("Unable to pause/resume a devices when not connected"), SyncthingErrorCategory::SpecificRequest, QNetworkReply::NoError);
        return false;
    }

    QJsonObject config = m_rawConfig;
    if (!setDevicesPaused(config, devIds, paused)) {
        return false;
    }

    QJsonDocument doc;
    doc.setObject(config);
    QNetworkReply *const reply = postData(QStringLiteral("system/config"), QUrlQuery(), doc.toJson(QJsonDocument::Compact));
    reply->setProperty("devIds", devIds);
    reply->setProperty("resume", !paused);
    QObject::connect(reply, &QNetworkReply::finished, this, &SyncthingConnection::readDevPauseResume);
    return true;
}

/*!
 * \brief Reads results of pauseDevice() and resumeDevice().
 */
void SyncthingConnection::readDevPauseResume()
{
    auto const [reply, response] = prepareReply(false);
    if (!reply) {
        return;
    }

    switch (reply->error()) {
    case QNetworkReply::NoError: {
        const QStringList devIds = reply->property("devIds").toStringList();
        const bool resume = reply->property("resume").toBool();
        setDevicesPaused(m_rawConfig, devIds, !resume);
        if (reply->property("resume").toBool()) {
            emit deviceResumeTriggered(devIds);
        } else {
            emit devicePauseTriggered(devIds);
        }
        break;
    }
    default:
        emitError(tr("Unable to request device pause/resume: "), SyncthingErrorCategory::SpecificRequest, reply);
    }
}

// pause/resume directories

/*!
 * \brief Pauses the directories with the specified IDs.
 * \remarks Calling this method when not connected results in an error because the *current* Syncthing config must
 *          be available for this call.
 * \returns Returns whether a request has been made.
 * The signal error() is emitted when the request was not successful.
 */
bool SyncthingConnection::pauseDirectories(const QStringList &dirIds)
{
    return pauseResumeDirectory(dirIds, true);
}

/*!
 * \brief Pauses all directories.
 * \remarks Calling this method when not connected results in an error because the *current* Syncthing config must
 *          be available for this call.
 *
 * The signal error() is emitted when the request was not successful.
 */
bool SyncthingConnection::pauseAllDirs()
{
    return pauseResumeDirectory(directoryIds(), true);
}

/*!
 * \brief Resumes the directories with the specified IDs.
 * \remarks Calling this method when not connected results in an error because the *current* Syncthing config must
 *          be available for this call.
 *
 * The signal error() is emitted when the request was not successful.
 */
bool SyncthingConnection::resumeDirectories(const QStringList &dirIds)
{
    return pauseResumeDirectory(dirIds, false);
}

/*!
 * \brief Resumes all directories.
 * \remarks Calling this method when not connected results in an error because the *current* Syncthing config must
 *          be available for this call.
 *
 * The signal error() is emitted when the request was not successful.
 */
bool SyncthingConnection::resumeAllDirs()
{
    return pauseResumeDirectory(directoryIds(), false);
}

/*!
 * \brief Internally used to pause/resume directories.
 * \returns Returns whether a request has been made.
 * \remarks This might currently result in errors caused by Syncthing not
 *          handling E notation correctly when using Qt < 5.9:
 *          https://github.com/syncthing/syncthing/issues/4001
 */
bool SyncthingConnection::pauseResumeDirectory(const QStringList &dirIds, bool paused)
{
    if (dirIds.isEmpty()) {
        return false;
    }
    if (!isConnected()) {
        emit error(tr("Unable to pause/resume a directories when not connected"), SyncthingErrorCategory::SpecificRequest, QNetworkReply::NoError);
        return false;
    }

    QJsonObject config = m_rawConfig;
    if (setDirectoriesPaused(config, dirIds, paused)) {
        QJsonDocument doc;
        doc.setObject(config);
        QNetworkReply *const reply = postData(QStringLiteral("system/config"), QUrlQuery(), doc.toJson(QJsonDocument::Compact));
        reply->setProperty("dirIds", dirIds);
        reply->setProperty("resume", !paused);
        QObject::connect(reply, &QNetworkReply::finished, this, &SyncthingConnection::readDirPauseResume);
        return true;
    }
    return false;
}

void SyncthingConnection::readDirPauseResume()
{
    auto const [reply, response] = prepareReply(false);
    if (!reply) {
        return;
    }

    switch (reply->error()) {
    case QNetworkReply::NoError: {
        const QStringList dirIds = reply->property("dirIds").toStringList();
        const bool resume = reply->property("resume").toBool();
        setDirectoriesPaused(m_rawConfig, dirIds, !resume);
        if (resume) {
            emit directoryResumeTriggered(dirIds);
        } else {
            emit directoryPauseTriggered(dirIds);
        }
        break;
    }
    default:
        emitError(tr("Unable to request directory pause/resume: "), SyncthingErrorCategory::SpecificRequest, reply);
    }
}

// rescan directories

/*!
 * \brief Requests rescanning all directories.
 *
 * Note that rescan is only requested for unpaused directories because requesting rescan for
 * paused directories only leads to an error.
 *
 * The signal error() is emitted when the request was not successful.
 */
void SyncthingConnection::rescanAllDirs()
{
    for (const SyncthingDir &dir : m_dirs) {
        if (!dir.paused) {
            rescan(dir.id);
        }
    }
}

/*!
 * \brief Requests rescanning the directory with the specified ID.
 *
 * The signal error() is emitted when the request was not successful.
 */
void SyncthingConnection::rescan(const QString &dirId, const QString &relpath)
{
    if (dirId.isEmpty()) {
        emit error(tr("Unable to rescan: No directory ID specified."), SyncthingErrorCategory::SpecificRequest, QNetworkReply::NoError,
            QNetworkRequest(), QByteArray());
        return;
    }

    QUrlQuery query;
    query.addQueryItem(QStringLiteral("folder"), dirId);
    if (!relpath.isEmpty()) {
        query.addQueryItem(QStringLiteral("sub"), relpath);
    }
    QNetworkReply *reply = postData(QStringLiteral("db/scan"), query);
    reply->setProperty("dirId", dirId);
    QObject::connect(reply, &QNetworkReply::finished, this, &SyncthingConnection::readRescan);
}

/*!
 * \brief Reads results of rescan().
 */
void SyncthingConnection::readRescan()
{
    auto const [reply, response] = prepareReply(false);
    if (!reply) {
        return;
    }

    switch (reply->error()) {
    case QNetworkReply::NoError:
        emit rescanTriggered(reply->property("dirId").toString());
        break;
    default:
        emitError(tr("Unable to request rescan: "), SyncthingErrorCategory::SpecificRequest, reply);
    }
}

// restart/shutdown Syncthing

/*!
 * \brief Requests Syncthing to restart.
 *
 * The signal error() is emitted when the request was not successful.
 */
void SyncthingConnection::restart()
{
    QObject::connect(postData(QStringLiteral("system/restart"), QUrlQuery()), &QNetworkReply::finished, this, &SyncthingConnection::readRestart);
}

/*!
 * \brief Reads results of restart().
 */
void SyncthingConnection::readRestart()
{
    auto const [reply, response] = prepareReply(false);
    if (!reply) {
        return;
    }

    switch (reply->error()) {
    case QNetworkReply::NoError:
        emit restartTriggered();
        break;
    default:
        emitError(tr("Unable to request restart: "), SyncthingErrorCategory::SpecificRequest, reply);
    }
}

/*!
 * \brief Requests Syncthing to exit and not restart.
 *
 * The signal error() is emitted when the request was not successful.
 */
void SyncthingConnection::shutdown()
{
    QObject::connect(postData(QStringLiteral("system/shutdown"), QUrlQuery()), &QNetworkReply::finished, this, &SyncthingConnection::readShutdown);
}

/*!
 * \brief Reads results of shutdown().
 */
void SyncthingConnection::readShutdown()
{
    auto const [reply, response] = prepareReply(false);
    if (!reply) {
        return;
    }

    switch (reply->error()) {
    case QNetworkReply::NoError:
        emit shutdownTriggered();
        break;
    default:
        emitError(tr("Unable to request shutdown: "), SyncthingErrorCategory::SpecificRequest, reply);
    }
}

// clear errors

/*!
 * \brief Requests clearing errors asynchronously.
 *
 * The signal error() is emitted in the error case.
 */
void SyncthingConnection::requestClearingErrors()
{
    QObject::connect(
        postData(QStringLiteral("system/error/clear"), QUrlQuery()), &QNetworkReply::finished, this, &SyncthingConnection::readClearingErrors);
}

/*!
 * \brief Reads results of requestClearingErrors().
 */
void SyncthingConnection::readClearingErrors()
{
    auto const [reply, response] = prepareReply(false);
    if (!reply) {
        return;
    }

    switch (reply->error()) {
    case QNetworkReply::NoError:
        break;
    default:
        emitError(tr("Unable to request clearing errors: "), SyncthingErrorCategory::SpecificRequest, reply);
    }
}

// overall Syncthing config (most importantly settings, directories and devices)

/*!
 * \brief Requests the Syncthing configuration asynchronously.
 *
 * The signal newConfig() is emitted on success; otherwise error() is emitted.
 */
void SyncthingConnection::requestConfig()
{
    if (m_configReply) {
        return;
    }
    QObject::connect(
        m_configReply = requestData(QStringLiteral("system/config"), QUrlQuery()), &QNetworkReply::finished, this, &SyncthingConnection::readConfig);
}

/*!
 * \brief Reads results of requestConfig().
 */
void SyncthingConnection::readConfig()
{
    auto const [reply, response] = prepareReply(m_configReply);
    if (!reply) {
        return;
    }

    switch (reply->error()) {
    case QNetworkReply::NoError: {
        QJsonParseError jsonError;
        const QJsonDocument replyDoc = QJsonDocument::fromJson(response, &jsonError);
        if (jsonError.error != QJsonParseError::NoError) {
            emitError(tr("Unable to parse Syncthing config: "), jsonError, reply, response);
            handleFatalConnectionError();
            return;
        }

        m_rawConfig = replyDoc.object();
        m_hasConfig = true;
        emit newConfig(m_rawConfig);

        if (m_keepPolling) {
            concludeReadingConfigAndStatus();
            return;
        }

        readDevs(m_rawConfig.value(QLatin1String("devices")).toArray());
        readDirs(m_rawConfig.value(QLatin1String("folders")).toArray());
        emit newConfigApplied();
        break;
    }
    case QNetworkReply::OperationCanceledError:
        return;
    default:
        emitError(tr("Unable to request Syncthing config: "), SyncthingErrorCategory::OverallConnection, reply);
        handleFatalConnectionError();
    }
}

/*!
 * \brief Reads directory results of requestConfig(); called by readConfig().
 * \remarks
 * - The devs are required to resolve the names of the devices a directory is shared with.
 *   So when parsing the config, readDevs() should be called first.
 * - The own device ID is required to filter it from the devices a directory is shared with.
 *   So the readStatus() should have been called first.
 */
void SyncthingConnection::readDirs(const QJsonArray &dirs)
{
    // store the new dirs in a temporary list which is assigned to m_dirs later
    std::vector<SyncthingDir> newDirs;
    newDirs.reserve(static_cast<size_t>(dirs.size()));

    int dummy;
    for (const QJsonValue &dirVal : dirs) {
        const QJsonObject dirObj(dirVal.toObject());
        SyncthingDir *const dirItem = addDirInfo(newDirs, dirObj.value(QLatin1String("id")).toString());
        if (!dirItem) {
            continue;
        }

        dirItem->label = dirObj.value(QLatin1String("label")).toString();
        dirItem->path = dirObj.value(QLatin1String("path")).toString();
        dirItem->deviceIds.clear();
        dirItem->deviceNames.clear();
        for (const QJsonValueRef devObj : dirObj.value(QLatin1String("devices")).toArray()) {
            const QString devId = devObj.toObject().value(QLatin1String("deviceID")).toString();
            if (devId.isEmpty() || devId == m_myId) {
                continue;
            }
            dirItem->deviceIds << devId;
            if (const SyncthingDev *const dev = findDevInfo(devId, dummy)) {
                dirItem->deviceNames << dev->name;
            }
        }
        dirItem->assignDirType(dirObj.value(QLatin1String("type")).toString());
        dirItem->rescanInterval = dirObj.value(QLatin1String("rescanIntervalS")).toInt(-1);
        dirItem->ignorePermissions = dirObj.value(QLatin1String("ignorePerms")).toBool(false);
        dirItem->ignoreDelete = dirObj.value(QLatin1String("ignoreDelete")).toBool(false);
        dirItem->autoNormalize = dirObj.value(QLatin1String("autoNormalize")).toBool(false);
        dirItem->minDiskFreePercentage = dirObj.value(QLatin1String("minDiskFreePct")).toInt(-1);
        dirItem->paused = dirObj.value(QLatin1String("paused")).toBool(dirItem->paused);
        dirItem->fileSystemWatcherEnabled = dirObj.value(QLatin1String("fsWatcherEnabled")).toBool(false);
        dirItem->fileSystemWatcherDelay = dirObj.value(QLatin1String("fsWatcherDelayS")).toDouble(0.0);
    }

    m_dirs.swap(newDirs);
    emit this->newDirs(m_dirs);
}

/*!
 * \brief Reads device results of requestConfig(); called by readConfig().
 */
void SyncthingConnection::readDevs(const QJsonArray &devs)
{
    // store the new devs in a temporary list which is assigned to m_devs later
    vector<SyncthingDev> newDevs;
    newDevs.reserve(static_cast<size_t>(devs.size()));

    for (const QJsonValue &devVal : devs) {
        const QJsonObject devObj(devVal.toObject());
        SyncthingDev *const devItem = addDevInfo(newDevs, devObj.value(QLatin1String("deviceID")).toString());
        if (!devItem) {
            continue;
        }

        devItem->name = devObj.value(QLatin1String("name")).toString();
        devItem->addresses = things(devObj.value(QLatin1String("addresses")).toArray(), [](const QJsonValue &value) { return value.toString(); });
        devItem->compression = devObj.value(QLatin1String("compression")).toString();
        devItem->certName = devObj.value(QLatin1String("certName")).toString();
        devItem->introducer = devObj.value(QLatin1String("introducer")).toBool(false);
        if (devItem->id == m_myId) {
            devItem->status = SyncthingDevStatus::OwnDevice;
            devItem->paused = false;
        } else {
            devItem->status = SyncthingDevStatus::Unknown;
            devItem->paused = devObj.value(QLatin1String("paused")).toBool(devItem->paused);
        }
    }

    m_devs.swap(newDevs);
    emit this->newDevices(m_devs);
}

// status of Syncthing (own ID, startup time)

/*!
 * \brief Requests the Syncthing status asynchronously.
 *
 * The signals myIdChanged() and tildeChanged() are emitted when those values have changed; error() is emitted in the error case.
 */
void SyncthingConnection::requestStatus()
{
    if (m_statusReply) {
        return;
    }
    QObject::connect(
        m_statusReply = requestData(QStringLiteral("system/status"), QUrlQuery()), &QNetworkReply::finished, this, &SyncthingConnection::readStatus);
}

/*!
 * \brief Reads results of requestStatus().
 */
void SyncthingConnection::readStatus()
{
    auto const [reply, response] = prepareReply(m_statusReply);
    if (!reply) {
        return;
    }

    switch (reply->error()) {
    case QNetworkReply::NoError: {
        QJsonParseError jsonError;
        const auto replyDoc(QJsonDocument::fromJson(response, &jsonError));
        if (jsonError.error != QJsonParseError::NoError) {
            emitError(tr("Unable to parse Syncthing status: "), jsonError, reply, response);
            handleFatalConnectionError();
            return;
        }

        const auto replyObj = replyDoc.object();
        emitMyIdChanged(replyObj.value(QLatin1String("myID")).toString());
        emitTildeChanged(replyObj.value(QLatin1String("tilde")).toString(), replyObj.value(QLatin1String("pathSeparator")).toString());
        m_startTime = parseTimeStamp(replyObj.value(QLatin1String("startTime")), QStringLiteral("start time"));
        m_hasStatus = true;

        if (m_keepPolling) {
            concludeReadingConfigAndStatus();
        }
        break;
    }
    case QNetworkReply::OperationCanceledError:
        return;
    default:
        emitError(tr("Unable to request Syncthing status: "), SyncthingErrorCategory::OverallConnection, reply);
        handleFatalConnectionError();
    }
}

/*!
 * \brief Requests the Syncthing configuration and status asynchronously.
 *
 * \sa requestConfig() and requestStatus() for emitted signals.
 */
void SyncthingConnection::requestConfigAndStatus()
{
    requestConfig();
    requestStatus();
}

// further info (connections, errors, ...)

/*!
 * \brief Requests current connections asynchronously.
 *
 * The signal devStatusChanged() is emitted for each device where the connection status has changed; error() is emitted in the error case.
 */
void SyncthingConnection::requestConnections()
{
    if (m_connectionsReply) {
        return;
    }
    QObject::connect(m_connectionsReply = requestData(QStringLiteral("system/connections"), QUrlQuery()), &QNetworkReply::finished, this,
        &SyncthingConnection::readConnections);
}

/*!
 * \brief Reads results of requestConnections().
 */
void SyncthingConnection::readConnections()
{
    auto const [reply, response] = prepareReply(m_connectionsReply);
    if (!reply) {
        return;
    }

    switch (reply->error()) {
    case QNetworkReply::NoError: {
        QJsonParseError jsonError;
        const QJsonDocument replyDoc = QJsonDocument::fromJson(response, &jsonError);
        if (jsonError.error != QJsonParseError::NoError) {
            emitError(tr("Unable to parse connections: "), jsonError, reply, response);
            return;
        }

        const QJsonObject replyObj(replyDoc.object());
        const QJsonObject totalObj(replyObj.value(QLatin1String("total")).toObject());

        // read traffic, the conversion to double is neccassary because toInt() doesn't work for high values
        const QJsonValue totalIncomingTrafficValue(totalObj.value(QLatin1String("inBytesTotal")));
        const QJsonValue totalOutgoingTrafficValue(totalObj.value(QLatin1String("outBytesTotal")));
        const std::uint64_t totalIncomingTraffic = totalIncomingTrafficValue.isDouble() ? jsonValueToInt(totalIncomingTrafficValue) : unknownTraffic;
        const std::uint64_t totalOutgoingTraffic = totalOutgoingTrafficValue.isDouble() ? jsonValueToInt(totalOutgoingTrafficValue) : unknownTraffic;
        double transferTime = 0.0;
        const bool hasDelta
            = !m_lastConnectionsUpdate.isNull() && ((transferTime = (DateTime::gmtNow() - m_lastConnectionsUpdate).totalSeconds()) != 0.0);
        m_totalIncomingRate = (hasDelta && totalIncomingTraffic != unknownTraffic && m_totalIncomingTraffic != unknownTraffic)
            ? static_cast<double>(totalIncomingTraffic - m_totalIncomingTraffic) * 0.008 / transferTime
            : 0.0;
        m_totalOutgoingRate = (hasDelta && totalOutgoingTraffic != unknownTraffic && m_totalOutgoingTraffic != unknownTraffic)
            ? static_cast<double>(totalOutgoingTraffic - m_totalOutgoingTraffic) * 0.008 / transferTime
            : 0.0;
        emit trafficChanged(m_totalIncomingTraffic = totalIncomingTraffic, m_totalOutgoingTraffic = totalOutgoingTraffic);

        // read connection status
        const QJsonObject connectionsObj(replyObj.value(QLatin1String("connections")).toObject());
        int index = 0;
        for (SyncthingDev &dev : m_devs) {
            const QJsonObject connectionObj(connectionsObj.value(dev.id).toObject());
            if (connectionObj.isEmpty()) {
                ++index;
                continue;
            }

            switch (dev.status) {
            case SyncthingDevStatus::OwnDevice:
                break;
            case SyncthingDevStatus::Disconnected:
            case SyncthingDevStatus::Unknown:
                if (connectionObj.value(QLatin1String("connected")).toBool(false)) {
                    dev.status = SyncthingDevStatus::Idle;
                } else {
                    dev.status = SyncthingDevStatus::Disconnected;
                }
                break;
            default:
                if (!connectionObj.value(QLatin1String("connected")).toBool(false)) {
                    dev.status = SyncthingDevStatus::Disconnected;
                }
            }
            dev.paused = dev.status == SyncthingDevStatus::OwnDevice ? false : connectionObj.value(QLatin1String("paused")).toBool(false);
            dev.totalIncomingTraffic = jsonValueToInt(connectionObj.value(QLatin1String("inBytesTotal")));
            dev.totalOutgoingTraffic = jsonValueToInt(connectionObj.value(QLatin1String("outBytesTotal")));
            dev.connectionAddress = connectionObj.value(QLatin1String("address")).toString();
            dev.connectionType = connectionObj.value(QLatin1String("type")).toString();
            dev.clientVersion = connectionObj.value(QLatin1String("clientVersion")).toString();
            emit devStatusChanged(dev, index);
            ++index;
        }

        m_lastConnectionsUpdate = DateTime::gmtNow();

        // since there seems no event for this data, keep polling
        if (m_keepPolling) {
            concludeConnection();
            if (m_trafficPollTimer.interval()) {
                m_trafficPollTimer.start();
            }
        }

        break;
    }
    case QNetworkReply::OperationCanceledError:
        handleAdditionalRequestCanceled();
        return;
    default:
        emitError(tr("Unable to request connections: "), SyncthingErrorCategory::OverallConnection, reply);
    }
}

/*!
 * \brief Requests errors asynchronously.
 *
 * The signal newNotification() is emitted on success; error() is emitted in the error case.
 */
void SyncthingConnection::requestErrors()
{
    if (m_errorsReply) {
        return;
    }
    QObject::connect(
        m_errorsReply = requestData(QStringLiteral("system/error"), QUrlQuery()), &QNetworkReply::finished, this, &SyncthingConnection::readErrors);
}

/*!
 * \brief Reads results of requestErrors().
 */
void SyncthingConnection::readErrors()
{
    auto const [reply, response] = prepareReply(m_errorsReply);
    if (!reply) {
        return;
    }

    // ignore any errors occurred before connecting
    if (m_lastErrorTime.isNull()) {
        m_lastErrorTime = DateTime::now();
    }

    switch (reply->error()) {
    case QNetworkReply::NoError: {
        QJsonParseError jsonError;
        const QJsonDocument replyDoc = QJsonDocument::fromJson(response, &jsonError);
        if (jsonError.error != QJsonParseError::NoError) {
            emitError(tr("Unable to parse errors: "), jsonError, reply, response);
            return;
        }

        const auto errors = replyDoc.object().value(QLatin1String("errors")).toArray();
        for (const QJsonValue &errorVal : errors) {
            const QJsonObject errorObj = errorVal.toObject();
            if (errorObj.isEmpty()) {
                continue;
            }
            if (const auto when = parseTimeStamp(errorObj.value(QLatin1String("when")), QStringLiteral("error message")); m_lastErrorTime < when) {
                emitNotification(m_lastErrorTime = when, errorObj.value(QLatin1String("message")).toString());
            }
        }

        // since there seems no event for this data, keep polling
        if (m_keepPolling) {
            concludeConnection();
            if (m_errorsPollTimer.interval()) {
                m_errorsPollTimer.start();
            }
        }
        break;
    }
    case QNetworkReply::OperationCanceledError:
        handleAdditionalRequestCanceled();
        return;
    default:
        emitError(tr("Unable to request errors: "), SyncthingErrorCategory::SpecificRequest, reply);
    }
}

/*!
 * \brief Requests statistics (last file, last scan) for all directories asynchronously.
 */
void SyncthingConnection::requestDirStatistics()
{
    if (m_dirStatsReply) {
        return;
    }
    QObject::connect(m_dirStatsReply = requestData(QStringLiteral("stats/folder"), QUrlQuery()), &QNetworkReply::finished, this,
        &SyncthingConnection::readDirStatistics);
}

/*!
 * \brief Reads results of requestDirStatistics().
 */
void SyncthingConnection::readDirStatistics()
{
    auto const [reply, response] = prepareReply(m_dirStatsReply);
    if (!reply) {
        return;
    }

    switch (reply->error()) {
    case QNetworkReply::NoError: {
        QJsonParseError jsonError;
        const QJsonDocument replyDoc = QJsonDocument::fromJson(response, &jsonError);
        if (jsonError.error != QJsonParseError::NoError) {
            emitError(tr("Unable to parse directory statistics: "), jsonError, reply, response);
            return;
        }

        const QJsonObject replyObj(replyDoc.object());
        int index = 0;
        for (SyncthingDir &dirInfo : m_dirs) {
            const QJsonObject dirObj(replyObj.value(dirInfo.id).toObject());
            if (dirObj.isEmpty()) {
                ++index;
                continue;
            }

            bool dirModified = false;
            const auto lastScan = dirObj.value(QLatin1String("lastScan")).toString().toUtf8();
            if (!lastScan.isEmpty()) {
                dirModified = true;
                dirInfo.lastScanTime = parseTimeStamp(dirObj.value(QLatin1String("lastScan")), QStringLiteral("last scan"));
            }
            const QJsonObject lastFileObj(dirObj.value(QLatin1String("lastFile")).toObject());
            if (!lastFileObj.isEmpty()) {
                dirInfo.lastFileName = lastFileObj.value(QLatin1String("filename")).toString();
                dirModified = true;
                if (!dirInfo.lastFileName.isEmpty()) {
                    dirInfo.lastFileDeleted = lastFileObj.value(QLatin1String("deleted")).toBool(false);
                    dirInfo.lastFileTime = parseTimeStamp(lastFileObj.value(QLatin1String("at")), QStringLiteral("dir statistics"));
                    if (!dirInfo.lastFileTime.isNull() && dirInfo.lastFileTime > m_lastFileTime) {
                        m_lastFileTime = dirInfo.lastFileTime;
                        m_lastFileName = dirInfo.lastFileName;
                        m_lastFileDeleted = dirInfo.lastFileDeleted;
                    }
                }
            }
            if (dirModified) {
                emit dirStatusChanged(dirInfo, index);
            }
            ++index;
        }

        if (m_keepPolling) {
            concludeConnection();
        }
        break;
    }
    case QNetworkReply::OperationCanceledError:
        handleAdditionalRequestCanceled();
        return;
    default:
        emitError(tr("Unable to request directory statistics: "), SyncthingErrorCategory::OverallConnection, reply);
    }
}

/*!
 * \brief Requests statistics (global and local status) for \a dirId asynchronously.
 */
void SyncthingConnection::requestDirStatus(const QString &dirId)
{
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("folder"), dirId);
    auto *const reply = requestData(QStringLiteral("db/status"), query);
    reply->setProperty("dirId", dirId);
    m_otherReplies << reply;
    QObject::connect(reply, &QNetworkReply::finished, this, &SyncthingConnection::readDirStatus, Qt::QueuedConnection);
}

/*!
 * \brief Reads data from requestDirStatus().
 */
void SyncthingConnection::readDirStatus()
{
    auto const [reply, response] = prepareReply(m_otherReplies);
    if (!reply) {
        return;
    }

    switch (reply->error()) {
    case QNetworkReply::NoError: {
        // determine relevant dir
        int index;
        const QString dirId(reply->property("dirId").toString());
        SyncthingDir *const dir = findDirInfo(dirId, index);
        if (!dir) {
            // discard status for unknown dirs
            return;
        }

        // parse JSON
        QJsonParseError jsonError;
        const QJsonDocument replyDoc = QJsonDocument::fromJson(response, &jsonError);
        if (jsonError.error != QJsonParseError::NoError) {
            emitError(tr("Unable to parse status for directory %1: ").arg(dirId), jsonError, reply, response);
            return;
        }

        readDirSummary(DateTime::now(), replyDoc.object(), *dir, index);

        if (m_keepPolling) {
            concludeConnection();
        }
        break;
    }
    case QNetworkReply::OperationCanceledError:
        handleAdditionalRequestCanceled();
        return;
    default:
        emitError(tr("Unable to request directory statistics: "), SyncthingErrorCategory::SpecificRequest, reply);
    }
}

/*!
 * \brief Requests pull errors for \a dirId asynchronously.
 *
 * The dirStatusChanged() signal is emitted on success and error() in the error case.
 */
void SyncthingConnection::requestDirPullErrors(const QString &dirId, int page, int perPage)
{
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("folder"), dirId);
    if (page > 0 && perPage > 0) {
        query.addQueryItem(QStringLiteral("page"), QString::number(page));
        query.addQueryItem(QStringLiteral("perpage"), QString::number(perPage));
    }
    auto *const reply = requestData(QStringLiteral("folder/pullerrors"), query);
    reply->setProperty("dirId", dirId);
    QObject::connect(reply, &QNetworkReply::finished, this, &SyncthingConnection::readDirPullErrors);
}

/*!
 * \brief Reads data from requestDirPullErrors().
 */
void SyncthingConnection::readDirPullErrors()
{
    auto const [reply, response] = prepareReply();
    if (!reply) {
        return;
    }

    // determine relevant dir
    int index;
    const QString dirId(reply->property("dirId").toString());
    SyncthingDir *const dir = findDirInfo(dirId, index);
    if (!dir) {
        // discard errors for unknown dirs
        return;
    }

    switch (reply->error()) {
    case QNetworkReply::NoError: {
        // parse JSON
        QJsonParseError jsonError;
        const QJsonDocument replyDoc = QJsonDocument::fromJson(response, &jsonError);
        if (jsonError.error != QJsonParseError::NoError) {
            emitError(tr("Unable to parse pull errors for directory %1: ").arg(dirId), jsonError, reply, response);
            return;
        }

        readFolderErrors(DateTime::now(), replyDoc.object(), *dir, index);
        break;
    }
    case QNetworkReply::OperationCanceledError:
        return;
    default:
        emitError(tr("Unable to request pull errors for directory %1: ").arg(dirId), SyncthingErrorCategory::SpecificRequest, reply);
    }
}

/*!
 * \brief Requests completion for \a devId and \a dirId asynchronously.
 */
void SyncthingConnection::requestCompletion(const QString &devId, const QString &dirId)
{
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("device"), devId);
    query.addQueryItem(QStringLiteral("folder"), dirId);
    auto *const reply = requestData(QStringLiteral("db/completion"), query);
    reply->setProperty("devId", devId);
    reply->setProperty("dirId", dirId);
    m_otherReplies << reply;
    QObject::connect(reply, &QNetworkReply::finished, this, &SyncthingConnection::readCompletion, Qt::QueuedConnection);
}

/// \cond
static void ensureCompletionNotConsideredRequested(const QString &devId, SyncthingDev *devInfo, const QString &dirId, SyncthingDir *dirInfo)
{
    if (devInfo) {
        devInfo->completionByDir[dirId].requested = false;
    }
    if (dirInfo) {
        dirInfo->completionByDevice[devId].requested = false;
    }
}
/// \endcond

/*!
 * \brief Reads data from requestCompletion().
 */
void SyncthingConnection::readCompletion()
{
    auto const [reply, response] = prepareReply(m_otherReplies);
    const auto cancelled = reply == nullptr;
    const auto *const sender = cancelled ? static_cast<QNetworkReply *>(this->sender()) : reply;

    // determine relevant dev/dir
    const auto devId = sender->property("devId").toString();
    const auto dirId = sender->property("dirId").toString();
    int devIndex, dirIndex;
    auto *const devInfo = findDevInfo(devId, devIndex);
    auto *const dirInfo = findDirInfo(dirId, dirIndex);
    if (!devInfo && !dirInfo) {
        return;
    }

    if (cancelled) {
        ensureCompletionNotConsideredRequested(devId, devInfo, dirId, dirInfo);
        return;
    }
    switch (reply->error()) {
    case QNetworkReply::NoError: {
        // parse JSON
        QJsonParseError jsonError;
        const auto replyDoc = QJsonDocument::fromJson(response, &jsonError);
        if (jsonError.error == QJsonParseError::NoError) {
            // update the relevant completion info
            readRemoteFolderCompletion(DateTime::now(), replyDoc.object(), devId, devInfo, devIndex, dirId, dirInfo, dirIndex);
            concludeConnection();
            return;
        }

        emitError(tr("Unable to parse completion for device/directory %1/%2: ").arg(devId, dirId), jsonError, reply, response);
        break;
    }
    case QNetworkReply::ContentNotFoundError:
        // assign empty completion when receiving 404 response
        // note: The connector generally tries to avoid requesting the completion for paused dirs/devs but if the completion is requested
        //       before it is aware that the dir/dev is paused it might still run into this error, e.g. when pausing a directory and completion
        //       is requested concurrently. Before Syncthing v1.15.0 we've got an empty completion instead of 404 anyways.
        readRemoteFolderCompletion(SyncthingCompletion(), devId, devInfo, devIndex, dirId, dirInfo, dirIndex);
        concludeConnection();
        return;
    case QNetworkReply::OperationCanceledError:
        handleAdditionalRequestCanceled();
        break;
    default:
        emitError(tr("Unable to request completion for device/directory %1/%2: ").arg(devId, dirId), SyncthingErrorCategory::SpecificRequest, reply);
    }
    ensureCompletionNotConsideredRequested(devId, devInfo, dirId, dirInfo);
}

/*!
 * \brief Requests device statistics asynchronously.
 */
void SyncthingConnection::requestDeviceStatistics()
{
    if (m_devStatsReply) {
        return;
    }
    QObject::connect(
        requestData(QStringLiteral("stats/device"), QUrlQuery()), &QNetworkReply::finished, this, &SyncthingConnection::readDeviceStatistics);
}

/*!
 * \brief Reads results of requestDeviceStatistics().
 */
void SyncthingConnection::readDeviceStatistics()
{
    auto const [reply, response] = prepareReply(m_devStatsReply);
    if (!reply) {
        return;
    }

    switch (reply->error()) {
    case QNetworkReply::NoError: {
        QJsonParseError jsonError;
        const QJsonDocument replyDoc = QJsonDocument::fromJson(response, &jsonError);
        if (jsonError.error != QJsonParseError::NoError) {
            emitError(tr("Unable to parse device statistics: "), jsonError, reply, response);
            return;
        }

        const QJsonObject replyObj(replyDoc.object());
        int index = 0;
        for (SyncthingDev &devInfo : m_devs) {
            const QJsonObject devObj(replyObj.value(devInfo.id).toObject());
            if (!devObj.isEmpty()) {
                devInfo.lastSeen = parseTimeStamp(devObj.value(QLatin1String("lastSeen")), QStringLiteral("last seen"), DateTime(), true);
                emit devStatusChanged(devInfo, index);
            }
            ++index;
        }
        // since there seems no event for this data, keep polling
        if (m_keepPolling) {
            concludeConnection();
            if (m_devStatsPollTimer.interval()) {
                m_devStatsPollTimer.start();
            }
        }
        break;
    }
    case QNetworkReply::OperationCanceledError:
        handleAdditionalRequestCanceled();
        return;
    default:
        emitError(tr("Unable to request device statistics: "), SyncthingErrorCategory::OverallConnection, reply);
    }
}

void SyncthingConnection::requestVersion()
{
    if (m_versionReply) {
        return;
    }
    QObject::connect(m_versionReply = requestData(QStringLiteral("system/version"), QUrlQuery()), &QNetworkReply::finished, this,
        &SyncthingConnection::readVersion);
}

/*!
 * \brief Reads data from requestVersion().
 */
void SyncthingConnection::readVersion()
{
    auto const [reply, response] = prepareReply(m_versionReply);
    if (!reply) {
        return;
    }

    switch (reply->error()) {
    case QNetworkReply::NoError: {
        QJsonParseError jsonError;
        const auto replyDoc(QJsonDocument::fromJson(response, &jsonError));
        if (jsonError.error != QJsonParseError::NoError) {
            emitError(tr("Unable to parse version: "), jsonError, reply, response);
            return;
        }

        const auto replyObj(replyDoc.object());
        m_syncthingVersion = replyObj.value(QLatin1String("longVersion")).toString();

        if (m_keepPolling) {
            concludeConnection();
        }
        break;
    }
    case QNetworkReply::OperationCanceledError:
        handleAdditionalRequestCanceled();
        return;
    default:
        emitError(tr("Unable to request version: "), SyncthingErrorCategory::OverallConnection, reply);
    }
}

/*!
 * \brief Requests a QR code for the specified \a text.
 *
 * qrCodeAvailable() is emitted on success; otherwise error() is emitted.
 */
void SyncthingConnection::requestQrCode(const QString &text)
{
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("text"), text);
    QNetworkReply *reply = requestData(QStringLiteral("/qr/"), query, false);
    reply->setProperty("qrText", text);
    QObject::connect(reply, &QNetworkReply::finished, this, &SyncthingConnection::readQrCode);
}

/*!
 * \brief Reads the QR code queried via requestQrCode().
 */
void SyncthingConnection::readQrCode()
{
    auto const [reply, response] = prepareReply();
    if (!reply) {
        return;
    }

    switch (reply->error()) {
    case QNetworkReply::NoError:
        emit qrCodeAvailable(reply->property("qrText").toString(), response);
        break;
    case QNetworkReply::OperationCanceledError:
        break;
    default:
        emit error(tr("Unable to request QR-Code: ") + reply->errorString(), SyncthingErrorCategory::SpecificRequest, reply->error());
    }
}

/*!
 * \brief Requests the Syncthing log.
 *
 * logAvailable() is emitted on success; otherwise error() is emitted.
 */
void SyncthingConnection::requestLog()
{
    if (m_logReply) {
        return;
    }
    QObject::connect(
        m_logReply = requestData(QStringLiteral("system/log"), QUrlQuery()), &QNetworkReply::finished, this, &SyncthingConnection::readLog);
}

/*!
 * \brief Reads log entries queried via requestLog().
 */
void SyncthingConnection::readLog()
{
    auto const [reply, response] = prepareReply(m_logReply);
    if (!reply) {
        return;
    }

    switch (reply->error()) {
    case QNetworkReply::NoError: {
        QJsonParseError jsonError;
        const QJsonDocument replyDoc = QJsonDocument::fromJson(response, &jsonError);
        if (jsonError.error != QJsonParseError::NoError) {
            emit error(tr("Unable to parse Syncthing log: ") + jsonError.errorString(), SyncthingErrorCategory::Parsing, QNetworkReply::NoError);
            return;
        }

        const QJsonArray log(replyDoc.object().value(QLatin1String("messages")).toArray());
        vector<SyncthingLogEntry> logEntries;
        logEntries.reserve(static_cast<size_t>(log.size()));
        for (const QJsonValue &logVal : log) {
            const QJsonObject logObj(logVal.toObject());
            logEntries.emplace_back(logObj.value(QLatin1String("when")).toString(), logObj.value(QLatin1String("message")).toString());
        }
        emit logAvailable(logEntries);
        break;
    }
    case QNetworkReply::OperationCanceledError:
        break;
    default:
        emit error(tr("Unable to request Syncthing log: ") + reply->errorString(), SyncthingErrorCategory::SpecificRequest, reply->error());
    }
}

// post config

/*!
 * \brief Posts the specified \a rawConfig.
 * \remarks The signal newConfigTriggered() is emitted when the config has been posted successfully. In the error case, error() is emitted.
 *          Besides, the newConfig() signal should be emitted as well, indicating Syncthing has actually applied the new configuration.
 */
void SyncthingConnection::postConfigFromJsonObject(const QJsonObject &rawConfig)
{
    QObject::connect(postData(QStringLiteral("system/config"), QUrlQuery(), QJsonDocument(rawConfig).toJson(QJsonDocument::Compact)),
        &QNetworkReply::finished, this, &SyncthingConnection::readPostConfig);
}

/*!
 * \brief Posts the specified \a rawConfig.
 * \param rawConfig A valid JSON document containing the configuration. It is directly passed to Syncthing.
 * \remarks The signal newConfigTriggered() is emitted when the config has been posted successfully. In the error case, error() is emitted.
 *          Besides, the newConfig() signal should be emitted as well, indicating Syncthing has actually applied the new configuration.
 */
void SyncthingConnection::postConfigFromByteArray(const QByteArray &rawConfig)
{
    QObject::connect(
        postData(QStringLiteral("system/config"), QUrlQuery(), rawConfig), &QNetworkReply::finished, this, &SyncthingConnection::readPostConfig);
}

/*!
 * \brief Reads data from postConfigFromJsonObject() and postConfigFromByteArray().
 */
void SyncthingConnection::readPostConfig()
{
    auto const [reply, response] = prepareReply(false, false);
    switch (reply->error()) {
    case QNetworkReply::NoError:
        emit newConfigTriggered();
        break;
    default:
        emitError(tr("Unable to post config: "), SyncthingErrorCategory::SpecificRequest, reply);
    }
}

/*!
 * \brief Reads data from requestDirStatus() and FolderSummary-event and stores them to \a dir.
 */
void SyncthingConnection::readDirSummary(DateTime eventTime, const QJsonObject &summary, SyncthingDir &dir, int index)
{
    if (summary.isEmpty() || dir.lastStatisticsUpdate > eventTime) {
        return;
    }

    // backup previous statistics -> if there's no difference after all, don't emit completed event
    auto &globalStats(dir.globalStats);
    auto &localStats(dir.localStats);
    auto &neededStats(dir.neededStats);
    const auto previouslyUpdated(!dir.lastStatisticsUpdate.isNull());
    const auto previouslyGlobal(globalStats);
    const auto previouslyNeeded(neededStats);

    // update statistics
    globalStats.bytes = jsonValueToInt(summary.value(QLatin1String("globalBytes")));
    globalStats.deletes = jsonValueToInt(summary.value(QLatin1String("globalDeleted")));
    globalStats.files = jsonValueToInt(summary.value(QLatin1String("globalFiles")));
    globalStats.dirs = jsonValueToInt(summary.value(QLatin1String("globalDirectories")));
    globalStats.symlinks = jsonValueToInt(summary.value(QLatin1String("globalSymlinks")));
    localStats.bytes = jsonValueToInt(summary.value(QLatin1String("localBytes")));
    localStats.deletes = jsonValueToInt(summary.value(QLatin1String("localDeleted")));
    localStats.files = jsonValueToInt(summary.value(QLatin1String("localFiles")));
    localStats.dirs = jsonValueToInt(summary.value(QLatin1String("localDirectories")));
    localStats.symlinks = jsonValueToInt(summary.value(QLatin1String("localSymlinks")));
    neededStats.bytes = jsonValueToInt(summary.value(QLatin1String("needBytes")));
    neededStats.deletes = jsonValueToInt(summary.value(QLatin1String("needDeletes")));
    neededStats.files = jsonValueToInt(summary.value(QLatin1String("needFiles")));
    neededStats.dirs = jsonValueToInt(summary.value(QLatin1String("needDirectories")));
    neededStats.symlinks = jsonValueToInt(summary.value(QLatin1String("needSymlinks")));
    dir.pullErrorCount = jsonValueToInt(summary.value(QLatin1String("pullErrors")));
    m_dirStatsAltered = true;

    dir.ignorePatterns = summary.value(QLatin1String("ignorePatterns")).toBool();
    dir.lastStatisticsUpdate = eventTime;

    // update status
    const auto lastStatusUpdate = parseTimeStamp(summary.value(QLatin1String("stateChanged")), QStringLiteral("state changed"), dir.lastStatusUpdate);
    if (dir.pullErrorCount) {
        // consider the directory still as out-of-sync if there are still pull errors
        // note: Syncthing can report an "idle" status despite pull errors.
        dir.status = SyncthingDirStatus::OutOfSync;
        dir.lastStatusUpdate = std::max(dir.lastStatusUpdate, lastStatusUpdate);
    } else if (const auto state = summary.value(QLatin1String("state")).toString(); !state.isEmpty()) {
        dir.assignStatus(state, lastStatusUpdate);
    }

    dir.completionPercentage = globalStats.bytes ? static_cast<int>((globalStats.bytes - neededStats.bytes) * 100 / globalStats.bytes) : 100;

    emit dirStatusChanged(dir, index);
    if (neededStats.isNull() && previouslyUpdated && (neededStats != previouslyNeeded || globalStats != previouslyGlobal)) {
        emit dirCompleted(eventTime, dir, index);
    }
}

/*!
 * \brief Reads data from "FolderRejected"-event.
 * \remarks "FolderRejected" is deprecated in favor of "PendingFoldersChanged" since Syncthing version v1.13.0
 *          but still handled for compatibility with older versions. Currently "PendingFoldersChanged" is ignored
 *          to avoid emitting events twice.
 */
void SyncthingConnection::readDirRejected(DateTime eventTime, const QString &dirId, const QJsonObject &eventData)
{
    // ignore if dir has already been added
    int row;
    const auto *const dir(findDirInfo(dirId, row));
    if (dir) {
        return;
    }

    // emit newDirAvailable() signal
    const auto dirLabel(eventData.value(QLatin1String("folderLabel")).toString());
    const auto devId(eventData.value(QLatin1String("device")).toString());
    const auto *const device(findDevInfo(devId, row));
    emit newDirAvailable(eventTime, devId, device, dirId, dirLabel);
}

/*!
 * \brief Reads data from "DeviceRejected"-event.
 * \remarks "DeviceRejected" is deprecated in favor of "PendingDevicesChanged" since Syncthing version v1.13.0
 *          but still handled for compatibility with older versions. Currently "PendingDevicesChanged" is ignored
 *          to avoid emitting events twice.
 */
void SyncthingConnection::readDevRejected(DateTime eventTime, const QString &devId, const QJsonObject &eventData)
{
    // ignore if dev has already been added
    int row;
    const auto *const dev(findDevInfo(devId, row));
    if (dev) {
        return;
    }

    // emit newDevAvailable() signal
    emit newDevAvailable(eventTime, devId, eventData.value(QLatin1String("address")).toString());
}

/*!
 * \brief Reads "LocalChangeDetected" and "RemoveChangeDetected" events from requestEvents() and requestDiskEvents().
 */
void SyncthingConnection::readChangeEvent(DateTime eventTime, const QString &eventType, const QJsonObject &eventData)
{
    // read ID via "folder" with fallback to "folderID" (which is deprecated since version v1.1.2)
    int index;
    auto *dirInfo = findDirInfo(QLatin1String("folder"), eventData, &index);
    if (!dirInfo && !(dirInfo = findDirInfo(QLatin1String("folderID"), eventData, &index))) {
        return;
    }

    SyncthingFileChange change;
    change.local = eventType.startsWith("Local");
    change.eventTime = eventTime;
    change.action = eventData.value(QLatin1String("action")).toString();
    change.type = eventData.value(QLatin1String("type")).toString();
    change.modifiedBy = eventData.value(QLatin1String("modifiedBy")).toString();
    change.path = eventData.value(QLatin1String("path")).toString();
    if (m_recordFileChanges) {
        dirInfo->recentChanges.emplace_back(move(change));
        emit dirStatusChanged(*dirInfo, index);
        emit fileChanged(*dirInfo, index, dirInfo->recentChanges.back());
    } else {
        emit fileChanged(*dirInfo, index, change);
    }
}

// events / long polling API

/*!
 * \brief Requests the Syncthing events (since the last successful call) asynchronously.
 *
 * The signal newEvents() is emitted on success; otherwise error() is emitted.
 */
void SyncthingConnection::requestEvents()
{
    if (m_eventsReply) {
        return;
    }
    QUrlQuery query;
    if (m_lastEventId) {
        query.addQueryItem(QStringLiteral("since"), QString::number(m_lastEventId));
    } else {
        query.addQueryItem(QStringLiteral("limit"), QStringLiteral("1"));
    }
    // force to return immediately after the first call
    if (!m_hasEvents) {
        query.addQueryItem(QStringLiteral("timeout"), QStringLiteral("0"));
    }
    QObject::connect(m_eventsReply = requestData(QStringLiteral("events"), query), &QNetworkReply::finished, this, &SyncthingConnection::readEvents);
}

/*!
 * \brief Reads results of requestEvents().
 */
void SyncthingConnection::readEvents()
{
    auto const [reply, response] = prepareReply(m_eventsReply);
    if (!reply) {
        return;
    }

    switch (reply->error()) {
    case QNetworkReply::NoError: {
        QJsonParseError jsonError;
        const QJsonDocument replyDoc = QJsonDocument::fromJson(response, &jsonError);
        if (jsonError.error != QJsonParseError::NoError) {
            emitError(tr("Unable to parse Syncthing events: "), jsonError, reply, response);
            handleFatalConnectionError();
            return;
        }

        m_hasEvents = true;
        const auto replyArray(replyDoc.array());
        emit newEvents(replyArray);
        readEventsFromJsonArray(replyArray, m_lastEventId);

        if (!replyArray.isEmpty() && (loggingFlags() & SyncthingConnectionLoggingFlags::Events)) {
            const auto log = replyDoc.toJson(QJsonDocument::Indented);
            cerr << Phrases::Info << "Received " << replyArray.size() << " Syncthing events:" << Phrases::End << log.data() << endl;
        }
        break;
    }
    case QNetworkReply::TimeoutError:
        // no new events available, keep polling
        break;
    case QNetworkReply::OperationCanceledError:
        handleAdditionalRequestCanceled();
        return;
    default:
        emitError(tr("Unable to request Syncthing events: "), SyncthingErrorCategory::OverallConnection, reply);
        handleFatalConnectionError();
        return;
    }

    if (m_keepPolling) {
        requestEvents();
        concludeConnection();
    } else {
        setStatus(SyncthingStatus::Disconnected);
    }
}

/*!
 * \brief Reads results of requestEvents().
 */
void SyncthingConnection::readEventsFromJsonArray(const QJsonArray &events, int &idVariable)
{
    for (const auto &eventVal : events) {
        const auto event = eventVal.toObject();
        const auto eventTime = parseTimeStamp(event.value(QLatin1String("time")), QStringLiteral("event time"));
        const auto eventType = event.value(QLatin1String("type")).toString();
        const auto eventData = event.value(QLatin1String("data")).toObject();

        idVariable = event.value(QLatin1String("id")).toInt(idVariable);

        if (eventType == QLatin1String("Starting")) {
            readStartingEvent(eventData);
        } else if (eventType == QLatin1String("StateChanged")) {
            readStatusChangedEvent(eventTime, eventData);
        } else if (eventType == QLatin1String("DownloadProgress")) {
            readDownloadProgressEvent(eventTime, eventData);
        } else if (eventType.startsWith(QLatin1String("Folder"))) {
            readDirEvent(eventTime, eventType, eventData);
        } else if (eventType.startsWith(QLatin1String("Device"))) {
            readDeviceEvent(eventTime, eventType, eventData);
        } else if (eventType == QLatin1String("ItemStarted")) {
            readItemStarted(eventTime, eventData);
        } else if (eventType == QLatin1String("ItemFinished")) {
            readItemFinished(eventTime, eventData);
        } else if (eventType == QLatin1String("RemoteIndexUpdated")) {
            readRemoteIndexUpdated(eventTime, eventData);
        } else if (eventType == QLatin1String("ConfigSaved")) {
            requestConfig(); // just consider current config as invalidated
        } else if (eventType.endsWith(QLatin1String("ChangeDetected"))) {
            readChangeEvent(eventTime, eventType, eventData);
        }
    }
    emitDirStatisticsChanged();
}

/*!
 * \brief Reads results of requestEvents().
 */
void SyncthingConnection::readStartingEvent(const QJsonObject &eventData)
{
    const QString configDir(eventData.value(QLatin1String("home")).toString());
    if (configDir != m_configDir) {
        emit configDirChanged(m_configDir = configDir);
    }
    emitMyIdChanged(eventData.value(QLatin1String("myID")).toString());
}

/*!
 * \brief Reads results of requestEvents().
 */
void SyncthingConnection::readStatusChangedEvent(DateTime eventTime, const QJsonObject &eventData)
{
    const QString dir(eventData.value(QLatin1String("folder")).toString());
    if (dir.isEmpty()) {
        return;
    }

    // find the directory
    int index;
    SyncthingDir *dirInfo = findDirInfo(dir, index);

    // add a new directory if the dir is not present yet
    const bool dirAlreadyPresent = dirInfo;
    if (!dirAlreadyPresent) {
        m_dirs.emplace_back(dir);
        dirInfo = &m_dirs.back();
    }

    // assign new status
    bool statusChanged = dirInfo->assignStatus(eventData.value(QLatin1String("to")).toString(), eventTime);
    if (dirInfo->status == SyncthingDirStatus::OutOfSync) {
        const QString errorMessage(eventData.value(QLatin1String("error")).toString());
        if (!errorMessage.isEmpty()) {
            dirInfo->globalError = errorMessage;
            statusChanged = true;
        }
    }
    if (dirAlreadyPresent) {
        // emit status changed when dir already present
        if (statusChanged) {
            emit dirStatusChanged(*dirInfo, index);
        }
    } else {
        // request config for complete meta data of new directory
        requestConfig();
    }
}

/*!
 * \brief Reads results of requestEvents().
 */
void SyncthingConnection::readDownloadProgressEvent(DateTime eventTime, const QJsonObject &eventData)
{
    CPP_UTILITIES_UNUSED(eventTime)
    for (SyncthingDir &dirInfo : m_dirs) {
        // disappearing implies that the download has been finished so just wipe old entries
        dirInfo.downloadingItems.clear();
        dirInfo.blocksAlreadyDownloaded = dirInfo.blocksToBeDownloaded = 0;

        // read progress of currently downloading items
        const QJsonObject dirObj(eventData.value(dirInfo.id).toObject());
        if (!dirObj.isEmpty()) {
            dirInfo.downloadingItems.reserve(static_cast<size_t>(dirObj.size()));
            for (auto filePair = dirObj.constBegin(), end = dirObj.constEnd(); filePair != end; ++filePair) {
                dirInfo.downloadingItems.emplace_back(dirInfo.path, filePair.key(), filePair.value().toObject());
                const SyncthingItemDownloadProgress &itemProgress = dirInfo.downloadingItems.back();
                dirInfo.blocksAlreadyDownloaded += itemProgress.blocksAlreadyDownloaded;
                dirInfo.blocksToBeDownloaded += itemProgress.totalNumberOfBlocks;
            }
        }
        dirInfo.downloadPercentage = (dirInfo.blocksAlreadyDownloaded > 0 && dirInfo.blocksToBeDownloaded > 0)
            ? (static_cast<unsigned int>(dirInfo.blocksAlreadyDownloaded) * 100 / static_cast<unsigned int>(dirInfo.blocksToBeDownloaded))
            : 0;
        dirInfo.downloadLabel
            = QStringLiteral("%1 / %2 - %3 %")
                  .arg(QString::fromLatin1(dataSizeToString(dirInfo.blocksAlreadyDownloaded > 0
                               ? static_cast<std::uint64_t>(dirInfo.blocksAlreadyDownloaded) * SyncthingItemDownloadProgress::syncthingBlockSize
                               : 0)
                                               .data()),
                      QString::fromLatin1(dataSizeToString(dirInfo.blocksToBeDownloaded > 0
                              ? static_cast<std::uint64_t>(dirInfo.blocksToBeDownloaded) * SyncthingItemDownloadProgress::syncthingBlockSize
                              : 0)
                                              .data()),
                      QString::number(dirInfo.downloadPercentage));
    }
    emit downloadProgressChanged();
}

/*!
 * \brief Reads results of requestEvents().
 */
void SyncthingConnection::readDirEvent(DateTime eventTime, const QString &eventType, const QJsonObject &eventData)
{
    // read dir ID
    const auto dirId([&eventData] {
        const auto folder(eventData.value(QLatin1String("folder")).toString());
        if (!folder.isEmpty()) {
            return folder;
        }
        return eventData.value(QLatin1String("id")).toString();
    }());
    if (dirId.isEmpty()) {
        // handle events which don't necessarily require a corresponding dir info
        if (eventType == QLatin1String("FolderCompletion")) {
            readFolderCompletion(eventTime, eventData, dirId, nullptr, -1);
        }
        return;
    }

    // handle "FolderRejected"-event which is a bit special because here the dir ID is supposed to be unknown
    if (eventType == QLatin1String("FolderRejected")) {
        readDirRejected(eventTime, dirId, eventData);
        return;
    }

    // find related dir info for other events (which are about well-known dirs)
    int index;
    auto *const dirInfo = findDirInfo(dirId, index);
    if (!dirInfo) {
        return;
    }

    // distinguish specific events
    if (eventType == QLatin1String("FolderErrors")) {
        readFolderErrors(eventTime, eventData, *dirInfo, index);
    } else if (eventType == QLatin1String("FolderSummary")) {
        readDirSummary(eventTime, eventData.value(QLatin1String("summary")).toObject(), *dirInfo, index);
    } else if (eventType == QLatin1String("FolderCompletion") && dirInfo->lastStatisticsUpdate < eventTime) {
        readFolderCompletion(eventTime, eventData, dirId, dirInfo, index);
    } else if (eventType == QLatin1String("FolderScanProgress")) {
        const double current = eventData.value(QLatin1String("current")).toDouble(0);
        const double total = eventData.value(QLatin1String("total")).toDouble(0);
        const double rate = eventData.value(QLatin1String("rate")).toDouble(0);
        if (current > 0 && total > 0) {
            dirInfo->scanningPercentage = static_cast<int>(current * 100 / total);
            dirInfo->scanningRate = rate;
            dirInfo->assignStatus(SyncthingDirStatus::Scanning, eventTime); // ensure state is scanning
            emit dirStatusChanged(*dirInfo, index);
        }
    } else if (eventType == QLatin1String("FolderPaused")) {
        if (!dirInfo->paused) {
            dirInfo->paused = true;
            emit dirStatusChanged(*dirInfo, index);
        }
    } else if (eventType == QLatin1String("FolderResumed")) {
        if (dirInfo->paused) {
            dirInfo->paused = false;
            emit dirStatusChanged(*dirInfo, index);
        }
    }
}

/*!
 * \brief Reads results of requestEvents().
 */
void SyncthingConnection::readDeviceEvent(DateTime eventTime, const QString &eventType, const QJsonObject &eventData)
{
    // ignore device events happened before the last connections update
    if (eventTime.isNull() && m_lastConnectionsUpdate.isNull() && eventTime < m_lastConnectionsUpdate) {
        return;
    }
    const QString dev(eventData.value(QLatin1String("device")).toString());
    if (dev.isEmpty()) {
        return;
    }

    // handle "DeviceRejected"-event
    if (eventType == QLatin1String("DeviceRejected")) {
        readDevRejected(eventTime, dev, eventData);
        return;
    }

    // find relevant device info
    int index;
    auto *const devInfo(findDevInfo(dev, index));
    if (!devInfo) {
        return;
    }

    // distinguish specific events
    SyncthingDevStatus status = devInfo->status;
    bool paused = devInfo->paused;
    if (eventType == QLatin1String("DeviceConnected")) {
        devInfo->setConnectedStateAccordingToCompletion();
    } else if (eventType == QLatin1String("DeviceDisconnected")) {
        status = SyncthingDevStatus::Disconnected;
    } else if (eventType == QLatin1String("DevicePaused")) {
        paused = true;
    } else if (eventType == QLatin1String("DeviceRejected")) {
        status = SyncthingDevStatus::Rejected;
    } else if (eventType == QLatin1String("DeviceResumed")) {
        paused = false;
        // FIXME: correct to assume device which has just been resumed is still disconnected?
        status = SyncthingDevStatus::Disconnected;
    } else if (eventType == QLatin1String("DeviceDiscovered")) {
        // we know about this device already, set status anyways because it might still be unknown
        if (status == SyncthingDevStatus::Unknown) {
            status = SyncthingDevStatus::Disconnected;
        }
    } else {
        return; // can't handle other event types currently
    }

    // assign new status
    if (devInfo->status != status || devInfo->paused != paused) {
        // don't mess with the status of the own device
        if (devInfo->status != SyncthingDevStatus::OwnDevice) {
            devInfo->status = status;
            devInfo->paused = paused;
        }
        emit devStatusChanged(*devInfo, index);
    }
}

/*!
 * \brief Reads results of requestEvents().
 * \todo Implement this.
 */
void SyncthingConnection::readItemStarted(DateTime eventTime, const QJsonObject &eventData)
{
    CPP_UTILITIES_UNUSED(eventTime)
    CPP_UTILITIES_UNUSED(eventData)
}

/*!
 * \brief Reads results of requestEvents().
 */
void SyncthingConnection::readItemFinished(DateTime eventTime, const QJsonObject &eventData)
{
    int index;
    auto *const dirInfo = findDirInfo(QLatin1String("folder"), eventData, &index);
    if (!dirInfo) {
        return;
    }

    // handle unsuccessful operation
    const auto error(eventData.value(QLatin1String("error")).toString()), item(eventData.value(QLatin1String("item")).toString());
    if (!error.isEmpty()) {
        // add error item if not already present
        if (dirInfo->status != SyncthingDirStatus::OutOfSync) {
            // FIXME: find better way to check whether the event is still relevant
            return;
        }
        for (const auto &itemError : dirInfo->itemErrors) {
            if (itemError.message == error && itemError.path == item) {
                return;
            }
        }
        dirInfo->itemErrors.emplace_back(error, item);
        if (dirInfo->pullErrorCount < dirInfo->itemErrors.size()) {
            dirInfo->pullErrorCount = dirInfo->itemErrors.size();
        }

        // emitNotification will trigger status update, so no need to call setStatus(status())
        emit dirStatusChanged(*dirInfo, index);
        emitNotification(eventTime, error);
        return;
    }

    // update last file
    if (dirInfo->lastFileTime.isNull() || eventTime < dirInfo->lastFileTime) {
        dirInfo->lastFileTime = eventTime;
        dirInfo->lastFileName = item;
        dirInfo->lastFileDeleted = (eventData.value(QLatin1String("action")) != QLatin1String("delete"));
        if (eventTime > m_lastFileTime) {
            m_lastFileTime = dirInfo->lastFileTime;
            m_lastFileName = dirInfo->lastFileName;
            m_lastFileDeleted = dirInfo->lastFileDeleted;
        }
        emit dirStatusChanged(*dirInfo, index);
    }
}

/*!
 * \brief Reads results of requestEvents() and requestDirPullErrors().
 */
void SyncthingConnection::readFolderErrors(DateTime eventTime, const QJsonObject &eventData, SyncthingDir &dirInfo, int index)
{
    // ignore errors occurred before the last time the directory was in "sync" state (Syncthing re-emits recurring errors)
    if (dirInfo.lastSyncStarted > eventTime) {
        return;
    }

    // clear previous errors (considering syncthing/lib/model/rwfolder.go it seems that also the event API always returns a
    // full list of events and not only new ones)
    dirInfo.itemErrors.clear();

    // add errors
    const auto errors = eventData.value(QLatin1String("errors")).toArray();
    for (const QJsonValue &errorVal : errors) {
        const QJsonObject error(errorVal.toObject());
        if (error.isEmpty()) {
            continue;
        }
        dirInfo.itemErrors.emplace_back(error.value(QLatin1String("error")).toString(), error.value(QLatin1String("path")).toString());
    }

    // set pullErrorCount in case it has not already been populated from the FolderSummary event
    if (dirInfo.pullErrorCount < dirInfo.itemErrors.size()) {
        dirInfo.pullErrorCount = dirInfo.itemErrors.size();
    }

    // ensure the directory is considered out-of-sync
    if (dirInfo.pullErrorCount) {
        dirInfo.assignStatus(SyncthingDirStatus::OutOfSync, eventTime);
    }

    emit dirStatusChanged(dirInfo, index);
}

/*!
 * \brief Reads results of requestEvents().
 */
void SyncthingConnection::readFolderCompletion(
    DateTime eventTime, const QJsonObject &eventData, const QString &dirId, SyncthingDir *dirInfo, int dirIndex)
{
    const auto devId = eventData.value(QLatin1String("device")).toString();
    int devIndex;
    auto *const devInfo = findDevInfo(devId, devIndex);
    readFolderCompletion(eventTime, eventData, devId, devInfo, devIndex, dirId, dirInfo, dirIndex);
}

/*!
 * \brief Reads results of requestEvents().
 */
void SyncthingConnection::readFolderCompletion(DateTime eventTime, const QJsonObject &eventData, const QString &devId, SyncthingDev *devInfo,
    int devIndex, const QString &dirId, SyncthingDir *dirInfo, int dirIndex)
{
    if (devInfo && !devId.isEmpty() && devId != myId()) {
        readRemoteFolderCompletion(eventTime, eventData, devId, devInfo, devIndex, dirId, dirInfo, dirIndex);
    } else if (dirInfo) {
        readLocalFolderCompletion(eventTime, eventData, *dirInfo, dirIndex);
    }
}

/*!
 * \brief Reads results of requestEvents().
 */
void SyncthingConnection::readLocalFolderCompletion(DateTime eventTime, const QJsonObject &eventData, SyncthingDir &dirInfo, int index)
{
    auto &neededStats(dirInfo.neededStats);
    auto &globalStats(dirInfo.globalStats);
    // backup previous statistics -> if there's no difference after all, don't emit completed event
    const auto previouslyUpdated(!dirInfo.lastStatisticsUpdate.isNull());
    const auto previouslyNeeded(neededStats);
    const auto previouslyGlobal(globalStats);
    // read values from event data
    globalStats.bytes = jsonValueToInt(eventData.value(QLatin1String("globalBytes")), static_cast<double>(globalStats.bytes));
    neededStats.bytes = jsonValueToInt(eventData.value(QLatin1String("needBytes")), static_cast<double>(neededStats.bytes));
    neededStats.deletes = jsonValueToInt(eventData.value(QLatin1String("needDeletes")), static_cast<double>(neededStats.deletes));
    neededStats.deletes = jsonValueToInt(eventData.value(QLatin1String("needItems")), static_cast<double>(neededStats.files));
    dirInfo.lastStatisticsUpdate = eventTime;
    dirInfo.completionPercentage = globalStats.bytes ? static_cast<int>((globalStats.bytes - neededStats.bytes) * 100 / globalStats.bytes) : 100;
    emit dirStatusChanged(dirInfo, index);
    if (neededStats.isNull() && previouslyUpdated && (neededStats != previouslyNeeded || globalStats != previouslyGlobal)
        && dirInfo.status != SyncthingDirStatus::WaitingToScan && dirInfo.status != SyncthingDirStatus::Scanning) {
        emit dirCompleted(eventTime, dirInfo, index);
    }
}

/*!
 * \brief Reads results of requestEvents().
 */
void SyncthingConnection::readRemoteFolderCompletion(DateTime eventTime, const QJsonObject &eventData, const QString &devId, SyncthingDev *devInfo,
    int devIndex, const QString &dirId, SyncthingDir *dirInfo, int dirIndex)
{
    // make new completion
    auto completion = SyncthingCompletion();
    auto &needed(completion.needed);
    completion.lastUpdate = eventTime;
    completion.percentage = eventData.value(QLatin1String("completion")).toDouble();
    completion.globalBytes = jsonValueToInt(eventData.value(QLatin1String("globalBytes")));
    needed.bytes = jsonValueToInt(eventData.value(QLatin1String("needBytes")), static_cast<double>(needed.bytes));
    needed.items = jsonValueToInt(eventData.value(QLatin1String("needItems")), static_cast<double>(needed.items));
    needed.deletes = jsonValueToInt(eventData.value(QLatin1String("needDeletes")), static_cast<double>(needed.deletes));

    // update dir and dev info
    readRemoteFolderCompletion(completion, devId, devInfo, devIndex, dirId, dirInfo, dirIndex);
}

/*!
 * \brief Reads \a completion (parsed from results of requestEvents()).
 */
void SyncthingConnection::readRemoteFolderCompletion(const SyncthingCompletion &completion, const QString &devId, SyncthingDev *devInfo, int devIndex,
    const QString &dirId, SyncthingDir *dirInfo, int dirIndex)
{
    // update dir info
    if (dirInfo) {
        auto &previousCompletion = dirInfo->completionByDevice[devId];
        const auto previouslyUpdated = !previousCompletion.lastUpdate.isNull();
        const auto previouslyNeeded = !previousCompletion.needed.isNull();
        const auto previousGlobalBytes = previousCompletion.globalBytes;
        previousCompletion = completion;
        emit dirStatusChanged(*dirInfo, dirIndex);
        if (devInfo && completion.needed.isNull() && previouslyUpdated && (previouslyNeeded || previousGlobalBytes != completion.globalBytes)) {
            emit dirCompleted(DateTime::now(), *dirInfo, dirIndex, devInfo);
        }
    }
    // update dev info
    if (devInfo) {
        auto &previousCompletion = devInfo->completionByDir[dirId];
        devInfo->overallCompletion -= previousCompletion;
        devInfo->overallCompletion += completion;
        devInfo->overallCompletion.recomputePercentage();
        previousCompletion = completion;
        if (devInfo->isConnected()) {
            devInfo->setConnectedStateAccordingToCompletion();
        }
        emit devStatusChanged(*devInfo, devIndex);
    }
}

/*!
 * \brief Reads results of requestEvents().
 */
void SyncthingConnection::readRemoteIndexUpdated(DateTime eventTime, const QJsonObject &eventData)
{
    // ignore those events if we're not updating completion automatically
    if (!m_requestCompletion && !m_keepPolling) {
        return;
    }

    // find dev/dir
    const auto devId(eventData.value(QLatin1String("device")).toString());
    const auto dirId(eventData.value(QLatin1String("folder")).toString());
    if (devId.isEmpty() || dirId.isEmpty()) {
        return;
    }
    int devIndex, dirIndex;
    auto *const devInfo = findDevInfo(devId, devIndex);
    auto *const dirInfo = findDirInfo(dirId, dirIndex);

    // discard if the related dev and dir are unknown
    if (!devInfo && !dirInfo) {
        return;
    }

    // ignore event if we don't know the device and if we don't share the directory with the device
    if (!devInfo && !dirInfo->deviceIds.contains(devId)) {
        return;
    }

    // request completion again if not already requested and out-of-date
    // note: Considering the current completion info stored within the dir info and the dev info. That
    //       should not be required because both should be in sync but theoretically a user of the library
    //       might meddle with that.
    auto *const completionFromDirInfo = dirInfo ? &dirInfo->completionByDevice[devId] : nullptr;
    auto *const completionFromDevInfo = devInfo ? &devInfo->completionByDir[dirId] : nullptr;
    if ((completionFromDirInfo && completionFromDirInfo->requested) || (completionFromDevInfo && completionFromDevInfo->requested)) {
        return;
    }
    const auto lastUpdate = completionFromDirInfo && completionFromDevInfo
        ? min(completionFromDirInfo->lastUpdate, completionFromDevInfo->lastUpdate)
        : (completionFromDirInfo ? completionFromDirInfo->lastUpdate : completionFromDevInfo->lastUpdate);
    if (lastUpdate >= eventTime) {
        return;
    }
    if (completionFromDirInfo) {
        completionFromDirInfo->requested = true;
    }
    if (completionFromDevInfo) {
        completionFromDevInfo->requested = true;
    }
    if (devInfo && dirInfo && !devInfo->paused && !dirInfo->paused) {
        requestCompletion(devId, dirId);
    }
}

/*!
 * \brief Reads results of requestEvents().
 */
void SyncthingConnection::requestDiskEvents(int limit)
{
    if (m_diskEventsReply) {
        return;
    }
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("limit"), QString::number(limit));
    if (m_lastDiskEventId) {
        query.addQueryItem(QStringLiteral("since"), QString::number(m_lastDiskEventId));
    }
    // force to return immediately after the first call
    if (!m_hasDiskEvents) {
        query.addQueryItem(QStringLiteral("timeout"), QStringLiteral("0"));
    }
    QObject::connect(
        m_diskEventsReply = requestData(QStringLiteral("events/disk"), query), &QNetworkReply::finished, this, &SyncthingConnection::readDiskEvents);
}

/*!
 * \brief Reads data from requestDiskEvents().
 */
void SyncthingConnection::readDiskEvents()
{
    auto const [reply, response] = prepareReply(m_diskEventsReply);
    if (!reply) {
        return;
    }

    switch (reply->error()) {
    case QNetworkReply::NoError: {
        QJsonParseError jsonError;
        const auto replyDoc(QJsonDocument::fromJson(response, &jsonError));
        if (jsonError.error != QJsonParseError::NoError) {
            emitError(tr("Unable to parse disk events: "), jsonError, reply, response);
            return;
        }

        const auto replyArray = replyDoc.array();
        m_hasDiskEvents = true;
        readEventsFromJsonArray(replyArray, m_lastDiskEventId);

        if (!replyArray.isEmpty() && (loggingFlags() & SyncthingConnectionLoggingFlags::Events)) {
            const auto log = replyDoc.toJson(QJsonDocument::Indented);
            cerr << Phrases::Info << "Received " << replyArray.size() << " Syncthing disk events:" << Phrases::End << log.data() << endl;
        }
        break;
    }
    case QNetworkReply::TimeoutError:
        // no new events available, keep polling
        break;
    case QNetworkReply::OperationCanceledError:
        handleAdditionalRequestCanceled();
        return;
    default:
        emitError(tr("Unable to request disk events: "), SyncthingErrorCategory::OverallConnection, reply);
        handleFatalConnectionError();
        return;
    }

    if (m_keepPolling) {
        requestDiskEvents();
        concludeConnection();
    }
}

} // namespace Data
