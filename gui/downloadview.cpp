#include "./downloadview.h"
#include "./downloaditemdelegate.h"

#include "../data/syncthingdownloadmodel.h"

#include <QHeaderView>
#include <QMouseEvent>
#include <QMenu>
#include <QCursor>
#include <QGuiApplication>
#include <QClipboard>

using namespace Data;

namespace QtGui {

DownloadView::DownloadView(QWidget *parent) :
    QTreeView(parent)
{
    header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    header()->hide();
    setItemDelegateForColumn(0, new DownloadItemDelegate(this));
    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &DownloadView::customContextMenuRequested, this, &DownloadView::showContextMenu);
}

void DownloadView::mouseReleaseEvent(QMouseEvent *event)
{
    QTreeView::mouseReleaseEvent(event);
    if(const SyncthingDownloadModel *dlModel = qobject_cast<SyncthingDownloadModel *>(model())) {
        const QPoint pos(event->pos());
        const QModelIndex clickedIndex(indexAt(event->pos()));
        if(clickedIndex.isValid() && clickedIndex.column() == 0) {
            const QRect itemRect(visualRect(clickedIndex));
            if(pos.x() > itemRect.right() - 17) {
                if(clickedIndex.parent().isValid()) {
                    if(pos.y() < itemRect.y() + itemRect.height() / 2) {
                        if(const SyncthingItemDownloadProgress *progress = dlModel->progressInfo(clickedIndex)) {
                            emit openItemDir(*progress);
                        }
                    }
                } else if(const SyncthingDir *dir = dlModel->dirInfo(clickedIndex)) {
                    emit openDir(*dir);
                }
            }
        }
    }
}

void DownloadView::showContextMenu()
{
    if(selectionModel() && selectionModel()->selectedRows(0).size() == 1) {
        QMenu menu;
        if(selectionModel()->selectedRows(0).at(0).parent().isValid()) {
            connect(menu.addAction(QIcon::fromTheme(QStringLiteral("edit-copy"), QIcon(QStringLiteral(":/icons/hicolor/scalable/actions/edit-copy.svg"))), tr("Copy value")), &QAction::triggered, this, &DownloadView::copySelectedItem);
        } else {
            connect(menu.addAction(QIcon::fromTheme(QStringLiteral("edit-copy"), QIcon(QStringLiteral(":/icons/hicolor/scalable/actions/edit-copy.svg"))), tr("Copy label/ID")), &QAction::triggered, this, &DownloadView::copySelectedItem);
        }
        menu.exec(QCursor::pos());
    }
}

void DownloadView::copySelectedItem()
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

}
