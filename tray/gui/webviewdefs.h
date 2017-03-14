// Created via CMake from template webviewdefs.h.in
// WARNING! Any changes to this file will be overwritten by the next CMake run!

#ifndef SYNCTHINGTRAY_WEB_VIEW_DEFINES
#define SYNCTHINGTRAY_WEB_VIEW_DEFINES

#include <QtGlobal>

#if defined(SYNCTHINGTRAY_USE_WEBENGINE)
# define SYNCTHINGTRAY_WEB_VIEW QWebEngineView
# define SYNCTHINGTRAY_WEB_PAGE QWebEnginePage
#elif defined(SYNCTHINGTRAY_USE_WEBKIT)
# define SYNCTHINGTRAY_WEB_VIEW QWebView
# define SYNCTHINGTRAY_WEB_PAGE QWebPage
# define SYNCTHINGTRAY_WEB_FRAME QWebFrame
#elif !defined(SYNCTHINGTRAY_NO_WEBVIEW)
# error "No definition for web view provider present."
#endif

#ifdef SYNCTHINGTRAY_WEB_VIEW
QT_FORWARD_DECLARE_CLASS(SYNCTHINGTRAY_WEB_VIEW)
#endif
#ifdef SYNCTHINGTRAY_WEB_PAGE
QT_FORWARD_DECLARE_CLASS(SYNCTHINGTRAY_WEB_PAGE)
#endif
#ifdef SYNCTHINGTRAY_WEB_FRAME
QT_FORWARD_DECLARE_CLASS(SYNCTHINGTRAY_WEB_FRAME)
#endif

#endif // SYNCTHINGTRAY_WEB_VIEW_DEFINES
