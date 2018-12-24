#ifndef SYNCTHINGMENUACTION_H
#define SYNCTHINGMENUACTION_H

#include "../connector/syncthingnotifier.h"

#include <KFileItemListProperties>

#include <QAction>

namespace Data {
enum class SyncthingStatus;
}

/*!
 * \brief The SyncthingMenuAction class provides the top-level menu "Syncthing" entry for the context menu.
 */
class SyncthingMenuAction : public QAction {
    Q_OBJECT

public:
    explicit SyncthingMenuAction(const KFileItemListProperties &properties = KFileItemListProperties(),
        const QList<QAction *> &actions = QList<QAction *>(), QWidget *parentWidget = nullptr);

private Q_SLOTS:
    void handleConnectedChanged();
    void updateActionStatus();

private:
    void createMenu(const QList<QAction *> &actions);

    KFileItemListProperties m_properties;
    Data::SyncthingNotifier m_notifier;
};

#endif // SYNCTHINGMENUACTION_H
