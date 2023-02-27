#ifdef SYNCTHINGWIDGETS_USE_WEBENGINE
#include "./webviewinterceptor.h"

#include <syncthingconnector/syncthingconnectionsettings.h>

#include <QWebEngineProfile>
#include <QWebEngineUrlRequestInterceptor>

namespace QtGui {

void WebViewInterceptor::interceptRequest(QWebEngineUrlRequestInfo &info)
{
    info.setHttpHeader(QByteArray("X-API-Key"), m_settings.apiKey);
}

} // namespace QtGui

#endif // SYNCTHINGWIDGETS_USE_WEBENGINE
