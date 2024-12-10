#include "./diffhighlighter.h"

#include <syncthingmodel/colors.h>

#include <QFont>
#include <QFontDatabase>
#include <QTextDocument>

using namespace std;
using namespace Data;

namespace QtGui {

DiffHighlighter::DiffHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
    , m_enabled(true)
{
    auto font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    m_baseFormat.setFont(font);

    font.setBold(true);
    m_addedFormat.setFont(font);
    m_addedFormat.setForeground(Colors::green(true));
    m_deletedFormat.setFont(font);
    m_deletedFormat.setForeground(Colors::red(true));
}

void DiffHighlighter::highlightBlock(const QString &text)
{
    if (text.startsWith(QChar('-'))) {
        setFormat(0, static_cast<int>(text.size()), QColor(Qt::red));
    } else if (text.startsWith(QChar('+'))) {
        setFormat(0, static_cast<int>(text.size()), QColor(Qt::green));
    }
}

} // namespace QtGui
