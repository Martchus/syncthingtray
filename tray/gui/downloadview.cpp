#include "./downloadview.h"
#include "./downloaditemdelegate.h"
#include "./helper.h"

#include "../../model/syncthingdownloadmodel.h"

#include <QClipboard>
#include <QGuiApplication>
#include <QHeaderView>
#include <QMenu>
#include <QMouseEvent>

using namespace Data;

namespace QtGui {

DownloadView::DownloadView(QWidget *parent)
    : QTreeView(parent)
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
    const auto *const dlModel = qobject_cast<const SyncthingDownloadModel *>(model());
    if (!dlModel) {
        return;
    }
    const QPoint pos(event->pos());
    const QModelIndex clickedIndex(indexAt(event->pos()));
    if (!clickedIndex.isValid() || clickedIndex.column() != 0) {
        return;
    }
    const QRect itemRect(visualRect(clickedIndex));
    if (pos.x() <= itemRect.right() - 17) {
        return;
    }
    if (clickedIndex.parent().isValid()) {
        if (pos.y() < itemRect.y() + itemRect.height() / 2) {
            if (const SyncthingItemDownloadProgress *const progress = dlModel->progressInfo(clickedIndex)) {
                emit openItemDir(*progress);
            }
        }
    } else if (const SyncthingDir *const dir = dlModel->dirInfo(clickedIndex)) {
        emit openDir(*dir);
    }
}

void DownloadView::showContextMenu(const QPoint &position)
{
    if (!selectionModel() || selectionModel()->selectedRows(0).size() != 1) {
        return;
    }
    QMenu menu(this);
    if (selectionModel()->selectedRows(0).at(0).parent().isValid()) {
        connect(menu.addAction(QIcon::fromTheme(QStringLiteral("edit-copy"), QIcon(QStringLiteral(":/icons/hicolor/scalable/actions/edit-copy.svg"))),
                    tr("Copy value")),
            &QAction::triggered, this, &DownloadView::copySelectedItem);
    } else {
        connect(menu.addAction(QIcon::fromTheme(QStringLiteral("edit-copy"), QIcon(QStringLiteral(":/icons/hicolor/scalable/actions/edit-copy.svg"))),
                    tr("Copy label/ID")),
            &QAction::triggered, this, &DownloadView::copySelectedItem);
    }
    showViewMenu(position, *this, menu);
}

void DownloadView::copySelectedItem()
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
        // dev label/id
        text = model()->data(selectedIndex).toString();
    }
    if (!text.isEmpty()) {
        QGuiApplication::clipboard()->setText(text);
    }
}
} // namespace QtGui
