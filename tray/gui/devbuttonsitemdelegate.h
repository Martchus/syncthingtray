#ifndef DEVBUTTONSITEMDELEGATE_H
#define DEVBUTTONSITEMDELEGATE_H

#include <QStyleOptionViewItem>
#include <QStyledItemDelegate>

namespace QtGui {

class DevButtonsItemDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    DevButtonsItemDelegate(QObject *parent);

    void paint(QPainter *, const QStyleOptionViewItem &, const QModelIndex &) const override;
};
} // namespace QtGui

#endif // DEVBUTTONSITEMDELEGATE_H
