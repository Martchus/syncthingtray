#include "./syncthingfileitemaction.h"
#include "../model/syncthingicons.h"

#include "../connector/syncthingconfig.h"
#include "../connector/syncthingconnectionsettings.h"
#include "../connector/syncthingdir.h"
#include "../connector/utils.h"

#include <qtutilities/aboutdialog/aboutdialog.h>
#include <qtutilities/resources/resources.h>

#include <c++utilities/application/argumentparser.h>

#include <KFileItem>
#include <KPluginFactory>
#include <KPluginLoader>

#include <QAction>
#include <QDir>
#include <QEvent>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QSettings>
#include <QWidget>

#include <functional>
#include <iostream>

#include "resources/config.h"

using namespace std;
using namespace Dialogs;
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
    int lastSep = path.lastIndexOf(QChar('/'));
    if (lastSep > 0) {
        name = path.mid(lastSep + 1);
    } else {
        name = path;
    }
}

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

SyncthingInfoWidget::SyncthingInfoWidget(const SyncthingInfoAction *action, QWidget *parent)
    : QWidget(parent)
    , m_textLabel(new QLabel(parent))
    , m_iconLabel(new QLabel(parent))
{
    auto *const layout = new QHBoxLayout(parent);
    layout->setMargin(4);
    layout->setSpacing(5);
    m_iconLabel->setFixedWidth(16);
    m_iconLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Minimum);
    layout->addWidget(m_iconLabel);
    layout->addWidget(m_textLabel);
    setLayout(layout);
    updateFromAction(action);
    connect(action, &QAction::changed, this, &SyncthingInfoWidget::updateFromSender);
}

void SyncthingInfoWidget::updateFromSender()
{
    updateFromAction(qobject_cast<const SyncthingInfoAction *>(QObject::sender()));
}

void SyncthingInfoWidget::updateFromAction(const SyncthingInfoAction *action)
{
    auto text(action->text());
    m_textLabel->setText(text.startsWith(QChar('&')) ? text.mid(1) : std::move(text));
    m_iconLabel->setPixmap(action->icon().pixmap(16));
    setVisible(action->isVisible());
}

SyncthingInfoAction::SyncthingInfoAction(QObject *parent)
    : QWidgetAction(parent)
{
}

QWidget *SyncthingInfoAction::createWidget(QWidget *parent)
{
    return new SyncthingInfoWidget(this, parent);
}

SyncthingDirActions::SyncthingDirActions(const SyncthingDir &dir, QObject *parent)
    : QObject(parent)
    , m_dirId(dir.id)
{
    m_infoAction.setSeparator(true);
    updateStatus(dir);
}

void SyncthingDirActions::updateStatus(const std::vector<SyncthingDir> &dirs)
{
    for (const SyncthingDir &dir : dirs) {
        if (updateStatus(dir)) {
            return;
        }
    }
    m_statusAction.setText(tr("Status: not available anymore"));
    m_statusAction.setIcon(statusIcons().disconnected);
}

