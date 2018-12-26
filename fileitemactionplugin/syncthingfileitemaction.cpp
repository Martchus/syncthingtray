#include "./syncthingfileitemaction.h"
#include "./syncthingdiractions.h"
#include "./syncthinginfoaction.h"
#include "./syncthingmenuaction.h"

#include "../model/syncthingicons.h"

#include <KFileItem>
#include <KPluginFactory>
#include <KPluginLoader>

#include <QAction>

#include <functional>

using namespace std;
using namespace Data;

K_PLUGIN_FACTORY(SyncthingFileItemActionFactory, registerPlugin<SyncthingFileItemAction>();)

struct SyncthingItem {
    SyncthingItem(const SyncthingDir *dir, const QString &path);
    const SyncthingDir *dir;
    QString path;
    QString name;
};

SyncthingItem::SyncthingItem(const SyncthingDir *dir, const QString &path)
    : dir(dir)
    , path(path)
{
    const auto lastSep = path.lastIndexOf(QChar('/'));
    if (lastSep >= 0) {
        name = path.mid(lastSep + 1);
    } else {
        name = path;
    }
}

SyncthingFileItemActionStaticData SyncthingFileItemAction::s_data;

SyncthingFileItemAction::SyncthingFileItemAction(QObject *parent, const QVariantList &)
    : KAbstractFileItemActionPlugin(parent)
{
    s_data.initialize();
}

QList<QAction *> SyncthingFileItemAction::actions(const KFileItemListProperties &fileItemInfo, QWidget *parentWidget)
{
    Q_UNUSED(parentWidget)

    // create actions
    const QList<QAction *> subActions = createActions(fileItemInfo, this);
    QList<QAction *> topLevelActions;

    // don't show anything if no relevant actions could be determined but successfully connected
    if (s_data.connection().isConnected() && !s_data.hasError() && subActions.isEmpty()) {
        return topLevelActions;
    }

    topLevelActions << new SyncthingMenuAction(fileItemInfo, subActions, this);
    return topLevelActions;
}

