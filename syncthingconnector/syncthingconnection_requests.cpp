#include "./syncthingconnection.h"
#include "./utils.h"

#if defined(LIB_SYNCTHING_CONNECTOR_CONNECTION_MOCKED) || defined(LIB_SYNCTHING_CONNECTOR_MOCKED)
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

#include <algorithm>
#include <iostream>
#include <utility>

using namespace std;
using namespace CppUtilities;
using namespace CppUtilities::EscapeCodes;

namespace Data {

// helper to create QNetworkRequest

/*!
 * \brief Formats the specified \a value so it can be passed to QUrlQuery::addQueryItem().
 * \remarks
 * The function QUrlQuery::addQueryItem() does *not* treat spaces and plus signs as the same,
 * ike HTML forms and Syncthing do. So it is required to use QUrl::toPercentEncoding() as Syncthing
 * would otherwise misinterpret plus signs as spaces.
 */
inline QString formatQueryItem(const QString &value)
{
    return QString::fromUtf8(QUrl::toPercentEncoding(value));
}

/*!
 * \brief Prepares a request for the specified \a path and \a query.
 */
QNetworkRequest SyncthingConnection::prepareRequest(const QString &path, const QUrlQuery &query, bool rest, bool longPolling)
{
    auto url = makeUrlWithCredentials();
    auto basePath = url.path();
    if (!basePath.endsWith(QChar('/'))) {
        basePath.append(QChar('/'));
    }
    url.setPath(rest ? (basePath % QStringLiteral("rest/") % path) : (basePath + path));
    url.setQuery(query);
    return prepareRequest(url, longPolling);
}

/*!
 * \brief Prepares a request for the specified \a url.
 */
QNetworkRequest SyncthingConnection::prepareRequest(const QUrl &url, bool longPolling)
{
    auto request = QNetworkRequest(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, QByteArrayLiteral("application/x-www-form-urlencoded"));
    request.setRawHeader("X-API-Key", m_apiKey);
#if (QT_VERSION >= QT_VERSION_CHECK(5, 9, 0))
    // ensure redirects to HTTPS are enabled/allowed regardless of the Qt version
    // note: This setting is only the default as of Qt 6 and only supported as of Qt 5.9.
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
#endif
#if (QT_VERSION >= QT_VERSION_CHECK(5, 15, 0))
    // give it a few seconds more time than the actual long polling interval set via the timeout query parameter
    request.setTransferTimeout(longPolling ? (m_longPollingTimeout ? m_longPollingTimeout + 5000 : 0) : m_requestTimeout);
#endif
#if (QT_VERSION >= QT_VERSION_CHECK(6, 8, 0))
    // set full local server name to support connecting to Unix domain sockets
    if (!localPath().isEmpty()) {
        request.setAttribute(QNetworkRequest::FullLocalServerNameAttribute, localPath());
    }
#endif
    return request;
}

/*!
 * \brief Requests asynchronously data using the rest API.
 */
QNetworkReply *SyncthingConnection::requestData(const QString &path, const QUrlQuery &query, bool rest, bool longPolling)
{
#if !defined(LIB_SYNCTHING_CONNECTOR_CONNECTION_MOCKED) && !defined(LIB_SYNCTHING_CONNECTOR_MOCKED)
    auto *const reply = networkAccessManager().get(prepareRequest(path, query, rest, longPolling));
#ifndef QT_NO_SSL
    QObject::connect(reply, &QNetworkReply::sslErrors, this, &SyncthingConnection::handleSslErrors);
#endif
    QObject::connect(reply, &QNetworkReply::redirected, this, &SyncthingConnection::handleRedirection);
    if (loggingFlags() && SyncthingConnectionLoggingFlags::ApiCalls) {
        cerr << Phrases::Info << "Querying API: GET " << reply->url().toString().toStdString() << Phrases::EndFlush;
    }
    return reply;
#else
    Q_UNUSED(longPolling)
    return MockedReply::forRequest(QStringLiteral("GET"), path, query, rest);
#endif
}

/*!
 * \brief Posts asynchronously data using the rest API.
 */
QNetworkReply *SyncthingConnection::postData(const QString &path, const QUrlQuery &query, const QByteArray &data)
{
#if !defined(LIB_SYNCTHING_CONNECTOR_CONNECTION_MOCKED) && !defined(LIB_SYNCTHING_CONNECTOR_MOCKED)
    auto *const reply = networkAccessManager().post(prepareRequest(path, query), data);
#ifndef QT_NO_SSL
    QObject::connect(reply, &QNetworkReply::sslErrors, this, &SyncthingConnection::handleSslErrors);
#endif
    if (loggingFlags() && SyncthingConnectionLoggingFlags::ApiCalls) {
        cerr << Phrases::Info << "Querying API: POST " << reply->url().toString().toStdString() << Phrases::EndFlush;
        cerr.write(data.data(), static_cast<std::streamsize>(data.size()));
    }
    return reply;
#else
    Q_UNUSED(data)
    return MockedReply::forRequest(QStringLiteral("POST"), path, query, true);
#endif
}

/*!
 * \brief Invokes an asynchronous request using the rest API.
 */
QNetworkReply *SyncthingConnection::sendData(const QByteArray &verb, const QString &path, const QUrlQuery &query, const QByteArray &data)
{
#if !defined(LIB_SYNCTHING_CONNECTOR_CONNECTION_MOCKED) && !defined(LIB_SYNCTHING_CONNECTOR_MOCKED)
    auto *const reply = networkAccessManager().sendCustomRequest(prepareRequest(path, query), verb, data);
#ifndef QT_NO_SSL
    QObject::connect(reply, &QNetworkReply::sslErrors, this, &SyncthingConnection::handleSslErrors);
#endif
    if (loggingFlags() && SyncthingConnectionLoggingFlags::ApiCalls) {
        cerr << Phrases::Info << "Querying API: " << verb.data() << ' ' << reply->url().toString().toStdString() << Phrases::EndFlush;
        cerr.write(data.data(), static_cast<std::streamsize>(data.size()));
    }
    return reply;
#else
    Q_UNUSED(data)
    return MockedReply::forRequest(verb, path, query, true);
#endif
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

#ifndef QT_NO_SSL
/*!
 * \brief Handles SSL errors of replies.
 * \remarks
 * - Ignores expected errors which are usually assigned via applySettings() or loadSelfSignedCertificate() to handle a self-signed
 *   certificate.
 * - If expected errors have previously been assigned to handle a self-signed certificate this function attempts to reload the
 *   certificate via loadSelfSignedCertificate() if it appears to be re-generated. This is done because Syncthing might re-generate
 *   the certificate if it will expire soon (indicated by the log message "Loading HTTPS certificate: certificate will soon expire"
 *   followed by "Creating new HTTPS certificate").
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

        // check whether the certificate has changed and reload it before emitting error
        if (const auto &certPath = m_certificatePath.isEmpty() ? m_dynamicallyDeterminedCertificatePath : m_certificatePath;
            !certPath.isEmpty() && m_certificateLastModified.isValid()) {
            if (const auto lastModified = QFileInfo(certPath).lastModified(); lastModified > m_certificateLastModified) {
                if (const auto ok = loadSelfSignedCertificate(); ok && !m_certificatePath.isEmpty()) {
                    m_certificateLastModified = lastModified;
                }
                // re-check whether error is expected after reloading and skip it accordingly
                if (m_expectedSslErrors.contains(error)) {
                    continue;
                }
            }
        }

        // handle the error by emitting the error signal with all the details including the certificate
        // note: Of course the failing request would cause a QNetworkReply::SslHandshakeFailedError anyway. However,
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
    if (!hasUnexpectedErrors || m_insecure) {
        reply->ignoreSslErrors();
    }
}
#endif

/*!
 * \brief Handles redirections.
 * \remarks The redirect policy will decide whether to follow the redirection or not.
 *          This function merely handles logging and loading the certificate in case this
 *          hasn't happened before (e.g. a redirection from http to https).
 */
void SyncthingConnection::handleRedirection(const QUrl &url)
{
    if (m_loggingFlags && SyncthingConnectionLoggingFlags::ApiReplies) {
        const auto urlStr = url.toString().toUtf8();
        cerr << Phrases::Info << "Got redirected to: " << std::string_view(urlStr.data(), static_cast<std::string_view::size_type>(urlStr.size()))
             << Phrases::EndFlush;
    }
#ifndef QT_NO_SSL
    if (m_expectedSslErrors.isEmpty() && url.scheme().endsWith(QChar('s'))) {
        loadSelfSignedCertificate(url);
    }
#endif
}

/*!
 * \brief Handles the specified \a reply; invoked by the prepareReply() functions.
 */
SyncthingConnection::Reply SyncthingConnection::handleReply(QNetworkReply *reply, bool readData, bool handleAborting)
{
    const auto log = m_loggingFlags && SyncthingConnectionLoggingFlags::ApiReplies;
    const auto data = Reply{
        .reply = (handleAborting && m_abortingAllRequests) ? nullptr : reply, // skip further processing if aborting to reconnect
        .response = ((readData || log) && reply->isOpen()) ? reply->readAll() : QByteArray(),
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
    if (handleAborting && m_abortingToReconnect) {
        handleAdditionalRequestCanceled();
    }
    return data;
}

/*!
 * \brief Returns the path to Syncthing's "config" route depending on whether deprecated routes should be used.
 */
QString SyncthingConnection::configPath() const
{
    return isUsingDeprecatedRoutes() ? QStringLiteral("system/config") : QStringLiteral("config");
}

/*!
 * \brief Returns the verb for posting the Syncthing config in accordance to the path returned by configPath().
 */
QByteArray SyncthingConnection::changeConfigVerb() const
{
    return isUsingDeprecatedRoutes() ? QByteArrayLiteral("POST") : QByteArrayLiteral("PUT");
}

/*!
 * \brief Returns the path to Syncthing's route to retrieve errors depending on whether deprecated routes should be used.
 */
QString SyncthingConnection::folderErrorsPath() const
{
    return isUsingDeprecatedRoutes() ? QStringLiteral("folder/pullerrors") : QStringLiteral("folder/errors");
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
 * \remarks This might result in errors caused by Syncthing not handling E notation correctly when using Qt < 5.9,
 *          see https://github.com/syncthing/syncthing/issues/4001.
 */
bool SyncthingConnection::pauseResumeDevice(const QStringList &devIds, bool paused, bool dueToMetered)
{
    if (devIds.isEmpty()) {
        return false;
    }
    if (!m_hasConfig) {
        emit error(tr("Unable to pause/resume a devices when not connected"), SyncthingErrorCategory::SpecificRequest, QNetworkReply::NoError);
        return false;
    }

    auto config = m_rawConfig;
    if (!setDevicesPaused(config, devIds, paused)) {
        return false;
    }

    auto doc = QJsonDocument();
    doc.setObject(config);
    auto *const reply = sendData(changeConfigVerb(), configPath(), QUrlQuery(), doc.toJson(QJsonDocument::Compact));
    reply->setProperty("devIds", devIds);
    reply->setProperty("resume", !paused);
    QObject::connect(reply, &QNetworkReply::finished, this, &SyncthingConnection::readDevPauseResume);

    // avoid considering manually paused or resumed devices when the network connection is no longer metered
    if (!dueToMetered && !m_devsPausedDueToMeteredConnection.isEmpty()) {
        for (const auto &devId : devIds) {
            m_devsPausedDueToMeteredConnection.removeAll(devId);
        }
    }
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
        emit error(tr("Unable to pause/resume a folders when not connected"), SyncthingErrorCategory::SpecificRequest, QNetworkReply::NoError);
        return false;
    }

    auto config = m_rawConfig;
    if (setDirectoriesPaused(config, dirIds, paused)) {
        auto doc = QJsonDocument();
        doc.setObject(config);
        auto *const reply = sendData(changeConfigVerb(), configPath(), QUrlQuery(), doc.toJson(QJsonDocument::Compact));
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
        emitError(tr("Unable to request folder pause/resume: "), SyncthingErrorCategory::SpecificRequest, reply);
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
    for (const auto &dir : m_dirs) {
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
        emit error(tr("Unable to rescan: No folder ID specified."), SyncthingErrorCategory::SpecificRequest, QNetworkReply::NoError,
            QNetworkRequest(), QByteArray());
        return;
    }

    auto query = QUrlQuery();
    query.addQueryItem(QStringLiteral("folder"), formatQueryItem(dirId));
    if (!relpath.isEmpty()) {
        query.addQueryItem(QStringLiteral("sub"), formatQueryItem(relpath));
    }
    auto *const reply = postData(QStringLiteral("db/scan"), query);
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
        emit errorsCleared();
        requestErrors();
        if (m_errorsPollTimer.isActive()) {
            m_errorsPollTimer.start(); // this stops and restarts the active timer to reset the remaining time
        }
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
    QObject::connect(m_configReply = requestData(configPath(), QUrlQuery()), &QNetworkReply::finished, this, &SyncthingConnection::readConfig);
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
        auto jsonError = QJsonParseError();
        const auto replyDoc = QJsonDocument::fromJson(response, &jsonError);
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
        }
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
    auto newDirs = std::vector<SyncthingDir>();
    newDirs.reserve(static_cast<std::size_t>(dirs.size()));

    auto index = int();
    for (const auto &dirVal : dirs) {
        const auto dirObj = dirVal.toObject();
        auto *const dirItem = addDirInfo(newDirs, dirObj.value(QLatin1String("id")).toString());
        if (!dirItem) {
            continue;
        }

        dirItem->label = dirObj.value(QLatin1String("label")).toString();
        dirItem->path = dirObj.value(QLatin1String("path")).toString();
        dirItem->deviceIds.clear();
        dirItem->deviceNames.clear();
        const auto devices = dirObj.value(QLatin1String("devices")).toArray();
        for (const auto devObj : devices) {
            const auto devId = devObj.toObject().value(QLatin1String("deviceID")).toString();
            if (devId.isEmpty() || devId == m_myId) {
                continue;
            }
            dirItem->deviceIds << devId;
            if (const SyncthingDev *const dev = findDevInfo(devId, index)) {
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
    m_hasOutOfSyncDirs.reset();
}

/*!
 * \brief Reads device results of requestConfig(); called by readConfig().
 */
void SyncthingConnection::readDevs(const QJsonArray &devs)
{
    // store the new devs in a temporary list which is assigned to m_devs later
    auto newDevs = std::vector<SyncthingDev>();
    newDevs.reserve(static_cast<std::size_t>(devs.size()));
    auto *const thisDevice = addDevInfo(newDevs, m_myId);
    if (thisDevice) { // m_myId might be empty, then thisDevice will be nullptr
        thisDevice->id = m_myId;
        thisDevice->status = SyncthingDevStatus::ThisDevice;
        thisDevice->paused = false;
    }

    for (const auto &devVal : devs) {
        const auto devObj = devVal.toObject();
        const auto deviceId = devObj.value(QLatin1String("deviceID")).toString();
        const auto isThisDevice = deviceId == m_myId;
        auto *const devItem = isThisDevice ? thisDevice : addDevInfo(newDevs, deviceId);
        if (!devItem) {
            continue;
        }

        devItem->name = devObj.value(QLatin1String("name")).toString();
        devItem->addresses = things(devObj.value(QLatin1String("addresses")).toArray(), [](const QJsonValue &value) { return value.toString(); });
        devItem->compression = devObj.value(QLatin1String("compression")).toString();
        devItem->certName = devObj.value(QLatin1String("certName")).toString();
        devItem->introducer = devObj.value(QLatin1String("introducer")).toBool(false);
        if (!isThisDevice) {
            devItem->status = SyncthingDevStatus::Unknown;
            devItem->paused = devObj.value(QLatin1String("paused")).toBool(devItem->paused);
        }
    }

    m_devs.swap(newDevs);
    emit this->newDevices(m_devs);
    if (m_pausingOnMeteredConnection) {
        handleMeteredConnection();
    }
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
        auto jsonError = QJsonParseError();
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
    m_connectionsReply = requestData(QStringLiteral("system/connections"), QUrlQuery());
    m_connectionsReply->setProperty("lastEventId", m_lastEventId);
    QObject::connect(m_connectionsReply, &QNetworkReply::finished, this, &SyncthingConnection::readConnections);
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
        auto jsonError = QJsonParseError();
        const auto replyDoc = QJsonDocument::fromJson(response, &jsonError);
        if (jsonError.error != QJsonParseError::NoError) {
            emitError(tr("Unable to parse connections: "), jsonError, reply, response);
            return;
        }

        const auto replyObj = replyDoc.object();
        const QJsonObject totalObj(replyObj.value(QLatin1String("total")).toObject());

        // read traffic, the conversion to double is necessary because toInt() doesn't work for high values
        const QJsonValue totalIncomingTrafficValue(totalObj.value(QLatin1String("inBytesTotal")));
        const QJsonValue totalOutgoingTrafficValue(totalObj.value(QLatin1String("outBytesTotal")));
        const std::uint64_t totalIncomingTraffic = totalIncomingTrafficValue.isDouble() ? jsonValueToInt(totalIncomingTrafficValue) : unknownTraffic;
        const std::uint64_t totalOutgoingTraffic = totalOutgoingTrafficValue.isDouble() ? jsonValueToInt(totalOutgoingTrafficValue) : unknownTraffic;
        double transferTime = 0.0;
        const bool hasDelta
            = !m_lastConnectionsUpdateTime.isNull() && ((transferTime = (DateTime::gmtNow() - m_lastConnectionsUpdateTime).totalSeconds()) != 0.0);
        m_totalIncomingRate = (hasDelta && totalIncomingTraffic != unknownTraffic && m_totalIncomingTraffic != unknownTraffic)
            ? static_cast<double>(totalIncomingTraffic - m_totalIncomingTraffic) * 0.008 / transferTime
            : 0.0;
        m_totalOutgoingRate = (hasDelta && totalOutgoingTraffic != unknownTraffic && m_totalOutgoingTraffic != unknownTraffic)
            ? static_cast<double>(totalOutgoingTraffic - m_totalOutgoingTraffic) * 0.008 / transferTime
            : 0.0;
        emit trafficChanged(m_totalIncomingTraffic = totalIncomingTraffic, m_totalOutgoingTraffic = totalOutgoingTraffic);

        // read connection status
        const auto connectionsObj = replyObj.value(QLatin1String("connections")).toObject();
        auto index = 0;
        auto statusRecomputationFlags = StatusRecomputation::None;
        for (auto &dev : m_devs) {
            const auto connectionObj = connectionsObj.value(dev.id).toObject();
            if (connectionObj.isEmpty()) {
                ++index;
                continue;
            }

            const auto previousStatus = dev.status;
            const auto previouslyPaused = dev.paused;
            switch (dev.status) {
            case SyncthingDevStatus::ThisDevice:
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
            dev.paused = dev.status == SyncthingDevStatus::ThisDevice ? false : connectionObj.value(QLatin1String("paused")).toBool(false);
            dev.totalIncomingTraffic = jsonValueToInt(connectionObj.value(QLatin1String("inBytesTotal")));
            dev.totalOutgoingTraffic = jsonValueToInt(connectionObj.value(QLatin1String("outBytesTotal")));
            dev.connectionAddress = connectionObj.value(QLatin1String("address")).toString();
            dev.connectionType = connectionObj.value(QLatin1String("type")).toString();
            dev.connectionLocal = connectionObj.value(QLatin1String("isLocal")).toBool();
            dev.clientVersion = connectionObj.value(QLatin1String("clientVersion")).toString();
            emit devStatusChanged(dev, index);
            if (previousStatus != dev.status || previouslyPaused != dev.paused) {
                statusRecomputationFlags += StatusRecomputation::Status | StatusRecomputation::RemoteCompletion;
            }
            ++index;
        }

        m_lastConnectionsUpdateEvent = reply->property("lastEventId").toULongLong();
        m_lastConnectionsUpdateTime = DateTime::gmtNow();

        // since there seems no event for this data, keep polling
        if (m_keepPolling) {
            concludeConnection(statusRecomputationFlags);
            if ((m_pollingFlags && PollingFlags::TrafficStatistics) && m_trafficPollTimer.interval()) {
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

    // do not emit notifications for any errors occurred before connecting; we might have already emitted a notification for it when we were previously
    // connected
    if (m_lastErrorTime.isNull()) {
        m_lastErrorTime = DateTime::now();
    }

    switch (reply->error()) {
    case QNetworkReply::NoError: {
        auto jsonError = QJsonParseError();
        const auto replyDoc = QJsonDocument::fromJson(response, &jsonError);
        if (jsonError.error != QJsonParseError::NoError) {
            emitError(tr("Unable to parse errors: "), jsonError, reply, response);
            return;
        }

        const auto errors = replyDoc.object().value(QLatin1String("errors")).toArray();
        auto newErrors = std::vector<SyncthingError>();
        newErrors.reserve(static_cast<std::size_t>(errors.size()));
        for (const QJsonValue &errorVal : errors) {
            const QJsonObject errorObj = errorVal.toObject();
            if (errorObj.isEmpty()) {
                continue;
            }
            auto &error = newErrors.emplace_back();
            error.when = parseTimeStamp(errorObj.value(QLatin1String("when")), QStringLiteral("error message"));
            error.message = errorObj.value(QLatin1String("message")).toString();
            if (m_lastErrorTime < error.when) {
                emit newNotification(m_lastErrorTime = error.when, error.message);
            }
        }
        if (!m_errors.empty() || !errors.empty()) {
            emit this->beforeNewErrors(m_errors, newErrors);
            m_errors.swap(newErrors);
            emit this->newErrors(m_errors);
        }

        // since there is no event for this data, keep polling
        // note: The return value of hasUnreadNotifications() might have changed. This is however not (yet) used to compute the overall status so
        //       we can avoid a status recomputation here.
        if (m_keepPolling) {
            concludeConnection(StatusRecomputation::None);
            if ((m_pollingFlags && PollingFlags::Errors) && m_errorsPollTimer.interval()) {
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
    m_dirStatsReply = requestData(QStringLiteral("stats/folder"), QUrlQuery());
    m_dirStatsReply->setProperty("lastEventId", m_lastEventId);
    QObject::connect(m_dirStatsReply, &QNetworkReply::finished, this, &SyncthingConnection::readDirStatistics);
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
        auto jsonError = QJsonParseError();
        const auto replyDoc = QJsonDocument::fromJson(response, &jsonError);
        if (jsonError.error != QJsonParseError::NoError) {
            emitError(tr("Unable to parse folder statistics: "), jsonError, reply, response);
            return;
        }

        const auto replyObj = replyDoc.object();
        auto index = int();
        for (SyncthingDir &dirInfo : m_dirs) {
            const QJsonObject dirObj(replyObj.value(dirInfo.id).toObject());
            if (dirObj.isEmpty()) {
                ++index;
                continue;
            }

            auto dirModified = false;
            const auto eventId = reply->property("lastEventId").toULongLong();
            const auto lastScan = dirObj.value(QLatin1String("lastScan")).toString().toUtf8();
            if (!lastScan.isEmpty()) {
                dirModified = true;
                dirInfo.lastScanTime = parseTimeStamp(dirObj.value(QLatin1String("lastScan")), QStringLiteral("last scan"));
            }
            const auto lastFileObj = dirObj.value(QLatin1String("lastFile")).toObject();
            if (!lastFileObj.isEmpty()) {
                dirInfo.lastFileEvent = eventId;
                dirInfo.lastFileName = lastFileObj.value(QLatin1String("filename")).toString();
                dirModified = true;
                if (!dirInfo.lastFileName.isEmpty()) {
                    dirInfo.lastFileDeleted = lastFileObj.value(QLatin1String("deleted")).toBool(false);
                    dirInfo.lastFileTime = parseTimeStamp(lastFileObj.value(QLatin1String("at")), QStringLiteral("dir statistics"));
                    if (!dirInfo.lastFileTime.isNull() && eventId >= m_lastFileEvent) {
                        m_lastFileEvent = eventId;
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
            concludeConnection(StatusRecomputation::Status);
        }
        break;
    }
    case QNetworkReply::OperationCanceledError:
        handleAdditionalRequestCanceled();
        return;
    default:
        emitError(tr("Unable to request folder statistics: "), SyncthingErrorCategory::OverallConnection, reply);
    }
}

/*!
 * \brief Requests statistics (global and local status) for \a dirId asynchronously.
 */
void SyncthingConnection::requestDirStatus(const QString &dirId)
{
    auto query = QUrlQuery();
    query.addQueryItem(QStringLiteral("folder"), formatQueryItem(dirId));
    auto *const reply = requestData(QStringLiteral("db/status"), query);
    reply->setProperty("dirId", dirId);
    reply->setProperty("lastEventId", m_lastEventId);
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
        auto index = int();
        const auto dirId = reply->property("dirId").toString();
        SyncthingDir *const dir = findDirInfo(dirId, index);
        if (!dir) {
            // discard status for unknown dirs
            return;
        }

        // parse JSON
        auto jsonError = QJsonParseError();
        const auto replyDoc = QJsonDocument::fromJson(response, &jsonError);
        if (jsonError.error != QJsonParseError::NoError) {
            emitError(tr("Unable to parse status for folder %1: ").arg(dirId), jsonError, reply, response);
            return;
        }

        const auto careAboutOutOfSyncDirs = m_hasOutOfSyncDirs.has_value();
        const auto wasOutOfSync = careAboutOutOfSyncDirs && dir->isOutOfSync();
        readDirSummary(reply->property("lastEventId").toULongLong(), DateTime::now(), replyDoc.object(), *dir, index);
        if (careAboutOutOfSyncDirs && wasOutOfSync != dir->isOutOfSync()) {
            m_hasOutOfSyncDirs.reset();
        }
        if (m_keepPolling) {
            concludeConnection(careAboutOutOfSyncDirs ? StatusRecomputation::StatusAndOutOfSyncDirs : StatusRecomputation::Status);
        }
        break;
    }
    case QNetworkReply::OperationCanceledError:
        handleAdditionalRequestCanceled();
        return;
    default:
        emitError(tr("Unable to request folder statistics: "), SyncthingErrorCategory::SpecificRequest, reply);
    }
}

/*!
 * \brief Requests pull errors for \a dirId asynchronously.
 *
 * The dirStatusChanged() signal is emitted on success and error() in the error case.
 */
void SyncthingConnection::requestDirPullErrors(const QString &dirId, int page, int perPage)
{
    auto query = QUrlQuery();
    query.addQueryItem(QStringLiteral("folder"), formatQueryItem(dirId));
    if (page > 0 && perPage > 0) {
        query.addQueryItem(QStringLiteral("page"), QString::number(page));
        query.addQueryItem(QStringLiteral("perpage"), QString::number(perPage));
    }
    auto *const reply = requestData(folderErrorsPath(), query);
    reply->setProperty("dirId", dirId);
    reply->setProperty("lastEventId", m_lastEventId);
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
    auto index = int();
    const auto dirId = reply->property("dirId").toString();
    SyncthingDir *const dir = findDirInfo(dirId, index);
    if (!dir) {
        // discard errors for unknown dirs
        return;
    }

    switch (reply->error()) {
    case QNetworkReply::NoError: {
        // parse JSON
        auto jsonError = QJsonParseError();
        const auto replyDoc = QJsonDocument::fromJson(response, &jsonError);
        if (jsonError.error != QJsonParseError::NoError) {
            emitError(tr("Unable to parse pull errors for folder %1: ").arg(dirId), jsonError, reply, response);
            return;
        }

        const auto wasOutOfSync = dir->isOutOfSync();
        readFolderErrors(reply->property("lastEventId").toULongLong(), DateTime::now(), replyDoc.object(), *dir, index);
        if (wasOutOfSync != dir->isOutOfSync()) {
            invalidateHasOutOfSyncDirs();
        }
        break;
    }
    case QNetworkReply::OperationCanceledError:
        return;
    default:
        emitError(tr("Unable to request pull errors for folder %1: ").arg(dirId), SyncthingErrorCategory::SpecificRequest, reply);
    }
}

/*!
 * \brief Requests completion for \a devId and \a dirId asynchronously.
 */
void SyncthingConnection::requestCompletion(const QString &devId, const QString &dirId)
{
    auto query = QUrlQuery();
    query.addQueryItem(QStringLiteral("device"), formatQueryItem(devId));
    query.addQueryItem(QStringLiteral("folder"), formatQueryItem(dirId));
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
        devInfo->completionByDir[dirId].requestedForEventId = 0;
    }
    if (dirInfo) {
        dirInfo->completionByDevice[devId].requestedForEventId = 0;
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
        auto jsonError = QJsonParseError();
        const auto replyDoc = QJsonDocument::fromJson(response, &jsonError);
        if (jsonError.error == QJsonParseError::NoError) {
            // update the relevant completion info
            readRemoteFolderCompletion(DateTime::now(), replyDoc.object(), devId, devInfo, devIndex, dirId, dirInfo, dirIndex);
            concludeConnection(StatusRecomputation::Status);
            return;
        }

        emitError(tr("Unable to parse completion for device/folder %1/%2: ").arg(devId, dirId), jsonError, reply, response);
        break;
    }
    case QNetworkReply::ContentNotFoundError:
        // assign empty completion when receiving 404 response
        // note: The connector generally tries to avoid requesting the completion for paused dirs/devs but if the completion is requested
        //       before it is aware that the dir/dev is paused it might still run into this error, e.g. when pausing a directory and completion
        //       is requested concurrently. Before Syncthing v1.15.0 we've got an empty completion instead of 404 anyway.
        readRemoteFolderCompletion(SyncthingCompletion(), devId, devInfo, devIndex, dirId, dirInfo, dirIndex);
        concludeConnection(StatusRecomputation::Status);
        return;
    case QNetworkReply::OperationCanceledError:
        handleAdditionalRequestCanceled();
        break;
    default:
        emitError(tr("Unable to request completion for device/folder %1/%2: ").arg(devId, dirId), SyncthingErrorCategory::SpecificRequest, reply);
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
        auto jsonError = QJsonParseError();
        const auto replyDoc = QJsonDocument::fromJson(response, &jsonError);
        if (jsonError.error != QJsonParseError::NoError) {
            emitError(tr("Unable to parse device statistics: "), jsonError, reply, response);
            return;
        }

        const auto replyObj = replyDoc.object();
        auto index = int();
        for (auto &devInfo : m_devs) {
            const QJsonObject devObj(replyObj.value(devInfo.id).toObject());
            if (!devObj.isEmpty()) {
                devInfo.lastSeen = parseTimeStamp(devObj.value(QLatin1String("lastSeen")), QStringLiteral("last seen"), DateTime(), true);
                emit devStatusChanged(devInfo, index);
            }
            ++index;
        }
        // since there seems no event for this data, keep polling
        if (m_keepPolling) {
            concludeConnection(StatusRecomputation::None);
            if ((m_pollingFlags && PollingFlags::DeviceStatistics) && m_devStatsPollTimer.interval()) {
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
        auto jsonError = QJsonParseError();
        const auto replyDoc(QJsonDocument::fromJson(response, &jsonError));
        if (jsonError.error != QJsonParseError::NoError) {
            emitError(tr("Unable to parse version: "), jsonError, reply, response);
            return;
        }
        const auto replyObj = replyDoc.object();
        m_syncthingVersion = replyObj.value(QLatin1String("longVersion")).toString();
        if (m_keepPolling) {
            concludeConnection(StatusRecomputation::None);
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
    auto query = QUrlQuery();
    query.addQueryItem(QStringLiteral("text"), formatQueryItem(text));
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
        auto jsonError = QJsonParseError();
        const auto replyDoc = QJsonDocument::fromJson(response, &jsonError);
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

/*!
 * \brief Request the override of the send only folder with the specified \a dirId.
 * \remarks
 * - Override means to make the local version latest, overriding changes made on other devices.
 * - This call does nothing if the folder is not a send only folder.
 */
void SyncthingConnection::requestOverride(const QString &dirId)
{
    auto query = QUrlQuery();
    query.addQueryItem(QStringLiteral("folder"), formatQueryItem(dirId));
    auto *const reply = postData(QStringLiteral("db/override"), query);
    reply->setProperty("dirId", dirId);
    QObject::connect(reply, &QNetworkReply::finished, this, &SyncthingConnection::readOverride, Qt::QueuedConnection);
}

/*!
 * \brief Reads data from requestOverride().
 */
void SyncthingConnection::readOverride()
{
    auto const [reply, response] = prepareReply(false);
    if (!reply) {
        return;
    }
    switch (reply->error()) {
    case QNetworkReply::NoError:
        emit overrideTriggered(reply->property("dirId").toString());
        break;
    default:
        emitError(tr("Unable to request directory override: "), SyncthingErrorCategory::SpecificRequest, reply);
    }
}

/*!
 * \brief Request the revert of the receive only folder with the specified \a dirId.
 * \remarks
 * - Reverting a folder means to undo all local changes.
 * - This call does nothing if the folder is not a receive only folder.
 */
void SyncthingConnection::requestRevert(const QString &dirId)
{
    auto query = QUrlQuery();
    query.addQueryItem(QStringLiteral("folder"), formatQueryItem(dirId));
    auto *const reply = postData(QStringLiteral("db/revert"), query);
    reply->setProperty("dirId", dirId);
    QObject::connect(reply, &QNetworkReply::finished, this, &SyncthingConnection::readRevert, Qt::QueuedConnection);
}

/*!
 * \brief Downloads a support bundle.
 */
SyncthingConnection::QueryResult SyncthingConnection::downloadSupportBundle()
{
    return QueryResult{ networkAccessManager().get(prepareRequest(QStringLiteral("debug/support"), QUrlQuery())) };
}

/*!
 * \brief Reads data from requestOverride().
 */
void SyncthingConnection::readRevert()
{
    auto const [reply, response] = prepareReply(false);
    if (!reply) {
        return;
    }
    switch (reply->error()) {
    case QNetworkReply::NoError:
        emit revertTriggered(reply->property("dirId").toString());
        break;
    default:
        emitError(tr("Unable to request directory revert: "), SyncthingErrorCategory::SpecificRequest, reply);
    }
}

/*!
 * \brief Performs a generic HTTP request to Syncthing.
 */
SyncthingConnection::QueryResult SyncthingConnection::sendCustomRequest(
    const QByteArray &verb, const QUrl &url, const QMap<QByteArray, QByteArray> &headers, QIODevice *data)
{
    auto request = prepareRequest(url, false);
    request.setTransferTimeout(0);
    for (auto i = headers.cbegin(), end = headers.cend(); i != end; ++i) {
        request.setRawHeader(i.key(), i.value());
    }
    auto *const reply = networkAccessManager().sendCustomRequest(request, verb, data);
    return QueryResult{ reply };
}

/*!
 * \brief Performs a generic HTTP request to the Syncthing REST-API.
 */
SyncthingConnection::QueryResult SyncthingConnection::requestJsonData(const QByteArray &verb, const QString &path, const QUrlQuery &query,
    const QByteArray &data, std::function<void(QJsonDocument &&, QString &&)> &&callback, bool rest, bool longPolling)
{
#if !defined(LIB_SYNCTHING_CONNECTOR_CONNECTION_MOCKED) && !defined(LIB_SYNCTHING_CONNECTOR_MOCKED)
    auto *const reply = networkAccessManager().sendCustomRequest(prepareRequest(path, query, rest, longPolling), verb, data);
#ifndef QT_NO_SSL
    QObject::connect(reply, &QNetworkReply::sslErrors, this, &SyncthingConnection::handleSslErrors);
#endif
    if (loggingFlags() && SyncthingConnectionLoggingFlags::ApiCalls) {
        cerr << Phrases::Info << "Querying API: " << verb.data() << ' ' << reply->url().toString().toStdString() << Phrases::EndFlush;
        cerr.write(data.data(), static_cast<std::streamsize>(data.size()));
    }
#else
    Q_UNUSED(data)
    Q_UNUSED(rest)
    Q_UNUSED(longPolling)
    auto *const reply = MockedReply::forRequest(verb, path, query, true);
#endif
    return { reply,
        QObject::connect(
            reply, &QNetworkReply::finished, this, [this, cb = std::move(callback)]() mutable { readJsonData(std::move(cb)); },
            Qt::QueuedConnection) };
}

/*!
 * \brief Lists items in the directory with the specified \a dirId down to \a levels (or fully if \a levels is 0) as of \a prefix.
 * \sa https://docs.syncthing.net/rest/db-browse-get.html
 * \remarks
 * In contrast to most other functions, this one uses a \a callback to return results (instead of a signal). This makes it easier
 * to consume results of a specific request. Errors are still reported via the error() signal so there's no extra error handling
 * required. Note that in case of an error \a callback is invoked with a non-empty string containing the error message.
 */
SyncthingConnection::QueryResult SyncthingConnection::browse(const QString &dirId, const QString &prefix, int levels,
    std::function<void(std::vector<std::unique_ptr<SyncthingItem>> &&, QString &&)> &&callback)
{
    auto query = QUrlQuery();
    query.addQueryItem(QStringLiteral("folder"), formatQueryItem(dirId));
    if (!prefix.isEmpty()) {
        query.addQueryItem(QStringLiteral("prefix"), formatQueryItem(prefix));
    }
    if (levels > 0) {
        query.addQueryItem(QStringLiteral("levels"), QString::number(levels));
    }
    auto *const reply = requestData(QStringLiteral("db/browse"), query);
    return { reply,
        QObject::connect(
            reply, &QNetworkReply::finished, this,
            [this, id = dirId, l = levels, cb = std::move(callback)]() mutable { readBrowse(id, l, std::move(cb)); }, Qt::QueuedConnection) };
}

/*!
 * \brief Queries the contents of ".stignore" and expansions of the directory with the specified \a dirId.
 * \sa https://docs.syncthing.net/rest/db-ignores-get.html
 * \remarks
 * In contrast to most other functions, this one uses a \a callback to return results (instead of a signal). This makes it easier
 * to consume results of a specific request. Errors are still reported via the error() signal so there's no extra error handling
 * required. Note that in case of an error \a callback is invoked with a non-empty string containing the error message.
 */
SyncthingConnection::QueryResult SyncthingConnection::ignores(const QString &dirId, std::function<void(SyncthingIgnores &&, QString &&)> &&callback)
{
    auto query = QUrlQuery();
    query.addQueryItem(QStringLiteral("folder"), formatQueryItem(dirId));
    auto *const reply = requestData(QStringLiteral("db/ignores"), query);
    return { reply,
        QObject::connect(
            reply, &QNetworkReply::finished, this, [this, id = dirId, cb = std::move(callback)]() mutable { readIgnores(id, std::move(cb)); },
            Qt::QueuedConnection) };
}

/*!
 * \brief Sets the contents of ".stignore" of the directory with the specified \a dirId.
 * \sa https://docs.syncthing.net/rest/db-ignores-post.html
 * \remarks
 * In contrast to most other functions, this one uses a \a callback to return results (instead of a signal). This makes it easier
 * to consume results of a specific request. Errors are still reported via the error() signal so there's no extra error handling
 * required. Note that in case of an error \a callback is invoked with a non-empty string containing the error message.
 */
SyncthingConnection::QueryResult SyncthingConnection::setIgnores(
    const QString &dirId, const SyncthingIgnores &ignores, std::function<void(QString &&)> &&callback)
{
    auto query = QUrlQuery();
    query.addQueryItem(QStringLiteral("folder"), formatQueryItem(dirId));
    auto ignoreArray = QJsonArray();
    for (const auto &ignore : ignores.ignore) {
        ignoreArray.append(ignore);
    }
    auto jsonObj = QJsonObject();
    jsonObj.insert(QLatin1String("ignore"), ignoreArray);
    auto jsonDoc = QJsonDocument();
    jsonDoc.setObject(jsonObj);
    auto *const reply = postData(QStringLiteral("db/ignores"), query, jsonDoc.toJson(QJsonDocument::Compact));
    return { reply,
        QObject::connect(
            reply, &QNetworkReply::finished, this, [this, id = dirId, cb = std::move(callback)]() mutable { readSetIgnores(id, std::move(cb)); },
            Qt::QueuedConnection) };
}

/// \cond
static void readSyncthingItems(const QJsonArray &array, std::vector<std::unique_ptr<SyncthingItem>> &into, int level, int levels)
{
    into.reserve(static_cast<std::size_t>(array.size()));
    for (const auto &jsonItem : array) {
        if (!jsonItem.isObject()) {
            continue;
        }
        const auto jsonItemObj = jsonItem.toObject();
        const auto typeValue = jsonItemObj.value(QLatin1String("type"));
        const auto index = into.size();
        const auto children = jsonItemObj.value(QLatin1String("children"));
        auto &item = into.emplace_back(std::make_unique<SyncthingItem>());
        item->name = jsonItemObj.value(QLatin1String("name")).toString();
        item->modificationTime = CppUtilities::DateTime::fromIsoStringGmt(jsonItemObj.value(QLatin1String("modTime")).toString().toUtf8().data());
        item->size = static_cast<std::size_t>(jsonItemObj
                .value(QLatin1String("size"))
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
                .toInteger()
#else
                .toDouble()
#endif
        );
        item->index = index;
        item->level = level;
        switch (typeValue.toInt(-1)) {
        case 0:
            item->type = SyncthingItemType::File;
            break;
        case 1:
            item->type = SyncthingItemType::Directory;
            break;
        case 2:
        case 3:
        case 4:
            item->type = SyncthingItemType::Symlink;
            break;
        default:
            const auto type = typeValue.toString();
            if (type == QLatin1String("FILE_INFO_TYPE_FILE")) {
                item->type = SyncthingItemType::File;
            } else if (type == QLatin1String("FILE_INFO_TYPE_DIRECTORY")) {
                item->type = SyncthingItemType::Directory;
            } else if (type == QLatin1String("FILE_INFO_TYPE_SYMLINK")) {
                item->type = SyncthingItemType::Symlink;
            }
        }
        readSyncthingItems(children.toArray(), item->children, level + 1, levels);
        item->childrenPopulated = !levels || level < levels;
    }
}
/// \endcond

/*!
 * \brief Reads the response of requestJsonData() and reports results via the specified \a callback. Emits error() in case of an error.
 * \remarks The \a callback is also emitted in the error case (with the error message as second parameter and an empty list of items).
 */
void SyncthingConnection::readJsonData(std::function<void(QJsonDocument &&, QString &&)> &&callback)
{
    auto const [reply, response] = prepareReply();
    if (!reply) {
        return;
    }
    switch (reply->error()) {
    case QNetworkReply::NoError: {
        auto jsonError = QJsonParseError();
        auto replyDoc = QJsonDocument::fromJson(response, &jsonError);
        if (jsonError.error != QJsonParseError::NoError) {
            auto errorMessage = tr("Unable to parse JSON response: ") + jsonError.errorString();
            emit error(errorMessage, SyncthingErrorCategory::Parsing, QNetworkReply::NoError);
            if (callback) {
                callback(std::move(replyDoc), std::move(errorMessage));
            }
            return;
        }
        if (callback) {
            callback(std::move(replyDoc), QString());
        }
        break;
    }
    default:
        auto errorMessage = tr("Unable to request: ") + reply->errorString();
        emitError(errorMessage, reply);
        if (callback) {
            callback(QJsonDocument(), std::move(errorMessage));
        }
    }
}

/*!
 * \brief Reads the response of browse() and reports results via the specified \a callback. Emits error() in case of an error.
 * \remarks The \a callback is also emitted in the error case (with the error message as second parameter and an empty list of items).
 */
void SyncthingConnection::readBrowse(
    const QString &dirId, int levels, std::function<void(std::vector<std::unique_ptr<SyncthingItem>> &&, QString &&)> &&callback)
{
    auto const [reply, response] = prepareReply();
    if (!reply) {
        return;
    }
    auto items = std::vector<std::unique_ptr<SyncthingItem>>();
    switch (reply->error()) {
    case QNetworkReply::NoError: {
        auto jsonError = QJsonParseError();
        const auto replyDoc = QJsonDocument::fromJson(response, &jsonError);
        if (jsonError.error != QJsonParseError::NoError) {
            auto errorMessage = tr("Unable to parse response for browsing \"%1\": ").arg(dirId) + jsonError.errorString();
            emit error(errorMessage, SyncthingErrorCategory::Parsing, QNetworkReply::NoError);
            if (callback) {
                callback(std::move(items), std::move(errorMessage));
            }
            return;
        }
        readSyncthingItems(replyDoc.array(), items, 0, levels);
        if (callback) {
            callback(std::move(items), QString());
        }
        break;
    }
    default:
        auto errorMessage = tr("Unable to browse \"%1\": ").arg(dirId) + reply->errorString();
        emitError(errorMessage, reply);
        if (callback) {
            callback(std::move(items), std::move(errorMessage));
        }
    }
}

/*!
 * \brief Reads the response of ignores() and reports results via the specified \a callback. Emits error() in case of an error.
 * \remarks The \a callback is also emitted in the error case (with the error message as second parameter and an empty list of items).
 */
void SyncthingConnection::readIgnores(const QString &dirId, std::function<void(SyncthingIgnores &&, QString &&)> &&callback)
{
    auto const [reply, response] = prepareReply();
    if (!reply) {
        return;
    }
    auto res = SyncthingIgnores();
    switch (reply->error()) {
    case QNetworkReply::NoError: {
        auto jsonError = QJsonParseError();
        const auto replyDoc = QJsonDocument::fromJson(response, &jsonError);
        if (jsonError.error != QJsonParseError::NoError) {
            auto errorMessage = tr("Unable to query ignore patterns of \"%1\": ").arg(dirId) + jsonError.errorString();
            emit error(errorMessage, SyncthingErrorCategory::Parsing, QNetworkReply::NoError);
            if (callback) {
                callback(std::move(res), std::move(errorMessage));
            }
            return;
        }
        const auto docObj = replyDoc.object();
        const auto ignores = docObj.value(QLatin1String("ignore")).toArray();
        const auto expanded = docObj.value(QLatin1String("expanded")).toArray();
        res.ignore.reserve(ignores.size());
        res.expanded.reserve(expanded.size());
        for (const auto &ignore : ignores) {
            res.ignore.append(ignore.toString());
        }
        for (const auto &expand : expanded) {
            res.expanded.append(expand.toString());
        }
        if (callback) {
            callback(std::move(res), QString());
        }
        break;
    }
    default:
        auto errorMessage = tr("Unable to query ignore patterns of \"%1\": ").arg(dirId) + reply->errorString();
        emitError(errorMessage, reply);
        if (callback) {
            callback(std::move(res), std::move(errorMessage));
        }
    }
}

/*!
 * \brief Reads the response of setIgnores() and reports results via the specified \a callback. Emits error() in case of an error.
 * \remarks The \a callback is also emitted in the error case (with the error message as second parameter and an empty list of items).
 */
void SyncthingConnection::readSetIgnores(const QString &dirId, std::function<void(QString &&)> &&callback)
{
    auto const [reply, response] = prepareReply();
    if (!reply) {
        return;
    }
    switch (reply->error()) {
    case QNetworkReply::NoError: {
        if (callback) {
            callback(QString());
        }
        break;
    }
    default:
        auto errorMessage = tr("Unable to change ignore patterns of \"%1\": ").arg(dirId) + reply->errorString();
        emitError(errorMessage, reply);
        if (callback) {
            callback(std::move(errorMessage));
        }
    }
}

// post config

/*!
 * \brief Posts the specified \a rawConfig.
 * \remarks The signal newConfigTriggered() is emitted when the config has been posted successfully. In the error case, error() is emitted.
 *          Besides, the newConfig() signal should be emitted as well, indicating Syncthing has actually applied the new configuration.
 */
SyncthingConnection::QueryResult SyncthingConnection::postConfigFromJsonObject(
    const QJsonObject &rawConfig, std::function<void(QString &&)> &&callback)
{
    return postConfigFromByteArray(QJsonDocument(rawConfig).toJson(QJsonDocument::Compact), std::move(callback));
}

/*!
 * \brief Posts the specified \a rawConfig.
 * \remarks
 * This function is a slot (in contrast to postConfigFromJsonObject()) and thus may be invoked via QMetaObject::invokeMethod() from
 * another thread.
 */
void Data::SyncthingConnection::postRawConfig(const QByteArray &rawConfig)
{
    postConfigFromByteArray(rawConfig, std::function<void(QString &&)>());
}

/*!
 * \brief Posts the specified \a rawConfig.
 * \param rawConfig A valid JSON document containing the configuration. It is directly passed to Syncthing.
 * \remarks The signal newConfigTriggered() is emitted when the config has been posted successfully. In the error case, error() is emitted.
 *          Besides, the newConfig() signal should be emitted as well, indicating Syncthing has actually applied the new configuration.
 */
SyncthingConnection::QueryResult SyncthingConnection::postConfigFromByteArray(const QByteArray &rawConfig, std::function<void(QString &&)> &&callback)
{
    auto *const reply = sendData(changeConfigVerb(), configPath(), QUrlQuery(), rawConfig);
    return { reply,
        QObject::connect(
            reply, &QNetworkReply::finished, this, [this, cb = std::move(callback)]() mutable { readPostConfig(std::move(cb)); },
            Qt::QueuedConnection) };
}

/*!
 * \brief Reads data from postConfigFromJsonObject() and postConfigFromByteArray().
 */
void SyncthingConnection::readPostConfig(std::function<void(QString &&)> &&callback)
{
    auto const [reply, response] = prepareReply(false, false);
    switch (reply->error()) {
    case QNetworkReply::NoError:
        emit newConfigTriggered();
        if (callback) {
            callback(QString());
        }
        break;
    default:
        auto errorMessage = tr("Unable to post config: ") + reply->errorString();
        emitError(errorMessage, SyncthingErrorCategory::SpecificRequest, reply);
        if (callback) {
            callback(std::move(errorMessage));
        }
    }
}

/*!
 * \brief Reads data from requestDirStatus() and FolderSummary-event and stores them to \a dir.
 * \remarks The fields of the summary are defined in `lib/model/folder_summary.go`.
 */
void SyncthingConnection::readDirSummary(SyncthingEventId eventId, DateTime eventTime, const QJsonObject &summary, SyncthingDir &dir, int index)
{
    if (summary.isEmpty() || dir.lastStatisticsUpdateEvent > eventId) {
        return;
    }

    // backup previous statistics -> if there's no difference after all, don't emit completed event
    auto &globalStats = dir.globalStats;
    auto &localStats = dir.localStats;
    auto &neededStats = dir.neededStats;
    auto &receiveOnlyStats = dir.receiveOnlyStats;
    const auto previouslyUpdated = !dir.lastStatisticsUpdateTime.isNull();
    const auto previouslyGlobal = globalStats;
    const auto previouslyNeeded = !neededStats.isNull();

    // update statistics
    globalStats.bytes = jsonValueToInt(summary.value(QLatin1String("globalBytes")));
    globalStats.deletes = jsonValueToInt(summary.value(QLatin1String("globalDeleted")));
    globalStats.files = jsonValueToInt(summary.value(QLatin1String("globalFiles")));
    globalStats.dirs = jsonValueToInt(summary.value(QLatin1String("globalDirectories")));
    globalStats.symlinks = jsonValueToInt(summary.value(QLatin1String("globalSymlinks")));
    globalStats.total = jsonValueToInt(summary.value(QLatin1String("globalTotalItems")));
    localStats.bytes = jsonValueToInt(summary.value(QLatin1String("localBytes")));
    localStats.deletes = jsonValueToInt(summary.value(QLatin1String("localDeleted")));
    localStats.files = jsonValueToInt(summary.value(QLatin1String("localFiles")));
    localStats.dirs = jsonValueToInt(summary.value(QLatin1String("localDirectories")));
    localStats.symlinks = jsonValueToInt(summary.value(QLatin1String("localSymlinks")));
    localStats.total = jsonValueToInt(summary.value(QLatin1String("localTotalItems")));
    neededStats.bytes = jsonValueToInt(summary.value(QLatin1String("needBytes")));
    neededStats.deletes = jsonValueToInt(summary.value(QLatin1String("needDeletes")));
    neededStats.files = jsonValueToInt(summary.value(QLatin1String("needFiles")));
    neededStats.dirs = jsonValueToInt(summary.value(QLatin1String("needDirectories")));
    neededStats.symlinks = jsonValueToInt(summary.value(QLatin1String("needSymlinks")));
    neededStats.total = jsonValueToInt(summary.value(QLatin1String("needTotalItems")));
    receiveOnlyStats.bytes = jsonValueToInt(summary.value(QLatin1String("receiveOnlyChangedBytes")));
    receiveOnlyStats.deletes = jsonValueToInt(summary.value(QLatin1String("receiveOnlyChangedDeletes")));
    receiveOnlyStats.files = jsonValueToInt(summary.value(QLatin1String("receiveOnlyChangedFiles")));
    receiveOnlyStats.dirs = jsonValueToInt(summary.value(QLatin1String("receiveOnlyChangedDirectories")));
    receiveOnlyStats.symlinks = jsonValueToInt(summary.value(QLatin1String("receiveOnlyChangedSymlinks")));
    receiveOnlyStats.total = jsonValueToInt(summary.value(QLatin1String("receiveOnlyTotalItems")));
    dir.pullErrorCount = jsonValueToInt(summary.value(QLatin1String("pullErrors")));
    m_statusRecomputationFlags += StatusRecomputation::DirStats;

    dir.ignorePatterns = summary.value(QLatin1String("ignorePatterns")).toBool();
    dir.lastStatisticsUpdateEvent = eventId;
    dir.lastStatisticsUpdateTime = eventTime;

    // update status
    const auto lastStatusUpdate
        = parseTimeStamp(summary.value(QLatin1String("stateChanged")), QStringLiteral("state changed"), dir.lastStatusUpdateTime);
    if (dir.pullErrorCount) {
        // consider the directory still as out-of-sync if there are still pull errors
        // note: Syncthing can report an "idle" status despite pull errors.
        dir.status = SyncthingDirStatus::OutOfSync;
        dir.lastStatusUpdateTime = std::max(dir.lastStatusUpdateTime, lastStatusUpdate);
    } else if (const auto state = summary.value(QLatin1String("state")).toString(); !state.isEmpty()) {
        dir.assignStatus(state, eventId, lastStatusUpdate);
    }

    dir.completionPercentage = globalStats.bytes ? static_cast<int>((globalStats.bytes - neededStats.bytes) * 100 / globalStats.bytes) : 100;

    emit dirStatusChanged(dir, index);
    if (neededStats.isNull() && previouslyUpdated && (previouslyNeeded || globalStats != previouslyGlobal)) {
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
    auto row = int();
    const auto *const dir = findDirInfo(dirId, row);
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
    auto row = int();
    const auto *const dev = findDevInfo(devId, row);
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
    auto index = int();
    auto *dirInfo = findDirInfo(QLatin1String("folder"), eventData, &index);
    if (!dirInfo && !(dirInfo = findDirInfo(QLatin1String("folderID"), eventData, &index))) {
        return;
    }

    auto change = SyncthingFileChange();
    change.local = eventType.startsWith("Local");
    change.eventTime = eventTime;
    change.action = eventData.value(QLatin1String("action")).toString();
    change.type = eventData.value(QLatin1String("type")).toString();
    change.modifiedBy = eventData.value(QLatin1String("modifiedBy")).toString();
    change.path = eventData.value(QLatin1String("path")).toString();
    if (m_recordFileChanges) {
        dirInfo->recentChanges.emplace_back(std::move(change));
        emit dirStatusChanged(*dirInfo, index);
        emit fileChanged(*dirInfo, index, dirInfo->recentChanges.back());
    } else {
        emit fileChanged(*dirInfo, index, change);
    }
}

// events / long polling API

/*!
 * \brief Requests the Syncthing events (since the last successful call) asynchronously.
 * \remarks
 * - This is a long-polling API call. Hence the request is repeated until the connection is aborted.
 * - The signal newEvents() is emitted on success; otherwise error() is emitted.
 * - The disk events are queried separately via requestDiskEvents() so they can be limited individually.
 */
void SyncthingConnection::requestEvents()
{
    if (m_eventsReply || !(m_pollingFlags && PollingFlags::MainEvents)) {
        return;
    }
    if (m_eventMask.isEmpty()) {
        m_eventMask = QStringLiteral("Starting,StateChanged,FolderRejected,FolderErrors,FolderSummary,FolderCompletion,"
                                     "FolderScanProgress,FolderPaused,FolderResumed,DeviceRejected,DeviceConnected,"
                                     "DeviceDisconnected,DevicePaused,DeviceResumed,ConfigSaved,LocalIndexUpdated");
        if (m_pollingFlags && PollingFlags::DownloadProgress) {
            m_eventMask += QStringLiteral(",DownloadProgress");
        }
        if (m_pollingFlags && PollingFlags::RemoteIndexUpdated) {
            m_eventMask += QStringLiteral(",RemoteIndexUpdated");
        }
        if (m_pollingFlags && PollingFlags::ItemFinished) {
            m_eventMask += QStringLiteral(",ItemFinished");
        }

        // reset/restore event ID as each mask creates its own stream of IDs
        m_lastConnectionsUpdateEvent = m_lastFileEvent = 0;
        if (!(m_lastEventId = m_lastEventIdByMask.value(m_eventMask, 0))) {
            m_hasEvents = false; // do initial requests again as we might have missed events
        }
    }
    auto query = QUrlQuery();
    query.addQueryItem(QStringLiteral("events"), m_eventMask);
    if (m_lastEventId && m_hasEvents) {
        query.addQueryItem(QStringLiteral("since"), QString::number(m_lastEventId));
    } else {
        query.addQueryItem(QStringLiteral("limit"), QStringLiteral("1"));
    }
    // force to return immediately after the first call
    if (!m_hasEvents) {
        query.addQueryItem(QStringLiteral("timeout"), QStringLiteral("0"));
    } else if (m_longPollingTimeout) {
        query.addQueryItem(QStringLiteral("timeout"), QString::number(m_longPollingTimeout / 1000));
    }
    QObject::connect(m_eventsReply = requestData(QStringLiteral("events"), query, true, m_hasEvents), &QNetworkReply::finished, this,
        &SyncthingConnection::readEvents);
}

/*!
 * \brief Reads results of requestEvents().
 */
void SyncthingConnection::readEvents()
{
    auto const expectedReply = m_eventsReply;
    auto const [reply, response] = prepareReply(m_eventsReply);
    if (!reply) {
        return;
    }

    if (m_hasOutOfSyncDirs.has_value()) {
        m_statusRecomputationFlags += StatusRecomputation::OutOfSyncDirs;
    }
    switch (reply->error()) {
    case QNetworkReply::NoError: {
        auto jsonError = QJsonParseError();
        const auto replyDoc = QJsonDocument::fromJson(response, &jsonError);
        if (jsonError.error != QJsonParseError::NoError) {
            emitError(tr("Unable to parse Syncthing events: "), jsonError, reply, response);
            handleFatalConnectionError();
            return;
        }

        const auto hadStateChanged = m_hasConfig && m_hasStatus && !m_hasEvents;
        m_hasEvents = true;
        const auto replyArray = replyDoc.array();
        emit newEvents(replyArray);
        const auto res = readEventsFromJsonArray(replyArray, m_lastEventId);
        emit allEventsProcessed();
        if (hadStateChanged) {
            emit hasStateChanged();
        }

        // request further statistics only *after* receiving the first event (and not in continueConnecting())
        // note: We avoid requesting the whole event history. So we rely on these statistics to tell the initial state. When the
        //       state of e.g. a directory changes before we receive the first event we would miss that state change if statistics
        //       were requested in continueConnecting().
        if (!m_statsRequested) {
            requestConnections();
            requestDirStatistics();
            requestDeviceStatistics();
            for (const SyncthingDir &dir : m_dirs) {
                requestDirStatus(dir.id);
            }
        }

        if (!res) {
            return;
        }

        if (!replyArray.isEmpty() && (loggingFlags() && SyncthingConnectionLoggingFlags::Events)) {
            const auto log = replyDoc.toJson(QJsonDocument::Indented);
            cerr << Phrases::Info << "Received " << replyArray.size() << " Syncthing events:" << Phrases::End << log.data() << endl;
        }
        break;
    }
    case QNetworkReply::TimeoutError:
        // no new events available, keep polling
        break;
    case QNetworkReply::OperationCanceledError:
        if (reply == expectedReply) {
            handleAdditionalRequestCanceled();
        }
        return;
    default:
        emitError(tr("Unable to request Syncthing events: "), SyncthingErrorCategory::OverallConnection, reply);
        handleFatalConnectionError();
        return;
    }

    if (m_keepPolling) {
        requestEvents();
        concludeConnection(StatusRecomputation::None);
    } else {
        setStatus(SyncthingStatus::Disconnected);
    }
}

/*!
 * \brief Reads results of requestEvents().
 */
bool SyncthingConnection::readEventsFromJsonArray(const QJsonArray &events, quint64 &idVariable)
{
    const auto lastId = idVariable;
    for (const auto &eventVal : events) {
        const auto event = eventVal.toObject();
        const auto eventTime = parseTimeStamp(event.value(QLatin1String("time")), QStringLiteral("event time"));
        const auto eventType = event.value(QLatin1String("type")).toString();
        const auto eventData = event.value(QLatin1String("data")).toObject();
        const auto eventIdValue = event.value(QLatin1String("id"));
        const auto eventId = static_cast<quint64>(std::max(eventIdValue.toDouble(), 0.0));
        if (eventIdValue.isDouble()) {
            if (eventId < lastId) {
                // re-connect if the event ID decreases as this indicates Syncthing has been restarted
                // note: The Syncthing docs say "A unique ID for this event on the events API. It always increases by 1: the
                // first event generated has id 1, the next has id 2 etc.".
                if (loggingFlags() && SyncthingConnectionLoggingFlags::ApiCalls) {
                    std::cerr << Phrases::Info << "Re-connecting as event ID is decreasing (" << eventId << " < " << lastId
                              << "), Syncthing has likely been restarted" << Phrases::End;
                }
                reconnect();
                return false;
            }
            if (eventId > idVariable) {
                idVariable = eventId;
            }
        }
        if (eventType == QLatin1String("Starting")) {
            readStartingEvent(eventData);
        } else if (eventType == QLatin1String("StateChanged")) {
            readStatusChangedEvent(eventId, eventTime, eventData);
        } else if (eventType == QLatin1String("DownloadProgress")) {
            readDownloadProgressEvent(eventData);
        } else if (eventType.startsWith(QLatin1String("Folder"))) {
            readDirEvent(eventId, eventTime, eventType, eventData);
        } else if (eventType.startsWith(QLatin1String("Device"))) {
            readDeviceEvent(eventId, eventTime, eventType, eventData);
        } else if (eventType == QLatin1String("ConfigSaved")) {
            requestConfig();
        } else if (eventType.endsWith(QLatin1String("ChangeDetected"))) {
            readChangeEvent(eventTime, eventType, eventData);
        } else if (eventType == QLatin1String("LocalIndexUpdated")) {
            requestDirStatistics();
        } else if (eventType == QLatin1String("RemoteIndexUpdated")) {
            readRemoteIndexUpdated(eventId, eventData);
        } else if (eventType == QLatin1String("ItemFinished")) {
            readItemFinished(eventId, eventTime, eventData);
        }
    }
    return true;
}

/*!
 * \brief Reads results of requestEvents().
 */
void SyncthingConnection::readStartingEvent(const QJsonObject &eventData)
{
    const auto configDir = eventData.value(QLatin1String("home")).toString();
    if (configDir != m_configDir) {
        emit configDirChanged(m_configDir = configDir);
    }
    emitMyIdChanged(eventData.value(QLatin1String("myID")).toString());
}

/*!
 * \brief Reads results of requestEvents().
 */
void SyncthingConnection::readStatusChangedEvent(SyncthingEventId eventId, DateTime eventTime, const QJsonObject &eventData)
{
    const auto dirId = eventData.value(QLatin1String("folder")).toString();
    if (dirId.isEmpty()) {
        return;
    }

    // find the directory
    auto index = int();
    auto *dirInfo = findDirInfo(dirId, index);

    // add a new directory if the dir is not present yet
    const auto dirAlreadyPresent = dirInfo != nullptr;
    if (!dirAlreadyPresent) {
        dirInfo = &m_dirs.emplace_back(dirId);
    }

    // assign new status
    const auto previousStatus = dirInfo->status;
    const auto wasOutOfSync = dirInfo->isOutOfSync();
    auto statusChanged = dirInfo->assignStatus(eventData.value(QLatin1String("to")).toString(), eventId, eventTime);
    switch (dirInfo->status) {
    case SyncthingDirStatus::Idle:
        if (previousStatus == SyncthingDirStatus::Scanning) {
            requestDirStatistics();
        }
        break;
    case SyncthingDirStatus::OutOfSync:
        if (const auto errorMessage = eventData.value(QLatin1String("error")).toString(); !errorMessage.isEmpty()) {
            dirInfo->globalError = errorMessage;
            statusChanged = true;
        }
        break;
    default:;
    }
    if (wasOutOfSync != dirInfo->isOutOfSync()) {
        m_hasOutOfSyncDirs.reset();
    }

    // request config for complete meta data of new directory
    if (!dirAlreadyPresent) {
        requestConfig();
        return;
    }

    // emit status changed when dir already present
    if (statusChanged) {
        m_statusRecomputationFlags += StatusRecomputation::Status;
        emit dirStatusChanged(*dirInfo, index);
    }
}

/*!
 * \brief Reads results of requestEvents().
 */
void SyncthingConnection::readDownloadProgressEvent(const QJsonObject &eventData)
{
    for (auto &dirInfo : m_dirs) {
        // disappearing implies that the download has been finished so just wipe old entries
        dirInfo.downloadingItems.clear();
        dirInfo.blocksAlreadyDownloaded = dirInfo.blocksToBeDownloaded = 0;

        // read progress of currently downloading items
        const auto dirObj = eventData.value(dirInfo.id).toObject();
        if (!dirObj.isEmpty()) {
            dirInfo.downloadingItems.reserve(static_cast<size_t>(dirObj.size()));
            for (auto filePair = dirObj.constBegin(), end = dirObj.constEnd(); filePair != end; ++filePair) {
                const auto &itemProgress = dirInfo.downloadingItems.emplace_back(dirInfo.path, filePair.key(), filePair.value().toObject());
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
void SyncthingConnection::readDirEvent(SyncthingEventId eventId, DateTime eventTime, const QString &eventType, const QJsonObject &eventData)
{
    // read dir ID
    const auto dirId = [&eventData] {
        const auto folder = eventData.value(QLatin1String("folder")).toString();
        if (!folder.isEmpty()) {
            return folder;
        }
        return eventData.value(QLatin1String("id")).toString();
    }();
    if (dirId.isEmpty()) {
        // handle events which don't necessarily require a corresponding dir info
        if (eventType == QLatin1String("FolderCompletion")) {
            readFolderCompletion(eventId, eventTime, eventData, dirId, nullptr, -1);
        }
        return;
    }

    // handle "FolderRejected"-event which is a bit special because here the dir ID is supposed to be unknown
    if (eventType == QLatin1String("FolderRejected")) {
        readDirRejected(eventTime, dirId, eventData);
        return;
    }

    // find related dir info for other events (which are about well-known dirs)
    auto index = int();
    auto *const dirInfo = findDirInfo(dirId, index);
    if (!dirInfo) {
        return;
    }

    // distinguish specific events, keep track of status changes
    const auto previousStatus = dirInfo->status;
    const auto wasOutOfSync = dirInfo->isOutOfSync();
    if (eventType == QLatin1String("FolderErrors")) {
        readFolderErrors(eventId, eventTime, eventData, *dirInfo, index);
    } else if (eventType == QLatin1String("FolderSummary")) {
        readDirSummary(eventId, eventTime, eventData.value(QLatin1String("summary")).toObject(), *dirInfo, index);
    } else if (eventType == QLatin1String("FolderCompletion")) {
        readFolderCompletion(eventId, eventTime, eventData, dirId, dirInfo, index);
    } else if (eventType == QLatin1String("FolderScanProgress")) {
        const auto current = eventData.value(QLatin1String("current")).toDouble(0);
        const auto total = eventData.value(QLatin1String("total")).toDouble(0);
        const auto rate = eventData.value(QLatin1String("rate")).toDouble(0);
        if (current > 0 && total > 0) {
            dirInfo->scanningPercentage = static_cast<int>(current * 100 / total);
            dirInfo->scanningRate = rate;
            dirInfo->assignStatus(SyncthingDirStatus::Scanning, eventId, eventTime); // ensure state is scanning
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
    if (previousStatus != dirInfo->status) {
        m_statusRecomputationFlags += StatusRecomputation::Status;
    }
    if (wasOutOfSync != dirInfo->isOutOfSync()) {
        m_hasOutOfSyncDirs.reset();
    }
}

/*!
 * \brief Reads results of requestEvents().
 */
void SyncthingConnection::readDeviceEvent(SyncthingEventId eventId, DateTime eventTime, const QString &eventType, const QJsonObject &eventData)
{
    // ignore device events happened before the last connections update
    if (eventId < m_lastConnectionsUpdateEvent) {
        return;
    }
    const auto devId = [&eventData] {
        const auto dev = eventData.value(QLatin1String("device")).toString();
        if (!dev.isEmpty()) {
            return dev;
        }
        return eventData.value(QLatin1String("id")).toString();
    }();
    if (devId.isEmpty()) {
        return;
    }

    // handle "DeviceRejected"-event
    if (eventType == QLatin1String("DeviceRejected")) {
        readDevRejected(eventTime, devId, eventData);
        return;
    }

    // find relevant device info
    auto index = int();
    auto *const devInfo = findDevInfo(devId, index);
    if (!devInfo) {
        return;
    }

    // distinguish specific events
    auto status = devInfo->status;
    auto paused = devInfo->paused;
    auto disconnectReason = devInfo->disconnectReason;
    if (eventType == QLatin1String("DeviceConnected")) {
        status = devInfo->computeConnectedStateAccordingToCompletion();
        disconnectReason.clear();
    } else if (eventType == QLatin1String("DeviceDisconnected")) {
        status = SyncthingDevStatus::Disconnected;
        disconnectReason = eventData.value(QLatin1String("error")).toString();
    } else if (eventType == QLatin1String("DevicePaused")) {
        paused = true;
    } else if (eventType == QLatin1String("DeviceResumed")) {
        paused = false;
    } else {
        return;
    }

    // assign new status
    if (devInfo->status != status || devInfo->paused != paused || devInfo->disconnectReason != disconnectReason) {
        // don't mess with the status of the own device
        if (devInfo->status != SyncthingDevStatus::ThisDevice) {
            devInfo->status = status;
            devInfo->paused = paused;
            devInfo->disconnectReason = disconnectReason;
        }
        m_statusRecomputationFlags += StatusRecomputation::Status | StatusRecomputation::RemoteCompletion;
        emit devStatusChanged(*devInfo, index);
    }
}

/*!
 * \brief Reads results of requestEvents().
 */
void SyncthingConnection::readItemFinished(SyncthingEventId eventId, DateTime eventTime, const QJsonObject &eventData)
{
    auto index = int();
    auto *const dirInfo = findDirInfo(QLatin1String("folder"), eventData, &index);
    if (!dirInfo) {
        return;
    }

    // handle unsuccessful operation
    const auto error = eventData.value(QLatin1String("error")).toString();
    const auto item = eventData.value(QLatin1String("item")).toString();
    if (!error.isEmpty()) {
        // add error item if not already present
        if (dirInfo->status != SyncthingDirStatus::OutOfSync) {
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
        emit newNotification(eventTime, error);
        return;
    }

    // update last file
    if (dirInfo->lastFileTime.isNull() || eventId >= dirInfo->lastFileEvent) {
        dirInfo->lastFileEvent = eventId;
        dirInfo->lastFileTime = eventTime;
        dirInfo->lastFileName = item;
        dirInfo->lastFileDeleted = (eventData.value(QLatin1String("action")) != QLatin1String("delete"));
        if (eventId >= m_lastFileEvent) {
            m_lastFileEvent = eventId;
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
void SyncthingConnection::readFolderErrors(
    SyncthingEventId eventId, DateTime eventTime, const QJsonObject &eventData, SyncthingDir &dirInfo, int index)
{
    // ignore errors occurred before the last time the directory was in "sync" state (Syncthing re-emits recurring errors)
    if (dirInfo.lastSyncStartedEvent > eventId) {
        return;
    }

    // clear previous errors (considering syncthing/lib/model/rwfolder.go it seems that also the event API always returns a
    // full list of events and not only new ones)
    dirInfo.itemErrors.clear();

    // add errors
    const auto errors = eventData.value(QLatin1String("errors")).toArray();
    for (const auto &errorVal : errors) {
        const auto error = errorVal.toObject();
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
        dirInfo.assignStatus(SyncthingDirStatus::OutOfSync, eventId, eventTime);
    }

    emit dirStatusChanged(dirInfo, index);
}

/*!
 * \brief Reads results of requestEvents().
 */
void SyncthingConnection::readFolderCompletion(
    SyncthingEventId eventId, DateTime eventTime, const QJsonObject &eventData, const QString &dirId, SyncthingDir *dirInfo, int dirIndex)
{
    const auto devId = eventData.value(QLatin1String("device")).toString();
    auto devIndex = int();
    auto *const devInfo = findDevInfo(devId, devIndex);
    readFolderCompletion(eventId, eventTime, eventData, devId, devInfo, devIndex, dirId, dirInfo, dirIndex);
}

/*!
 * \brief Reads results of requestEvents().
 */
void SyncthingConnection::readFolderCompletion(SyncthingEventId eventId, DateTime eventTime, const QJsonObject &eventData, const QString &devId,
    SyncthingDev *devInfo, int devIndex, const QString &dirId, SyncthingDir *dirInfo, int dirIndex)
{
    if (devInfo && !devId.isEmpty() && devId != myId()) {
        readRemoteFolderCompletion(eventTime, eventData, devId, devInfo, devIndex, dirId, dirInfo, dirIndex);
    } else if (dirInfo) {
        readLocalFolderCompletion(eventId, eventTime, eventData, *dirInfo, dirIndex);
    }
}

/*!
 * \brief Reads results of requestEvents().
 */
void SyncthingConnection::readLocalFolderCompletion(
    SyncthingEventId eventId, DateTime eventTime, const QJsonObject &eventData, SyncthingDir &dirInfo, int index)
{
    auto &neededStats(dirInfo.neededStats);
    auto &globalStats(dirInfo.globalStats);
    // backup previous statistics -> if there's no difference after all, don't emit completed event
    const auto previouslyUpdated = !dirInfo.lastStatisticsUpdateTime.isNull();
    const auto previouslyNeeded = !neededStats.isNull();
    const auto previouslyGlobal = globalStats;
    // read values from event data
    globalStats.bytes = jsonValueToInt(eventData.value(QLatin1String("globalBytes")), static_cast<double>(globalStats.bytes));
    neededStats.bytes = jsonValueToInt(eventData.value(QLatin1String("needBytes")), static_cast<double>(neededStats.bytes));
    neededStats.deletes = jsonValueToInt(eventData.value(QLatin1String("needDeletes")), static_cast<double>(neededStats.deletes));
    neededStats.total = jsonValueToInt(eventData.value(QLatin1String("needItems")), static_cast<double>(neededStats.files));
    dirInfo.lastStatisticsUpdateEvent = eventId;
    dirInfo.lastStatisticsUpdateTime = eventTime;
    dirInfo.completionPercentage = globalStats.bytes ? static_cast<int>((globalStats.bytes - neededStats.bytes) * 100 / globalStats.bytes) : 100;
    emit dirStatusChanged(dirInfo, index);
    if (neededStats.isNull() && previouslyUpdated && (previouslyNeeded || globalStats != previouslyGlobal)
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
    auto &needed = completion.needed;
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
            emit dirCompleted(completion.lastUpdate, *dirInfo, dirIndex, devInfo);
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
        m_statusRecomputationFlags += StatusRecomputation::RemoteCompletion;
    }
}

/*!
 * \brief Reads results of requestEvents().
 */
void SyncthingConnection::readRemoteIndexUpdated(SyncthingEventId eventId, const QJsonObject &eventData)
{
    // ignore those events if we're not updating completion automatically
    if (!m_requestCompletion && !m_keepPolling) {
        return;
    }

    // find dev/dir
    const auto devId = eventData.value(QLatin1String("device")).toString();
    const auto dirId = eventData.value(QLatin1String("folder")).toString();
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
    if ((completionFromDirInfo && completionFromDirInfo->requestedForEventId >= eventId)
        || (completionFromDevInfo && completionFromDevInfo->requestedForEventId >= eventId)) {
        return;
    }
    if (completionFromDirInfo) {
        completionFromDirInfo->requestedForEventId = eventId;
    }
    if (completionFromDevInfo) {
        completionFromDevInfo->requestedForEventId = eventId;
    }
    if (devInfo && dirInfo && !devInfo->paused && !dirInfo->paused) {
        requestCompletion(devId, dirId);
    }
}

/*!
 * \brief Requests the Syncthing disk events (since the last successful call) asynchronously.
 * \remarks
 * - This is a long-polling API just like requestEvents(). Note that the official UI actually calls this API (as well as
 *   requestDirStatistics()) regularly on `LocalIndexUpdated` events and does not always keep a request open (which would
 *   be an alternative to consider here as well).
 * - This is handled separately from the main events so \a limit can be applied to disk events specifically.
 * - The default limit of 25 is in accordance with the official UI.
 */
void SyncthingConnection::requestDiskEvents(int limit)
{
    if (m_diskEventsReply || !(m_pollingFlags && PollingFlags::DiskEvents)) {
        return;
    }
    auto query = QUrlQuery();
    query.addQueryItem(QStringLiteral("limit"), QString::number(limit));
    query.addQueryItem(QStringLiteral("events"), QStringLiteral("LocalChangeDetected,RemoteChangeDetected"));
    if (m_lastDiskEventId && m_hasDiskEvents) {
        query.addQueryItem(QStringLiteral("since"), QString::number(m_lastDiskEventId));
    }
    // force to return immediately after the first call
    if (!m_hasDiskEvents) {
        query.addQueryItem(QStringLiteral("timeout"), QStringLiteral("0"));
    } else if (m_longPollingTimeout) {
        query.addQueryItem(QStringLiteral("timeout"), QString::number(m_longPollingTimeout / 1000));
    }
    QObject::connect(m_diskEventsReply = requestData(QStringLiteral("events/disk"), query, true, m_hasDiskEvents), &QNetworkReply::finished, this,
        &SyncthingConnection::readDiskEvents);
}

/*!
 * \brief Reads data from requestDiskEvents().
 */
void SyncthingConnection::readDiskEvents()
{
    auto const expectedReply = m_diskEventsReply;
    auto const [reply, response] = prepareReply(m_diskEventsReply);
    if (!reply) {
        return;
    }

    switch (reply->error()) {
    case QNetworkReply::NoError: {
        auto jsonError = QJsonParseError();
        const auto replyDoc = QJsonDocument::fromJson(response, &jsonError);
        if (jsonError.error != QJsonParseError::NoError) {
            emitError(tr("Unable to parse disk events: "), jsonError, reply, response);
            return;
        }

        m_hasDiskEvents = true;
        const auto replyArray = replyDoc.array();
        if (!readEventsFromJsonArray(replyArray, m_lastDiskEventId)) {
            return;
        }

        if (!replyArray.isEmpty() && (loggingFlags() && SyncthingConnectionLoggingFlags::Events)) {
            const auto log = replyDoc.toJson(QJsonDocument::Indented);
            cerr << Phrases::Info << "Received " << replyArray.size() << " Syncthing disk events:" << Phrases::End << log.data() << endl;
        }
        break;
    }
    case QNetworkReply::TimeoutError:
        // no new events available, keep polling
        break;
    case QNetworkReply::OperationCanceledError:
        if (reply == expectedReply) {
            handleAdditionalRequestCanceled();
        }
        return;
    default:
        emitError(tr("Unable to request disk events: "), SyncthingErrorCategory::OverallConnection, reply);
        handleFatalConnectionError();
        return;
    }

    if (m_keepPolling) {
        requestDiskEvents();
        concludeConnection(StatusRecomputation::None);
    }
}

} // namespace Data
