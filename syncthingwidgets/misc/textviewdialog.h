#ifndef SYNCTHINGWIDGETS_TEXTVIEWDIALOG_H
#define SYNCTHINGWIDGETS_TEXTVIEWDIALOG_H

#include "../global.h"

#include <QWidget>

#include <functional>

QT_FORWARD_DECLARE_CLASS(QTextBrowser)
QT_FORWARD_DECLARE_CLASS(QVBoxLayout)

namespace Data {
class SyncthingConnection;
struct SyncthingDir;
struct SyncthingLogEntry;
} // namespace Data

namespace QtGui {

class SYNCTHINGWIDGETS_EXPORT TextViewDialog : public QWidget {
    Q_OBJECT
public:
    explicit TextViewDialog(const QString &title = QString(), QWidget *parent = nullptr);

    QTextBrowser *browser();
    QVBoxLayout *layout();
    void setCloseHandler(std::function<bool(TextViewDialog *)> &&closeHandler);
    static TextViewDialog *forLogEntries(Data::SyncthingConnection &connection);
    static TextViewDialog *forLogEntries(const std::vector<Data::SyncthingLogEntry> &logEntries, const QString &title = QString());

Q_SIGNALS:
    void reload();

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

private:
    void showLogEntries(const std::vector<Data::SyncthingLogEntry> &logEntries);

    QTextBrowser *m_browser;
    QVBoxLayout *m_layout;
    std::function<bool(TextViewDialog *)> m_closeHandler;
};

inline QTextBrowser *TextViewDialog::browser()
{
    return m_browser;
}

inline QVBoxLayout *TextViewDialog::layout()
{
    return m_layout;
}

inline void TextViewDialog::setCloseHandler(std::function<bool(TextViewDialog *)> &&closeHandler)
{
    m_closeHandler = std::move(closeHandler);
}

} // namespace QtGui

#endif // SYNCTHINGWIDGETS_TEXTVIEWDIALOG_H
