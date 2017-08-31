#include "./dirview.h"
#include "./dirbuttonsitemdelegate.h"

#include "../../connector/syncthingconnection.h"
#include "../../model/syncthingdirectorymodel.h"
#include "../../widgets/misc/textviewdialog.h"

#include <QClipboard>
#include <QCursor>
#include <QGuiApplication>
#include <QHeaderView>
#include <QMenu>
#include <QMouseEvent>

using namespace Data;

namespace QtGui {

DirView::DirView(QWidget *parent)
    : QTreeView(parent)
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
    if (const SyncthingDirectoryModel *dirModel = qobject_cast<const SyncthingDirectoryModel *>(model())) {
        const QPoint pos(event->pos());
        const QModelIndex clickedIndex(indexAt(event->pos()));
        if (clickedIndex.isValid() && clickedIndex.column() == 1) {
            if (const SyncthingDir *dir = dirModel->dirInfo(clickedIndex)) {
                if (!clickedIndex.parent().isValid()) {
                    // open/scan dir buttons
                    const QRect itemRect(visualRect(clickedIndex));
                    if (pos.x() > itemRect.right() - 58) {
                        if (pos.x() < itemRect.right() - 34) {
                            if (!dir->paused) {
                                emit scanDir(*dir);
                            }
                        } else if (pos.x() < itemRect.right() - 17) {
                            emit pauseResumeDir(*dir);
                        } else {
                            emit openDir(*dir);
                        }
                    }
                } else if (clickedIndex.row() == 9 && !dir->itemErrors.empty()) {
                    auto *const textViewDlg = TextViewDialog::forDirectoryErrors(*dir);
                    textViewDlg->setAttribute(Qt::WA_DeleteOnClose);
                    textViewDlg->show();
                }
            }
        }
    }
}

void DirView::showContextMenu()
{
    if (selectionModel() && selectionModel()->selectedRows(0).size() == 1) {
        QMenu menu;
        if (selectionModel()->selectedRows(0).at(0).parent().isValid()) {
            connect(
                menu.addAction(QIcon::fromTheme(QStringLiteral("edit-copy"), QIcon(QStringLiteral(":/icons/hicolor/scalable/actions/edit-copy.svg"))),
                    tr("Copy value")),
                &QAction::triggered, this, &DirView::copySelectedItem);
        } else {
            connect(
                menu.addAction(QIcon::fromTheme(QStringLiteral("edit-copy"), QIcon(QStringLiteral(":/icons/hicolor/scalable/actions/edit-copy.svg"))),
                    tr("Copy label/ID")),
                &QAction::triggered, this, &DirView::copySelectedItem);
            connect(
                menu.addAction(QIcon::fromTheme(QStringLiteral("edit-copy"), QIcon(QStringLiteral(":/icons/hicolor/scalable/actions/edit-copy.svg"))),
                    tr("Copy path")),
                &QAction::triggered, this, &DirView::copySelectedItemPath);
        }
        menu.exec(QCursor::pos());
    }
}

void DirView::copySelectedItem()
{
    if (selectionModel() && selectionModel()->selectedRows(0).size() == 1) {
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
}

void DirView::copySelectedItemPath()
{
    if (selectionModel() && selectionModel()->selectedRows(0).size() == 1) {
        const QModelIndex selectedIndex = selectionModel()->selectedRows(0).at(0);
        QString text;
        if (selectedIndex.parent().isValid()) {
            // dev attribute: should be handled by copySelectedItem() only
        } else {
            // dev path
            text = model()->data(model()->index(1, 1, selectedIndex)).toString();
        }
        if (!text.isEmpty()) {
            QGuiApplication::clipboard()->setText(text);
        }
    }
}
}
