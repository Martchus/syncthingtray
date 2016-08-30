#include "./dirview.h"
#include "./dirbuttonsitemdelegate.h"

#include <QHeaderView>
#include <QMouseEvent>
#include <QMenu>
#include <QCursor>
#include <QGuiApplication>
#include <QClipboard>

namespace QtGui {

DirView::DirView(QWidget *parent) :
    QTreeView(parent)
{
    header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    header()->hide();
    setItemDelegateForColumn(1, new DirButtonsItemDelegate(this));
    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &DirView::customContextMenuRequested, this, &DirView::showContextMenu);
}

void DirView::mouseReleaseEvent(QMouseEvent *event)
{
    QTreeView::mouseReleaseEvent(event);
    const QPoint pos(event->pos());
    const QModelIndex clickedIndex(indexAt(event->pos()));
    if(clickedIndex.isValid() && clickedIndex.column() == 1 && !clickedIndex.parent().isValid()) {
        const QRect itemRect(visualRect(clickedIndex));
        if(pos.x() > itemRect.right() - 34) {
            if(pos.x() > itemRect.right() - 17) {
                emit openDir(clickedIndex);
            } else {
                emit scanDir(clickedIndex);
            }
        }
    }
}

void DirView::showContextMenu()
{
    if(selectionModel() && selectionModel()->selectedRows(0).size() == 1) {
        QMenu menu;
        if(selectionModel()->selectedRows(0).at(0).parent().isValid()) {
            connect(menu.addAction(QIcon::fromTheme(QStringLiteral("edit-copy"), QIcon(QStringLiteral(":/icons/hicolor/scalable/actions/edit-copy.svg"))), tr("Copy value")), &QAction::triggered, this, &DirView::copySelectedItem);
        } else {
            connect(menu.addAction(QIcon::fromTheme(QStringLiteral("edit-copy"), QIcon(QStringLiteral(":/icons/hicolor/scalable/actions/edit-copy.svg"))), tr("Copy label/ID")), &QAction::triggered, this, &DirView::copySelectedItem);
            connect(menu.addAction(QIcon::fromTheme(QStringLiteral("edit-copy"), QIcon(QStringLiteral(":/icons/hicolor/scalable/actions/edit-copy.svg"))), tr("Copy path")), &QAction::triggered, this, &DirView::copySelectedItemPath);
        }
        menu.exec(QCursor::pos());
    }
}

void DirView::copySelectedItem()
{
    if(selectionModel() && selectionModel()->selectedRows(0).size() == 1) {
        const QModelIndex selectedIndex = selectionModel()->selectedRows(0).at(0);
        QString text;
        if(selectedIndex.parent().isValid()) {
            // dev attribute
            text = model()->data(model()->index(selectedIndex.row(), 1, selectedIndex.parent())).toString();
        } else {
            // dev label/id
            text = model()->data(selectedIndex).toString();
        }
        if(!text.isEmpty()) {
            QGuiApplication::clipboard()->setText(text);
        }
    }
}

void DirView::copySelectedItemPath()
{
    if(selectionModel() && selectionModel()->selectedRows(0).size() == 1) {
        const QModelIndex selectedIndex = selectionModel()->selectedRows(0).at(0);
        QString text;
        if(selectedIndex.parent().isValid()) {
            // dev attribute: should be handled by copySelectedItem() only
        } else {
            // dev path
            text = model()->data(model()->index(1, 1, selectedIndex)).toString();
        }
        if(!text.isEmpty()) {
            QGuiApplication::clipboard()->setText(text);
        }
    }
}

}
