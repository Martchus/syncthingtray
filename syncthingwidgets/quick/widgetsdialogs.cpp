#include "./widgetsdialogs.h"

#include <QFileDialog>
#include <QQuickWindow>
#include <QWindow>

namespace QtGui {

FileDialog::FileDialog(QQuickItem *parent)
    : QQuickItem(parent)
{
}

FileDialog::~FileDialog()
{
    delete m_dialog;
}

void FileDialog::setTitle(const QString &title)
{
    if (m_title == title) {
        return;
    }
    m_title = title;
    Q_EMIT titleChanged();
    if (m_dialog) {
        m_dialog->setWindowTitle(m_title);
    }
}

void FileDialog::setCurrentFolder(const QUrl &folder)
{
    QUrl resolved = folder;
    if (resolved.scheme().isEmpty() && !resolved.isEmpty()) {
        resolved = QUrl::fromLocalFile(resolved.toString());
    }
    if (m_currentFolder == resolved) {
        return;
    }
    m_currentFolder = resolved;
    Q_EMIT currentFolderChanged();
    if (m_dialog) {
        if (m_currentFolder.isLocalFile()) {
            m_dialog->setDirectory(m_currentFolder.toLocalFile());
        } else {
            m_dialog->setDirectory(m_currentFolder.toString());
        }
    }
}

void FileDialog::setSelectedFile(const QUrl &file)
{
    QUrl resolved = file;
    if (resolved.scheme().isEmpty() && !resolved.isEmpty()) {
        resolved = QUrl::fromLocalFile(resolved.toString());
    }
    if (m_selectedFile == resolved) {
        return;
    }
    m_selectedFile = resolved;
    Q_EMIT selectedFileChanged();
}

void FileDialog::setFileMode(FileMode mode)
{
    if (m_fileMode == mode) {
        return;
    }
    m_fileMode = mode;
    Q_EMIT fileModeChanged();
}

void FileDialog::setOptions(int options)
{
    if (m_options == options) {
        return;
    }
    m_options = options;
    Q_EMIT optionsChanged();
}

void FileDialog::setPopupType(int popupType)
{
    if (m_popupType == popupType) {
        return;
    }
    m_popupType = popupType;
    Q_EMIT popupTypeChanged();
}

void FileDialog::setVisible(bool visible)
{
    if (m_visible == visible) {
        return;
    }
    if (visible) {
        open();
    } else {
        close();
    }
}

void FileDialog::open()
{
    ensureDialog();

    m_dialog->setWindowTitle(m_title);

    if (m_fileMode == SaveFile) {
        m_dialog->setFileMode(QFileDialog::AnyFile);
        m_dialog->setAcceptMode(QFileDialog::AcceptSave);
    } else if (m_fileMode == OpenFiles) {
        m_dialog->setFileMode(QFileDialog::ExistingFiles);
        m_dialog->setAcceptMode(QFileDialog::AcceptOpen);
    } else {
        m_dialog->setFileMode(QFileDialog::ExistingFile);
        m_dialog->setAcceptMode(QFileDialog::AcceptOpen);
    }

    QFileDialog::Options qopts = m_dialog->options();
    if (m_options & DontUseNativeDialog) {
        qopts |= QFileDialog::DontUseNativeDialog;
    } else {
        qopts &= ~QFileDialog::DontUseNativeDialog;
    }
    m_dialog->setOptions(qopts);

    if (!m_currentFolder.isEmpty()) {
        if (m_currentFolder.isLocalFile()) {
            m_dialog->setDirectory(m_currentFolder.toLocalFile());
        } else {
            m_dialog->setDirectory(m_currentFolder.toString());
        }
    }

    if (auto *w = window()) {
        if (auto *handle = m_dialog->windowHandle()) {
            handle->setTransientParent(w);
        }
    }

    m_visible = true;
    Q_EMIT visibleChanged();

    m_dialog->open();
}

void FileDialog::close()
{
    reject();
}

void FileDialog::accept()
{
    if (m_dialog && m_dialog->isVisible()) {
        static_cast<QDialog *>(m_dialog)->accept();
    } else {
        m_visible = false;
        Q_EMIT visibleChanged();
        Q_EMIT accepted();
    }
}

void FileDialog::reject()
{
    if (m_dialog && m_dialog->isVisible()) {
        m_dialog->reject();
    } else {
        m_visible = false;
        Q_EMIT visibleChanged();
        Q_EMIT rejected();
    }
}

void FileDialog::ensureDialog()
{
    if (m_dialog) {
        return;
    }
    m_dialog = new QFileDialog();

    connect(m_dialog, &QFileDialog::accepted, this, [this]() {
        const auto urls = m_dialog->selectedUrls();
        if (!urls.isEmpty()) {
            m_selectedFile = urls.first();
            Q_EMIT selectedFileChanged();
        }
        m_visible = false;
        Q_EMIT visibleChanged();
        Q_EMIT accepted();
    });

    connect(m_dialog, &QFileDialog::rejected, this, [this]() {
        m_visible = false;
        Q_EMIT visibleChanged();
        Q_EMIT rejected();
    });
}

FolderDialog::FolderDialog(QQuickItem *parent)
    : QQuickItem(parent)
{
}

FolderDialog::~FolderDialog()
{
    delete m_dialog;
}

void FolderDialog::setTitle(const QString &title)
{
    if (m_title == title) {
        return;
    }
    m_title = title;
    Q_EMIT titleChanged();
    if (m_dialog) {
        m_dialog->setWindowTitle(m_title);
    }
}

void FolderDialog::setCurrentFolder(const QUrl &folder)
{
    QUrl resolved = folder;
    if (resolved.scheme().isEmpty() && !resolved.isEmpty()) {
        resolved = QUrl::fromLocalFile(resolved.toString());
    }
    if (m_currentFolder == resolved) {
        return;
    }
    m_currentFolder = resolved;
    Q_EMIT currentFolderChanged();
    if (m_dialog) {
        if (m_currentFolder.isLocalFile()) {
            m_dialog->setDirectory(m_currentFolder.toLocalFile());
        } else {
            m_dialog->setDirectory(m_currentFolder.toString());
        }
    }
}

void FolderDialog::setSelectedFolder(const QUrl &folder)
{
    QUrl resolved = folder;
    if (resolved.scheme().isEmpty() && !resolved.isEmpty()) {
        resolved = QUrl::fromLocalFile(resolved.toString());
    }
    if (m_selectedFolder == resolved) {
        return;
    }
    m_selectedFolder = resolved;
    Q_EMIT selectedFolderChanged();
}

void FolderDialog::setOptions(int options)
{
    if (m_options == options) {
        return;
    }
    m_options = options;
    Q_EMIT optionsChanged();
}

void FolderDialog::setPopupType(int popupType)
{
    if (m_popupType == popupType) {
        return;
    }
    m_popupType = popupType;
    Q_EMIT popupTypeChanged();
}

void FolderDialog::setVisible(bool visible)
{
    if (m_visible == visible) {
        return;
    }
    if (visible) {
        open();
    } else {
        close();
    }
}

void FolderDialog::open()
{
    ensureDialog();

    m_dialog->setWindowTitle(m_title);

    m_dialog->setFileMode(QFileDialog::Directory);
    m_dialog->setOption(QFileDialog::ShowDirsOnly, true);

    QFileDialog::Options qopts = m_dialog->options();
    if (m_options & DontUseNativeDialog) {
        qopts |= QFileDialog::DontUseNativeDialog;
    } else {
        qopts &= ~QFileDialog::DontUseNativeDialog;
    }
    m_dialog->setOptions(qopts);

    if (!m_currentFolder.isEmpty()) {
        if (m_currentFolder.isLocalFile()) {
            m_dialog->setDirectory(m_currentFolder.toLocalFile());
        } else {
            m_dialog->setDirectory(m_currentFolder.toString());
        }
    }

    if (auto *w = window()) {
        if (auto *handle = m_dialog->windowHandle()) {
            handle->setTransientParent(w);
        }
    }

    m_visible = true;
    Q_EMIT visibleChanged();

    m_dialog->open();
}

void FolderDialog::close()
{
    reject();
}

void FolderDialog::accept()
{
    if (m_dialog && m_dialog->isVisible()) {
        static_cast<QDialog *>(m_dialog)->accept();
    } else {
        m_visible = false;
        Q_EMIT visibleChanged();
        Q_EMIT accepted();
    }
}

void FolderDialog::reject()
{
    if (m_dialog && m_dialog->isVisible()) {
        m_dialog->reject();
    } else {
        m_visible = false;
        Q_EMIT visibleChanged();
        Q_EMIT rejected();
    }
}

void FolderDialog::ensureDialog()
{
    if (m_dialog) {
        return;
    }
    m_dialog = new QFileDialog();

    connect(m_dialog, &QFileDialog::accepted, this, [this]() {
        const auto urls = m_dialog->selectedUrls();
        if (!urls.isEmpty()) {
            m_selectedFolder = urls.first();
            Q_EMIT selectedFolderChanged();
        }
        m_visible = false;
        Q_EMIT visibleChanged();
        Q_EMIT accepted();
    });

    connect(m_dialog, &QFileDialog::rejected, this, [this]() {
        m_visible = false;
        Q_EMIT visibleChanged();
        Q_EMIT rejected();
    });
}

} // namespace QtGui

#ifndef SYNTAX_CHECK
#include "moc_widgetsdialogs.cpp"
#endif
