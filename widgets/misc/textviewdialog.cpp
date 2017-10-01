#include "./textviewdialog.h"

#include "../../connector/syncthingconnection.h"
#include "../../connector/syncthingdir.h"

// use meta-data of syncthingtray application here
#include "resources/../../tray/resources/config.h"

#include <qtutilities/misc/dialogutils.h>

#include <QDir>
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

#include <algorithm>
#include <functional>
#include <limits>

using namespace std;
using namespace std::placeholders;
using namespace Dialogs;
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

QString printDirectories(const QString &message, const QStringList &dirs)
{
    return QStringLiteral("<p>") % message % QStringLiteral("</p><ul><li>") % dirs.join(QStringLiteral("</li><li>")) % QStringLiteral("</ul>");
}

TextViewDialog *TextViewDialog::forDirectoryErrors(const Data::SyncthingDir &dir)
{
    // create TextViewDialog
    auto *const textViewDlg = new TextViewDialog(tr("Errors of %1").arg(dir.displayName()));
    auto *const browser = textViewDlg->browser();

    // add errors to text view and find errors about non-empty directories to be removed
    QStringList nonEmptyDirs;
    for (const SyncthingItemError &error : dir.itemErrors) {
        browser->append(error.path % QChar(':') % QChar('\n') % error.message % QChar('\n'));
        if (error.message.endsWith(QStringLiteral("directory not empty"))) {
            nonEmptyDirs << dir.path + error.path;
        }
    }

    // add layout to show status and additional buttons
    auto *const buttonLayout = new QHBoxLayout;
    buttonLayout->setMargin(0);

    // add label for overall status
    auto *const statusLabel = new QLabel(textViewDlg);
    statusLabel->setText(tr("%1 item(s) out-of-sync", nullptr, static_cast<int>(min<size_t>(dir.itemErrors.size(), numeric_limits<int>::max())))
                             .arg(dir.itemErrors.size()));
    QFont boldFont(statusLabel->font());
    boldFont.setBold(true);
    statusLabel->setFont(boldFont);
    buttonLayout->addWidget(statusLabel);

    // add a button for removing all non-empty directories
    if (!nonEmptyDirs.isEmpty()) {
        auto *const rmNonEmptyDirsButton = new QPushButton(textViewDlg);
        rmNonEmptyDirsButton->setText(tr("Remove non-empty directories"));
        rmNonEmptyDirsButton->setIcon(QIcon::fromTheme(QStringLiteral("remove")));
        buttonLayout->setMargin(0);
        buttonLayout->addItem(new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum));
        buttonLayout->addWidget(rmNonEmptyDirsButton);

        // define directory removal function
        const QString title(tr("Remove non-empty directories for folder \"%1\"").arg(dir.displayName()));
        connect(rmNonEmptyDirsButton, &QPushButton::clicked, [textViewDlg, nonEmptyDirs, title] {
            if (QMessageBox::warning(textViewDlg, title,
                    printDirectories(tr("Do you really want to remove the following directories:"), nonEmptyDirs), QMessageBox::YesToAll,
                    QMessageBox::NoToAll | QMessageBox::Default | QMessageBox::Escape)
                == QMessageBox::YesToAll) {
                QStringList failedDirs;
                for (const QString &dirPath : nonEmptyDirs) {
                    QDir dir(dirPath);
                    if (!dir.exists() || !dir.removeRecursively()) {
                        failedDirs << dirPath;
                    }
                }
                if (!failedDirs.isEmpty()) {
                    QMessageBox::critical(textViewDlg, title, printDirectories(tr("Unable to remove the following dirs:"), failedDirs));
                }
            }
        });
    }

    textViewDlg->m_layout->addLayout(buttonLayout);
    return textViewDlg;
}

TextViewDialog *TextViewDialog::forLogEntries(SyncthingConnection &connection)
{
    auto *const dlg = new TextViewDialog(tr("Log"));
    const auto loadLog = [dlg, &connection] {
        connect(dlg, &QWidget::destroyed,
            bind(static_cast<bool (*)(const QMetaObject::Connection &)>(&QObject::disconnect),
                connection.requestLog(bind(&TextViewDialog::showLogEntries, dlg, _1))));
    };
    connect(dlg, &TextViewDialog::reload, loadLog);
    loadLog();
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
