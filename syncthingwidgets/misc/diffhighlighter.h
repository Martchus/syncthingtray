#ifndef SYNCTHINGWIDGETS_DIFF_HIGHLIGHTER_H
#define SYNCTHINGWIDGETS_DIFF_HIGHLIGHTER_H

#include "../global.h"

#include <QSyntaxHighlighter>
#include <QTextCharFormat>

namespace QtGui {

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

} // namespace QtGui

#endif // SYNCTHINGWIDGETS_DIFF_HIGHLIGHTER_H
