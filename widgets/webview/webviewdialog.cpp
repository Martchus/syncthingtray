#ifndef SYNCTHINGWIDGETS_NO_WEBVIEW
#include "./webviewdialog.h"
#include "./webpage.h"
#include "./webviewinterceptor.h"

#include "../settings/settings.h"

#include <qtutilities/misc/dialogutils.h>

#include <QCloseEvent>
#include <QIcon>
#include <QKeyEvent>
#if defined(SYNCTHINGWIDGETS_USE_WEBENGINE)
#include <QWebEngineProfile>
#include <QtWebEngineWidgetsVersion>
#endif

using namespace QtUtilities;

namespace QtGui {

WebViewDialog::WebViewDialog(QWidget *parent)
    : QMainWindow(parent)
    , m_view(new SYNCTHINGWIDGETS_WEB_VIEW(this))
{
    setWindowTitle(tr("Syncthing"));
    setWindowIcon(QIcon(QStringLiteral(":/icons/hicolor/scalable/app/syncthingtray.svg")));
    setCentralWidget(m_view);

#if defined(SYNCTHINGWIDGETS_USE_WEBENGINE)
    m_profile = new QWebEngineProfile(objectName(), this);
#if (QTWEBENGINEWIDGETS_VERSION >= QT_VERSION_CHECK(5, 13, 0))
    m_profile->setUrlRequestInterceptor(new WebViewInterceptor(m_connectionSettings, m_profile));
#else
    m_profile->setRequestInterceptor(new WebViewInterceptor(m_connectionSettings, m_profile));
#endif
    m_view->setPage(new WebPage(m_profile, this, m_view));
#else
    m_view->setPage(new WebPage(this, m_view));
#endif
    connect(m_view, &SYNCTHINGWIDGETS_WEB_VIEW::titleChanged, this, &WebViewDialog::setWindowTitle);

#if defined(SYNCTHINGWIDGETS_USE_WEBENGINE)
    m_view->installEventFilter(this);
    if (m_view->focusProxy()) {
        m_view->focusProxy()->installEventFilter(this);
    }
#endif

    if (Settings::values().webView.geometry.isEmpty()) {
        resize(1200, 800);
        centerWidget(this);
    } else {
        restoreGeometry(Settings::values().webView.geometry);
    }
}

QtGui::WebViewDialog::~WebViewDialog()
{
    Settings::values().webView.geometry = saveGeometry();
}

void QtGui::WebViewDialog::applySettings(const Data::SyncthingConnectionSettings &connectionSettings, bool aboutToShow)
{
    // delete the web view if currently hidden and the configuration to keep it running in the background isn't enabled
    const auto &settings(Settings::values());
    if (!aboutToShow && !settings.webView.keepRunning && isHidden()) {
        deleteLater();
        return;
    }

    // apply settings to the view
    m_connectionSettings = connectionSettings;
    if (!WebPage::isSamePage(m_view->url(), connectionSettings.syncthingUrl)) { // prevent reload if the URL remains the same
        m_view->setUrl(connectionSettings.syncthingUrl);
    }
    m_view->setZoomFactor(settings.webView.zoomFactor);
}

#if defined(SYNCTHINGWIDGETS_USE_WEBKIT)
bool WebViewDialog::isModalVisible() const
{
    if (m_view->page()->mainFrame()) {
        return m_view->page()->mainFrame()->evaluateJavaScript(QStringLiteral("$('.modal-dialog').is(':visible')")).toBool();
    }
    return false;
}
#endif

void WebViewDialog::closeUnlessModalVisible()
{
#if defined(SYNCTHINGWIDGETS_USE_WEBKIT)
    if (!isModalVisible()) {
        close();
    }
#elif defined(SYNCTHINGWIDGETS_USE_WEBENGINE)
    m_view->page()->runJavaScript(QStringLiteral("$('.modal-dialog').is(':visible')"), [this](const QVariant &modalVisible) {
        if (!modalVisible.toBool()) {
            close();
        }
    });
#else
    close();
#endif
}

void QtGui::WebViewDialog::closeEvent(QCloseEvent *event)
{
    if (!Settings::values().webView.keepRunning) {
        deleteLater();
    }
    event->accept();
}

void WebViewDialog::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_F5:
        m_view->reload();
        event->accept();
        break;
    case Qt::Key_Escape:
        closeUnlessModalVisible();
        event->accept();
        break;
    default:
        QMainWindow::keyPressEvent(event);
    }
}

#if defined(SYNCTHINGWIDGETS_USE_WEBENGINE)
bool WebViewDialog::eventFilter(QObject *watched, QEvent *event)
{
    switch (event->type()) {
    case QEvent::ChildAdded:
        if (m_view->focusProxy()) {
            m_view->focusProxy()->installEventFilter(this);
        }
        break;
    case QEvent::KeyPress:
        keyPressEvent(static_cast<QKeyEvent *>(event));
        break;
    default:;
    }
    return QMainWindow::eventFilter(watched, event);
}
#endif

} // namespace QtGui

#endif // SYNCTHINGWIDGETS_NO_WEBVIEW
