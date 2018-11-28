#ifndef SYNCTHINGMENUACTION_H
#define SYNCTHINGMENUACTION_H

#include <KFileItemListProperties>

#include <QAction>

namespace Data {
enum class SyncthingStatus;
}

class SyncthingMenuAction : public QAction {
    Q_OBJECT

public:
    explicit SyncthingMenuAction(const KFileItemListProperties &properties = KFileItemListProperties(),
        const QList<QAction *> &actions = QList<QAction *>(), QWidget *parentWidget = nullptr);

public Q_SLOTS:
    void updateStatus(Data::SyncthingStatus status);

private:
    KFileItemListProperties m_properties;
};

#endif // SYNCTHINGMENUACTION_H
