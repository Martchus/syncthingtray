#include "./devbuttonsitemdelegate.h"

#include "./helper.h"

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

DevButtonsItemDelegate::DevButtonsItemDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

void DevButtonsItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    auto opt = option;
    initStyleOption(&opt, index);
    opt.viewItemPosition = QStyleOptionViewItem::OnlyOne;
    if (index.parent().isValid()) {
        QStyledItemDelegate::paint(painter, opt, index);
    } else {
        // init style options to use drawControl(), except for the text
        opt.text.clear();
        opt.features = QStyleOptionViewItem::None;
        drawBasicItemViewItem(*painter, opt);

        // draw text
        QRectF textRect = option.rect;
        textRect.setWidth(textRect.width() - 20);
        QTextOption textOption;
        textOption.setAlignment(opt.displayAlignment);
        setupPainterToDrawViewItemText(painter, opt);
        painter->drawText(textRect, displayText(index.data(Qt::DisplayRole), option.locale), textOption);

        // draw buttons
        if (index.data(SyncthingDeviceModel::IsThisDevice).toBool()) {
            return;
        }
        const int buttonY = option.rect.y() + centerObj(option.rect.height(), 16);
        QtForkAwesome::Renderer::global().render(
            index.data(SyncthingDeviceModel::DevicePaused).toBool() ? QtForkAwesome::Icon::Play : QtForkAwesome::Icon::Pause, painter,
            QRect(option.rect.right() - 16, buttonY, 16, 16), QGuiApplication::palette().color(QPalette::Text));
    }
}
} // namespace QtGui
