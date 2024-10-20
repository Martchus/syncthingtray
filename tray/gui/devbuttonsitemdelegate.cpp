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
        drawField(this, painter, opt, index, SyncthingDeviceModel::DeviceDetail);
    } else {
        drawIdAndStatus(this, painter, opt, index, SyncthingDeviceModel::DeviceStatusString, SyncthingDeviceModel::DeviceStatusColor, 20);

        // draw button
        if (index.data(SyncthingDeviceModel::IsThisDevice).toBool()) {
            return;
        }
        const int buttonY = option.rect.y() + centerObj(option.rect.height(), iconSize);
        QtForkAwesome::Renderer::global().render(
            index.data(SyncthingDeviceModel::DevicePaused).toBool() ? QtForkAwesome::Icon::Play : QtForkAwesome::Icon::Pause, painter,
            QRect(option.rect.right() - iconSize, buttonY, iconSize, iconSize), QGuiApplication::palette().color(QPalette::Text));
    }
}
} // namespace QtGui
