#include "./helper.h"

#include <QMenu>
#include <QPoint>
#include <QTreeView>

namespace QtGui {

void showViewMenu(const QPoint &position, const QTreeView &view, QMenu &menu)
{
    // map the coordinates to top-level widget if it is a QMenu (not sure why this is required)
    const auto *const topLevelWidget = view.topLevelWidget();
    if (qobject_cast<const QMenu *>(topLevelWidget)) {
        menu.exec(topLevelWidget->mapToGlobal(position));
    } else {
        menu.exec(view.viewport()->mapToGlobal(position));
    }
}

} // namespace QtGui
