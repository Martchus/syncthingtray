#ifndef DIRVIEW_H
#define DIRVIEW_H

#include <QTreeView>

namespace QtGui {

class DirView : public QTreeView
{
    Q_OBJECT
public:
    DirView(QWidget *parent = nullptr);

Q_SIGNALS:
    void openDir(const QModelIndex &index);
    void scanDir(const QModelIndex &index);

protected:
    void mouseReleaseEvent(QMouseEvent *event);

private Q_SLOTS:
    void showContextMenu();
    void copySelectedItem();
    void copySelectedItemPath();

};

}

#endif // DIRVIEW_H
