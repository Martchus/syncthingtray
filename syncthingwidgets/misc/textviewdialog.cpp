#include "./textviewdialog.h"

#include <syncthingconnector/syncthingconnection.h>
#include <syncthingconnector/syncthingdir.h>

// use meta-data of syncthingtray application here
#include "resources/../../tray/resources/config.h"

#include <qtutilities/misc/dialogutils.h>

#include <QFontDatabase>
#include <QHBoxLayout>
#include <QIcon>
#include <QKeyEvent>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QStringBuilder>
#include <QTextBrowser>
#include <QVBoxLayout>

using namespace std;
using namespace std::placeholders;
using namespace QtUtilities;
using namespace Data;

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
    m_layout = new QVBoxLayout;
    m_layout->setAlignment(Qt::AlignCenter);
    m_layout->addWidget(m_browser);
    setLayout(m_layout);

    // default position and size
    resize(600, 500);
    centerWidget(this);
}

TextViewDialog *TextViewDialog::forLogEntries(SyncthingConnection &connection)
{
    auto *const dlg = new TextViewDialog(tr("Log"));
    QObject::connect(&connection, &SyncthingConnection::logAvailable, dlg, &TextViewDialog::showLogEntries);
    connect(dlg, &TextViewDialog::reload, &connection, &SyncthingConnection::requestLog);
    connection.requestLog();
    return dlg;
}

TextViewDialog *TextViewDialog::forLogEntries(const std::vector<SyncthingLogEntry> &logEntries, const QString &title)
{
    auto *const dlg = new TextViewDialog(title.isEmpty() ? tr("Log") : title);
    dlg->showLogEntries(logEntries);
    return dlg;
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

void TextViewDialog::showLogEntries(const std::vector<SyncthingLogEntry> &logEntries)
{
    browser()->clear();
    for (const SyncthingLogEntry &entry : logEntries) {
        browser()->append(entry.when % QChar(':') % QChar(' ') % QChar('\n') % entry.message % QChar('\n'));
    }
}
} // namespace QtGui
