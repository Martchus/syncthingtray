#ifndef DEVVIEW_H
#define DEVVIEW_H

#include <QTreeView>

namespace QtGui {

class DevView : public QTreeView
{
    Q_OBJECT
public:
    DevView(QWidget *parent = nullptr);

Q_SIGNALS:
    void pauseResumeDev(const QModelIndex &index);

protected:
    void mouseReleaseEvent(QMouseEvent *event);

private Q_SLOTS:
    void showContextMenu();
    void copySelectedItem();
    void copySelectedItemId();

};

}

#endif // DEVVIEW_H
