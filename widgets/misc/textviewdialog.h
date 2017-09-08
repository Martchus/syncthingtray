#ifndef SYNCTHINGWIDGETS_TEXTVIEWDIALOG_H
#define SYNCTHINGWIDGETS_TEXTVIEWDIALOG_H

#include "../global.h"

#include <QWidget>

QT_FORWARD_DECLARE_CLASS(QTextBrowser)

namespace Data {
class SyncthingConnection;
struct SyncthingDir;
struct SyncthingLogEntry;
}

namespace QtGui {

class SYNCTHINGWIDGETS_EXPORT TextViewDialog : public QWidget {
    Q_OBJECT
public:
    TextViewDialog(const QString &title = QString(), QWidget *parent = nullptr);

    QTextBrowser *browser();
    static TextViewDialog *forDirectoryErrors(const Data::SyncthingDir &dir);
    static TextViewDialog *forLogEntries(Data::SyncthingConnection &connection);
    static TextViewDialog *forLogEntries(const std::vector<Data::SyncthingLogEntry> &logEntries, const QString &title = QString());

Q_SIGNALS:
    void reload();

protected:
    void keyPressEvent(QKeyEvent *event);

private:
    void showLogEntries(const std::vector<Data::SyncthingLogEntry> &logEntries);

    QTextBrowser *m_browser;
};

inline QTextBrowser *TextViewDialog::browser()
{
    return m_browser;
}
}

#endif // SYNCTHINGWIDGETS_TEXTVIEWDIALOG_H
