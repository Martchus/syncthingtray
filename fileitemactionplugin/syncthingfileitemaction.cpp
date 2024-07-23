#include "./syncthingfileitemaction.h"
#include "./syncthingdiractions.h"
#include "./syncthinginfoaction.h"
#include "./syncthingmenuaction.h"

#include <syncthingmodel/syncthingicons.h>

#include <qtutilities/misc/desktoputils.h>

#include <KFileItem>
#include <KPluginFactory>

#include <QAction>
#include <QDir>
#include <QEvent>
#include <QRegularExpression>

#include <functional>
#include <utility>

using namespace std;

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
K_PLUGIN_CLASS_WITH_JSON(SyncthingFileItemAction, "metadata.json");
#else
K_PLUGIN_FACTORY(SyncthingFileItemActionFactory, registerPlugin<SyncthingFileItemAction>();)
#endif

struct SyncthingItem {
    SyncthingItem(const Data::SyncthingDir *dir, const QString &path);
    const Data::SyncthingDir *dir;
    QString path;
    QString name;
};

SyncthingItem::SyncthingItem(const Data::SyncthingDir *dir, const QString &path)
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
    const auto subActions = createActions(fileItemInfo, this);

    // don't show anything if no relevant actions could be determined but successfully connected
    if (s_data.connection().isConnected() && !s_data.hasError() && subActions.isEmpty()) {
        return QList<QAction *>();
    }

    if ((m_parentWidget = parentWidget)) {
        s_data.applyBrightCustomColorsSetting(QtUtilities::isPaletteDark(parentWidget->palette()));
        parentWidget->installEventFilter(this);
    }

    return QList<QAction *>{new SyncthingMenuAction(fileItemInfo, subActions, parentWidget)};
}

struct DirStats {
    explicit DirStats(const QList<const Data::SyncthingDir *> &dirs);

    QStringList ids;
    bool anyPaused = false;
    bool allPaused = true;
};

