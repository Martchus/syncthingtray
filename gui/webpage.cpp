#if defined(SYNCTHINGTRAY_USE_WEBENGINE) || defined(SYNCTHINGTRAY_USE_WEBKIT)
#include "./webpage.h"

#include "../application/settings.h"
#include "../data/syncthingconnection.h"

#include "resources/config.h"

#include <QDesktopServices>
#include <QAuthenticator>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#if defined(SYNCTHINGTRAY_USE_WEBENGINE)
# include <QWebEngineSettings>
# include <QWebEngineView>
#elif defined(SYNCTHINGTRAY_USE_WEBKIT)
# include <QWebSettings>
# include <QWebView>
# include <QWebFrame>
#endif

namespace QtGui {

WebPage::WebPage(WEB_VIEW_PROVIDER *view) :
    WEB_PAGE_PROVIDER(view),
    m_view(view)
{
#if defined(SYNCTHINGTRAY_USE_WEBENGINE)
    settings()->setAttribute(QWebEngineSettings::JavascriptCanOpenWindows, true);
    connect(this, &WebPage::authenticationRequired, this, static_cast<void(WebPage::*)(const QUrl &, QAuthenticator *)>(&WebPage::supplyCredentials));
#else
    settings()->setAttribute(QWebSettings::JavascriptCanOpenWindows, true);
    setNetworkAccessManager(&Data::networkAccessManager());
    connect(&Data::networkAccessManager(), &QNetworkAccessManager::authenticationRequired, this, static_cast<void(WebPage::*)(QNetworkReply *, QAuthenticator *)>(&WebPage::supplyCredentials));
#endif
    if(!m_view) {
        // delegate to external browser if no view is assigned
#if defined(SYNCTHINGTRAY_USE_WEBENGINE)
        connect(this, &WebPage::urlChanged, this, &WebPage::delegateToExternalBrowser);
#else
        connect(this->mainFrame(), &QWebFrame::urlChanged, this, &WebPage::delegateToExternalBrowser);
#endif
        m_view = new WEB_VIEW_PROVIDER;
        m_view->setPage(this);
    }
}

WEB_PAGE_PROVIDER *WebPage::createWindow(WEB_PAGE_PROVIDER::WebWindowType type)
{
    return new WebPage;
}

void WebPage::delegateToExternalBrowser(const QUrl &url)
{
    openUrlExternal(url);
    // this page and the associated view are useless
    m_view->deleteLater();
    deleteLater();
}

void WebPage::supplyCredentials(const QUrl &requestUrl, QAuthenticator *authenticator)
{
    Q_UNUSED(requestUrl)
    supplyCredentials(authenticator);
}

void WebPage::supplyCredentials(QNetworkReply *reply, QAuthenticator *authenticator)
{
    Q_UNUSED(reply)
    supplyCredentials(authenticator);
}

void WebPage::supplyCredentials(QAuthenticator *authenticator)
{
    if(Settings::authEnabled()) {
        authenticator->setUser(Settings::userName());
        authenticator->setPassword(Settings::password());
    }
}

void WebPage::openUrlExternal(const QUrl &url)
{
    QDesktopServices::openUrl(url);
}

}

#endif // defined(SYNCTHINGTRAY_USE_WEBENGINE) || defined(SYNCTHINGTRAY_USE_WEBKIT)
