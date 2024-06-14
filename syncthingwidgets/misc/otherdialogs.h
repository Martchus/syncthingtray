#ifndef SYNCTHINGWIDGETS_OTHERDIALOGS_H
#define SYNCTHINGWIDGETS_OTHERDIALOGS_H

#include "../global.h"

#include <QtGlobal>

#include <QSyntaxHighlighter>
#include <QTextCharFormat>

QT_FORWARD_DECLARE_CLASS(QDialog)
QT_FORWARD_DECLARE_CLASS(QWidget)

namespace Data {
class SyncthingConnection;
struct SyncthingDir;
} // namespace Data

namespace QtGui {
class TextViewDialog;

class SYNCTHINGWIDGETS_EXPORT DiffHighlighter : public QSyntaxHighlighter {
    Q_OBJECT
    Q_PROPERTY(bool enabled READ isEnabled WRITE setEnabled)
public:
    explicit DiffHighlighter(QTextDocument *parent = nullptr);

    bool isEnabled() const
    {
        return m_enabled;
    }
    void setEnabled(bool enabled)
    {
        if (enabled != m_enabled) {
            m_enabled = enabled;
            rehighlight();
        }
    }

protected:
    void highlightBlock(const QString &text) override;

private:
    QTextCharFormat m_baseFormat, m_addedFormat, m_deletedFormat;
    bool m_enabled;
};

SYNCTHINGWIDGETS_EXPORT QDialog *ownDeviceIdDialog(Data::SyncthingConnection &connection);
SYNCTHINGWIDGETS_EXPORT QWidget *ownDeviceIdWidget(Data::SyncthingConnection &connection, int size, QWidget *parent = nullptr);
SYNCTHINGWIDGETS_EXPORT QDialog *browseRemoteFilesDialog(
    Data::SyncthingConnection &connection, const Data::SyncthingDir &dir, QWidget *parent = nullptr);
SYNCTHINGWIDGETS_EXPORT TextViewDialog *ignorePatternsDialog(
    Data::SyncthingConnection &connection, const Data::SyncthingDir &dir, QWidget *parent = nullptr);

} // namespace QtGui

#endif // SYNCTHINGWIDGETS_OTHERDIALOGS_H
