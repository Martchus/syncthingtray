#ifndef DEVBUTTONSITEMDELEGATE_H
#define DEVBUTTONSITEMDELEGATE_H

#include <QPixmap>
#include <QStyleOptionViewItem>
#include <QStyledItemDelegate>

namespace QtGui {

class DevButtonsItemDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    DevButtonsItemDelegate(QObject *parent);

    void paint(QPainter *, const QStyleOptionViewItem &, const QModelIndex &) const override;

private:
    const QPixmap m_pauseIcon;
    const QPixmap m_resumeIcon;
};
} // namespace QtGui

#endif // DEVBUTTONSITEMDELEGATE_H
