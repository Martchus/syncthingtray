#include "./devbuttonsitemdelegate.h"

#include "./helper.h"

#include <syncthingconnector/syncthingconnection.h>
#include <syncthingmodel/syncthingdevicemodel.h>
#include <syncthingmodel/syncthingicons.h>

#include <qtforkawesome/icon.h>
#include <qtforkawesome/renderer.h>

#include <QPainter>
#include <QPalette>
#include <QStyle>
#include <QStyleOptionViewItem>

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
        drawIdAndStatus(this, painter, opt, index, SyncthingDeviceModel::DeviceStatusString, SyncthingDeviceModel::DeviceStatusColor, listItemIconsSize(0) + listItemSpacing);

        // draw button
        if (index.data(SyncthingDeviceModel::IsThisDevice).toBool()) {
            return;
        }
        const int buttonY = option.rect.y() + centerObj(option.rect.height(), listItemIconSize);
        QtForkAwesome::Renderer::global().render(
            index.data(SyncthingDeviceModel::DevicePaused).toBool() ? QtForkAwesome::Icon::Play : QtForkAwesome::Icon::Pause, painter,
            QRect(option.rect.right() - listItemIconsSize(0), buttonY, listItemIconSize, listItemIconSize), QGuiApplication::palette().color(QPalette::Text));
    }
}
} // namespace QtGui
