#ifndef WEBPAGE_H
#define WEBPAGE_H
#ifndef SYNCTHINGWIDGETS_NO_WEBVIEW

#include "./webviewdefs.h"
#include "./webviewincludes.h"

#include "../global.h"

#if (QTWEBENGINEWIDGETS_VERSION >= QT_VERSION_CHECK(6, 0, 0))
#include <QWebEngineCertificateError>
#endif

QT_FORWARD_DECLARE_CLASS(QAuthenticator)
QT_FORWARD_DECLARE_CLASS(QNetworkReply)
QT_FORWARD_DECLARE_CLASS(QNetworkRequest)
QT_FORWARD_DECLARE_CLASS(QSslError)

namespace QtGui {

class WebViewDialog;

class SYNCTHINGWIDGETS_EXPORT WebPage : public SYNCTHINGWIDGETS_WEB_PAGE {
    Q_OBJECT
public:
#ifdef SYNCTHINGWIDGETS_USE_WEBENGINE
    WebPage(QWebEngineProfile *profile = nullptr, WebViewDialog *dlg = nullptr, SYNCTHINGWIDGETS_WEB_VIEW *view = nullptr);
#else
    WebPage(WebViewDialog *dlg = nullptr, SYNCTHINGWIDGETS_WEB_VIEW *view = nullptr);
#endif

    static bool isSamePage(const QUrl &url1, const QUrl &url2);

protected:
    SYNCTHINGWIDGETS_WEB_PAGE *createWindow(WebWindowType type) override;
#ifdef SYNCTHINGWIDGETS_USE_WEBENGINE
#if (QTWEBENGINEWIDGETS_VERSION < QT_VERSION_CHECK(6, 0, 0))
    bool certificateError(const QWebEngineCertificateError &certificateError) override; // signal in Qt >= 6
#endif
    bool acceptNavigationRequest(const QUrl &url, NavigationType type, bool isMainFrame) override;
    void javaScriptConsoleMessage(
        QWebEnginePage::JavaScriptConsoleMessageLevel level, const QString &message, int lineNumber, const QString &sourceID) override;
#else // SYNCTHINGWIDGETS_USE_WEBKIT
    bool acceptNavigationRequest(QWebFrame *frame, const QNetworkRequest &request, NavigationType type) override;
    void javaScriptConsoleMessage(const QString &message, int lineNumber, const QString &sourceID) override;
#endif

private Q_SLOTS:
    void delegateNewWindowToExternalBrowser(const QUrl &url);
    void supplyCredentials(const QUrl &requestUrl, QAuthenticator *authenticator);
    void supplyCredentials(QNetworkReply *reply, QAuthenticator *authenticator);
    void supplyCredentials(QAuthenticator *authenticator);
#ifdef SYNCTHINGWIDGETS_USE_WEBENGINE
#if (QTWEBENGINEWIDGETS_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    void handleCertificateError(QWebEngineCertificateError certificateError);
#endif
#else // SYNCTHINGWIDGETS_USE_WEBKIT
    void handleSslErrors(QNetworkReply *, const QList<QSslError> &errors);
#endif
    void injectJavaScripts(bool ok);
    void processJavaScriptConsoleMessage(const QString &message);
    void injectJavaScript(const QString &scriptSource);
    void showFolderPathSelection(const QString &defaultDir);

private:
#ifdef SYNCTHINGWIDGETS_USE_WEBENGINE
    bool canIgnoreCertificateError(const QWebEngineCertificateError &certificateError) const;
#endif
    static bool handleNavigationRequest(const QUrl &currentUrl, const QUrl &url);

    WebViewDialog *m_dlg;
    SYNCTHINGWIDGETS_WEB_VIEW *m_view;
};
} // namespace QtGui

#endif // SYNCTHINGWIDGETS_NO_WEBVIEW
#endif // WEBPAGE_H
