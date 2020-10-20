#ifndef DEVVIEW_H
#define DEVVIEW_H

#include <QTreeView>

namespace Data {
struct SyncthingDev;
class SyncthingDeviceModel;
class SyncthingSortFilterModel;
} // namespace Data

namespace QtGui {

class DevView : public QTreeView {
    Q_OBJECT
public:
    using ModelType = Data::SyncthingDeviceModel;
    using SortFilterModelType = Data::SyncthingSortFilterModel;

    DevView(QWidget *parent = nullptr);

Q_SIGNALS:
    void pauseResumeDev(const Data::SyncthingDev &dev);

protected:
    void mouseReleaseEvent(QMouseEvent *event) override;

private Q_SLOTS:
    void showContextMenu(const QPoint &position);
};
} // namespace QtGui

#endif // DEVVIEW_H
