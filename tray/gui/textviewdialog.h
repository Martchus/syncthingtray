#ifndef TEXTVIEWDIALOG_H
#define TEXTVIEWDIALOG_H

#include <QWidget>

QT_FORWARD_DECLARE_CLASS(QTextBrowser)

namespace QtGui {

class TextViewDialog : public QWidget
{
    Q_OBJECT
public:
    TextViewDialog(const QString &title = QString(), QWidget *parent = nullptr);

    QTextBrowser *browser();

private:
    QTextBrowser *m_browser;
};

inline QTextBrowser *TextViewDialog::browser()
{
    return m_browser;
}

}

#endif // TEXTVIEWDIALOG_H
