#ifndef WEBVIEW_DIALOG_H
#define WEBVIEW_DIALOG_H

#include "../global.h"

#include <syncthingconnector/syncthingconnectionsettings.h>

QT_FORWARD_DECLARE_CLASS(QString)
QT_FORWARD_DECLARE_CLASS(QWidget)

#ifndef SYNCTHINGWIDGETS_NO_WEBVIEW
#include "./webviewdefs.h"

#include <QMainWindow>

QT_FORWARD_DECLARE_CLASS(WEB_VIEW_PROVIDER)
QT_FORWARD_DECLARE_CLASS(QWebEngineProfile)

namespace QtGui {

class SYNCTHINGWIDGETS_EXPORT WebViewDialog : public QMainWindow {
    Q_OBJECT
public:
    WebViewDialog(QWidget *parent = nullptr);
    ~WebViewDialog() override;

public Q_SLOTS:
    void applySettings(const Data::SyncthingConnectionSettings &connectionSettings, bool aboutToShow);
    const Data::SyncthingConnectionSettings &connectionSettings() const;
#if defined(SYNCTHINGWIDGETS_USE_WEBKIT)
    bool isModalVisible() const;
#endif
    void closeUnlessModalVisible();

protected:
    void closeEvent(QCloseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
#if defined(SYNCTHINGWIDGETS_USE_WEBENGINE)
    bool eventFilter(QObject *watched, QEvent *event) override;
#endif

private:
    SYNCTHINGWIDGETS_WEB_VIEW *m_view;
    Data::SyncthingConnectionSettings m_connectionSettings;
#if defined(SYNCTHINGWIDGETS_USE_WEBENGINE)
    QWebEngineProfile *m_profile;
#endif
};

inline const Data::SyncthingConnectionSettings &WebViewDialog::connectionSettings() const
{
    return m_connectionSettings;
}

} // namespace QtGui

#else // SYNCTHINGWIDGETS_NO_WEBVIEW
namespace QtGui {
using WebViewDialog = void;
}

#endif // SYNCTHINGWIDGETS_NO_WEBVIEW

namespace QtGui {
SYNCTHINGWIDGETS_EXPORT WebViewDialog *showWebUI(
    const QString &url, const Data::SyncthingConnectionSettings *settings, WebViewDialog *dlg = nullptr, QWidget *parent = nullptr);
}

#endif // WEBVIEW_DIALOG_H
