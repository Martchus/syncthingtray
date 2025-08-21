#include "./helper.h"

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0) && QT_VERSION < QT_VERSION_CHECK(6, 3, 0)
#include <QLibraryInfo>
#endif

#include <QApplication>
#include <QFontMetrics>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QPalette>
#include <QPoint>
#include <QStyleHints>
#include <QStyleOptionViewItem>
#include <QTextOption>
#include <QTreeView>

namespace QtGui {

/*!
 * \brief Returns on what mouse event the context menu should be shown.
 */
static inline QEvent::Type contextMenuEventType()
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
    if (const auto *const hints = QGuiApplication::styleHints()) {
        switch (hints->contextMenuTrigger()) {
        case Qt::ContextMenuTrigger::Press:
            return QEvent::MouseButtonPress;
        case Qt::ContextMenuTrigger::Release:
            return QEvent::MouseButtonRelease;
        default:;
        };
    }
#endif
#ifdef Q_OS_WINDOWS
    return QEvent::MouseButtonRelease;
#else
    return QEvent::MouseButtonPress;
#endif
}

BasicTreeView::BasicTreeView(QWidget *parent)
    : QTreeView(parent)
    , m_contextMenuEventType(contextMenuEventType())
{
    // emit customContextMenuRequested() manually because as of Qt 6.8 it otherwise does not work when the widget is
    // a child of a popup
    setContextMenuPolicy(Qt::NoContextMenu);
}

/*!
 * \brief Emits customContextMenuRequested() if context menus are supposed to be shown on \a event.
 */
void BasicTreeView::handleContextMenu(QMouseEvent *event)
{
    if (event->button() == Qt::RightButton && event->type() == m_contextMenuEventType) {
        emit customContextMenuRequested(event->pos());
    }
}

void BasicTreeView::mouseReleaseEvent(QMouseEvent *event)
{
    QTreeView::mouseReleaseEvent(event);
    handleContextMenu(event);
}

void BasicTreeView::mousePressEvent(QMouseEvent *event)
{
    QTreeView::mousePressEvent(event);
    handleContextMenu(event);
}

/*!
 * \brief Shows \a menu at the specified \a position for the specified \a view.
 * \remarks
 * - Maps \a position to the viewport of \a view for correct positioning for mouse events.
 * - The hack to map coordinates to top-level widget (to workaround behavior fixed by Qt commit
 *   d0b5adb3b28bf5b9d94ef46cecf402994e7c5b38) is no longer necessary as the code no uses mouse
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

void drawField(const QStyledItemDelegate *delegate, QPainter *painter, QStyleOptionViewItem &opt, const QModelIndex &index, int detailRole)
{
    // init style options to use drawControl(), except for the text
    opt.text.clear();
    opt.features = QStyleOptionViewItem::None;
    drawBasicItemViewItem(*painter, opt);

    const auto fieldName = delegate->displayText(index.data(Qt::DisplayRole), opt.locale);
    const auto fieldValue = delegate->displayText(index.data(detailRole), opt.locale);

    // draw icon
    auto iconRect
        = QRect(opt.rect.x() + listItemPadding, opt.rect.y() + centerObj(opt.rect.height(), listItemIconSize), listItemIconSize, listItemIconSize);
    painter->drawPixmap(iconRect, index.data(Qt::DecorationRole).value<QIcon>().pixmap(listItemIconSize, listItemIconSize));

    // compute rectangle for field name and value
    auto textRect = QRectF(opt.rect);
    textRect.setX(iconRect.right() + listItemSpacing);
    textRect.setWidth(textRect.width() - listItemPadding);
    auto textOption = QTextOption();
    textOption.setWrapMode(QTextOption::NoWrap);
    textOption.setAlignment(opt.displayAlignment);
    auto fieldNameRect = painter->boundingRect(textRect, fieldName, textOption);

    // draw field name
    setupPainterToDrawViewItemText(painter, opt);
    painter->drawText(textRect, fieldName, textOption);

    // draw status text
    textOption.setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    textRect.setX(textRect.x() + fieldNameRect.width() + listItemSpacing);
    painter->drawText(textRect, painter->fontMetrics().elidedText(fieldValue, Qt::ElideRight, static_cast<int>(textRect.width())), textOption);
}

void drawIdAndStatus(const QStyledItemDelegate *delegate, QPainter *painter, QStyleOptionViewItem &opt, const QModelIndex &index,
    int statusStringRole, int statusColorRole, int buttonWidth)
{
    const auto id = delegate->displayText(index.data(Qt::DisplayRole), opt.locale);
    const auto statusText = delegate->displayText(index.data(statusStringRole), opt.locale);

    // init style options to use drawControl(), except for the text
    opt.text.clear();
    opt.features = QStyleOptionViewItem::None;
    drawBasicItemViewItem(*painter, opt);

    // draw icon
    auto iconRect
        = QRect(opt.rect.x() + listItemPadding, opt.rect.y() + centerObj(opt.rect.height(), listItemIconSize), listItemIconSize, listItemIconSize);
    painter->drawPixmap(iconRect, index.data(Qt::DecorationRole).value<QIcon>().pixmap(listItemIconSize, listItemIconSize));

    // compute rectangle for label/ID and rectangle for status text
    auto textRect = QRectF(opt.rect);
    textRect.setX(iconRect.right() + listItemSpacing);
    textRect.setWidth(textRect.width() - buttonWidth);
    auto textOption = QTextOption();
    textOption.setWrapMode(QTextOption::NoWrap);
    textOption.setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    auto statusTextRect = painter->boundingRect(textRect, statusText, textOption);
    textRect.setWidth(textRect.width() - statusTextRect.width());

    // draw label/ID
    textOption.setAlignment(opt.displayAlignment);
    setupPainterToDrawViewItemText(painter, opt);
    painter->drawText(textRect, painter->fontMetrics().elidedText(id, Qt::ElideRight, static_cast<int>(textRect.width())), textOption);

    // draw status text
    if (const auto color = index.data(statusColorRole).value<QColor>(); color.isValid()) {
        opt.palette.setColor(QPalette::Text, color);
    }
    textOption.setAlignment(Qt::AlignRight);
    setupPainterToDrawViewItemText(painter, opt);
    painter->drawText(statusTextRect, statusText, textOption);
}

} // namespace QtGui
