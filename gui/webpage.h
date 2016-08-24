#ifndef WEBPAGE_H
#define WEBPAGE_H
#if defined(SYNCTHINGTRAY_USE_WEBENGINE) || defined(SYNCTHINGTRAY_USE_WEBKIT)

#include "./webviewprovider.h"

#if defined(SYNCTHINGTRAY_USE_WEBENGINE)
# include <QWebEnginePage>
#elif defined(SYNCTHINGTRAY_USE_WEBKIT)
# include <QWebPage>
#endif

QT_FORWARD_DECLARE_CLASS(WEB_VIEW_PROVIDER)
QT_FORWARD_DECLARE_CLASS(QAuthenticator)
QT_FORWARD_DECLARE_CLASS(QNetworkReply)

namespace QtGui {

class WebPage : public WEB_PAGE_PROVIDER
{
    Q_OBJECT
public:
    WebPage(WEB_VIEW_PROVIDER *view = nullptr);

public slots:
    void openUrlExternal(const QUrl &url);

protected:
    WEB_PAGE_PROVIDER *createWindow(WebWindowType type);

private slots:
    void delegateToExternalBrowser(const QUrl &url);
    void supplyCredentials(const QUrl &requestUrl, QAuthenticator *authenticator);
    void supplyCredentials(QNetworkReply *reply, QAuthenticator *authenticator);
    void supplyCredentials(QAuthenticator *authenticator);

private:
    WEB_VIEW_PROVIDER *m_view;
};

}

#endif // defined(SYNCTHINGTRAY_USE_WEBENGINE) || defined(SYNCTHINGTRAY_USE_WEBKIT)
#endif // WEBPAGE_H