bool SyncthingDirActions::updateStatus(const SyncthingDir &dir)
{
    if (dir.id != m_dirId) {
        return false;
    }
    m_infoAction.setText(tr("Directory info for %1").arg(dir.displayName()));
    m_infoAction.setIcon(QIcon::fromTheme(QStringLiteral("dialog-information")));
    m_statusAction.setText(tr("Status: ") + dir.statusString());
    if (dir.paused && dir.status != SyncthingDirStatus::OutOfSync) {
        m_statusAction.setIcon(statusIcons().pause);
    } else if (dir.isUnshared()) {
        m_statusAction.setIcon(statusIcons().disconnected);
    } else {
        switch (dir.status) {
        case SyncthingDirStatus::Unknown:
            m_statusAction.setIcon(statusIcons().disconnected);
            break;
        case SyncthingDirStatus::Idle:
            m_statusAction.setIcon(statusIcons().idling);
            break;
        case SyncthingDirStatus::Scanning:
            m_statusAction.setIcon(statusIcons().scanninig);
            break;
        case SyncthingDirStatus::Synchronizing:
            m_statusAction.setIcon(statusIcons().sync);
            break;
        case SyncthingDirStatus::OutOfSync:
            m_statusAction.setIcon(statusIcons().error);
            break;
        }
    }
    m_globalStatusAction.setText(tr("Global: ") + directoryStatusString(dir.globalStats));
    m_localStatusAction.setText(tr("Local: ") + directoryStatusString(dir.localStats));
    m_lastScanAction.setText(tr("Last scan time: ") + agoString(dir.lastScanTime));
    m_lastScanAction.setIcon(QIcon::fromTheme(QStringLiteral("accept_time_event")));
    m_rescanIntervalAction.setText(tr("Rescan interval: %1 seconds").arg(dir.rescanInterval));
    if (!dir.pullErrorCount) {
        m_errorsAction.setVisible(false);
    } else {
        m_errorsAction.setVisible(true);
        m_errorsAction.setIcon(QIcon::fromTheme(QStringLiteral("dialog-error")));
        m_errorsAction.setText(tr("%1 item(s) out-of-sync", nullptr, trQuandity(dir.pullErrorCount)).arg(dir.pullErrorCount));
    }
    return true;
}

QList<QAction *> &operator<<(QList<QAction *> &actions, SyncthingDirActions &dirActions)
{
    return actions << &dirActions.m_infoAction << &dirActions.m_statusAction << &dirActions.m_globalStatusAction << &dirActions.m_localStatusAction
                   << &dirActions.m_lastScanAction << &dirActions.m_rescanIntervalAction << &dirActions.m_errorsAction;
}

SyncthingFileItemActionStaticData::SyncthingFileItemActionStaticData()
    : m_initialized(false)
{
}

SyncthingFileItemActionStaticData::~SyncthingFileItemActionStaticData()
{
}

void SyncthingFileItemActionStaticData::initialize()
{
    if (m_initialized) {
        return;
    }

    LOAD_QT_TRANSLATIONS;

    // load settings
    const QSettings settingsFile(QSettings::IniFormat, QSettings::UserScope, QStringLiteral(PROJECT_NAME));

    // determine path of Syncthing config file
    m_configFilePath = [&] {
        const QByteArray configPathFromEnv(qgetenv("KIO_SYNCTHING_CONFIG_PATH"));
        if (!configPathFromEnv.isEmpty()) {
            return QString::fromLocal8Bit(configPathFromEnv);
        }
        const QString configPathFromSettings = settingsFile.value(QStringLiteral("syncthingConfigPath")).toString();
        if (!configPathFromSettings.isEmpty()) {
            return configPathFromSettings;
        }
        return SyncthingConfig::locateConfigFile();
    }();
    applySyncthingConfiguration(m_configFilePath);

    // prevent unnecessary API calls (for the purpose of the context menu)
    m_connection.disablePolling();

    // connect Signals & Slots for logging
    connect(&m_connection, &SyncthingConnection::error, this, &SyncthingFileItemActionStaticData::logConnectionError);
    if (qEnvironmentVariableIsSet("KIO_SYNCTHING_LOG_STATUS")) {
        connect(&m_connection, &SyncthingConnection::statusChanged, this, &SyncthingFileItemActionStaticData::logConnectionStatus);
    }

    m_initialized = true;
}

void SyncthingFileItemActionStaticData::logConnectionStatus()
{
    cerr << "Syncthing connection status changed to: " << m_connection.statusText().toLocal8Bit().data() << endl;
}

void SyncthingFileItemActionStaticData::logConnectionError(const QString &errorMessage, SyncthingErrorCategory errorCategory)
{
    switch (errorCategory) {
    case SyncthingErrorCategory::Parsing:
    case SyncthingErrorCategory::SpecificRequest:
        QMessageBox::critical(nullptr, tr("Syncthing connection error"), errorMessage);
        break;
    default:
        cerr << "Syncthing connection error: " << errorMessage.toLocal8Bit().data() << endl;
    }
}

void SyncthingFileItemActionStaticData::rescanDir(const QString &dirId, const QString &relpath)
{
    m_connection.rescan(dirId, relpath);
}

