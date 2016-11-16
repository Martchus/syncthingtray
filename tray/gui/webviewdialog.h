#ifndef WEBVIEW_DIALOG_H
#define WEBVIEW_DIALOG_H
#ifndef SYNCTHINGTRAY_NO_WEBVIEW

#include "./webviewprovider.h"

#include "../application/settings.h"

#include <QMainWindow>

QT_FORWARD_DECLARE_CLASS(WEB_VIEW_PROVIDER)

namespace Settings {
struct ConnectionSettings;
}

namespace QtGui {

class WebViewDialog : public QMainWindow
{
    Q_OBJECT
public:
    WebViewDialog(QWidget *parent = nullptr);
    ~WebViewDialog();

public slots:
    void applySettings(const Data::SyncthingConnectionSettings &connectionSettings);
    const Data::SyncthingConnectionSettings &settings() const;
#if defined(SYNCTHINGTRAY_USE_WEBKIT)
    bool isModalVisible() const;
#endif
    void closeUnlessModalVisible();

protected:
    void closeEvent(QCloseEvent *event);
    void keyPressEvent(QKeyEvent *event);

private:
    WEB_VIEW_PROVIDER *m_view;
    Data::SyncthingConnectionSettings m_settings;
};

inline const Data::SyncthingConnectionSettings &WebViewDialog::settings() const
{
    return m_settings;
}

}

#endif // SYNCTHINGTRAY_NO_WEBVIEW
#endif // WEBVIEW_DIALOG_H