DirStats::DirStats(const QList<const Data::SyncthingDir *> &dirs)
{
    ids.reserve(dirs.size());
    for (const Data::SyncthingDir *const dir : dirs) {
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
    auto actions = QList<QAction *>();
    auto &data = s_data;
    auto &connection = data.connection();
    const auto &dirs = connection.dirInfo();

    // get all paths
    auto paths = QStringList();
    paths.reserve(fileItemInfo.items().size());
    const auto items = fileItemInfo.items();
    for (const KFileItem &item : items) {
        if (!item.isLocalFile()) {
            // don't show any actions when remote files are selected
            return actions;
        }
        paths << QDir::cleanPath(item.localPath());
    }

    // determine relevant Syncthing dirs
    QList<const Data::SyncthingDir *> detectedDirs;
    QList<const Data::SyncthingDir *> containingDirs;
    QList<SyncthingItem> detectedItems;
    const Data::SyncthingDir *lastDir = nullptr;
    for (const Data::SyncthingDir &dir : dirs) {
        auto dirPath = Data::substituteTilde(QDir::cleanPath(dir.path), connection.tilde(), connection.pathSeparator());
        auto dirPathWithSlash = dirPath + QChar('/');
        for (const QString &path : std::as_const(paths)) {
            if (path == dirPath) {
                lastDir = &dir;
                if (!detectedDirs.contains(lastDir)) {
                    detectedDirs << lastDir;
                }
            } else if (path.startsWith(dirPathWithSlash)) {
                detectedItems << SyncthingItem(&dir, path.mid(dirPathWithSlash.size()));
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
            detectedDirs.size() == 1 ? tr("Rescan \"%1\"").arg(detectedDirs.front()->displayName()) : tr("Rescan selected folders"), parent);
        if (connection.isConnected() && !detectedDirsStats.allPaused) {
            for (const Data::SyncthingDir *dir : std::as_const(detectedDirs)) {
                connect(actions.back(), &QAction::triggered, bind(&SyncthingFileItemActionStaticData::rescanDir, &data, dir->id, QString()));
                containingDirs.removeAll(dir);
            }
        } else {
            actions.back()->setEnabled(false);
        }

        // pause/resume item
        if (detectedDirsStats.anyPaused) {
            actions << new QAction(QIcon::fromTheme(QStringLiteral("media-playback-start")),
                detectedDirs.size() == 1 ? tr("Resume \"%1\"").arg(detectedDirs.front()->displayName()) : tr("Resume selected folders"), parent);
        } else {
            actions << new QAction(QIcon::fromTheme(QStringLiteral("media-playback-pause")),
                detectedDirs.size() == 1 ? tr("Pause \"%1\"").arg(detectedDirs.front()->displayName()) : tr("Pause selected folders"), parent);
        }
        if (connection.isConnected()) {
            connect(actions.back(), &QAction::triggered,
                bind(detectedDirsStats.anyPaused ? &Data::SyncthingConnection::resumeDirectories : &Data::SyncthingConnection::pauseDirectories,
                    &connection, detectedDirsStats.ids));
        } else {
            actions.back()->setEnabled(false);
        }
    }

    // add actions for the Syncthing dirs containing selected items
    if (!containingDirs.isEmpty()) {
        // rescan item
        actions << new QAction(QIcon::fromTheme(QStringLiteral("folder-sync")),
            containingDirs.size() == 1 ? tr("Rescan \"%1\"").arg(containingDirs.front()->displayName()) : tr("Rescan containing folders"), parent);
        if (connection.isConnected() && !containingDirsStats.allPaused) {
            for (const Data::SyncthingDir *dir : std::as_const(containingDirs)) {
                connect(actions.back(), &QAction::triggered, bind(&SyncthingFileItemActionStaticData::rescanDir, &data, dir->id, QString()));
            }
        } else {
            actions.back()->setEnabled(false);
        }

        // pause/resume item
        if (containingDirsStats.anyPaused) {
            actions << new QAction(QIcon::fromTheme(QStringLiteral("media-playback-start")),
                containingDirs.size() == 1 ? tr("Resume \"%1\"").arg(containingDirs.front()->displayName()) : tr("Resume containing folders"),
                parent);
        } else {
            actions << new QAction(QIcon::fromTheme(QStringLiteral("media-playback-pause")),
                containingDirs.size() == 1 ? tr("Pause \"%1\"").arg(containingDirs.front()->displayName()) : tr("Pause containing folders"), parent);
        }
        if (connection.isConnected()) {
            connect(actions.back(), &QAction::triggered,
                bind(containingDirsStats.anyPaused ? &Data::SyncthingConnection::resumeDirectories : &Data::SyncthingConnection::pauseDirectories,
                    &connection, containingDirsStats.ids));
        } else {
            actions.back()->setEnabled(false);
        }
    }

    // add actions to show further information about directory if the selection is only about one particular Syncthing dir
    if (lastDir && detectedDirs.size() + containingDirs.size() == 1) {
        auto *statusActions = new SyncthingDirActions(*lastDir, &data, parent);
        connect(&connection, &Data::SyncthingConnection::newDirs, statusActions,
            static_cast<void (SyncthingDirActions::*)(const std::vector<Data::SyncthingDir> &)>(&SyncthingDirActions::updateStatus));
        connect(&connection, &Data::SyncthingConnection::dirStatusChanged, statusActions,
            static_cast<bool (SyncthingDirActions::*)(const Data::SyncthingDir &)>(&SyncthingDirActions::updateStatus));
        actions << *statusActions;
    }

    // add note if no actions are available within the current folder
    if (actions.isEmpty()) {
        auto *const note = new QAction(parent);
        note->setText(tr("Not a shared directory"));
        note->setEnabled(false);
        actions << note;
    }

    // add separator
    auto *const separator = new QAction(parent);
    separator->setSeparator(true);
    actions << separator;

    // add error action
    QAction *const errorAction = new SyncthingInfoAction(parent);
    errorAction->setText(data.currentError());
    errorAction->setIcon(QIcon::fromTheme(QStringLiteral("state-error")));
    errorAction->setVisible(data.hasError());
    connect(&data, &SyncthingFileItemActionStaticData::currentErrorChanged, errorAction, &QAction::setText);
    connect(&data, &SyncthingFileItemActionStaticData::hasErrorChanged, errorAction, &QAction::setVisible);
    connect(&data, &SyncthingFileItemActionStaticData::currentErrorChanged, errorAction, &QAction::changed);
    actions << errorAction;

    // show Syncthing version
    if (!connection.syncthingVersion().isEmpty()) {
        static const auto versionRegex = QRegularExpression("(syncthing.*v.* \".*\").*");
        auto *const versionAction = new QAction(parent);
        if (const auto match = versionRegex.match(connection.syncthingVersion()); match.isValid()) {
            versionAction->setText(match.captured(1));
        } else {
            versionAction->setText(connection.syncthingVersion());
        }
        versionAction->setEnabled(false);
        actions << versionAction;
    }

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
