#include "./dirview.h"
#include "./dirbuttonsitemdelegate.h"
#include "./helper.h"

#include "../../connector/syncthingconnection.h"
#include "../../model/syncthingdirectorymodel.h"
#include "../../model/syncthingsortfilterdirectorymodel.h"
#include "../../widgets/misc/direrrorsdialog.h"

#include <QClipboard>
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

    // get SyncthingDir object for clicked index
    auto *const sortDirModel = qobject_cast<SortFilterModelType *>(model());
    auto *const dirModel = qobject_cast<ModelType *>(sortDirModel ? sortDirModel->sourceModel() : model());
    if (!dirModel) {
        return;
    }
    const auto pos = event->pos();
    const auto clickedProxyIndex = indexAt(event->pos());
    const auto clickedIndex = sortDirModel ? sortDirModel->mapToSource(clickedProxyIndex) : clickedProxyIndex;
    if (!clickedIndex.isValid() || clickedIndex.column() != 1) {
        return;
    }
    const auto *const dir = dirModel->dirInfo(clickedIndex);
    if (!dir) {
        return;
    }

    if (!clickedIndex.parent().isValid()) {
        // open/scan dir buttons
        const QRect itemRect = visualRect(clickedProxyIndex);
        if (pos.x() <= itemRect.right() - 58) {
            return;
        }
        if (pos.x() < itemRect.right() - 34) {
            if (!dir->paused) {
                emit scanDir(*dir);
            }
        } else if (pos.x() < itemRect.right() - 17) {
            emit pauseResumeDir(*dir);
        } else {
            emit openDir(*dir);
        }
    } else if (clickedIndex.row() == 9 && dir->pullErrorCount) {
        auto &connection(*dirModel->connection());
        connection.requestDirPullErrors(dir->id);

        auto *const textViewDlg = new DirectoryErrorsDialog(connection, *dir);
        textViewDlg->setAttribute(Qt::WA_DeleteOnClose);
        textViewDlg->show();
    }
}

void DirView::showContextMenu(const QPoint &position)
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
            &QAction::triggered,
            copyToClipboard(selectedRow.model->data(selectedRow.model->index(selectedIndex.row(), 1, selectedIndex.parent())).toString()));
    } else {
        const auto *const dir = selectedRow.data;
        connect(menu.addAction(QIcon::fromTheme(QStringLiteral("edit-copy"), QIcon(QStringLiteral(":/icons/hicolor/scalable/actions/edit-copy.svg"))),
                    tr("Copy label/ID")),
            &QAction::triggered, copyToClipboard(dir->displayName()));
        connect(menu.addAction(QIcon::fromTheme(QStringLiteral("edit-copy"), QIcon(QStringLiteral(":/icons/hicolor/scalable/actions/edit-copy.svg"))),
                    tr("Copy path")),
            &QAction::triggered, copyToClipboard(dir->path));
        menu.addSeparator();
        connect(menu.addAction(
                    QIcon::fromTheme(QStringLiteral("view-refresh"), QIcon(QStringLiteral(":/icons/hicolor/scalable/actions/view-refresh.svg"))),
                    tr("Rescan")),
            &QAction::triggered, triggerActionForSelectedRow(this, &DirView::scanDir));
        if (dir->paused) {
            connect(menu.addAction(QIcon::fromTheme(QStringLiteral("media-playback-start"),
                                       QIcon(QStringLiteral(":/icons/hicolor/scalable/actions/media-playback-start.svg"))),
                        tr("Resume")),
                &QAction::triggered, triggerActionForSelectedRow(this, &DirView::pauseResumeDir));
        } else {
            connect(menu.addAction(QIcon::fromTheme(QStringLiteral("media-playback-pause"),
                                       QIcon(QStringLiteral(":/icons/hicolor/scalable/actions/media-playback-pause.svg"))),
                        tr("Pause")),
                &QAction::triggered, triggerActionForSelectedRow(this, &DirView::pauseResumeDir));
        }
        connect(menu.addAction(QIcon::fromTheme(QStringLiteral("folder"), QIcon(QStringLiteral(":/icons/hicolor/scalable/places/folder-open.svg"))),
                    tr("Open in file browser")),
            &QAction::triggered, triggerActionForSelectedRow(this, &DirView::openDir));
    }
    showViewMenu(position, *this, menu);
}

} // namespace QtGui
