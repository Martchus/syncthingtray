#include "./textviewdialog.h"

#include "./syncthinglauncher.h"

#include <syncthingconnector/syncthingconnection.h>
#include <syncthingconnector/syncthingdir.h>

// use meta-data of syncthingtray application here
#include "resources/../../tray/resources/config.h"

#include <qtutilities/misc/dialogutils.h>

#include <QCloseEvent>
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
    : QDialog(parent)
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

TextViewDialog *TextViewDialog::forLogEntries(SyncthingConnection &connection, QObject *gui)
{
    auto *const dlg = new TextViewDialog(tr("Log"));
    auto *const launcher = SyncthingLauncher::mainInstance();
    auto *const helpLabel = new QLabel(dlg);
    auto helpText = tr("Press F5 to reload.");
    if (gui && launcher && launcher->isRunning() && connection.isLocal()) {
        helpText += tr(" Checkout <a href=\"openLauncherSettings\">launcher settings</a> for continuous log of local Syncthing instance.");
        QObject::connect(helpLabel, &QLabel::linkActivated, gui, [gui](const QString &link) {
            if (link == QLatin1String("openLauncherSettings")) {
                QMetaObject::invokeMethod(gui, "showLauncherSettings");
            }
        });
    }
    helpLabel->setWordWrap(true);
    helpLabel->setText(helpText);
    QObject::connect(&connection, &SyncthingConnection::logAvailable, dlg, &TextViewDialog::showLogEntries);
    connect(dlg, &TextViewDialog::reload, &connection, &SyncthingConnection::requestLog);
    connection.requestLog();
    dlg->layout()->addWidget(helpLabel);
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
    case Qt::Key_S:
        if (event->modifiers() == Qt::ControlModifier) {
            emit save();
        }
        break;
    default:;
    }
}

void TextViewDialog::closeEvent(QCloseEvent *event)
{
    if (m_closeHandler && m_closeHandler(this)) {
        event->ignore();
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