QList<QAction *> SyncthingFileItemAction::createActions(const KFileItemListProperties &fileItemInfo, QObject *parent)
{
    QList<QAction *> actions;
    auto &data = s_data;
    auto &connection = data.connection();
    const auto &dirs = connection.dirInfo();

    // get all paths
    QStringList paths;
    paths.reserve(fileItemInfo.items().size());
    for (const KFileItem &item : fileItemInfo.items()) {
        if (!item.isLocalFile()) {
            // don't show any actions when remote files are selected
            return actions;
        }
        paths << item.localPath();
    }

    // determine relevant Syncthing dirs
    QList<const SyncthingDir *> detectedDirs;
    QList<const SyncthingDir *> containingDirs;
    QList<SyncthingItem> detectedItems;
    const SyncthingDir *lastDir = nullptr;
    for (const SyncthingDir &dir : dirs) {
        QStringRef dirPath(dir.pathWithoutTrailingSlash());
        for (const QString &path : paths) {
            if (path == dirPath) {
                lastDir = &dir;
                if (!detectedDirs.contains(lastDir)) {
                    detectedDirs << lastDir;
                }
            } else if (path.startsWith(dir.path)) {
                detectedItems << SyncthingItem(&dir, path.mid(dir.path.size()));
                lastDir = &dir;
                if (!containingDirs.contains(lastDir)) {
                    containingDirs << lastDir;
                }
            }
        }
    }

    // add actions for the selected items itself
    actions.reserve(32);
    if (!detectedItems.isEmpty()) {
        QString rescanLabel;
        if (detectedItems.size() > 1) {
            rescanLabel = tr("Rescan selected items");
        } else {
            rescanLabel = tr("Rescan \"%1\"").arg(detectedItems.front().name);
        }
        actions << new QAction(QIcon::fromTheme(QStringLiteral("view-refresh")), rescanLabel, parent);
        if (connection.isConnected()) {
            for (const SyncthingItem &item : detectedItems) {
                connect(actions.back(), &QAction::triggered, bind(&SyncthingFileItemActionStaticData::rescanDir, &data, item.dir->id, item.path));
            }
        } else {
            actions.back()->setEnabled(false);
        }
    }

    // add actions for explicitely selected Syncthing dirs
    if (!detectedDirs.isEmpty()) {
        // rescan item
        actions << new QAction(QIcon::fromTheme(QStringLiteral("folder-sync")),
            detectedDirs.size() == 1 ? tr("Rescan \"%1\"").arg(detectedDirs.front()->displayName()) : tr("Rescan selected directories"), parent);
        if (connection.isConnected()) {
            for (const SyncthingDir *dir : detectedDirs) {
                connect(actions.back(), &QAction::triggered, bind(&SyncthingFileItemActionStaticData::rescanDir, &data, dir->id, QString()));
                containingDirs.removeAll(dir);
            }
        } else {
            actions.back()->setEnabled(false);
        }

        // pause/resume item
        QStringList ids;
        ids.reserve(detectedDirs.size());
        bool isPaused = false;
        for (const SyncthingDir *dir : detectedDirs) {
            ids << dir->id;
            if (dir->paused) {
                isPaused = true;
                break;
            }
        }
        if (isPaused) {
            actions << new QAction(QIcon::fromTheme(QStringLiteral("media-playback-start")),
                detectedDirs.size() == 1 ? tr("Resume \"%1\"").arg(detectedDirs.front()->displayName()) : tr("Resume selected directories"), parent);
        } else {
            actions << new QAction(QIcon::fromTheme(QStringLiteral("media-playback-pause")),
                detectedDirs.size() == 1 ? tr("Pause \"%1\"").arg(detectedDirs.front()->displayName()) : tr("Pause selected directories"), parent);
        }
        if (connection.isConnected()) {
            connect(actions.back(), &QAction::triggered,
                bind(isPaused ? &SyncthingConnection::resumeDirectories : &SyncthingConnection::pauseDirectories, &connection, ids));
        } else {
            actions.back()->setEnabled(false);
        }
    }

    // add actions for the Syncthing dirs containing selected items
    if (!containingDirs.isEmpty()) {
        // rescan item
        actions << new QAction(QIcon::fromTheme(QStringLiteral("folder-sync")),
            containingDirs.size() == 1 ? tr("Rescan \"%1\"").arg(containingDirs.front()->displayName()) : tr("Rescan containing directories"),
            parent);
        if (connection.isConnected()) {
            for (const SyncthingDir *dir : containingDirs) {
                connect(actions.back(), &QAction::triggered, bind(&SyncthingFileItemActionStaticData::rescanDir, &data, dir->id, QString()));
            }
        } else {
            actions.back()->setEnabled(false);
        }

        // pause/resume item
        QStringList ids;
        ids.reserve(containingDirs.size());
        bool isPaused = false;
        for (const SyncthingDir *dir : containingDirs) {
            ids << dir->id;
            if (dir->paused) {
                isPaused = true;
                break;
            }
        }
        if (isPaused) {
            actions << new QAction(QIcon::fromTheme(QStringLiteral("media-playback-start")),
                containingDirs.size() == 1 ? tr("Resume \"%1\"").arg(containingDirs.front()->displayName()) : tr("Resume containing directories"),
                parent);
        } else {
            actions << new QAction(QIcon::fromTheme(QStringLiteral("media-playback-pause")),
                containingDirs.size() == 1 ? tr("Pause \"%1\"").arg(containingDirs.front()->displayName()) : tr("Pause containing directories"),
                parent);
        }
        if (connection.isConnected()) {
            connect(actions.back(), &QAction::triggered,
                bind(isPaused ? &SyncthingConnection::resumeDirectories : &SyncthingConnection::pauseDirectories, &connection, ids));
        } else {
            actions.back()->setEnabled(false);
        }
    }

    // add actions to show further information about directory if the selection is only about one particular Syncthing dir
    if (lastDir && detectedDirs.size() + containingDirs.size() == 1) {
        auto *statusActions = new SyncthingDirActions(*lastDir, parent);
        connect(&connection, &SyncthingConnection::newDirs, statusActions,
            static_cast<void (SyncthingDirActions::*)(const vector<SyncthingDir> &)>(&SyncthingDirActions::updateStatus));
        connect(&connection, &SyncthingConnection::dirStatusChanged, statusActions,
            static_cast<bool (SyncthingDirActions::*)(const SyncthingDir &)>(&SyncthingDirActions::updateStatus));
        actions << *statusActions;
    }

    // add separator
    if (!actions.isEmpty()) {
        QAction *const separator = new QAction(parent);
        separator->setSeparator(true);
        actions << separator;
    }

    // add error action
    QAction *const errorAction = new SyncthingInfoAction(parent);
    errorAction->setText(data.currentError());
    errorAction->setIcon(QIcon::fromTheme(QStringLiteral("state-error")));
    errorAction->setVisible(data.hasError());
    connect(&data, &SyncthingFileItemActionStaticData::currentErrorChanged, errorAction, &QAction::setText);
    connect(&data, &SyncthingFileItemActionStaticData::hasErrorChanged, errorAction, &QAction::setVisible);
    connect(&data, &SyncthingFileItemActionStaticData::currentErrorChanged, errorAction, &QAction::changed);
    actions << errorAction;

    // add config file selection
    QAction *const configFileAction = new QAction(QIcon::fromTheme(QStringLiteral("settings-configure")), tr("Select Syncthing config ..."), parent);
    connect(configFileAction, &QAction::triggered, &data, &SyncthingFileItemActionStaticData::selectSyncthingConfig);
    actions << configFileAction;

    // about about action
    QAction *const aboutAction = new QAction(QIcon::fromTheme(QStringLiteral("help-about")), tr("About"), parent);
    connect(aboutAction, &QAction::triggered, &SyncthingFileItemActionStaticData::showAboutDialog);
    actions << aboutAction;

    return actions;
}

#include <syncthingfileitemaction.moc>