void SyncthingFileItemActionStaticData::showAboutDialog()
{
    auto *aboutDialog = new AboutDialog(nullptr, QStringLiteral(APP_NAME), QStringLiteral(APP_AUTHOR "\nSyncthing icons from Syncthing project"),
        QStringLiteral(APP_VERSION), ApplicationUtilities::dependencyVersions2, QStringLiteral(APP_URL), QStringLiteral(APP_DESCRIPTION),
        QImage(statusIcons().scanninig.pixmap(128).toImage()));
    aboutDialog->setWindowTitle(tr("About") + QStringLiteral(" - " APP_NAME));
    aboutDialog->setWindowIcon(QIcon::fromTheme(QStringLiteral("syncthingtray")));
    aboutDialog->setAttribute(Qt::WA_DeleteOnClose);
    aboutDialog->show();
}

void SyncthingFileItemActionStaticData::selectSyncthingConfig()
{
    const auto configFilePath = QFileDialog::getOpenFileName(nullptr, tr("Select Syncthing config file") + QStringLiteral(" - " APP_NAME));
    if (!configFilePath.isEmpty() && applySyncthingConfiguration(configFilePath)) {
        QSettings(QSettings::IniFormat, QSettings::UserScope, QStringLiteral(PROJECT_NAME))
            .setValue(QStringLiteral("syncthingConfigPath"), m_configFilePath = configFilePath);
    }
}

bool SyncthingFileItemActionStaticData::applySyncthingConfiguration(const QString &syncthingConfigFilePath)
{
    clearCurrentError();

    // check for empty path
    if (syncthingConfigFilePath.isEmpty()) {
        setCurrentError(tr("Syncthing config file can not be automatically located"));
        return false;
    }

    // load Syncthing config
    SyncthingConfig config;
    if (!config.restore(syncthingConfigFilePath)) {
        auto errorMessage = tr("Unable to load Syncthing config from \"%1\"").arg(syncthingConfigFilePath);
        if (!m_configFilePath.isEmpty() && m_configFilePath != syncthingConfigFilePath) {
            errorMessage += QChar('\n');
            errorMessage += tr("(still using config from \"%1\")").arg(m_configFilePath);
        }
        setCurrentError(errorMessage);
        return false;
    }
    cerr << "Syncthing config loaded from \"" << syncthingConfigFilePath.toLocal8Bit().data() << "\"" << endl;

    // make connection settings
    SyncthingConnectionSettings settings;
    settings.syncthingUrl = config.syncthingUrl();
    settings.apiKey.append(config.guiApiKey);

    // establish connection
    bool ok;
    int reconnectInterval = qEnvironmentVariableIntValue("KIO_SYNCTHING_RECONNECT_INTERVAL", &ok);
    if (!ok || reconnectInterval < 0) {
        reconnectInterval = 10000;
    }
    m_connection.setAutoReconnectInterval(reconnectInterval);
    m_connection.reconnect(settings);
    return true;
}

void SyncthingFileItemActionStaticData::setCurrentError(const QString &currentError)
{
    if (m_currentError == currentError) {
        return;
    }
    const bool hadError = hasError();
    m_currentError = currentError;
    if (hadError != hasError()) {
        emit hasErrorChanged(hasError());
    }
    emit currentErrorChanged(m_currentError);
}

SyncthingFileItemActionStaticData SyncthingFileItemAction::s_data;

SyncthingFileItemAction::SyncthingFileItemAction(QObject *parent, const QVariantList &)
    : KAbstractFileItemActionPlugin(parent)
{
    s_data.initialize();
}

QList<QAction *> SyncthingFileItemAction::actions(const KFileItemListProperties &fileItemInfo, QWidget *parentWidget)
{
    // handle case when not connected yet
    if (!s_data.connection().isConnected()) {
        s_data.connection().connect();
        auto *menuAction = new SyncthingMenuAction(fileItemInfo, QList<QAction *>(), parentWidget);
        connect(&s_data.connection(), &SyncthingConnection::statusChanged, menuAction, &SyncthingMenuAction::updateStatus);
        return QList<QAction *>() << menuAction;
    }

    const QList<QAction *> actions = createActions(fileItemInfo, parentWidget);
    // don't show anything if no relevant actions could be determined
    if (actions.isEmpty()) {
        return actions;
    }

    return QList<QAction *>({ new SyncthingMenuAction(fileItemInfo, actions, parentWidget) });
}

