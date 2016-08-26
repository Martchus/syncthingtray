#include "./devview.h"
#include "./devbuttonsitemdelegate.h"

#include <QHeaderView>
#include <QMouseEvent>
#include <QMenu>
#include <QCursor>
#include <QGuiApplication>
#include <QClipboard>

namespace QtGui {

DevView::DevView(QWidget *parent) :
    QTreeView(parent)
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
    const QPoint pos(event->pos());
    const QModelIndex clickedIndex(indexAt(event->pos()));
    if(clickedIndex.isValid() && clickedIndex.column() == 1 && !clickedIndex.parent().isValid()) {
        const QRect itemRect(visualRect(clickedIndex));
        //if(pos.x() > itemRect.right() - 34) {
            if(pos.x() > itemRect.right() - 17) {
                emit pauseResumeDev(clickedIndex);
        //    } else {
        //        emit scanDir(clickedIndex);
        //    }
        }
    }
}

void DevView::showContextMenu()
{
    if(selectionModel() && selectionModel()->selectedRows(0).size() == 1) {
        QMenu menu;
        connect(menu.addAction(QIcon::fromTheme(QStringLiteral("edit-copy")), tr("Copy")), &QAction::triggered, this, &DevView::copySelectedItem);
        menu.exec(QCursor::pos());
    }
}

void DevView::copySelectedItem()
{
    if(selectionModel() && selectionModel()->selectedRows(0).size() == 1) {
        const QModelIndex selectedIndex = selectionModel()->selectedRows(0).at(0);
        QString text;
        if(selectedIndex.parent().isValid()) {
            // dev attribute
            text = model()->data(model()->index(selectedIndex.row(), 1, selectedIndex.parent())).toString();
        } else {
            // dev name/id
            text = model()->data(selectedIndex).toString();
        }
        if(!text.isEmpty()) {
            QGuiApplication::clipboard()->setText(text);
        }
    }
}

}
