#include "./devbuttonsitemdelegate.h"

#include <syncthingconnector/syncthingconnection.h>
#include <syncthingmodel/syncthingdevicemodel.h>
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

DevButtonsItemDelegate::DevButtonsItemDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

void DevButtonsItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
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
        QRectF textRect = option.rect;
        textRect.setWidth(textRect.width() - 20);
        QTextOption textOption;
        textOption.setAlignment(opt.displayAlignment);
        painter->setFont(opt.font);
        painter->setPen(opt.palette.color(opt.state & QStyle::State_Selected ? QPalette::HighlightedText : QPalette::Text));
        painter->drawText(textRect, displayText(index.data(Qt::DisplayRole), option.locale), textOption);

        // draw buttons
        if (index.data(SyncthingDeviceModel::IsOwnDevice).toBool()) {
            return;
        }
        const int buttonY = option.rect.y() + centerObj(option.rect.height(), 16);
        IconManager::instance().forkAwesomeRenderer().render(
            index.data(SyncthingDeviceModel::DevicePaused).toBool() ? QtForkAwesome::Icon::Play : QtForkAwesome::Icon::Pause, painter,
            QRect(option.rect.right() - 16, buttonY, 16, 16), QGuiApplication::palette().color(QPalette::Text));
    }
}
} // namespace QtGui