QList<QAction *> SyncthingFileItemAction::createActions(const KFileItemListProperties &fileItemInfo, QWidget *parentWidget)
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
        actions << new QAction(QIcon::fromTheme(QStringLiteral("view-refresh")),
            detectedItems.size() == 1 ? tr("Rescan %1 (in %2)").arg(detectedItems.front().name, detectedItems.front().dir->displayName())
                                      : tr("Rescan selected items"),
            parentWidget);
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
            detectedDirs.size() == 1 ? tr("Rescan %1").arg(detectedDirs.front()->displayName()) : tr("Rescan selected directories"), parentWidget);
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
                detectedDirs.size() == 1 ? tr("Resume %1").arg(detectedDirs.front()->displayName()) : tr("Resume selected directories"),
                parentWidget);
        } else {
            actions << new QAction(QIcon::fromTheme(QStringLiteral("media-playback-pause")),
                detectedDirs.size() == 1 ? tr("Pause %1").arg(detectedDirs.front()->displayName()) : tr("Pause selected directories"), parentWidget);
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
            containingDirs.size() == 1 ? tr("Rescan %1").arg(containingDirs.front()->displayName()) : tr("Rescan containing directories"),
            parentWidget);
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
                containingDirs.size() == 1 ? tr("Resume %1").arg(containingDirs.front()->displayName()) : tr("Resume containing directories"),
                parentWidget);
        } else {
            actions << new QAction(QIcon::fromTheme(QStringLiteral("media-playback-pause")),
                containingDirs.size() == 1 ? tr("Pause %1").arg(containingDirs.front()->displayName()) : tr("Pause containing directories"),
                parentWidget);
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
        auto *statusActions = new SyncthingDirActions(*lastDir, parentWidget);
        connect(&connection, &SyncthingConnection::newDirs, statusActions,
            static_cast<void (SyncthingDirActions::*)(const vector<SyncthingDir> &)>(&SyncthingDirActions::updateStatus));
        connect(&connection, &SyncthingConnection::dirStatusChanged, statusActions,
            static_cast<bool (SyncthingDirActions::*)(const SyncthingDir &)>(&SyncthingDirActions::updateStatus));
        actions << *statusActions;
    }

    // add separator
    if (!actions.isEmpty()) {
        QAction *const separator = new QAction(parentWidget);
        separator->setSeparator(true);
        actions << separator;
    }

    // add error action
    QAction *const errorAction = new SyncthingInfoAction(parentWidget);
    errorAction->setText(data.currentError());
    errorAction->setIcon(QIcon::fromTheme(QStringLiteral("state-error")));
    errorAction->setVisible(data.hasError());
    connect(&data, &SyncthingFileItemActionStaticData::currentErrorChanged, errorAction, &QAction::setText);
    connect(&data, &SyncthingFileItemActionStaticData::hasErrorChanged, errorAction, &QAction::setVisible);
    connect(&data, &SyncthingFileItemActionStaticData::currentErrorChanged, errorAction, &QAction::changed);
    actions << errorAction;

    // add config file selection
    QAction *const configFileAction = new QAction(QIcon::fromTheme(QStringLiteral("settings-configure")), tr("Select Syncthing config ..."));
    connect(configFileAction, &QAction::triggered, &data, &SyncthingFileItemActionStaticData::selectSyncthingConfig);
    actions << configFileAction;

    // about about action
    QAction *const aboutAction = new QAction(QIcon::fromTheme(QStringLiteral("help-about")), tr("About"));
    connect(aboutAction, &QAction::triggered, &SyncthingFileItemActionStaticData::showAboutDialog);
    actions << aboutAction;

    return actions;
}

#include <syncthingfileitemaction.moc>
