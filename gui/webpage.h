#ifndef WEBPAGE_H
#define WEBPAGE_H
#ifndef SYNCTHINGTRAY_NO_WEBVIEW

#include "./webviewprovider.h"

#if defined(SYNCTHINGTRAY_USE_WEBENGINE)
# include <QWebEnginePage>
#elif defined(SYNCTHINGTRAY_USE_WEBKIT)
# include <QWebPage>
#endif

QT_FORWARD_DECLARE_CLASS(WEB_VIEW_PROVIDER)
QT_FORWARD_DECLARE_CLASS(QAuthenticator)
QT_FORWARD_DECLARE_CLASS(QNetworkReply)
QT_FORWARD_DECLARE_CLASS(QSslError)

namespace QtGui {

class WebPage : public WEB_PAGE_PROVIDER
{
    Q_OBJECT
public:
    WebPage(WEB_VIEW_PROVIDER *view = nullptr);

protected:
    WEB_PAGE_PROVIDER *createWindow(WebWindowType type);
#ifdef SYNCTHINGTRAY_USE_WEBENGINE
    bool certificateError(const QWebEngineCertificateError &certificateError);
#endif

private slots:
    void delegateToExternalBrowser(const QUrl &url);
    void supplyCredentials(const QUrl &requestUrl, QAuthenticator *authenticator);
    void supplyCredentials(QNetworkReply *reply, QAuthenticator *authenticator);
    void supplyCredentials(QAuthenticator *authenticator);
#ifdef SYNCTHINGTRAY_USE_WEBKIT
    void handleSslErrors(QNetworkReply *, const QList<QSslError> &errors);
#endif

private:
    WEB_VIEW_PROVIDER *m_view;
};

}

#endif // SYNCTHINGTRAY_NO_WEBVIEW
#endif // WEBPAGE_H
