#ifndef SYNCTHINGWIDGETS_NO_WEBVIEW
#include "./webviewdialog.h"
#include "./webpage.h"

#include "../settings/settings.h"

#include <qtutilities/misc/dialogutils.h>

#include <QIcon>
#include <QCloseEvent>
#include <QKeyEvent>

using namespace Dialogs;

namespace QtGui {

WebViewDialog::WebViewDialog(QWidget *parent) :
    QMainWindow(parent),
    m_view(new SYNCTHINGWIDGETS_WEB_VIEW(this))
{
    setWindowTitle(tr("Syncthing"));
    setWindowIcon(QIcon(QStringLiteral(":/icons/hicolor/scalable/app/syncthingtray.svg")));
    setCentralWidget(m_view);

    m_view->setPage(new WebPage(this, m_view));
    connect(m_view, &SYNCTHINGWIDGETS_WEB_VIEW::titleChanged, this, &WebViewDialog::setWindowTitle);

#if defined(SYNCTHINGWIDGETS_USE_WEBENGINE)
    m_view->installEventFilter(this);
    if(m_view->focusProxy()) {
        m_view->focusProxy()->installEventFilter(this);
    }
#endif

    if(Settings::values().webView.geometry.isEmpty()) {
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

void QtGui::WebViewDialog::applySettings(const Data::SyncthingConnectionSettings &connectionSettings)
{
    m_settings = connectionSettings;
    if(!WebPage::isSamePage(m_view->url(), connectionSettings.syncthingUrl)) {
        m_view->setUrl(connectionSettings.syncthingUrl);
    }
    m_view->setZoomFactor(Settings::values().webView.zoomFactor);
}

#if defined(SYNCTHINGWIDGETS_USE_WEBKIT)
bool WebViewDialog::isModalVisible() const
{
    if(m_view->page()->mainFrame()) {
        return m_view->page()->mainFrame()->evaluateJavaScript(QStringLiteral("$('.modal-dialog').is(':visible')")).toBool();
    }
    return false;
}
#endif

void WebViewDialog::closeUnlessModalVisible()
{
#if defined(SYNCTHINGWIDGETS_USE_WEBKIT)
    if(!isModalVisible()) {
        close();
    }
#elif defined(SYNCTHINGWIDGETS_USE_WEBENGINE)
    m_view->page()->runJavaScript(QStringLiteral("$('.modal-dialog').is(':visible')"), [this] (const QVariant &modalVisible) {
        if(!modalVisible.toBool()) {
            close();
        }
    });
#else
    close();
#endif
}

void QtGui::WebViewDialog::closeEvent(QCloseEvent *event)
{
    if(!Settings::values().webView.keepRunning) {
        deleteLater();
    }
    event->accept();
}

void WebViewDialog::keyPressEvent(QKeyEvent *event)
{
    switch(event->key()) {
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
    switch(event->type()) {
    case QEvent::ChildAdded:
        if(m_view->focusProxy()) {
            m_view->focusProxy()->installEventFilter(this);
        }
        break;
    case QEvent::KeyPress:
        keyPressEvent(static_cast<QKeyEvent *>(event));
        break;
    default:
        ;
    }
    return QMainWindow::eventFilter(watched, event);
}
#endif

}

#endif // SYNCTHINGWIDGETS_NO_WEBVIEW
