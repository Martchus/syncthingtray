#include "./syncthingfileitemaction.h"
#include "./syncthingdiractions.h"
#include "./syncthinginfoaction.h"
#include "./syncthingmenuaction.h"

#include <syncthingmodel/syncthingicons.h>

#include <qtutilities/misc/desktoputils.h>

#include <KFileItem>
#include <KPluginFactory>
#include <KPluginLoader>

#include <QAction>
#include <QEvent>

#include <functional>
#include <utility>

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
    , m_parentWidget(nullptr)
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

    if ((m_parentWidget = parentWidget)) {
        s_data.applyBrightCustomColorsSetting(QtUtilities::isPaletteDark(parentWidget->palette()));
        parentWidget->installEventFilter(this);
    }

    topLevelActions << new SyncthingMenuAction(fileItemInfo, subActions, this);
    return topLevelActions;
}

struct DirStats {
    explicit DirStats(const QList<const SyncthingDir *> &dirs);

    QStringList ids;
    bool anyPaused = false;
    bool allPaused = true;
};

DirStats::DirStats(const QList<const SyncthingDir *> &dirs)
{
    ids.reserve(dirs.size());
    for (const SyncthingDir *const dir : dirs) {
        ids << dir->id;
        if (dir->paused) {
            anyPaused = true;
            if (!allPaused) {
                break;
            }
        } else {
            allPaused = false;
            if (anyPaused) {
                break;
            }
        }
    }
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
    const auto items = fileItemInfo.items();
    for (const KFileItem &item : items) {
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
        for (const QString &path : std::as_const(paths)) {
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

    // compute dir stats
    const auto detectedDirsStats = DirStats(detectedDirs);
    const auto containingDirsStats = DirStats(containingDirs);

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
        if (connection.isConnected() && !containingDirsStats.allPaused) {
            for (const SyncthingItem &item : std::as_const(detectedItems)) {
                connect(actions.back(), &QAction::triggered, bind(&SyncthingFileItemActionStaticData::rescanDir, &data, item.dir->id, item.path));
            }
        } else {
            actions.back()->setEnabled(false);
        }
    }

    // add actions for explicitly selected Syncthing dirs
    if (!detectedDirs.isEmpty()) {

        // rescan item
        actions << new QAction(QIcon::fromTheme(QStringLiteral("folder-sync")),
            detectedDirs.size() == 1 ? tr("Rescan \"%1\"").arg(detectedDirs.front()->displayName()) : tr("Rescan selected directories"), parent);
        if (connection.isConnected() && !detectedDirsStats.allPaused) {
            for (const SyncthingDir *dir : std::as_const(detectedDirs)) {
                connect(actions.back(), &QAction::triggered, bind(&SyncthingFileItemActionStaticData::rescanDir, &data, dir->id, QString()));
                containingDirs.removeAll(dir);
            }
        } else {
            actions.back()->setEnabled(false);
        }

        // pause/resume item
        if (detectedDirsStats.anyPaused) {
            actions << new QAction(QIcon::fromTheme(QStringLiteral("media-playback-start")),
                detectedDirs.size() == 1 ? tr("Resume \"%1\"").arg(detectedDirs.front()->displayName()) : tr("Resume selected directories"), parent);
        } else {
            actions << new QAction(QIcon::fromTheme(QStringLiteral("media-playback-pause")),
                detectedDirs.size() == 1 ? tr("Pause \"%1\"").arg(detectedDirs.front()->displayName()) : tr("Pause selected directories"), parent);
        }
        if (connection.isConnected()) {
            connect(actions.back(), &QAction::triggered,
                bind(detectedDirsStats.anyPaused ? &SyncthingConnection::resumeDirectories : &SyncthingConnection::pauseDirectories, &connection,
                    detectedDirsStats.ids));
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
        if (connection.isConnected() && !containingDirsStats.allPaused) {
            for (const SyncthingDir *dir : std::as_const(containingDirs)) {
                connect(actions.back(), &QAction::triggered, bind(&SyncthingFileItemActionStaticData::rescanDir, &data, dir->id, QString()));
            }
        } else {
            actions.back()->setEnabled(false);
        }

        // pause/resume item
        if (containingDirsStats.anyPaused) {
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
                bind(containingDirsStats.anyPaused ? &SyncthingConnection::resumeDirectories : &SyncthingConnection::pauseDirectories, &connection,
                    containingDirsStats.ids));
        } else {
            actions.back()->setEnabled(false);
        }
    }

    // add actions to show further information about directory if the selection is only about one particular Syncthing dir
    if (lastDir && detectedDirs.size() + containingDirs.size() == 1) {
        auto *statusActions = new SyncthingDirActions(*lastDir, &data, parent);
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

    // add config items
    QAction *const configFileAction = new QAction(QIcon::fromTheme(QStringLiteral("settings-configure")), tr("Select Syncthing config ..."), parent);
    connect(configFileAction, &QAction::triggered, &data, &SyncthingFileItemActionStaticData::selectSyncthingConfig);
    actions << configFileAction;

    // about about action
    QAction *const aboutAction = new QAction(QIcon::fromTheme(QStringLiteral("help-about")), tr("About"), parent);
    connect(aboutAction, &QAction::triggered, &SyncthingFileItemActionStaticData::showAboutDialog);
    actions << aboutAction;

    return actions;
}

bool SyncthingFileItemAction::eventFilter(QObject *object, QEvent *event)
{
    if (object == m_parentWidget && event->type() == QEvent::PaletteChange) {
        s_data.handlePaletteChanged(m_parentWidget->palette());
    }
    return false;
}

#include <syncthingfileitemaction.moc>
