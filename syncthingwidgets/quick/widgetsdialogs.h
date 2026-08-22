#pragma once

#include <QObject>
#include <QString>
#include <QUrl>
#include <QtQmlIntegration/qqmlintegration.h>

QT_FORWARD_DECLARE_CLASS(QFileDialog)

namespace QtGui {

class FileDialog : public QObject {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY titleChanged)
    Q_PROPERTY(QUrl currentFolder READ currentFolder WRITE setCurrentFolder NOTIFY currentFolderChanged)
    Q_PROPERTY(QUrl selectedFile READ selectedFile WRITE setSelectedFile NOTIFY selectedFileChanged)
    Q_PROPERTY(FileMode fileMode READ fileMode WRITE setFileMode NOTIFY fileModeChanged)
    Q_PROPERTY(int options READ options WRITE setOptions NOTIFY optionsChanged)
    Q_PROPERTY(int popupType READ popupType WRITE setPopupType NOTIFY popupTypeChanged)
    Q_PROPERTY(bool visible READ visible WRITE setVisible NOTIFY visibleChanged)

public:
    enum FileMode { OpenFile = 0, OpenFiles = 1, SaveFile = 2 };
    Q_ENUM(FileMode)

    enum Option { DontUseNativeDialog = 0x00000010 };
    Q_ENUM(Option)

    explicit FileDialog(QObject *parent = nullptr);
    ~FileDialog() override;

    QString title() const
    {
        return m_title;
    }
    void setTitle(const QString &title);

    QUrl currentFolder() const
    {
        return m_currentFolder;
    }
    void setCurrentFolder(const QUrl &folder);

    QUrl selectedFile() const
    {
        return m_selectedFile;
    }
    void setSelectedFile(const QUrl &file);

    FileMode fileMode() const
    {
        return m_fileMode;
    }
    void setFileMode(FileMode mode);

    int options() const
    {
        return m_options;
    }
    void setOptions(int options);

    int popupType() const
    {
        return m_popupType;
    }
    void setPopupType(int popupType);

    bool visible() const
    {
        return m_visible;
    }
    void setVisible(bool visible);

public Q_SLOTS:
    void open();
    void close();
    void accept();
    void reject();

Q_SIGNALS:
    void titleChanged();
    void currentFolderChanged();
    void selectedFileChanged();
    void fileModeChanged();
    void optionsChanged();
    void popupTypeChanged();
    void visibleChanged();
    void accepted();
    void rejected();

private:
    void ensureDialog();

    QString m_title;
    QUrl m_currentFolder;
    QUrl m_selectedFile;
    FileMode m_fileMode = OpenFile;
    int m_options = 0;
    int m_popupType = 0;
    bool m_visible = false;
    QFileDialog *m_dialog = nullptr;
};

class FolderDialog : public QObject {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY titleChanged)
    Q_PROPERTY(QUrl currentFolder READ currentFolder WRITE setCurrentFolder NOTIFY currentFolderChanged)
    Q_PROPERTY(QUrl selectedFolder READ selectedFolder WRITE setSelectedFolder NOTIFY selectedFolderChanged)
    Q_PROPERTY(int options READ options WRITE setOptions NOTIFY optionsChanged)
    Q_PROPERTY(int popupType READ popupType WRITE setPopupType NOTIFY popupTypeChanged)
    Q_PROPERTY(bool visible READ visible WRITE setVisible NOTIFY visibleChanged)

public:
    enum Option { DontUseNativeDialog = 0x00000010 };
    Q_ENUM(Option)

    explicit FolderDialog(QObject *parent = nullptr);
    ~FolderDialog() override;

    QString title() const
    {
        return m_title;
    }
    void setTitle(const QString &title);

    QUrl currentFolder() const
    {
        return m_currentFolder;
    }
    void setCurrentFolder(const QUrl &folder);

    QUrl selectedFolder() const
    {
        return m_selectedFolder;
    }
    void setSelectedFolder(const QUrl &folder);

    int options() const
    {
        return m_options;
    }
    void setOptions(int options);

    int popupType() const
    {
        return m_popupType;
    }
    void setPopupType(int popupType);

    bool visible() const
    {
        return m_visible;
    }
    void setVisible(bool visible);

public Q_SLOTS:
    void open();
    void close();
    void accept();
    void reject();

Q_SIGNALS:
    void titleChanged();
    void currentFolderChanged();
    void selectedFolderChanged();
    void optionsChanged();
    void popupTypeChanged();
    void visibleChanged();
    void accepted();
    void rejected();

private:
    void ensureDialog();

    QString m_title;
    QUrl m_currentFolder;
    QUrl m_selectedFolder;
    int m_options = 0;
    int m_popupType = 0;
    bool m_visible = false;
    QFileDialog *m_dialog = nullptr;
};

} // namespace QtGui
