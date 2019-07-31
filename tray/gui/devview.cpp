#include "./devview.h"
#include "./devbuttonsitemdelegate.h"

#include "../../model/syncthingdevicemodel.h"

#include <QClipboard>
#include <QGuiApplication>
#include <QHeaderView>
#include <QMenu>
#include <QMouseEvent>

using namespace Data;

namespace QtGui {

DevView::DevView(QWidget *parent)
    : QTreeView(parent)
{
    header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    header()->hide();
    setItemDelegateForColumn(1, new DevButtonsItemDelegate(this));
    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &DevView::customContextMenuRequested, this, &DevView::showContextMenu);
}

void DevView::mouseReleaseEvent(QMouseEvent *event)
{
    QTreeView::mouseReleaseEvent(event);
    const auto *const devModel = qobject_cast<const SyncthingDeviceModel *>(model());
    if (!devModel) {
        return;
    }
    const QPoint pos(event->pos());
    const QModelIndex clickedIndex(indexAt(event->pos()));
    if (!clickedIndex.isValid() || clickedIndex.column() != 1 || clickedIndex.parent().isValid()) {
        return;
    }
    const SyncthingDev *const devInfo = devModel->devInfo(clickedIndex);
    if (!devInfo) {
        return;
    }
    const QRect itemRect(visualRect(clickedIndex));
    if (pos.x() > itemRect.right() - 17) {
        emit pauseResumeDev(*devInfo);
    }
}

void DevView::showContextMenu(const QPoint &position)
{
    if (!selectionModel() || selectionModel()->selectedRows(0).size() != 1) {
        return;
    }
    QMenu menu(this);
    if (selectionModel()->selectedRows(0).at(0).parent().isValid()) {
        connect(menu.addAction(QIcon::fromTheme(QStringLiteral("edit-copy"), QIcon(QStringLiteral(":/icons/hicolor/scalable/actions/edit-copy.svg"))),
                    tr("Copy value")),
            &QAction::triggered, this, &DevView::copySelectedItem);
    } else {
        connect(menu.addAction(QIcon::fromTheme(QStringLiteral("edit-copy"), QIcon(QStringLiteral(":/icons/hicolor/scalable/actions/edit-copy.svg"))),
                    tr("Copy name")),
            &QAction::triggered, this, &DevView::copySelectedItem);
        connect(menu.addAction(QIcon::fromTheme(QStringLiteral("edit-copy"), QIcon(QStringLiteral(":/icons/hicolor/scalable/actions/edit-copy.svg"))),
                    tr("Copy ID")),
            &QAction::triggered, this, &DevView::copySelectedItemId);
    }

    // map the coordinates to top-level widget if it is a QMenu (not sure why this is required)
    const auto *const topLevelWidget = this->topLevelWidget();
    if (qobject_cast<const QMenu *>(topLevelWidget)) {
        menu.exec(topLevelWidget->mapToGlobal(position));
    } else {
        menu.exec(viewport()->mapToGlobal(position));
    }
}

void DevView::copySelectedItem()
{
    if (!selectionModel() || selectionModel()->selectedRows(0).size() != 1) {
        return;
    }
    const QModelIndex selectedIndex = selectionModel()->selectedRows(0).at(0);
    QString text;
    if (selectedIndex.parent().isValid()) {
        // dev attribute
        text = model()->data(model()->index(selectedIndex.row(), 1, selectedIndex.parent())).toString();
    } else {
        // dev name/id
        text = model()->data(selectedIndex).toString();
    }
    if (!text.isEmpty()) {
        QGuiApplication::clipboard()->setText(text);
    }
}

void DevView::copySelectedItemId()
{
    if (!selectionModel() || selectionModel()->selectedRows(0).size() != 1) {
        return;
    }
    const QModelIndex selectedIndex = selectionModel()->selectedRows(0).at(0);
    QString text;
    if (selectedIndex.parent().isValid()) {
        // dev attribute: should be handled by copySelectedItemId()
    } else {
        // dev name/id
        text = model()->data(model()->index(0, 1, selectedIndex)).toString();
    }
    if (!text.isEmpty()) {
        QGuiApplication::clipboard()->setText(text);
    }
}
} // namespace QtGui
