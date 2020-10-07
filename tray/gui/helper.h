#ifndef TRAY_GUI_HELPER_H
#define TRAY_GUI_HELPER_H

#include <c++utilities/misc/traits.h>

#include <QClipboard>
#include <QGuiApplication>
#include <QModelIndex>
#include <QTreeView>

QT_FORWARD_DECLARE_CLASS(QPoint)
QT_FORWARD_DECLARE_CLASS(QMenu)

namespace QtGui {

void showViewMenu(const QPoint &position, const QTreeView &view, QMenu &menu);

inline auto copyToClipboard(const QString &text)
{
    return [=] { QGuiApplication::clipboard()->setText(text); };
}

template <typename ViewType> struct SelectedRow {
    explicit SelectedRow(ViewType *view);
    operator bool() const;

    typename ViewType::ModelType *model = nullptr;
    QModelIndex index;
    decltype(model->info(index)) data = decltype(model->info(index))();
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
    index = selectedRows.at(0);
    model = qobject_cast<typename ViewType::ModelType *>(view->model());
    if (model) {
        data = model->info(index);
    }
}

template <typename ViewType> SelectedRow<ViewType>::operator bool() const
{
    if (!model || !index.isValid()) {
        return false;
    }
    if constexpr (CppUtilities::Traits::IsSpecializationOf<decltype(data), QPair>::value) {
        return data.first;
    } else {
        return data;
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

} // namespace QtGui

#endif // TRAY_GUI_HELPER_H
