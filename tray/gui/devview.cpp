#include "./devview.h"
#include "./devbuttonsitemdelegate.h"

#include "../../model/syncthingdevicemodel.h"

#include <QClipboard>
#include <QCursor>
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
    if (const auto *devModel = qobject_cast<SyncthingDeviceModel *>(model())) {
        const QPoint pos(event->pos());
        const QModelIndex clickedIndex(indexAt(event->pos()));
        if (clickedIndex.isValid() && clickedIndex.column() == 1 && !clickedIndex.parent().isValid()) {
            if (const SyncthingDev *devInfo = devModel->devInfo(clickedIndex)) {
                const QRect itemRect(visualRect(clickedIndex));
                if (pos.x() > itemRect.right() - 17) {
                    emit pauseResumeDev(*devInfo);
                }
            }
        }
    }
}

void DevView::showContextMenu()
{
    if (selectionModel() && selectionModel()->selectedRows(0).size() == 1) {
        QMenu menu;
        if (selectionModel()->selectedRows(0).at(0).parent().isValid()) {
            connect(
                menu.addAction(QIcon::fromTheme(QStringLiteral("edit-copy"), QIcon(QStringLiteral(":/icons/hicolor/scalable/actions/edit-copy.svg"))),
                    tr("Copy value")),
                &QAction::triggered, this, &DevView::copySelectedItem);
        } else {
            connect(
                menu.addAction(QIcon::fromTheme(QStringLiteral("edit-copy"), QIcon(QStringLiteral(":/icons/hicolor/scalable/actions/edit-copy.svg"))),
                    tr("Copy name")),
                &QAction::triggered, this, &DevView::copySelectedItem);
            connect(
                menu.addAction(QIcon::fromTheme(QStringLiteral("edit-copy"), QIcon(QStringLiteral(":/icons/hicolor/scalable/actions/edit-copy.svg"))),
                    tr("Copy ID")),
                &QAction::triggered, this, &DevView::copySelectedItemId);
        }
        menu.exec(QCursor::pos());
    }
}

void DevView::copySelectedItem()
{
    if (selectionModel() && selectionModel()->selectedRows(0).size() == 1) {
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
}

void DevView::copySelectedItemId()
{
    if (selectionModel() && selectionModel()->selectedRows(0).size() == 1) {
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
}
}
