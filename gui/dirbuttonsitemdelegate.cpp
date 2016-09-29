#include "./dirbuttonsitemdelegate.h"

#include <QPixmap>
#include <QPainter>
#include <QApplication>
#include <QStyle>
#include <QTextOption>
#include <QStyleOptionViewItem>
#include <QBrush>
#include <QPalette>

namespace QtGui {

inline int centerObj(int avail, int size)
{
    return (avail - size) / 2;
}

DirButtonsItemDelegate::DirButtonsItemDelegate(QObject* parent) :
    QStyledItemDelegate(parent),
    m_refreshIcon(QIcon::fromTheme(QStringLiteral("view-refresh"), QIcon(QStringLiteral(":/icons/hicolor/scalable/actions/view-refresh.svg"))).pixmap(QSize(16, 16))),
    m_folderIcon(QIcon::fromTheme(QStringLiteral("folder-open"), QIcon(QStringLiteral(":/icons/hicolor/scalable/places/folder-open.svg"))).pixmap(QSize(16, 16)))
{}

void DirButtonsItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    // use the customization only on top-level rows
    if(index.parent().isValid()) {
        QStyledItemDelegate::paint(painter, option, index);
    } else {
        // init style options to use drawControl(), except for the text
        QStyleOptionViewItem opt = option;
        initStyleOption(&opt, index);
        opt.text.clear();
        opt.features = QStyleOptionViewItem::None;
        QApplication::style()->drawControl(QStyle::CE_ItemViewItem, &opt, painter);

        // draw text
        QRectF textRect = option.rect;
        textRect.setWidth(textRect.width() - 38);
        QTextOption textOption;
        textOption.setAlignment(opt.displayAlignment);
        painter->setFont(opt.font);
        painter->setPen(opt.palette.color(opt.state & QStyle::State_Selected ? QPalette::HighlightedText : QPalette::Text));
        painter->drawText(textRect, displayText(index.data(Qt::DisplayRole), option.locale), textOption);

        // draw buttons
        const int buttonY = option.rect.y() + centerObj(option.rect.height(), 16);
        painter->drawPixmap(option.rect.right() - 34, buttonY, 16, 16, m_refreshIcon);
        painter->drawPixmap(option.rect.right() - 16, buttonY, 16, 16, m_folderIcon);
    }

}

}
