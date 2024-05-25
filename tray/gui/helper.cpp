#include "./helper.h"

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0) && QT_VERSION < QT_VERSION_CHECK(6, 3, 0)
#include <QLibraryInfo>
#endif

#include <QApplication>
#include <QMenu>
#include <QPoint>
#include <QStyleOptionViewItem>
#include <QTreeView>

namespace QtGui {

/*!
 * \class UnifiedItemDelegate
 * \brief The UnifiedItemDelegate class draws view items without visual separation.
 *
 * This style sets the view item position to "OnlyOne" to prevent styles from drawing separations
 * too noisily between items. It is used to achieve a cleaner look of the directory/devices tree
 * view.
 *
 * \remarks
 * The main motivation for this is the Windows 11 style which otherwise draws vertical lines between
 * the columns which does not look nice at all in these tree views.
 */

UnifiedItemDelegate::UnifiedItemDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

void UnifiedItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    auto opt = option;
    initStyleOption(&opt, index);
    opt.viewItemPosition = QStyleOptionViewItem::OnlyOne;
    QStyledItemDelegate::paint(painter, opt, index);
}

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
    if (needsHack && (topLevelWidget = qobject_cast<const QMenu *>(view.topLevelWidget()))
        && (topLevelWidget->windowFlags() & Qt::Popup) == Qt::Popup) {
        menu.exec(topLevelWidget->mapToGlobal(position));
    } else {
        menu.exec(view.viewport()->mapToGlobal(position));
    }
}

void drawBasicItemViewItem(QPainter &painter, const QStyleOptionViewItem &option)
{
    if (auto *const style = option.widget ? option.widget->style() : QApplication::style()) {
        style->drawControl(QStyle::CE_ItemViewItem, &option, &painter, option.widget);
    }
}

} // namespace QtGui
