#ifndef DOWNLOADVIEW_H
#define DOWNLOADVIEW_H

#include <QTreeView>

namespace Data {
struct SyncthingItemDownloadProgress;
struct SyncthingDir;
class SyncthingDownloadModel;
} // namespace Data

namespace QtGui {

class DownloadView : public QTreeView {
    Q_OBJECT
public:
    using ModelType = Data::SyncthingDownloadModel;
    using SortFilterModelType = void;

    DownloadView(QWidget *parent = nullptr);

Q_SIGNALS:
    void openDir(const Data::SyncthingDir &dir);
    void openItemDir(const Data::SyncthingItemDownloadProgress &dir);

protected:
    void mouseReleaseEvent(QMouseEvent *event) override;

private Q_SLOTS:
    void showContextMenu(const QPoint &position);

private:
    void emitOpenDir(QPair<const Data::SyncthingDir *, const Data::SyncthingItemDownloadProgress *> info);
};
} // namespace QtGui

#endif // DOWNLOADVIEW_H
