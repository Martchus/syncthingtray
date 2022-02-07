#include "./helper.h"

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0) && QT_VERSION < QT_VERSION_CHECK(6, 3, 0)
#include <QLibraryInfo>
#endif
#include <QMenu>
#include <QPoint>
#include <QTreeView>

namespace QtGui {

void showViewMenu(const QPoint &position, const QTreeView &view, QMenu &menu)
{
    // map the coordinates to top-level widget if it is a QMenu
    // note: This necessity is actually considered a bug which is fixed by d0b5adb3b28bf5b9d94ef46cecf402994e7c5b38 which is
    //       part of Qt 6.2.3 and later.
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    constexpr auto needsHack = true;
#elif QT_VERSION >= QT_VERSION_CHECK(6, 3, 0)
    constexpr auto needsHack = false;
#else
    static const auto needsHack = QLibraryInfo::version() < QVersionNumber(6, 2, 3);
#endif
    const QMenu *topLevelWidget;
    if (needsHack && (topLevelWidget = qobject_cast<const QMenu *>(view.topLevelWidget()))) {
        menu.exec(topLevelWidget->mapToGlobal(position));
    } else {
        menu.exec(view.viewport()->mapToGlobal(position));
    }
}

} // namespace QtGui
