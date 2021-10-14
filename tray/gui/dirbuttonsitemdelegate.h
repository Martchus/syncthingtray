#ifndef DIRBUTTONSITEMDELEGATE_H
#define DIRBUTTONSITEMDELEGATE_H

#include <QStyledItemDelegate>

namespace QtGui {

class DirButtonsItemDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    DirButtonsItemDelegate(QObject *parent);

    void paint(QPainter *, const QStyleOptionViewItem &, const QModelIndex &) const override;
};
} // namespace QtGui

#endif // DIRBUTTONSITEMDELEGATE_H
