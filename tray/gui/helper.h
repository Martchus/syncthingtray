#ifndef TRAY_GUI_HELPER_H
#define TRAY_GUI_HELPER_H

#include <QtGlobal>

QT_FORWARD_DECLARE_CLASS(QPoint)
QT_FORWARD_DECLARE_CLASS(QTreeView)
QT_FORWARD_DECLARE_CLASS(QMenu)

namespace QtGui {

void showViewMenu(const QPoint &position, const QTreeView &view, QMenu &menu);

} // namespace QtGui

#endif // TRAY_GUI_HELPER_H
