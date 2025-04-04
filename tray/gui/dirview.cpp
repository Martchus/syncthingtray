#include "./dirview.h"
#include "./dirbuttonsitemdelegate.h"

#include <syncthingconnector/syncthingconnection.h>
#include <syncthingmodel/syncthingdirectorymodel.h>
#include <syncthingmodel/syncthingsortfiltermodel.h>
#include <syncthingwidgets/misc/direrrorsdialog.h>
#include <syncthingwidgets/settings/settings.h>

#include <QClipboard>
#include <QGuiApplication>
#include <QHeaderView>
#include <QMenu>
#include <QMouseEvent>

using namespace Data;

namespace QtGui {

DirView::DirView(QWidget *parent)
    : BasicTreeView(parent)
{
    header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    header()->hide();
    setItemDelegate(new DirButtonsItemDelegate(this));
    connect(this, &BasicTreeView::customContextMenuRequested, this, &DirView::showContextMenu);
}

void DirView::mouseReleaseEvent(QMouseEvent *event)
{
    BasicTreeView::mouseReleaseEvent(event);
    if (event->button() != Qt::LeftButton) {
        return;
    }

    const auto pos = event->pos();
    const auto clickedRow = ClickedRow(this, pos);
    if (!clickedRow) {
        return;
    }

    if (!clickedRow.index.parent().isValid()) {
        // open/scan dir buttons
        const QRect itemRect = visualRect(clickedRow.proxyIndex);
        if (pos.x() <= itemRect.right() - (listItemIconsSize(2) + listItemSpacing / 2)) {
            return;
        }
        if (pos.x() < itemRect.right() - (listItemIconsSize(1) + listItemIconSpacing / 2)) {
            if (!clickedRow.data->paused) {
                emit scanDir(*clickedRow.data);
            }
        } else if (pos.x() < itemRect.right() - (listItemIconsSize(0) + listItemIconSpacing / 2)) {
            emit pauseResumeDir(*clickedRow.data);
        } else {
            emit openDir(*clickedRow.data);
        }
    } else if (clickedRow.index.row() == 10 && clickedRow.data->pullErrorCount) {
        auto &connection(*clickedRow.model->connection());
        connection.requestDirPullErrors(clickedRow.data->id);

        auto *const textViewDlg = new DirectoryErrorsDialog(connection, *clickedRow.data);
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
        connect(menu.addAction(QIcon::fromTheme(QStringLiteral("document-open-remote"),
                                   QIcon(QStringLiteral(":/icons/hicolor/scalable/places/document-open-remote.svg"))),
                    tr("Browse remote files")),
            &QAction::triggered, triggerActionForSelectedRow(this, &DirView::browseRemoteFiles));
        connect(menu.addAction(QIcon::fromTheme(QStringLiteral("document-edit")), tr("Show/edit ignore patterns")), &QAction::triggered,
            triggerActionForSelectedRow(this, &DirView::showIgnorePatterns));
    }
    showViewMenu(position, *this, menu);
}

} // namespace QtGui
