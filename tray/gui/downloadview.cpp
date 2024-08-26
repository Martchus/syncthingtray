#include "./downloadview.h"
#include "./downloaditemdelegate.h"

#include <syncthingconnector/syncthingdir.h>
#include <syncthingmodel/syncthingdownloadmodel.h>

#include <QClipboard>
#include <QGuiApplication>
#include <QHeaderView>
#include <QMenu>
#include <QMouseEvent>

using namespace Data;

namespace QtGui {

DownloadView::DownloadView(QWidget *parent)
    : BasicTreeView(parent)
{
    header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    header()->hide();
    setItemDelegateForColumn(0, new DownloadItemDelegate(this));
    connect(this, &BasicTreeView::customContextMenuRequested, this, &DownloadView::showContextMenu);
}

void DownloadView::mouseReleaseEvent(QMouseEvent *event)
{
    BasicTreeView::mouseReleaseEvent(event);
    if (event->button() != Qt::LeftButton) {
        return;
    }

    const auto *const dlModel = qobject_cast<const SyncthingDownloadModel *>(model());
    if (!dlModel) {
        return;
    }
    const auto pos = event->pos();
    const auto clickedIndex(indexAt(pos));
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
    const auto selectedRow = SelectedRow(this);
    const auto &selectedIndex = selectedRow.index;
    if (!selectedRow) {
        return;
    }
    QMenu menu(this);
    if (selectedIndex.parent().isValid()) {
        connect(menu.addAction(QIcon::fromTheme(QStringLiteral("edit-copy"), QIcon(QStringLiteral(":/icons/hicolor/scalable/actions/edit-copy.svg"))),
                    tr("Copy value")),
            &QAction::triggered, copyToClipboard(model()->data(model()->index(selectedIndex.row(), 1, selectedIndex.parent())).toString()));
    } else {
        const auto [dir, progress] = selectedRow.data;
        connect(menu.addAction(QIcon::fromTheme(QStringLiteral("edit-copy"), QIcon(QStringLiteral(":/icons/hicolor/scalable/actions/edit-copy.svg"))),
                    tr("Copy label/ID")),
            &QAction::triggered, copyToClipboard(dir->displayName()));
        menu.addSeparator();
        connect(menu.addAction(QIcon::fromTheme(QStringLiteral("folder"), QIcon(QStringLiteral(":/icons/hicolor/scalable/places/folder-open.svg"))),
                    tr("Open in file browser")),
            &QAction::triggered, triggerActionForSelectedRow(this, &DownloadView::emitOpenDir));
    }
    showViewMenu(position, *this, menu);
}

void DownloadView::emitOpenDir(QPair<const SyncthingDir *, const SyncthingItemDownloadProgress *> info)
{
    if (info.second) {
        emit openItemDir(*info.second);
    } else {
        emit openDir(*info.first);
    }
}

} // namespace QtGui
