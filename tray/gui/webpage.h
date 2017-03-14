#ifndef WEBPAGE_H
#define WEBPAGE_H
#ifndef SYNCTHINGTRAY_NO_WEBVIEW

#include "./webviewdefs.h"
#include "./webviewincludes.h"

QT_FORWARD_DECLARE_CLASS(QAuthenticator)
QT_FORWARD_DECLARE_CLASS(QNetworkReply)
QT_FORWARD_DECLARE_CLASS(QNetworkRequest)
QT_FORWARD_DECLARE_CLASS(QSslError)

namespace QtGui {

class WebViewDialog;

class WebPage : public SYNCTHINGTRAY_WEB_PAGE
{
    Q_OBJECT
public:
    WebPage(WebViewDialog *dlg = nullptr, SYNCTHINGTRAY_WEB_VIEW *view = nullptr);

    static bool isSamePage(const QUrl &url1, const QUrl &url2);

protected:
    SYNCTHINGTRAY_WEB_PAGE *createWindow(WebWindowType type);
#ifdef SYNCTHINGTRAY_USE_WEBENGINE
    bool certificateError(const QWebEngineCertificateError &certificateError);
    bool acceptNavigationRequest(const QUrl &url, NavigationType type, bool isMainFrame);
#else
    bool acceptNavigationRequest(QWebFrame *frame, const QNetworkRequest &request, NavigationType type);
#endif

private slots:
    void delegateNewWindowToExternalBrowser(const QUrl &url);
    void supplyCredentials(const QUrl &requestUrl, QAuthenticator *authenticator);
    void supplyCredentials(QNetworkReply *reply, QAuthenticator *authenticator);
    void supplyCredentials(QAuthenticator *authenticator);
#ifdef SYNCTHINGTRAY_USE_WEBKIT
    void handleSslErrors(QNetworkReply *, const QList<QSslError> &errors);
#endif

private:
    static bool handleNavigationRequest(const QUrl &currentUrl, const QUrl &url);

    WebViewDialog *m_dlg;
    SYNCTHINGTRAY_WEB_VIEW *m_view;
};

}

#endif // SYNCTHINGTRAY_NO_WEBVIEW
#endif // WEBPAGE_H
