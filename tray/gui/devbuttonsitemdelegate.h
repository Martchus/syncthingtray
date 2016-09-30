#ifndef DEVBUTTONSITEMDELEGATE_H
#define DEVBUTTONSITEMDELEGATE_H

#include <QStyledItemDelegate>
#include <QPixmap>

namespace QtGui {

class DevButtonsItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    DevButtonsItemDelegate(QObject *parent);

    void paint(QPainter *, const QStyleOptionViewItem &, const QModelIndex &) const;

private:
    const QPixmap m_pauseIcon;
    const QPixmap m_resumeIcon;
};

}

#endif // DEVBUTTONSITEMDELEGATE_H
