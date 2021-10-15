#include "./dirbuttonsitemdelegate.h"

#include <syncthingmodel/syncthingdirectorymodel.h>
#include <syncthingmodel/syncthingicons.h>

#include <qtforkawesome/icon.h>
#include <qtforkawesome/renderer.h>

#include <QApplication>
#include <QBrush>
#include <QPainter>
#include <QPalette>
#include <QPixmap>
#include <QStyle>
#include <QStyleOptionViewItem>
#include <QTextOption>

using namespace Data;

namespace QtGui {

inline int centerObj(int avail, int size)
{
    return (avail - size) / 2;
}

DirButtonsItemDelegate::DirButtonsItemDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

void DirButtonsItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    // use the customization only on top-level rows
    if (index.parent().isValid()) {
        QStyledItemDelegate::paint(painter, option, index);
    } else {
        // init style options to use drawControl(), except for the text
        QStyleOptionViewItem opt = option;
        initStyleOption(&opt, index);
        opt.text.clear();
        opt.features = QStyleOptionViewItem::None;
        QApplication::style()->drawControl(QStyle::CE_ItemViewItem, &opt, painter);

        // draw text
        auto textRect = QRectF(option.rect);
        textRect.setWidth(textRect.width() - 58);
        QTextOption textOption;
        textOption.setAlignment(opt.displayAlignment);
        painter->setFont(opt.font);
        painter->setPen(opt.palette.color(opt.state & QStyle::State_Selected ? QPalette::HighlightedText : QPalette::Text));
        painter->drawText(textRect, displayText(index.data(Qt::DisplayRole), option.locale), textOption);

        // draw buttons
        const int buttonY = option.rect.y() + centerObj(option.rect.height(), 16);
        const bool dirPaused = index.data(SyncthingDirectoryModel::DirectoryPaused).toBool();
        const auto iconColor = QGuiApplication::palette().color(QPalette::Text);
        auto &forkAwesomeRenderer = IconManager::instance().forkAwesomeRenderer();
        if (!dirPaused) {
            forkAwesomeRenderer.render(QtForkAwesome::Icon::Refresh, painter, QRect(option.rect.right() - 52, buttonY, 16, 16), iconColor);
        }
        forkAwesomeRenderer.render(
            dirPaused ? QtForkAwesome::Icon::Play : QtForkAwesome::Icon::Pause, painter, QRect(option.rect.right() - 34, buttonY, 16, 16), iconColor);
        forkAwesomeRenderer.render(QtForkAwesome::Icon::Folder, painter, QRect(option.rect.right() - 16, buttonY, 16, 16), iconColor);
    }
}
} // namespace QtGui
