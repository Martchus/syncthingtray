#ifndef DOWNLOADVIEW_H
#define DOWNLOADVIEW_H

#include <QTreeView>

namespace Data {
struct SyncthingItemDownloadProgress;
struct SyncthingDir;
}

namespace QtGui {

class DownloadView : public QTreeView
{
    Q_OBJECT
public:
    DownloadView(QWidget *parent = nullptr);

Q_SIGNALS:
    void openDir(const Data::SyncthingDir &dir);
    void openItemDir(const Data::SyncthingItemDownloadProgress &dir);

protected:
    void mouseReleaseEvent(QMouseEvent *event);

private Q_SLOTS:
    void showContextMenu();
    void copySelectedItem();

};

}

#endif // DOWNLOADVIEW_H
