#ifndef SYNCTHINGWIDGETS_NO_WEBVIEW
#include "./webpage.h"
#include "./webviewdialog.h"

#include "../settings/settings.h"

#include <syncthingconnector/syncthingconnection.h>

#include "resources/config.h"

#include <QAuthenticator>
#include <QDesktopServices>
#include <QFileDialog>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QStringBuilder>
#if defined(SYNCTHINGWIDGETS_USE_WEBENGINE)
#include <QWebEngineCertificateError>
#include <QWebEngineSettings>
#include <QWebEngineView>
#elif defined(SYNCTHINGWIDGETS_USE_WEBKIT)
#include <QNetworkRequest>
#include <QSslError>
#include <QWebFrame>
#include <QWebSettings>
#include <QWebView>
#endif

#ifdef SYNCTHINGWIDGETS_LOG_JAVASCRIPT_CONSOLE
#include <iostream>
#endif

using namespace Data;

namespace QtGui {

#ifdef SYNCTHINGWIDGETS_USE_WEBENGINE
WebPage::WebPage(QWebEngineProfile *profile, WebViewDialog *dlg, SYNCTHINGWIDGETS_WEB_VIEW *view)
    : SYNCTHINGWIDGETS_WEB_PAGE(profile, view)
#else
WebPage::WebPage(WebViewDialog *dlg, SYNCTHINGWIDGETS_WEB_VIEW *view)
    : SYNCTHINGWIDGETS_WEB_PAGE(view)
#endif
    , m_dlg(dlg)
    , m_view(view)
{
#ifdef SYNCTHINGWIDGETS_USE_WEBENGINE
    settings()->setAttribute(QWebEngineSettings::JavascriptCanOpenWindows, true);
#if (QTWEBENGINEWIDGETS_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    connect(this, &WebPage::certificateError, this, &WebPage::handleCertificateError);
#endif
    connect(
        this, &WebPage::authenticationRequired, this, static_cast<void (WebPage::*)(const QUrl &, QAuthenticator *)>(&WebPage::supplyCredentials));
#else // SYNCTHINGWIDGETS_USE_WEBKIT
    settings()->setAttribute(QWebSettings::JavascriptCanOpenWindows, true);
    setNetworkAccessManager(&Data::networkAccessManager());
    connect(&Data::networkAccessManager(), &QNetworkAccessManager::authenticationRequired, this,
        static_cast<void (WebPage::*)(QNetworkReply *, QAuthenticator *)>(&WebPage::supplyCredentials));
    connect(&Data::networkAccessManager(), &QNetworkAccessManager::sslErrors, this,
        static_cast<void (WebPage::*)(QNetworkReply *, const QList<QSslError> &errors)>(&WebPage::handleSslErrors));
#endif

    if (!m_view) {
        // initialization for new window
        // -> delegate to external browser if no view is assigned
#ifdef SYNCTHINGWIDGETS_USE_WEBENGINE
        connect(this, &WebPage::urlChanged, this, &WebPage::delegateNewWindowToExternalBrowser);
#else
        connect(this->mainFrame(), &QWebFrame::urlChanged, this, &WebPage::delegateNewWindowToExternalBrowser);
#endif
        // -> there need to be a view, though
        m_view = new SYNCTHINGWIDGETS_WEB_VIEW;
        m_view->setPage(this);
        setParent(m_view);
    }
}

bool WebPage::isSamePage(const QUrl &url1, const QUrl &url2)
{
    if (url1.scheme() == url2.scheme() && url1.host() == url2.host() && url1.port() == url2.port()) {
        QString path1 = url1.path();
        while (path1.endsWith(QChar('/'))) {
            path1.resize(path1.size() - 1);
        }
        QString path2 = url2.path();
        while (path2.endsWith(QChar('/'))) {
            path2.resize(path2.size() - 1);
        }
        if (path1 == path2) {
            return true;
        }
    }
    return false;
}

/*!
 * \brief Delegates creation of new windows to a new instance of WebPage which will show it in an external browser.
 */
SYNCTHINGWIDGETS_WEB_PAGE *WebPage::createWindow(SYNCTHINGWIDGETS_WEB_PAGE::WebWindowType type)
{
    Q_UNUSED(type)
    return new WebPage;
}

#ifdef SYNCTHINGWIDGETS_USE_WEBENGINE
/*!
 * \brief Returns whether \a certificateError can be ignored.
 * \remarks Before Qt 5.14 any self-signed certificates are accepted.
 */
bool WebPage::canIgnoreCertificateError(const QWebEngineCertificateError &certificateError) const
{
    // never ignore errors other than CertificateCommonNameInvalid and CertificateAuthorityInvalid
    switch (certificateError
#if (QTWEBENGINEWIDGETS_VERSION >= QT_VERSION_CHECK(6, 0, 0))
                .type()
#else
                .error()
#endif
    ) {
    case QWebEngineCertificateError::CertificateCommonNameInvalid:
    case QWebEngineCertificateError::CertificateAuthorityInvalid:
        break;
    default:
        return false;
    }

    // don't ignore the error if there are no expected self-signed SSL certificates configured
    if (!m_dlg || m_dlg->connectionSettings().expectedSslErrors.isEmpty()) {
        return false;
    }

    // ignore only certificate errors matching the expected URL of the Syncthing instance
    const auto urlWithError = certificateError.url();
    const auto expectedUrl = m_view->url();
    if (urlWithError.scheme() != expectedUrl.scheme() || urlWithError.host() != expectedUrl.host()) {
        return false;
    }

#if (QTWEBENGINEWIDGETS_VERSION >= QT_VERSION_CHECK(5, 14, 0))
    // don't ignore the error if no certificate is provided at all (possible?)
    const auto certificateChain = certificateError.certificateChain();
    if (certificateChain.isEmpty()) {
        return false;
    }

    // don't ignore the error if the first certificate in the chain (the peer's immediate certificate) does
    // not match the expected SSL certificate
    // note: All the SSL errors in the settings refer to the same certificate so it is sufficient to just pick
    //      the first one.
    if (certificateChain.first() != m_dlg->connectionSettings().expectedSslErrors.first().certificate()) {
        return false;
    }
#endif

    // accept the self-signed certificate
    return true;
}

/*!
 * \brief Accepts self-signed certificates used by the Syncthing GUI as configured.
 * \remarks
 * Judging by https://github.com/qt/qtwebengine/blob/dev/examples/webenginewidgets/simplebrowser/webpage.cpp
 * the QWebEngineCertificateError is really supposed to be a copy. One would assume that
 * modifying only the copy has no effect but it actually works, e.g. when removing the
 * call `certificateError.acceptCertificate()` one only gets `ERR_CERT_AUTHORITY_INVALID`
 * when trying to access e.g. `https://127.0.0.1:8080` but with the call it works.
 */
#if (QTWEBENGINEWIDGETS_VERSION >= QT_VERSION_CHECK(6, 0, 0))
void WebPage::handleCertificateError(QWebEngineCertificateError certificateError)
{
    if (canIgnoreCertificateError(certificateError)) {
        certificateError.acceptCertificate();
    } else {
        certificateError.rejectCertificate();
    }
}
#else
bool WebPage::certificateError(const QWebEngineCertificateError &certificateError)
{
    return canIgnoreCertificateError(certificateError);
}
#endif

/*!
 * \brief Accepts navigation requests only on the same page.
 */
bool WebPage::acceptNavigationRequest(const QUrl &url, SYNCTHINGWIDGETS_WEB_PAGE::NavigationType type, bool isMainFrame)
{
    Q_UNUSED(isMainFrame)
    Q_UNUSED(type)
    return handleNavigationRequest(this->url(), url);
}

/*!
 * \brief Invokes processing JavaScript messages printed via "console.log()".
 */
void WebPage::javaScriptConsoleMessage(
    QWebEnginePage::JavaScriptConsoleMessageLevel level, const QString &message, int lineNumber, const QString &sourceID)
{
    Q_UNUSED(level)
    Q_UNUSED(lineNumber)
    Q_UNUSED(sourceID)
    if (level == QWebEnginePage::InfoMessageLevel) {
        processJavaScriptConsoleMessage(message);
    }
}

#else // SYNCTHINGWIDGETS_USE_WEBKIT
/*!
 * \brief Accepts navigation requests only on the same page.
 */
bool WebPage::acceptNavigationRequest(QWebFrame *frame, const QNetworkRequest &request, SYNCTHINGWIDGETS_WEB_PAGE::NavigationType type)
{
    Q_UNUSED(frame)
    Q_UNUSED(type)
    return handleNavigationRequest(mainFrame() ? mainFrame()->url() : QUrl(), request.url());
}

/*!
 * \brief Invokes processing JavaScript messages printed via "console.log()".
 */
void WebPage::javaScriptConsoleMessage(const QString &message, int lineNumber, const QString &sourceID)
{
    Q_UNUSED(lineNumber)
    Q_UNUSED(sourceID)
    processJavaScriptConsoleMessage(message);
}
#endif

/*!
 * \brief Shows the specified \a url in the default browser and deletes the page and associated view.
 */
void WebPage::delegateNewWindowToExternalBrowser(const QUrl &url)
{
    QDesktopServices::openUrl(url);
    // this page and the associated view are useless
    m_view->deleteLater();
}

/*!
 * \brief Supplies credentials from the dialog's settings.
 */
void WebPage::supplyCredentials(const QUrl &requestUrl, QAuthenticator *authenticator)
{
    Q_UNUSED(requestUrl)
    supplyCredentials(authenticator);
}

/*!
 * \brief Supplies credentials from the dialog's settings.
 */
void WebPage::supplyCredentials(QNetworkReply *reply, QAuthenticator *authenticator)
{
    Q_UNUSED(reply)
    supplyCredentials(authenticator);
}

/*!
 * \brief Supplies credentials from the dialog's settings.
 */
void WebPage::supplyCredentials(QAuthenticator *authenticator)
{
    if (m_dlg && m_dlg->connectionSettings().authEnabled) {
        authenticator->setUser(m_dlg->connectionSettings().userName);
        authenticator->setPassword(m_dlg->connectionSettings().password);
    }
}

/*!
 * \brief Allows initial request and navigation on the same page but opens everything else in an external browser.
 */
bool WebPage::handleNavigationRequest(const QUrl &currentUrl, const QUrl &targetUrl)
{
    if (currentUrl.isEmpty()) {
        // allow initial request
        return true;
    }
    // only allow navigation on the same page
    if (isSamePage(currentUrl, targetUrl)) {
        return true;
    }
    // otherwise open URL in external browser
    QDesktopServices::openUrl(targetUrl);
    return false;
}

#ifdef SYNCTHINGWIDGETS_USE_WEBKIT
/*!
 * \brief Ignores SSL errors that are known exceptions to support self-signed certificates.
 */
void WebPage::handleSslErrors(QNetworkReply *reply, const QList<QSslError> &errors)
{
    Q_UNUSED(errors)
    if (m_dlg && reply->request().url().host() == m_view->url().host()) {
        reply->ignoreSslErrors(m_dlg->connectionSettings().expectedSslErrors);
    }
}
#endif

/*!
 * \brief Injects the specified JavaScript.
 */
void WebPage::injectJavaScript(const QString &scriptSource)
{
#ifdef SYNCTHINGWIDGETS_USE_WEBENGINE
    runJavaScript(scriptSource);
#else // SYNCTHINGWIDGETS_USE_WEBKIT
    mainFrame()->evaluateJavaScript(scriptSource);
#endif
}

/*!
 * \brief Injects the JavaScript code required for additional features.
 * \remarks Called when the page has been loaded.
 */
void WebPage::injectJavaScripts(bool ok)
{
    Q_UNUSED(ok)
    // show folder path selection when double-clicking input
    injectJavaScript(QStringLiteral("var button = jQuery('<button type=\"button\" class=\"btn btn-sm\" "
                                    "style=\"float: right;\">Select directory …</button>');"
                                    "button.click(function(event) {"
                                    "    if (!document.getElementById('folderPath').getAttribute('readonly')) {"
                                    "        console.log('nativeInterface.showFolderPathSelection: ' + event.target.value);"
                                    "    }"
                                    "});"
                                    "var help = jQuery('#folderPath + * + .help-block');"
                                    "help.prepend(button);"));
}

/*!
 * \brief Invokes native methods requested via JavaScript log.
 */
void WebPage::processJavaScriptConsoleMessage(const QString &message)
{
#ifdef SYNCTHINGWIDGETS_LOG_JAVASCRIPT_CONSOLE
    std::cerr << "JS console: " << message.toLocal8Bit().data() << std::endl;
#endif
    if (message.startsWith(QLatin1String("nativeInterface.showFolderPathSelection: "))) {
        showFolderPathSelection(message.mid(41));
    } else if (message == QLatin1String("UIOnline")) {
        injectJavaScripts(true);
    }
}

/*!
 * \brief Shows the folder path selection and sets the selected path in the page.
 */
void WebPage::showFolderPathSelection(const QString &defaultDir)
{
    const QString dir(QFileDialog::getExistingDirectory(m_view, tr("Select path for Syncthing directory …"), defaultDir));
    if (!dir.isEmpty()) {
        injectJavaScript(QStringLiteral("document.getElementById('folderPath').value = '") % dir % QStringLiteral("';"));
    }
}
} // namespace QtGui

#endif // SYNCTHINGWIDGETS_NO_WEBVIEW
