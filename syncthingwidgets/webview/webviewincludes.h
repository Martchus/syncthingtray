// Created via CMake from template webviewincludes.h.in
// WARNING! Any changes to this file will be overwritten by the next CMake run!

#ifndef SYNCTHINGWIDGETS_WEB_VIEW_INCLUDES
#define SYNCTHINGWIDGETS_WEB_VIEW_INCLUDES

#include <QtGlobal>

#if defined(SYNCTHINGWIDGETS_USE_WEBENGINE)
# include <QWebEngineView>
# include <QWebEnginePage>
# include <QtWebEngineWidgetsVersion>
#elif defined(SYNCTHINGWIDGETS_USE_WEBKIT)
# include <QWebView>
# include <QWebPage>
# include <QWebFrame>
#elif !defined(SYNCTHINGWIDGETS_NO_WEBVIEW)
# error "No definition for web view provider present."
#endif

#endif // SYNCTHINGWIDGETS_WEB_VIEW_INCLUDES
