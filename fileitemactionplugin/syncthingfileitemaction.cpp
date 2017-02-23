#include "./syncthingfileitemaction.h"
#include "../model/syncthingicons.h"

#include "../connector/syncthingconfig.h"
#include "../connector/syncthingconnectionsettings.h"
#include "../connector/syncthingdir.h"
#include "../connector/utils.h"

#include <qtutilities/aboutdialog/aboutdialog.h>

#include <KFileItemListProperties>
#include <KFileItem>
#include <KPluginFactory>
#include <KPluginLoader>

#include <QMenu>
#include <QAction>
#include <QWidget>
#include <QDir>
#include <QEvent>

#include <iostream>
#include <functional>

#include "resources/config.h"

using namespace std;
using namespace Dialogs;
using namespace Data;

K_PLUGIN_FACTORY(SyncthingFileItemActionFactory, registerPlugin<SyncthingFileItemAction>();)

struct SyncthingItem
{
    SyncthingItem(const SyncthingDir *dir, const QString &path);
    const SyncthingDir *dir;
    QString path;
    QString name;
};

SyncthingItem::SyncthingItem(const SyncthingDir *dir, const QString &path) :
    dir(dir),
    path(path)
{
    int lastSep = path.lastIndexOf(QChar('/'));
    if(lastSep > 0) {
        name = path.mid(lastSep + 1);
    } else {
        name = path;
    }
}

SyncthingConnection SyncthingFileItemAction::s_connection;

SyncthingFileItemAction::SyncthingFileItemAction(QObject *parent, const QVariantList &) :
    KAbstractFileItemActionPlugin(parent)
{
    if(s_connection.apiKey().isEmpty()) {
        // determine path of Syncthing config file
        const QByteArray configPathFromEnv(qgetenv("KIO_SYNCTHING_CONFIG_PATH"));
        const QString configPath = !configPathFromEnv.isEmpty()
                ? QString::fromLocal8Bit(configPathFromEnv)
                : SyncthingConfig::locateConfigFile();
        if(configPath.isEmpty()) {
            cerr << "Unable to determine location of Syncthing config. Set KIO_SYNCTHING_CONFIG_PATH to specify location." << endl;
            return;
        }

        // load Syncthing config
        SyncthingConfig config;
        if(!config.restore(configPath)) {
            cerr << "Unable to load Syncthing config from \"" << configPath.toLocal8Bit().data() << "\"" << endl;
            if(configPathFromEnv.isEmpty()) {
                cerr << "Note: Set KIO_SYNCTHING_CONFIG_PATH to specify config file explicitely." << endl;
            }
            return;
        }
        cerr << "Syncthing config loaded from \"" << configPath.toLocal8Bit().data() << "\"" << endl;
        SyncthingConnectionSettings settings;
        settings.syncthingUrl = config.syncthingUrl();
        settings.apiKey.append(config.guiApiKey);

        // establish connection
        bool ok;
        int reconnectInterval = qEnvironmentVariableIntValue("KIO_SYNCTHING_RECONNECT_INTERVAL", &ok);
        if(!ok || reconnectInterval < 0) {
            reconnectInterval = 10000;
        }
        s_connection.setAutoReconnectInterval(reconnectInterval);
        s_connection.reconnect(settings);
        connect(&s_connection, &SyncthingConnection::error, &SyncthingFileItemAction::logConnectionError);
        connect(&s_connection, &SyncthingConnection::statusChanged, &SyncthingFileItemAction::logConnectionStatus);
    }
}

