#ifndef DIRBUTTONSITEMDELEGATE_H
#define DIRBUTTONSITEMDELEGATE_H

#include <QStyledItemDelegate>
#include <QPixmap>

namespace QtGui {

class DirButtonsItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    DirButtonsItemDelegate(QObject *parent);

    void paint(QPainter *, const QStyleOptionViewItem &, const QModelIndex &) const;

private:
    const QPixmap m_refreshIcon;
    const QPixmap m_folderIcon;
    const QPixmap m_pauseIcon;
    const QPixmap m_resumeIcon;
};

}

#endif // DIRBUTTONSITEMDELEGATE_H
