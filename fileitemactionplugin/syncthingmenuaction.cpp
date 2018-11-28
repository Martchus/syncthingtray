#include "./syncthingmenuaction.h"
#include "./syncthingfileitemaction.h"

#include "../model/syncthingicons.h"

#include "../connector/syncthingconnection.h"

#include <QAction>
#include <QMenu>

using namespace Data;

SyncthingMenuAction::SyncthingMenuAction(const KFileItemListProperties &properties, const QList<QAction *> &actions, QWidget *parentWidget)
    : QAction(parentWidget)
    , m_properties(properties)
{
    if (!actions.isEmpty()) {
        auto *menu = new QMenu(parentWidget);
        menu->addActions(actions);
        setMenu(menu);
    }
    updateStatus(SyncthingFileItemAction::staticData().connection().status());
}

void SyncthingMenuAction::updateStatus(SyncthingStatus status)
{
    if (status != SyncthingStatus::Disconnected && status != SyncthingStatus::Reconnecting && status != SyncthingStatus::BeingDestroyed) {
        setText(tr("Syncthing"));
        setIcon(statusIcons().scanninig);
        if (!menu()) {
            const QList<QAction *> actions = SyncthingFileItemAction::createActions(m_properties, parentWidget());
            if (!actions.isEmpty()) {
                auto *menu = new QMenu(parentWidget());
                menu->addActions(actions);
                setMenu(menu);
            }
        }
    } else {
        if (status != SyncthingStatus::Reconnecting) {
            SyncthingFileItemAction::staticData().connection().connect();
        }
        setText(tr("Syncthing - connecting"));
        setIcon(statusIcons().disconnected);
        if (QMenu *menu = this->menu()) {
            setMenu(nullptr);
            delete menu;
        }
    }
}
