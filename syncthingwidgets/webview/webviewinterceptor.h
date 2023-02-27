#ifndef WEBVIEW_INTERCEPTOR_H
#define WEBVIEW_INTERCEPTOR_H
#ifdef SYNCTHINGWIDGETS_USE_WEBENGINE

#include <QWebEngineUrlRequestInterceptor>

namespace Data {
struct SyncthingConnectionSettings;
}

namespace QtGui {

class WebViewInterceptor : public QWebEngineUrlRequestInterceptor {
    Q_OBJECT

public:
    explicit WebViewInterceptor(const Data::SyncthingConnectionSettings &settings, QObject *parent = nullptr);

    virtual void interceptRequest(QWebEngineUrlRequestInfo &info) override;

private:
    const Data::SyncthingConnectionSettings &m_settings;
};

inline WebViewInterceptor::WebViewInterceptor(const Data::SyncthingConnectionSettings &settings, QObject *parent)
    : QWebEngineUrlRequestInterceptor(parent)
    , m_settings(settings)
{
}

} // namespace QtGui

#endif // SYNCTHINGWIDGETS_USE_WEBENGINE
#endif // WEBVIEW_INTERCEPTOR_H
