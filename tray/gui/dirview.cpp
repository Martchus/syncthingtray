#include "./dirview.h"
#include "./dirbuttonsitemdelegate.h"
#include "./textviewdialog.h"

#include "../../model/syncthingdirectorymodel.h"
#include "../../connector/syncthingconnection.h"

#include <QHeaderView>
#include <QMouseEvent>
#include <QMenu>
#include <QCursor>
#include <QGuiApplication>
#include <QClipboard>
#include <QTextBrowser>
#include <QStringBuilder>

using namespace Data;

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
    if(const SyncthingDirectoryModel *dirModel = qobject_cast<SyncthingDirectoryModel *>(model())) {
        const QPoint pos(event->pos());
        const QModelIndex clickedIndex(indexAt(event->pos()));
        if(clickedIndex.isValid() && clickedIndex.column() == 1) {
            if(const SyncthingDir *dir = dirModel->dirInfo(clickedIndex)) {
                if(!clickedIndex.parent().isValid()) {
                    // open/scan dir buttons
                    const QRect itemRect(visualRect(clickedIndex));
                    if(pos.x() > itemRect.right() - 34) {
                        if(pos.x() > itemRect.right() - 17) {
                            emit openDir(*dir);
                        } else {
                            emit scanDir(*dir);
                        }
                    }
                } else if(clickedIndex.row() == 7) {
                    // show errors
                    auto *textViewDlg = new TextViewDialog(tr("Errors of %1").arg(dir->label.isEmpty() ? dir->id : dir->label));
                    auto *browser = textViewDlg->browser();
                    for(const SyncthingDirError &error : dir->errors) {
                        browser->append(error.path % QChar(':') % QChar(' ') % QChar('\n') % error.message % QChar('\n'));
                    }
                    textViewDlg->show();
                }
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
