#include "./helper.h"

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0) && QT_VERSION < QT_VERSION_CHECK(6, 3, 0)
#include <QLibraryInfo>
#endif

#include <QApplication>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
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

BasicTreeView::BasicTreeView(QWidget *parent)
    : QTreeView(parent)
{
    // emit customContextMenuRequested() manually because as of Qt 6.8 it otherwise does not work when the widget is
    // a child of a popup
    setContextMenuPolicy(Qt::NoContextMenu);
}

void BasicTreeView::mouseReleaseEvent(QMouseEvent *event)
{
    QTreeView::mouseReleaseEvent(event);
    if (event->button() == Qt::RightButton) {
        emit customContextMenuRequested(event->pos());
    }
}

/*!
 * \brief Shows \a menu at the specified \a position for the specified \a view.
 * \remarks
 * - Maps \a position to the viewport of \a view for correct positioning for mouse events.
 * - The hack to map coordinates to top-level widget (to workaround behavior fixed by Qt commit
 *   d0b5adb3b28bf5b9d94ef46cecf402994e7c5b38) is no longer necassary as the code no uses mouse
 *   events directly (instead of the context menu event).
 */
void showViewMenu(const QPoint &position, const QTreeView &view, QMenu &menu)
{
    menu.exec(view.viewport()->mapToGlobal(position));
}

void drawBasicItemViewItem(QPainter &painter, const QStyleOptionViewItem &option)
{
    if (auto *const style = option.widget ? option.widget->style() : QApplication::style()) {
        style->drawControl(QStyle::CE_ItemViewItem, &option, &painter, option.widget);
    }
}

void setupPainterToDrawViewItemText(QPainter *painter, QStyleOptionViewItem &opt)
{
    painter->setFont(opt.font);

    if (!(opt.state & QStyle::State_Selected)) {
        painter->setPen(opt.palette.color(QPalette::Text));
        return;
    }

    // set pen/palette in accordance with the Windows 11 or Windows Vista style for selected items
    // note: These styles unfortunately don't just use the highlighted text color and just using it would
    //       lead to a very bad contrast.
#if defined(Q_OS_WINDOWS) && QT_VERSION >= QT_VERSION_CHECK(6, 1, 0)
    auto *const style = opt.widget ? opt.widget->style() : nullptr;
    const auto styleName = style ? style->name() : QString();
    if (styleName.compare(QLatin1String("windows11"), Qt::CaseInsensitive) == 0) {
        painter->setPen(QPen(opt.palette.buttonText().color()));
        return;
    } else if (styleName.compare(QLatin1String("windowsvista"), Qt::CaseInsensitive) == 0) {
        opt.palette.setColor(QPalette::All, QPalette::HighlightedText, opt.palette.color(QPalette::Active, QPalette::Text));
        opt.palette.setColor(QPalette::All, QPalette::Highlight, opt.palette.base().color().darker(108));
    }
#endif

    painter->setPen(opt.palette.color(QPalette::HighlightedText));
}

} // namespace QtGui
