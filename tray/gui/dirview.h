#ifndef DIRVIEW_H
#define DIRVIEW_H

#include <QTreeView>

namespace Data {
struct SyncthingDir;
}

namespace QtGui {

class DirView : public QTreeView {
    Q_OBJECT
public:
    DirView(QWidget *parent = nullptr);

Q_SIGNALS:
    void openDir(const Data::SyncthingDir &dir);
    void scanDir(const Data::SyncthingDir &dir);
    void pauseResumeDir(const Data::SyncthingDir &dir);

protected:
    void mouseReleaseEvent(QMouseEvent *event) override;

private Q_SLOTS:
    void showContextMenu();
    void copySelectedItem();
    void copySelectedItemPath();
};
} // namespace QtGui

#endif // DIRVIEW_H
