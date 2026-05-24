#ifndef TRAY_GUI_HELPER_H
#define TRAY_GUI_HELPER_H

#include <c++utilities/misc/traits.h>

#include <QClipboard>
#include <QGuiApplication>
#include <QModelIndex>
#include <QStyledItemDelegate>
#include <QTreeView>

#ifdef SYNCTHINGTRAY_SETUP_TOOLS_ENABLED
#include <QMessageBox>
#endif

#include <functional>
#include <type_traits>

QT_FORWARD_DECLARE_CLASS(QPainter)
QT_FORWARD_DECLARE_CLASS(QPoint)
QT_FORWARD_DECLARE_CLASS(QMenu)
QT_FORWARD_DECLARE_CLASS(QStyleOptionViewItem)

namespace QtGui {

class BasicTreeView : public QTreeView {
    Q_OBJECT
public:
    explicit BasicTreeView(QWidget *parent);

protected:
    void handleContextMenu(QMouseEvent *event);

    void mouseReleaseEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private:
    QEvent::Type m_contextMenuEventType;
};

constexpr auto listItemPadding = 4;
constexpr auto listItemSpacing = listItemPadding;
constexpr auto listItemIconSize = 16;
constexpr auto listItemIconSpacing = listItemSpacing / 2;
constexpr auto listItemIconsSize(int iconIndex)
{
    return (iconIndex + 1) * listItemIconSize + iconIndex * listItemIconSpacing + listItemPadding;
}

void showViewMenu(const QPoint &position, const QTreeView &view, QMenu &menu);
void drawBasicItemViewItem(QPainter &painter, const QStyleOptionViewItem &option);
void setupPainterToDrawViewItemText(QPainter *painter, QStyleOptionViewItem &opt);
void drawField(const QStyledItemDelegate *delegate, QPainter *painter, QStyleOptionViewItem &opt, const QModelIndex &index, int detailRole);
void drawIdAndStatus(const QStyledItemDelegate *delegate, QPainter *painter, QStyleOptionViewItem &opt, const QModelIndex &index,
    int statusStringRole, int statusColorRole, int buttonWidth);

inline auto copyToClipboard(const QString &text)
{
    return [=] { QGuiApplication::clipboard()->setText(text); };
}

template <typename ViewType> struct BasicRowData {
    operator bool() const;

    typename ViewType::ModelType *model = nullptr;
    QModelIndex index;
    decltype(model->info(index)) data = decltype(model->info(index))();
};

template <typename ViewType> BasicRowData<ViewType>::operator bool() const
{
    if (!model || !index.isValid()) {
        return false;
    }
    if constexpr (CppUtilities::Traits::IsSpecializationOf<decltype(data), QPair>::value
        || CppUtilities::Traits::IsSpecializationOf<decltype(data), std::pair>::value) {
        return data.first;
    } else {
        return data;
    }
}

template <typename ViewType> struct SelectedRow : public BasicRowData<ViewType> {
    explicit SelectedRow(ViewType *view);
};

template <typename ViewType> SelectedRow<ViewType>::SelectedRow(ViewType *view)
{
    auto *const selectionModel = view->selectionModel();
    if (!selectionModel) {
        return;
    }
    const auto selectedRows = selectionModel->selectedRows(0);
    if (selectedRows.size() != 1) {
        return;
    }
    this->index = selectedRows.at(0);
    if constexpr (std::is_void_v<typename ViewType::SortFilterModelType>) {
        this->model = qobject_cast<typename ViewType::ModelType *>(view->model());
    } else {
        const auto *const sortFilterModel = qobject_cast<typename ViewType::SortFilterModelType *>(view->model());
        if (sortFilterModel) {
            this->index = sortFilterModel->mapToSource(this->index);
            this->model = qobject_cast<typename ViewType::ModelType *>(sortFilterModel->sourceModel());
        }
    }
    if (this->model) {
        this->data = this->model->info(this->index);
    }
}

template <typename ViewType> struct ClickedRow : public BasicRowData<ViewType> {
    explicit ClickedRow(ViewType *view, const QPoint &clickPoint);

    typename ViewType::SortFilterModelType *proxyModel = nullptr;
    QModelIndex proxyIndex;
};

template <typename ViewType> ClickedRow<ViewType>::ClickedRow(ViewType *view, const QPoint &clickPoint)
{
    if constexpr (!std::is_void_v<typename ViewType::SortFilterModelType>) {
        proxyModel = qobject_cast<typename ViewType::SortFilterModelType *>(view->model());
    }
    this->model = qobject_cast<typename ViewType::ModelType *>(proxyModel ? proxyModel->sourceModel() : view->model());
    proxyIndex = view->indexAt(clickPoint);
    this->index = proxyModel ? proxyModel->mapToSource(proxyIndex) : proxyIndex;
    if (this->model && this->index.isValid()) {
        this->data = this->model->info(this->index);
    }
}

template <typename ViewType, typename ActionType> inline auto triggerActionForSelectedRow(ViewType *view, ActionType action)
{
    return [=] {
        if (const auto selectedRow = SelectedRow(view)) {
            std::invoke(action, view, CppUtilities::Traits::dereferenceMaybe(selectedRow.data));
        }
    };
}

inline int centerObj(int avail, int size)
{
    return (avail - size) / 2;
}

} // namespace QtGui

#endif // TRAY_GUI_HELPER_H