QList<QAction *> SyncthingFileItemAction::actions(const KFileItemListProperties &fileItemInfo, QWidget *parentWidget)
{
    // check whether any directories are known
    const auto &dirs = s_connection.dirInfo();
    if(dirs.empty()) {
        return QList<QAction*>();
    }

    // get all paths
    QStringList paths;
    paths.reserve(fileItemInfo.items().size());
    for(const KFileItem &item : fileItemInfo.items()) {
        if(!item.isLocalFile()) {
            // don't show any actions when remote files are selected
            return QList<QAction*>();
        }
        paths << item.localPath();
    }

    // determine relevant Syncthing dirs
    QList<const SyncthingDir *> detectedDirs;
    QList<const SyncthingDir *> containingDirs;
    QList<SyncthingItem> detectedItems;
    const SyncthingDir *lastDir;
    for(const SyncthingDir &dir : dirs) {
        QStringRef dirPath(&dir.path);
        while(dirPath.endsWith(QChar('/'))) {
#if QT_VERSION_MAJOR >= 5 && QT_VERSION_MINOR >= 8
            dirPath.chop(1);
#else
            dirPath = dirPath.left(dirPath.size() - 1);
#endif
        }
        for(const QString &path : paths) {
            if(path == dirPath) {
                detectedDirs << (lastDir = &dir);
            } else if(path.startsWith(dir.path)) {
                detectedItems << SyncthingItem(&dir, path.mid(dir.path.size()));
                containingDirs << (lastDir = &dir);
            }
        }
    }

    // add actions for explicitely selected Syncthing dirs
    QList<QAction*> actions;
    if(!detectedDirs.isEmpty()) {
        actions << new QAction(
                       QIcon::fromTheme(QStringLiteral("folder-sync")),
                       detectedDirs.size() == 1
                       ? tr("Rescan ") + detectedDirs.front()->displayName()
                       : tr("Rescan selected directories"),
                       parentWidget);
        if(s_connection.isConnected()) {
            for(const SyncthingDir *dir : detectedDirs) {
                connect(actions.back(), &QAction::triggered, bind(&SyncthingFileItemAction::rescanDir, dir->id, QString()));
                containingDirs.removeAll(dir);
            }
        } else {
            actions.back()->setEnabled(false);
        }
    }

    // add actions for the Syncthing dirs containing selected items
    if(!containingDirs.isEmpty()) {
        actions << new QAction(
                       QIcon::fromTheme(QStringLiteral("folder-sync")),
                       containingDirs.size() == 1
                       ? tr("Rescan ") + containingDirs.front()->displayName()
                       : tr("Rescan containing directories"),
                       parentWidget);
        if(s_connection.isConnected()) {
            for(const SyncthingDir *dir : containingDirs) {
                connect(actions.back(), &QAction::triggered, bind(&SyncthingFileItemAction::rescanDir, dir->id, QString()));
            }
        } else {
            actions.back()->setEnabled(false);
        }
    }

    // add actions for the selected items itself
    if(!detectedItems.isEmpty()) {
        actions << new QAction(
                       QIcon::fromTheme(QStringLiteral("view-refresh")),
                       detectedItems.size() == 1
                       ? tr("Rescan %1 (in %2)").arg(detectedItems.front().name, detectedItems.front().dir->displayName())
                       : tr("Rescan selected items"),
                       parentWidget);
        if(s_connection.isConnected()) {
            for(const SyncthingItem &item : detectedItems) {
                connect(actions.back(), &QAction::triggered, bind(&SyncthingFileItemAction::rescanDir, item.dir->id, item.path));
            }
        } else {
            actions.back()->setEnabled(false);
        }
    }

    // don't show anything if relevant actions could be determined
    if(actions.isEmpty()) {
        return actions;
    }

    // create the menu
    QAction *menuAction = new QAction(statusIcons().scanninig, tr("Syncthing"), this);
    QMenu *menu = new QMenu(parentWidget);
    menuAction->setMenu(menu);
    menu->addActions(actions);

    // add action to show further information about directory if the selection is only about
    // one particular Syncthing dir
    if(detectedDirs.size() + containingDirs.size() == 1) {
        QAction *infoAction = menu->addSeparator();
        infoAction->setIcon(QIcon::fromTheme(QStringLiteral("dialog-information")));
        infoAction->setText(tr("Directory"));
        QAction *statusAction = menu->addAction(tr("Status: ") + lastDir->statusString());
        if(lastDir->paused && lastDir->status != SyncthingDirStatus::OutOfSync) {
            statusAction->setIcon(statusIcons().pause);
        } else {
            switch(lastDir->status) {
            case SyncthingDirStatus::Unknown:
            case SyncthingDirStatus::Unshared:
                statusAction->setIcon(statusIcons().disconnected);
                break;
            case SyncthingDirStatus::Idle:
                statusAction->setIcon(statusIcons().idling);
                break;
            case SyncthingDirStatus::Scanning:
                statusAction->setIcon(statusIcons().scanninig);
                break;
            case SyncthingDirStatus::Synchronizing:
                statusAction->setIcon(statusIcons().sync);
                break;
            case SyncthingDirStatus::OutOfSync:
                statusAction->setIcon(statusIcons().error);
                break;
            }
        }
        menu->addAction(QIcon::fromTheme(QStringLiteral("accept_time_event")),
                        tr("Last scan time: ") + agoString(lastDir->lastScanTime))->setEnabled(false);
        menu->addAction(tr("Rescan interval: %1 seconds").arg(lastDir->rescanInterval))->setEnabled(false);
    }

    // about about action
    menu->addSeparator();
    menu->addAction(QIcon::fromTheme(QStringLiteral("help-about")), tr("About"), &SyncthingFileItemAction::showAboutDialog);

    return QList<QAction*>() << menuAction;
}

void SyncthingFileItemAction::logConnectionStatus()
{
    cerr << "Syncthing connection status changed to: " << s_connection.statusText().toLocal8Bit().data() << endl;
}

void SyncthingFileItemAction::logConnectionError(const QString &errorMessage)
{
    cerr << "Syncthing connection error: " << errorMessage.toLocal8Bit().data() << endl;
}

void SyncthingFileItemAction::rescanDir(const QString &dirId, const QString &relpath)
{
    s_connection.rescan(dirId, relpath);
}

void SyncthingFileItemAction::showAboutDialog()
{
    auto *aboutDialog = new AboutDialog(nullptr, QStringLiteral(APP_NAME), QStringLiteral(APP_AUTHOR), QStringLiteral(APP_VERSION), QStringLiteral(APP_URL), QStringLiteral(APP_DESCRIPTION), QImage(statusIcons().scanninig.pixmap(128).toImage()));
    aboutDialog->setWindowTitle(tr("About") + QStringLiteral(" - " APP_NAME));
    aboutDialog->setWindowIcon(QIcon::fromTheme(QStringLiteral("syncthingtray")));
    aboutDialog->setWindowFlags(static_cast<Qt::WindowFlags>(aboutDialog->windowFlags() | Qt::WA_DeleteOnClose));
    aboutDialog->show();
}


#include <syncthingfileitemaction.moc>
