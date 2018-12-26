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
    , m_notifier(SyncthingFileItemAction::staticData().connection())
{
    // init according to current state
    createMenu(actions);
    updateActionStatus();

    // setup handling future state change
    m_notifier.setEnabledNotifications(SyncthingHighLevelNotification::ConnectedDisconnected);
    connect(&m_notifier, &SyncthingNotifier::connected, this, &SyncthingMenuAction::handleConnectedChanged);
    connect(&m_notifier, &SyncthingNotifier::disconnected, this, &SyncthingMenuAction::handleConnectedChanged);
}

void SyncthingMenuAction::handleConnectedChanged()
{
    // update the current menu
    if (QMenu *const menu = this->menu()) {
        menu->deleteLater();
        setMenu(nullptr);
    }
    createMenu(SyncthingFileItemAction::createActions(m_properties, parentWidget()));

    // update status of action itself
    updateActionStatus();
}

void SyncthingMenuAction::updateActionStatus()
{
    auto &data = SyncthingFileItemAction::staticData();
    auto &connection = data.connection();

    // handle case when already connected
    if (connection.isConnected()) {
        setText(tr("Syncthing"));
        setIcon(statusIcons().scanninig);
        return;
    }

    // attempt to connect if not reconnecting anyways and there's no configuration issue
    if (connection.status() != SyncthingStatus::Reconnecting && !data.hasError()) {
        connection.connect();
    }

    if (connection.hasPendingRequests()) {
        setText(tr("Syncthing - connecting"));
    } else {
        setText(tr("Syncthing - not connected"));
    }
    setIcon(statusIcons().disconnected);
}

void SyncthingMenuAction::createMenu(const QList<QAction *> &actions)
{
    if (actions.isEmpty()) {
        return;
    }
    auto *const menu = new QMenu(parentWidget());
    menu->addActions(actions);
    setMenu(menu);
}
