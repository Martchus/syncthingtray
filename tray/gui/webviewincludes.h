// Created via CMake from template webviewincludes.h.in
// WARNING! Any changes to this file will be overwritten by the next CMake run!

#ifndef SYNCTHINGTRAY_WEB_VIEW_INCLUDES
#define SYNCTHINGTRAY_WEB_VIEW_INCLUDES

#include <QtGlobal>

#if defined(SYNCTHINGTRAY_USE_WEBENGINE)
# include <QWebEngineView>
# include <QWebEnginePage>
#elif defined(SYNCTHINGTRAY_USE_WEBKIT)
# include <QWebView>
# include <QWebPage>
# include <QWebFrame>
#elif !defined(SYNCTHINGTRAY_NO_WEBVIEW)
# error "No definition for web view provider present."
#endif

#endif // SYNCTHINGTRAY_WEB_VIEW_INCLUDES
