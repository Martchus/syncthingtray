#ifndef WEBVIEW_DIALOG_H
#define WEBVIEW_DIALOG_H
#ifndef SYNCTHINGTRAY_NO_WEBVIEW

#include "./webviewprovider.h"

#include <QMainWindow>

QT_FORWARD_DECLARE_CLASS(WEB_VIEW_PROVIDER)

namespace QtGui {

class WebViewDialog : public QMainWindow
{
    Q_OBJECT
public:
    WebViewDialog(QWidget *parent = nullptr);
    ~WebViewDialog();

public slots:
    void applySettings();

protected:
    void closeEvent(QCloseEvent *event);

private:
    WEB_VIEW_PROVIDER *m_view;
};

}

#endif
#endif // WEBVIEW_DIALOG_H
