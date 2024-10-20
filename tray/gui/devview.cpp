#include "./devview.h"
#include "./devbuttonsitemdelegate.h"

#include <syncthingconnector/syncthingdev.h>
#include <syncthingmodel/syncthingdevicemodel.h>
#include <syncthingmodel/syncthingsortfiltermodel.h>

#include <QClipboard>
#include <QGuiApplication>
#include <QHeaderView>
#include <QMenu>
#include <QMouseEvent>

using namespace Data;

namespace QtGui {

DevView::DevView(QWidget *parent)
    : BasicTreeView(parent)
{
    header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    header()->hide();
    setItemDelegate(new DevButtonsItemDelegate(this));
    connect(this, &BasicTreeView::customContextMenuRequested, this, &DevView::showContextMenu);
}

void DevView::mouseReleaseEvent(QMouseEvent *event)
{
    BasicTreeView::mouseReleaseEvent(event);
    if (event->button() != Qt::LeftButton) {
        return;
    }

    const auto pos = event->pos();
    if (event->button() == Qt::RightButton) {
        showContextMenu(pos);
        return;
    }

    const auto clickedRow = ClickedRow(this, pos);
    if (!clickedRow) {
        return;
    }
    if (clickedRow.index.parent().isValid()) {
        return;
    }

    const auto itemRect = visualRect(clickedRow.proxyIndex);
    const auto &device = *clickedRow.data;
    if (device.status != SyncthingDevStatus::ThisDevice && pos.x() > itemRect.right() - 17) {
        emit pauseResumeDev(device);
    }
}

void DevView::showContextMenu(const QPoint &position)
{
    const auto selectedRow = SelectedRow(this);
    const auto &selectedIndex = selectedRow.index;
    if (!selectedRow) {
        return;
    }
    QMenu menu(this);
    if (selectedIndex.parent().isValid()) {
        connect(menu.addAction(QIcon::fromTheme(QStringLiteral("edit-copy"), QIcon(QStringLiteral(":/icons/hicolor/scalable/actions/edit-copy.svg"))),
                    tr("Copy value")),
            &QAction::triggered,
            copyToClipboard(selectedRow.model->data(selectedRow.model->index(selectedIndex.row(), 1, selectedIndex.parent())).toString()));
    } else {
        const auto *const dev = selectedRow.data;
        connect(menu.addAction(QIcon::fromTheme(QStringLiteral("edit-copy"), QIcon(QStringLiteral(":/icons/hicolor/scalable/actions/edit-copy.svg"))),
                    tr("Copy name")),
            &QAction::triggered, copyToClipboard(dev->displayName()));
        connect(menu.addAction(QIcon::fromTheme(QStringLiteral("edit-copy"), QIcon(QStringLiteral(":/icons/hicolor/scalable/actions/edit-copy.svg"))),
                    tr("Copy ID")),
            &QAction::triggered, copyToClipboard(dev->id));
        menu.addSeparator();
        if (dev->paused) {
            connect(menu.addAction(QIcon::fromTheme(QStringLiteral("media-playback-start"),
                                       QIcon(QStringLiteral(":/icons/hicolor/scalable/actions/media-playback-start.svg"))),
                        tr("Resume")),
                &QAction::triggered, triggerActionForSelectedRow(this, &DevView::pauseResumeDev));
        } else if (dev->status != SyncthingDevStatus::ThisDevice) {
            connect(menu.addAction(QIcon::fromTheme(QStringLiteral("media-playback-pause"),
                                       QIcon(QStringLiteral(":/icons/hicolor/scalable/actions/media-playback-pause.svg"))),
                        tr("Pause")),
                &QAction::triggered, triggerActionForSelectedRow(this, &DevView::pauseResumeDev));
        }
    }
    showViewMenu(position, *this, menu);
}

} // namespace QtGui
