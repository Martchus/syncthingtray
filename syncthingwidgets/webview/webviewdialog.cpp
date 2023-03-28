#include "./webviewdialog.h"
#include "../settings/settings.h"

#include "resources/config.h"

#include <syncthingconnector/syncthingprocess.h>

#include <QCoreApplication>
#include <QDesktopServices>
#include <QMessageBox>
#include <QUrl>

#ifdef Q_OS_WINDOWS
#include <QFile>
#include <QSettings> // for reading registry
#endif

#ifndef SYNCTHINGWIDGETS_NO_WEBVIEW
#include "./webpage.h"
#include "./webviewinterceptor.h"

#include <qtutilities/misc/compat.h>
#include <qtutilities/misc/dialogutils.h>

#include <QCloseEvent>
#include <QIcon>
#include <QKeyEvent>
#if defined(SYNCTHINGWIDGETS_USE_WEBENGINE)
#include <QWebEngineProfile>
#include <QtWebEngineWidgetsVersion>
#endif

#include <initializer_list>

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

namespace QtGui {

static QStringList chromiumBasedBrowserBinaries()
{
    static const auto envOverride = qEnvironmentVariable(PROJECT_VARNAME_UPPER "_CHROMIUM_BASED_BROWSER");
    if (!envOverride.isEmpty()) {
        return {envOverride};
    }
    static const auto relevantBinaries = std::initializer_list<QString>{
#ifdef Q_OS_WINDOWS
        QStringLiteral("msedge.exe"), QStringLiteral("chromium.exe"), QStringLiteral("chrome.exe"),
#else
        QStringLiteral("chromium"), QStringLiteral("chrome"), QStringLiteral("msedge"),
#endif
    };
#ifdef Q_OS_WINDOWS
    const auto appPath = QSettings(QStringLiteral("HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths"), QSettings::NativeFormat);
    for (const auto &binaryName : relevantBinaries) {
        const auto binaryPath = appPath.value(binaryName + QStringLiteral("/Default")).toString();
        if (!binaryPath.isEmpty() && QFile::exists(binaryPath)) {
            return {binaryPath};
        }
    }
#endif
    return relevantBinaries;
}

/*!
 * \brief Opens the specified \a url as "app" in a Chromium-based web browser.
 * \todo Check for other Chromium-based browsers and use the Windows registry to find apps under Windows.
 */
static void openBrowserInAppMode(const QString &url)
{
    const auto appList = chromiumBasedBrowserBinaries();
    auto *const process = new Data::SyncthingProcess();
    QObject::connect(process, &Data::SyncthingProcess::finished, process, &QObject::deleteLater);
    QObject::connect(process, &Data::SyncthingProcess::errorOccurred, process, [process] {
        auto messageBox = QMessageBox();
        messageBox.setWindowTitle(QStringLiteral("Syncthing"));
        messageBox.setWindowIcon(QIcon(QStringLiteral(":/icons/hicolor/scalable/app/syncthingtray.svg")));
        messageBox.setIcon(QMessageBox::Critical);
        messageBox.setText(QCoreApplication::translate("QtGui", "Unable to open Syncthing UI via \"%1\": %2").arg(process->program(), process->errorString()));
        messageBox.exec();
    });
    process->setProcessChannelMode(QProcess::ForwardedChannels);
    process->start(
                appList
#ifndef LIB_SYNCTHING_CONNECTOR_BOOST_PROCESS
                .first()
#endif
                , QStringList{ QStringLiteral("--app=") + url });

}

/*!
 * \brief Opens the Syncthing UI using the configured web view mode.
 * \param url The URL of the Syncthing UI.
 * \param settings The connection settings to be used (instead of the \a url) in case a WebViewDialog is used. Allowed to be nullptr.
 * \param dlg An existing WebViewDialog that may be reused in case a WebViewDialog is used.
 * \param parent The parent to use when creating a new WebViewDialog. Allowed to be nullptr (as usual with parents).
 * \returns Returns the used WebViewDialog or nullptr if another method was used.
 */
WebViewDialog *showWebUI(const QString &url, const Data::SyncthingConnectionSettings *settings, WebViewDialog *dlg, QWidget *parent)
{
    switch (Settings::values().webView.mode) {
#ifndef SYNCTHINGWIDGETS_NO_WEBVIEW
    case Settings::WebView::Mode::Builtin:
        if (!dlg) {
            dlg = new WebViewDialog(parent);
        }
        if (settings) {
            dlg->applySettings(*settings, true);
        }
        return dlg;
#else
        Q_UNUSED(settings)
        Q_UNUSED(dlg)
        Q_UNUSED(parent)
#endif
    case Settings::WebView::Mode::Command:
        openBrowserInAppMode(url);
        break;
    default:
        QDesktopServices::openUrl(url);
    }
    return nullptr;
}

} // namespace QtGui
