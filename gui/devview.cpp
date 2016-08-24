#include "./devview.h"

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
    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &DevView::customContextMenuRequested, this, &DevView::showContextMenu);
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
