#include "./dirbuttonsitemdelegate.h"

#include "./helper.h"

#include <syncthingmodel/syncthingdirectorymodel.h>
#include <syncthingmodel/syncthingicons.h>

#include <qtforkawesome/icon.h>
#include <qtforkawesome/renderer.h>

#include <QPainter>
#include <QPalette>
#include <QStyle>
#include <QStyleOptionViewItem>

using namespace Data;

namespace QtGui {

DirButtonsItemDelegate::DirButtonsItemDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

void DirButtonsItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    auto opt = option;
    initStyleOption(&opt, index);
    opt.viewItemPosition = QStyleOptionViewItem::OnlyOne; // avoid visual separation between columns

    if (index.parent().isValid()) {
        drawField(this, painter, opt, index, SyncthingDirectoryModel::DirectoryDetail);
    } else {
        drawIdAndStatus(this, painter, opt, index, SyncthingDirectoryModel::DirectoryStatusString, SyncthingDirectoryModel::DirectoryStatusColor, 58);

        // draw buttons
        const int buttonY = option.rect.y() + centerObj(option.rect.height(), iconSize);
        const bool dirPaused = index.data(SyncthingDirectoryModel::DirectoryPaused).toBool();
        const auto iconColor = QGuiApplication::palette().color(QPalette::Text);
        auto &forkAwesomeRenderer = QtForkAwesome::Renderer::global();
        if (!dirPaused) {
            forkAwesomeRenderer.render(QtForkAwesome::Icon::Refresh, painter, QRect(option.rect.right() - 52, buttonY, iconSize, iconSize), iconColor);
        }
        forkAwesomeRenderer.render(
            dirPaused ? QtForkAwesome::Icon::Play : QtForkAwesome::Icon::Pause, painter, QRect(option.rect.right() - 34, buttonY, iconSize, iconSize), iconColor);
        forkAwesomeRenderer.render(QtForkAwesome::Icon::Folder, painter, QRect(option.rect.right() - iconSize, buttonY, iconSize, iconSize), iconColor);
    }
}
} // namespace QtGui
