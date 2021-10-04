#include "./direrrorsdialog.h"

#include <syncthingconnector/syncthingconnection.h>
#include <syncthingconnector/syncthingdir.h>
#include <syncthingconnector/utils.h>

#include <QDir>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QStringBuilder>
#include <QTextBrowser>
#include <QVBoxLayout>

#include <algorithm>
#include <iostream>
#include <limits>

using namespace std;
using namespace CppUtilities;
using namespace Data;

namespace QtGui {

DirectoryErrorsDialog::DirectoryErrorsDialog(const Data::SyncthingConnection &connection, const Data::SyncthingDir &dir, QWidget *parent)
    : TextViewDialog(tr("Errors for directory %1").arg(dir.displayName()), parent)
    , m_connection(connection)
    , m_dirId(dir.id)
{
    // add layout to show status and additional buttons
    auto *const buttonLayout = new QHBoxLayout;
    buttonLayout->setContentsMargins(0, 0, 0, 0);
    layout()->addLayout(buttonLayout);

    // add label for overall status
    m_statusLabel = new QLabel(this);
    QFont boldFont(m_statusLabel->font());
    boldFont.setBold(true);
    m_statusLabel->setFont(boldFont);
    buttonLayout->addWidget(m_statusLabel);

    // add a button for removing all non-empty directories
    m_rmNonEmptyDirsButton = new QPushButton(this);
    m_rmNonEmptyDirsButton->setText(tr("Remove non-empty directories"));
    m_rmNonEmptyDirsButton->setIcon(QIcon::fromTheme(QStringLiteral("remove")));
    buttonLayout->setContentsMargins(0, 0, 0, 0);
    buttonLayout->addItem(new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum));
    buttonLayout->addWidget(m_rmNonEmptyDirsButton);

    // connect signals and slots
    connect(&connection, &SyncthingConnection::dirStatusChanged, this, &DirectoryErrorsDialog::handleDirStatusChanged);
    connect(&connection, &SyncthingConnection::newDirs, this, &DirectoryErrorsDialog::handleNewDirs);
    connect(m_rmNonEmptyDirsButton, &QPushButton::clicked, this, &DirectoryErrorsDialog::removeNonEmptyDirs);

    // populate initially available errors
    updateErrors(dir);
}

DirectoryErrorsDialog::~DirectoryErrorsDialog()
{
}

void DirectoryErrorsDialog::handleDirStatusChanged(const SyncthingDir &dir)
{
    if (dir.id == m_dirId) {
        updateErrors(dir);
    }
}

void DirectoryErrorsDialog::handleNewDirs()
{
    int index;
    if (const auto *const dir = m_connection.findDirInfo(m_dirId, index)) {
        updateErrors(*dir);
    }
}

void DirectoryErrorsDialog::updateErrors(const Data::SyncthingDir &dir)
{
    // update status
    m_statusLabel->setText(tr("%1 item(s) out-of-sync", nullptr, trQuandity(dir.pullErrorCount)).arg(dir.pullErrorCount));
    m_rmNonEmptyDirsButton->setHidden(m_nonEmptyDirs.empty());

    // clear previous errors
    auto *const textBrowser = browser();
    textBrowser->clear();
    m_nonEmptyDirs.clear();

    // add item errors to textBrowser
    for (const SyncthingItemError &error : dir.itemErrors) {
        textBrowser->append(error.path % QChar(':') % QChar('\n') % error.message % QChar('\n'));
        if (error.message.endsWith(QStringLiteral("directory not empty"))) {
            m_nonEmptyDirs << dir.path + error.path;
        }
    }
}

QString printDirectories(const QString &message, const QStringList &dirs)
{
    return QStringLiteral("<p>") % message % QStringLiteral("</p><ul><li>") % dirs.join(QStringLiteral("</li><li>")) % QStringLiteral("</ul>");
}

void DirectoryErrorsDialog::removeNonEmptyDirs()
{
    int index;
    const auto *const dir = m_connection.findDirInfo(m_dirId, index);
    if (!dir) {
        return;
    }

    const QString title(tr("Remove non-empty directories for folder \"%1\"").arg(dir->displayName()));
    if (QMessageBox::warning(this, title, printDirectories(tr("Do you really want to remove the following directories:"), m_nonEmptyDirs),
            QMessageBox::YesToAll | QMessageBox::NoToAll, QMessageBox::NoToAll)
        != QMessageBox::YesToAll) {
        return;
    }
    QStringList removedDirs, failedDirs;
    for (const QString &dirPath : m_nonEmptyDirs) {
        auto ok = false;
        auto dirObj = QDir(dirPath);
        if (!dirObj.exists() || !dirObj.removeRecursively()) {
            // check whether dir has already been removed by removing its parent
            for (const QString &removedDir : removedDirs) {
                if (dirPath.startsWith(removedDir)) {
                    ok = true;
                    break;
                }
            }
        } else {
            ok = true;
        }
        (ok ? removedDirs : failedDirs) << dirPath;
    }
    if (!failedDirs.isEmpty()) {
        QMessageBox::critical(this, title, printDirectories(tr("Unable to remove the following dirs:"), failedDirs));
    }
}

} // namespace QtGui
