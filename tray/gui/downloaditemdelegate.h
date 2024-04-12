#ifndef DOWNLOADITEMDELEGATE_H
#define DOWNLOADITEMDELEGATE_H

#include <QPixmap>
#include <QStyledItemDelegate>

namespace QtGui {

class DownloadItemDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    explicit DownloadItemDelegate(QObject *parent);

    void paint(QPainter *, const QStyleOptionViewItem &, const QModelIndex &) const override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};
} // namespace QtGui

#endif // DOWNLOADITEMDELEGATE_H
