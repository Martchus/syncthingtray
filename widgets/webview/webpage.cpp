#ifndef SYNCTHINGWIDGETS_NO_WEBVIEW
#include "./webpage.h"
#include "./webviewdialog.h"

#include "../settings/settings.h"

#include "../../connector/syncthingconnection.h"

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
    connect(this, &SYNCTHINGWIDGETS_WEB_PAGE::loadFinished, this, &WebPage::injectJavaScripts);

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
 * \brief Accepts self-signed certificate.
 * \todo Only ignore the error if the used certificate matches the certificate known to be used by the Syncthing GUI.
 */
bool WebPage::certificateError(const QWebEngineCertificateError &certificateError)
{
    switch (certificateError.error()) {
    case QWebEngineCertificateError::CertificateCommonNameInvalid:
    case QWebEngineCertificateError::CertificateAuthorityInvalid:
        return true;
    default:
        return false;
    }
}

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
    injectJavaScript(QStringLiteral("jQuery('#folderPath').dblclick(function(event) {"
                                    "    if (event.target && !event.target.getAttribute('readonly')) {"
                                    "        console.log('nativeInterface.showFolderPathSelection: ' + event.target.value);"
                                    "    }"
                                    "});"));
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
    }
}

/*!
 * \brief Shows the folder path selection and sets the selected path in the page.
 */
void WebPage::showFolderPathSelection(const QString &defaultDir)
{
    const QString dir(QFileDialog::getExistingDirectory(m_view, tr("Select path for Syncthing directory ..."), defaultDir));
    if (!dir.isEmpty()) {
        injectJavaScript(QStringLiteral("document.getElementById('folderPath').value = '") % dir % QStringLiteral("';"));
    }
}
} // namespace QtGui

#endif // SYNCTHINGWIDGETS_NO_WEBVIEW
