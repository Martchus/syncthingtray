#include "./textviewdialog.h"

#include "resources/config.h"

#include <qtutilities/misc/dialogutils.h>

#include <QFontDatabase>
#include <QIcon>
#include <QKeyEvent>
#include <QTextBrowser>
#include <QVBoxLayout>

using namespace Dialogs;

namespace QtGui {

TextViewDialog::TextViewDialog(const QString &title, QWidget *parent)
    : QWidget(parent, Qt::Window)
{
    // set window title and icon
    if (title.isEmpty()) {
        setWindowTitle(QStringLiteral(APP_NAME));
    } else {
        setWindowTitle(title + QStringLiteral(" - " APP_NAME));
    }
    setWindowIcon(QIcon(QStringLiteral(":/icons/hicolor/scalable/app/syncthingtray.svg")));

    // by default, delete on close
    setAttribute(Qt::WA_DeleteOnClose);

    // setup browser
    m_browser = new QTextBrowser(this);
    m_browser->setReadOnly(true);
    m_browser->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));

    // setup layout
    auto *layout = new QVBoxLayout(this);
    layout->setAlignment(Qt::AlignCenter);
    layout->addWidget(m_browser);
    setLayout(layout);

    // default position and size
    resize(600, 500);
    centerWidget(this);
}

void TextViewDialog::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_Escape:
        close();
        break;
    case Qt::Key_F5:
        emit reload();
        break;
    default:;
    }
}
}
