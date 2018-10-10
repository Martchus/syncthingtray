#ifndef DIRBUTTONSITEMDELEGATE_H
#define DIRBUTTONSITEMDELEGATE_H

#include <QPixmap>
#include <QStyledItemDelegate>

namespace QtGui {

class DirButtonsItemDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    DirButtonsItemDelegate(QObject *parent);

    void paint(QPainter *, const QStyleOptionViewItem &, const QModelIndex &) const override;

private:
    const QPixmap m_refreshIcon;
    const QPixmap m_folderIcon;
    const QPixmap m_pauseIcon;
    const QPixmap m_resumeIcon;
};
} // namespace QtGui

#endif // DIRBUTTONSITEMDELEGATE_H
