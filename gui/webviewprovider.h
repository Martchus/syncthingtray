#ifndef WEB_VIEW_PROVIDER

#if defined(SYNCTHINGTRAY_USE_WEBENGINE)
# define WEB_VIEW_PROVIDER QWebEngineView
# define WEB_PAGE_PROVIDER QWebEnginePage
#elif defined(SYNCTHINGTRAY_USE_WEBKIT)
# define WEB_VIEW_PROVIDER QWebView
# define WEB_PAGE_PROVIDER QWebPage
#endif

#endif
