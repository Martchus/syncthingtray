#ifndef DEVVIEW_H
#define DEVVIEW_H

#include <QTreeView>

namespace QtGui {

class DevView : public QTreeView
{
    Q_OBJECT
public:
    DevView(QWidget *parent = nullptr);

private Q_SLOTS:
    void showContextMenu();
    void copySelectedItem();

};

}

#endif // DEVVIEW_H
