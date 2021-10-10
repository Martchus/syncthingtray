#include "./syncthingmenuaction.h"
#include "./syncthingfileitemaction.h"

#include <syncthingmodel/syncthingicons.h>

#include <syncthingconnector/syncthingconnection.h>

#ifdef CPP_UTILITIES_DEBUG_BUILD
#include <c++utilities/io/ansiescapecodes.h>
#endif

#include <QAction>
#include <QMenu>

#ifdef CPP_UTILITIES_DEBUG_BUILD
#include <iostream>
#endif

using namespace CppUtilities;
using namespace Data;

SyncthingMenuAction::SyncthingMenuAction(const KFileItemListProperties &properties, const QList<QAction *> &actions, QObject *parent)
    : QAction(parent)
    , m_properties(properties)
    , m_notifier(SyncthingFileItemAction::staticData().connection())
{
#ifdef CPP_UTILITIES_DEBUG_BUILD
    std::cerr << EscapeCodes::Phrases::Info << "Creating SyncthingMenuAction: " << this << EscapeCodes::Phrases::EndFlush;
#endif

    // init according to current state
    createMenu(actions);
    updateActionStatus();

    // setup handling future state change
    m_notifier.setEnabledNotifications(SyncthingHighLevelNotification::ConnectedDisconnected);
    connect(&m_notifier, &SyncthingNotifier::connected, this, &SyncthingMenuAction::handleConnectedChanged);
    connect(&m_notifier, &SyncthingNotifier::disconnected, this, &SyncthingMenuAction::handleConnectedChanged);
}

#ifdef CPP_UTILITIES_DEBUG_BUILD
SyncthingMenuAction::~SyncthingMenuAction()
{
    std::cerr << EscapeCodes::Phrases::Info << "Destroying SyncthingMenuAction: " << this << EscapeCodes::Phrases::EndFlush;
}
#endif

void SyncthingMenuAction::handleConnectedChanged()
{
    // update the current menu
    if (QMenu *const menu = this->menu()) {
        menu->deleteLater();
        setMenu(nullptr);
    }
    createMenu(SyncthingFileItemAction::createActions(m_properties, parent()));

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
        setIcon(QIcon(QStringLiteral("syncthing.fa")));
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
